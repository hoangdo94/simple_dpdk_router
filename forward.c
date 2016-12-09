#include "main.h"

#define DEFAULT_RULE_PATH			"rules.conf"
#define ACL_LEAD_CHAR				('@')
#define ROUTE_LEAD_CHAR				('R')
#define COMMENT_LEAD_CHAR			('#')
#define ROUTE_ENTRY_LINE_MEMBERS	7
#define ACL_ENTRY_LINE_MEMBERS		6
#define ROUTE_ENTRY_PRIORITY		0x1
#define ACL_ENTRY_PRIORITY			0x0

#define GET_CB_FIELD(in, fd, base, lim, dlm)	do {            \
	unsigned long val;                                      \
	char *end;                                              \
	errno = 0;                                              \
	val = strtoul((in), &end, (base));                      \
	if (errno != 0 || end[0] != (dlm) || val > (lim))       \
		return -EINVAL;                               \
	(fd) = (typeof(fd))val;                                 \
	(in) = end + 1;                                         \
} while (0)

enum {
	PROTO_FIELD_IPV4,
	SRC_FIELD_IPV4,
	DST_FIELD_IPV4,
	SRCP_FIELD_IPV4,
	DSTP_FIELD_IPV4,
	NUM_FIELDS_IPV4
};

/*
 * Meta-data of the ACL rules
 * 5 tuples: IP addresses, ports, and protocol.
 */
struct rte_acl_field_def ipv4_field_formats[NUM_FIELDS_IPV4] = {
	{
		.type = RTE_ACL_FIELD_TYPE_BITMASK,
		.size = sizeof(uint8_t),
		.field_index = PROTO_FIELD_IPV4,
		.input_index = PROTO_FIELD_IPV4,
		.offset = sizeof(struct ether_hdr) +
			offsetof(struct ipv4_hdr, next_proto_id),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SRC_FIELD_IPV4,
		.input_index = SRC_FIELD_IPV4,
		.offset = sizeof(struct ether_hdr) +
			offsetof(struct ipv4_hdr, src_addr),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DST_FIELD_IPV4,
		.input_index = DST_FIELD_IPV4,
		.offset = sizeof(struct ether_hdr) +
			offsetof(struct ipv4_hdr, dst_addr),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint16_t),
		.field_index = SRCP_FIELD_IPV4,
		.input_index = SRCP_FIELD_IPV4,
		.offset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint16_t),
		.field_index = DSTP_FIELD_IPV4,
		.input_index = SRCP_FIELD_IPV4,
		.offset = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) +
			sizeof(uint16_t),
	},
};

static int parse_ipv4_net(const char *in, uint32_t *addr, uint32_t *mask_len) {
	uint8_t a, b, c, d, m;

	GET_CB_FIELD(in, a, 0, UINT8_MAX, '.');
	GET_CB_FIELD(in, b, 0, UINT8_MAX, '.');
	GET_CB_FIELD(in, c, 0, UINT8_MAX, '.');
	GET_CB_FIELD(in, d, 0, UINT8_MAX, '/');
	GET_CB_FIELD(in, m, 0, sizeof(uint32_t) * CHAR_BIT, 0);

	addr[0] = IPv4(a, b, c, d);
	mask_len[0] = m;

	return 0;
}

static int parse_range(const char *in, const char splitter, uint32_t *lo, uint32_t *hi) {
	uint32_t a, b;

	GET_CB_FIELD(in, a, 0, UINT32_MAX, splitter);
	GET_CB_FIELD(in, b, 0, UINT32_MAX, 0);

	lo[0] = a;
	hi[0] = b;

	return 0;
}

static inline int is_bypass_line(char *buff) {
	int i = 0;

	/* comment line */
	if (buff[0] == COMMENT_LEAD_CHAR)
		return 1;
	/* empty line */
	while (buff[i] != '\0') {
		if (!isspace(buff[i]))
			return 0;
		i++;
	}
	return 1;
}

static int parse_rule_members(char *str, char **in, int lim) {
	int i;
	char *s, *sp;
	static const char *dlm = " \t\n";
	s = str;

	for (i = 0; i != lim; i++, s = NULL) {
		in[i] = strtok_r(s, dlm, &sp);
		if (in[i] == NULL)
			return -EINVAL;
	}
	
	return 0;
}

