#include "main.h"

struct app_params app;

static const char usage[] = "\n";

void app_print_usage(void) {
	printf(usage);
}

static int app_parse_port_mask(const char *arg) {
	char *end = NULL;
	uint64_t port_mask;
	uint32_t i;

	if (arg[0] == '\0')
		return -1;

	port_mask = strtoul(arg, &end, 16);
	if ((end == NULL) || (*end != '\0'))
		return -2;

	if (port_mask == 0)
		return -3;

	app.n_ports = 0;
	for (i = 0; i < 64; i++) {
		if ((port_mask & (1LLU << i)) == 0)
			continue;

		if (app.n_ports >= APP_MAX_PORTS)
			return -4;

		app.ports[app.n_ports] = i;
		app.n_ports++;
	}

	if (!rte_is_power_of_2(app.n_ports))
		return -5;

	return 0;
}

int app_parse_args(int argc, char **argv)
{
	int opt, ret;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];
	static struct option lgopts[] = {
		{"track-packets", 0, 0, 0},
		{NULL, 0, 0, 0}
	};
	uint32_t lcores[3], n_lcores, lcore_id;

	/* EAL args */
	n_lcores = 0;
	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id) == 0)
			continue;

		if (n_lcores >= 3) {
			RTE_LOG(ERR, USER1, "Number of cores must be 3\n");
			app_print_usage();
			return -1;
		}

		lcores[n_lcores] = lcore_id;
		n_lcores++;
	}

	if (n_lcores != 3) {
		RTE_LOG(ERR, USER1, "Number of cores must be 3\n");
		app_print_usage();
		return -1;
	}

	app.core_rx = lcores[0];
	app.core_fw = lcores[1];
	app.core_tx = lcores[2];

	/* Non-EAL args */
	argvopt = argv;

	app.track_packets = 0;

	while ((opt = getopt_long(argc, argvopt, "p:",
			lgopts, &option_index)) != EOF) {
		switch (opt) {
		case 'p':
			if (app_parse_port_mask(optarg) < 0) {
				app_print_usage();
				return -1;
			}
			break;
		case 0:
			app.track_packets = 1;
			break;
		default:
			return -1;
		}
	}

	RTE_LOG(INFO, USER1, "Packets tracking is %s\n", app.track_packets?"enabled":"disabled");

	if (optind >= 0)
		argv[optind - 1] = prgname;

	ret = optind - 1;
	optind = 0; /* reset getopt lib */
	return ret;
}
