#include <stdlib.h>
#include <libubus.h>

#include "log.h"
#include "reporter.h"
#include "ubus.h"
#include "udp.h"

#define FOSR_SUBMISSION_RETRY 60

static int fosr_metrics_submit(struct fosr *fosr, struct fosr_metrics *metrics)
{
	MSG(DEBUG, "Submitting metrics: soc=%u, charging=%u, temperature=%.2f\n",
	    metrics->soc, metrics->charging, metrics->temperature);

	/* Submit metrics here */
	return fosr_udp_submit(fosr, metrics);
}

static void fosr_work(struct uloop_timeout *timeout)
{
	struct fosr *fosr = container_of(timeout, struct fosr, work_timeout);
	int ret;

	/* Update system uptime */
	sysinfo(&fosr->sysinfo);

	/* Always update battery status */
	fosr_ubus_battery_status_update(fosr);

	/* Update metrics if changed */
	if (fosr->metrics_updated) {
		fosr->metrics_updated = 0;
		fosr->pending_submission = FOSR_SUBMISSION_RETRY;
		MSG(DEBUG, "MCU reported changed metrics\n");
	}

	/* Regular update interval */
	if (!fosr->last_submission || fosr->sysinfo.uptime - fosr->last_submission > fosr->config.interval) {
		fosr->last_submission = fosr->sysinfo.uptime;
		fosr->pending_submission = FOSR_SUBMISSION_RETRY;
		MSG(DEBUG, "Metrics submission interval reached\n");
	}

	if (fosr->pending_submission) {
		/* Attempt to send */
		ret = fosr_metrics_submit(fosr, &fosr->metrics);
		fosr->pending_submission--;

		if (ret) {
			/* Could not send - Don't spam log despite error */
			MSG(DEBUG, "Failed to submit metrics\n");
			if (!fosr->pending_submission) {
				MSG(ERROR, "Failed to submit metrics - Retries exceeded\n");
			}
		} else {
			MSG(INFO, "Metrics submitted soc=%u, charging=%u, temperature=%.2f\n",
			    fosr->metrics.soc, fosr->metrics.charging, fosr->metrics.temperature);
			fosr->pending_submission = 0;
		}
	}

	uloop_timeout_set(&fosr->work_timeout, 1000);
}

static int fosr_parse_config(struct fosr *fosr, int argc, char *argv[])
{
	int c;

	while ((c = getopt (argc, argv, "h:i:l:p:s")) != -1) {
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