static int add_route_rule(char *buff, struct rte_pipeline *p, uint32_t table_id) {
	char *in[ROUTE_ENTRY_LINE_MEMBERS];
	uint32_t addr, mask, lo, hi;
	int rc, key_found;

	parse_rule_members(buff, in, ROUTE_ENTRY_LINE_MEMBERS);

	struct rte_pipeline_table_entry table_entry = {
	 	.action = RTE_PIPELINE_ACTION_PORT,
	 	{.port_id = atoi(in[6])}
	};
	struct rte_table_acl_rule_add_params rule_params;
	struct rte_pipeline_table_entry *entry_ptr;

	memset(&rule_params, 0, sizeof(rule_params));

	printf("%s %s %s %s %s %s %s\n",
		in[0], in[1], in[2], in[3], in[4], in[5], in[6]);

	/* Set the rule values */
	parse_ipv4_net(in[1], &addr, &mask);
	rule_params.field_value[SRC_FIELD_IPV4].value.u32 = addr;
	rule_params.field_value[SRC_FIELD_IPV4].mask_range.u32 = mask;

	parse_ipv4_net(in[2], &addr, &mask);
	rule_params.field_value[DST_FIELD_IPV4].value.u32 = addr;
	rule_params.field_value[DST_FIELD_IPV4].mask_range.u32 = mask;

	parse_range(in[3], ':', &lo, &hi);
	rule_params.field_value[SRCP_FIELD_IPV4].value.u16 = lo;
	rule_params.field_value[SRCP_FIELD_IPV4].mask_range.u16 = hi;

	parse_range(in[4], ':', &lo, &hi);
	rule_params.field_value[DSTP_FIELD_IPV4].value.u16 = lo;
	rule_params.field_value[DSTP_FIELD_IPV4].mask_range.u16 = hi;

	parse_range(in[5], '/', &lo, &hi);
	rule_params.field_value[PROTO_FIELD_IPV4].value.u8 = lo;
	rule_params.field_value[PROTO_FIELD_IPV4].mask_range.u8 = hi;

	rule_params.priority = ROUTE_ENTRY_PRIORITY;

	rc = rte_pipeline_table_entry_add(p, table_id, &rule_params,
		&table_entry, &key_found, &entry_ptr);
	if (rc < 0)
		rte_panic("Unable to add entry to table %u (%d)\n",
				table_id, rc);

	return rc;
}

static int add_acl_rule(char *buff, struct rte_pipeline *p, uint32_t table_id) {
	char *in[ACL_ENTRY_LINE_MEMBERS];
	uint32_t addr, mask, lo, hi;
	int rc, key_found;

	parse_rule_members(buff, in, ACL_ENTRY_LINE_MEMBERS);

	struct rte_pipeline_table_entry table_entry = {
	 	.action = RTE_PIPELINE_ACTION_DROP
	};
	struct rte_table_acl_rule_add_params rule_params;
	struct rte_pipeline_table_entry *entry_ptr;

	memset(&rule_params, 0, sizeof(rule_params));

	printf("%s %s %s %s %s %s\n",
		in[0], in[1], in[2], in[3], in[4], in[5]);

	/* Set the rule values */
	parse_ipv4_net(in[1], &addr, &mask);
	rule_params.field_value[SRC_FIELD_IPV4].value.u32 = addr;
	rule_params.field_value[SRC_FIELD_IPV4].mask_range.u32 = mask;

	parse_ipv4_net(in[2], &addr, &mask);
	rule_params.field_value[DST_FIELD_IPV4].value.u32 = addr;
	rule_params.field_value[DST_FIELD_IPV4].mask_range.u32 = mask;

	parse_range(in[3], ':', &lo, &hi);
	rule_params.field_value[SRCP_FIELD_IPV4].value.u16 = lo;
	rule_params.field_value[SRCP_FIELD_IPV4].mask_range.u16 = hi;

	parse_range(in[4], ':', &lo, &hi);
	rule_params.field_value[DSTP_FIELD_IPV4].value.u16 = lo;
	rule_params.field_value[DSTP_FIELD_IPV4].mask_range.u16 = hi;

	parse_range(in[5], '/', &lo, &hi);
	rule_params.field_value[PROTO_FIELD_IPV4].value.u8 = lo;
	rule_params.field_value[PROTO_FIELD_IPV4].mask_range.u8 = hi;

	rule_params.priority = ACL_ENTRY_PRIORITY;

	rc = rte_pipeline_table_entry_add(p, table_id, &rule_params,
		&table_entry, &key_found, &entry_ptr);
	if (rc < 0)
		rte_panic("Unable to add entry to table %u (%d)\n",
				table_id, rc);

	return rc;
}

