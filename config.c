#include "main.h"

struct app_params app;

static const char usage[] = "Usage: ./simple-router [EAL options]"
 			" -- [Router options]\n"
			" - EAL options: \n"
			"\t+ -c: coremask, must specify 3 cores\n"
			"\t+ -n: number of channel, default 4\n"
			" - Router options: \n"
			"\t+ -p: portmask, must specify 2 or 4 ports\n"
			"\t+ -b: burst size, must be power of 2,"
			" default 64\n"
			"\t+ -r: rule path, default\"rules.conf\"\n";

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

static int app_parse_burst_size(const char *arg) {
	int burst_size = atoi(arg);
	if (!rte_is_power_of_2(burst_size))
		return -5;

	app.burst_size_rx_read = burst_size;
	app.burst_size_rx_write = burst_size;
	app.burst_size_fw_read = burst_size;
	app.burst_size_fw_write = burst_size;
	app.burst_size_tx_read = burst_size;
	app.burst_size_tx_write = burst_size;

	return 0;
}

int app_parse_args(int argc, char **argv)
{
	int opt, ret;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];
	static struct option lgopts[] = {
		{0, 0, 0, 0},
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

	while ((opt = getopt_long(argc, argvopt, "p:b:r:",
			lgopts, &option_index)) != EOF) {
		switch (opt) {
		case 'p':
			if (app_parse_port_mask(optarg) < 0) {
				return -1;
			}
			break;
		case 'b':
			if (app_parse_burst_size(optarg) < 0) {
				return -1;
			}
			break;
		case 'r':
			app.rule_path = optarg;
			break;
		default:
			return -1;
		}
	}

	RTE_LOG(INFO, USER1, "Burst size is %d\n", app.burst_size_fw_write);
	
	if (optind >= 0)
		argv[optind - 1] = prgname;

	ret = optind - 1;
	optind = 0; /* reset getopt lib */
	return ret;
}
