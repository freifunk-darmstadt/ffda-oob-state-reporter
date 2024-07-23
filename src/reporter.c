#include <stdlib.h>
#include <libubus.h>

#include "log.h"
#include "reporter.h"
#include "ubus.h"
#include "udp.h"


static int fosr_metrics_submit(struct fosr *fosr, struct fosr_metrics *metrics)
{
	MSG(DEBUG, "Submitting metrics: soc=%u, charging=%u, temperature=%.2f\n",
	    metrics->soc, metrics->charging, metrics->temperature);

	/* Submit metrics here */
	return fosr_udp_submit(fosr, metrics);
}

static uint32_t fosr_should_submit(struct fosr *fosr)
{
	struct fosr_metrics *current = &fosr->metrics.current;
	struct fosr_metrics *last_submitted = &fosr->metrics.last_submitted;
	uint32_t submission_reasons;

	submission_reasons = 0;

	/* Submit if charging-state changed */
	if (current->charging != last_submitted->charging) {
		MSG(INFO, "Charging status changed: charging=%u\n", current->charging);
		submission_reasons |= FOSR_SUBMISSION_REASON_CHARGING;
	}

	/* Submit if state-of-charge changed on battery */
	if (!current->charging && current->soc != last_submitted->soc) {
		MSG(INFO, "SOC changed on battery: soc=%u\n", current->soc);
		submission_reasons |= FOSR_SUBMISSION_REASON_SOC;
	}

	/* Submit in configured interval */
	if (!fosr->last_interval_submission || fosr->sysinfo.uptime - fosr->last_interval_submission > fosr->config.interval) {
		MSG(INFO, "Submission interval reached\n");
		submission_reasons |= FOSR_SUBMISSION_REASON_INTERVAL;
	}

	return submission_reasons;
}

static void fosr_work(struct uloop_timeout *timeout)
{
	struct fosr *fosr = container_of(timeout, struct fosr, work_timeout);
	uint32_t should_submit;
	int ret;

	/* Update system uptime */
	sysinfo(&fosr->sysinfo);

	/* Always update battery status */
	fosr_ubus_battery_status_update(fosr);

	/* Check if metrics should be submitted */
	should_submit = fosr_should_submit(fosr);

	if (should_submit) {
		/* Submit metrics */
		ret = fosr_metrics_submit(fosr, &fosr->metrics.current);
		if (ret) {
			MSG(ERROR, "Failed to submit metrics\n");
		}

		/* Update last submitted metrics*/
		memcpy(&fosr->metrics.last_submitted, &fosr->metrics.current, sizeof(fosr->metrics.current));

		/* Update last-interval submission */
		if (should_submit & FOSR_SUBMISSION_REASON_INTERVAL) {
			fosr->last_interval_submission = fosr->sysinfo.uptime;
		}
	}

	uloop_timeout_set(&fosr->work_timeout, 1000);
}

static int fosr_parse_config(struct fosr *fosr, int argc, char *argv[])
{
	int reporter_id = 0;
	int c;

	while ((c = getopt (argc, argv, "h:i:l:p:r:s")) != -1) {
		switch (c){
			case 'h':
				fosr->config.host = optarg;
				break;
			case 'i':
				fosr->config.interval = atoi(optarg);
				break;
			case 'l':
				fosr_log_set_level(atoi(optarg));
				break;
			case 'p':
				fosr->config.port = atoi(optarg);
				break;
			case 'r':
				reporter_id = atoi(optarg);
				break;
			case 's':
				fosr_log_set_syslog(1);
				break;
			default:
				MSG(ERROR, "Invalid argument: %c\n", c);
				return 1;
		}
	}

	/* Validate config */
	if (!fosr->config.host) {
		MSG(ERROR, "Missing host\n");
		return 1;
	}

	if (!fosr->config.port) {
		MSG(ERROR, "Missing port\n");
		return 1;
	}

	if (!fosr->config.interval) {
		MSG(ERROR, "Missing interval\n");
		return 1;
	}

	if (fosr->config.interval < 30) {
		MSG(ERROR, "Interval too low. Minimum: 30\n");
		return 1;
	}

	if (!reporter_id) {
		MSG(ERROR, "Missing reporter id\n");
		return 1;
	}

	if (reporter_id < 0 || reporter_id > 65535) {
		MSG(ERROR, "Invalid reporter id. Minimum: 0, Maximum: 65535\n");
		return 1;
	}

	fosr->config.reporter_id = reporter_id;

	return 0;
}

int main(int argc, char *argv[])
{
	struct fosr fosr = {0};
	int ret;

	/* Parse config */
	ret = fosr_parse_config(&fosr, argc, argv);
	if (ret) {
		MSG(ERROR, "Failed to parse config\n");
		return 1;
	}

	MSG(INFO, "Starting reporter\n");

	ret = fosr_ubus_connect(&fosr);
	if (ret) {
		MSG(ERROR, "Failed to connect to ubus: %s\n", ubus_strerror(ret));
	}

	MSG(INFO, "Connected to ubus\n");

	uloop_init();
	ubus_add_uloop(&fosr.ctx);

	fosr.work_timeout.cb = fosr_work;
	fosr.work_timeout.cb(&fosr.work_timeout);

	uloop_run();
	uloop_done();

	return 0;
}