static void add_table_entries(struct rte_pipeline *p, uint32_t table_id) {
	char buff[LINE_MAX];

	FILE *fh = fopen(app.rule_path?app.rule_path:DEFAULT_RULE_PATH, "rb");
	unsigned int i = 0;

	if (fh == NULL)
		rte_exit(EXIT_FAILURE, "%s: Open %s failed\n", __func__,
			app.rule_path);
	
	i = 0;
	while (fgets(buff, LINE_MAX, fh) != NULL) {
		i++;

		if (is_bypass_line(buff))
			continue;

		char s = buff[0];

		/* Route entry */
		if (s == ROUTE_LEAD_CHAR) {
			add_route_rule(buff, p, table_id);
		}
		/* ACL entry */
		else if (s == ACL_LEAD_CHAR) {
			add_acl_rule(buff, p, table_id);
		}
		/* Illegal line */
		else
			rte_exit(EXIT_FAILURE,
				"%s Line %u: should start with leading "
				"char %c or %c\n",
				app.rule_path, i, ROUTE_LEAD_CHAR, ACL_LEAD_CHAR);
	}

	fclose(fh);
}

void app_main_loop_fw(void) {
	struct rte_pipeline_params pipeline_params = {
		.name = "simple-pipeline",
		.socket_id = rte_socket_id(),
	};

	struct rte_pipeline *p;
	uint32_t port_in_id[APP_MAX_PORTS];
	uint32_t port_out_id[APP_MAX_PORTS];
	uint32_t table_id;
	uint32_t i;

	RTE_LOG(INFO, USER1, "Core %u is doing FW\n", rte_lcore_id());

	/* Pipeline configuration */
	p = rte_pipeline_create(&pipeline_params);
	if (p == NULL)
		rte_panic("Unable to configure the pipeline\n");

	/* Input port configuration */
	for (i = 0; i < app.n_ports; i++) {
		struct rte_port_ring_reader_params port_ring_params = {
			.ring = app.rings_rx[i],
		};

		struct rte_pipeline_port_in_params port_params = {
			.ops = &rte_port_ring_reader_ops,
			.arg_create = (void *) &port_ring_params,
			.f_action = NULL,
			.arg_ah = NULL,
			.burst_size = app.burst_size_fw_read,
		};

		if (rte_pipeline_port_in_create(p, &port_params, &port_in_id[i]))
			rte_panic("Unable to configure input port for ring %d\n", i);
	}

	/* Output port configuration */
	for (i = 0; i < app.n_ports; i++) {
		struct rte_port_ring_writer_params port_ring_params = {
			.ring = app.rings_tx[i],
			.tx_burst_sz = app.burst_size_fw_write,
		};

		struct rte_pipeline_port_out_params port_params = {
			.ops = &rte_port_ring_writer_ops,
			.arg_create = (void *) &port_ring_params,
			.f_action = NULL,
			.arg_ah = NULL,
		};

		if (rte_pipeline_port_out_create(p, &port_params, &port_out_id[i]))
			rte_panic("Unable to configure output port for ring %d\n", i);
	}

	/* Table configuration */
	{
		struct rte_table_acl_params table_acl_params = {
			.name = "simple-rules",
			.n_rules = 1 << 5,
			.n_rule_fields = DIM(ipv4_field_formats),
		};
		/* Copy in the rule meta-data defined above into the params */
		memcpy(table_acl_params.field_format, ipv4_field_formats,
			sizeof(ipv4_field_formats));

		struct rte_pipeline_table_params table_params = {
			.ops = &rte_table_acl_ops,
			.arg_create = &table_acl_params,
			.f_action_hit = NULL,
			.f_action_miss = NULL,
			.arg_ah = NULL,
			.action_data_size = 0,
		};

		if (rte_pipeline_table_create(p, &table_params, &table_id))
			rte_panic("Unable to configure the ACL table\n");
	}

	/* Interconnecting ports and tables */
	for (i = 0; i < app.n_ports; i++)
		if (rte_pipeline_port_in_connect_to_table(p, port_in_id[i],
			table_id))
			rte_panic("Unable to connect input port %u to table %u\n",
				port_in_id[i],  table_id);

	/* Add entries to tables */
	add_table_entries(p, table_id);

	/* Enable input ports */
	for (i = 0; i < app.n_ports; i++)
		if (rte_pipeline_port_in_enable(p, port_in_id[i]))
			rte_panic("Unable to enable input port %u\n",
				port_in_id[i]);

	/* Check pipeline consistency */
	if (rte_pipeline_check(p) < 0)
		rte_panic("Pipeline consistency check failed\n");

	/* Run-time */
	for ( ; ; )
		rte_pipeline_run(p);
}
