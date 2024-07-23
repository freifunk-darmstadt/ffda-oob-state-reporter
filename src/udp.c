#include <netdb.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "log.h"
#include "reporter.h"
#include "udp.h"

static int fosr_udp_submit_getaddr(int family, const char *host, uint16_t port, struct addrinfo **res)
{
	struct addrinfo hints = {0};
	char port_string[6];

	snprintf(port_string, sizeof(port_string), "%u", port);

	hints.ai_family = family;
	hints.ai_socktype = SOCK_DGRAM;

	if (getaddrinfo(host, port_string, &hints, res) != 0) {
		return -1;
	}

	return 0;
}

int fosr_udp_submit(struct fosr *fosr, struct fosr_metrics *metrics)
{
	uint8_t submission_bufffer[6];
	struct addrinfo *res = NULL;
	uint16_t reporter_id;
	int ret = 0;
	int fd = -1;

	if (fosr_udp_submit_getaddr(AF_INET, fosr->config.host, fosr->config.port, &res) < 0) {
		MSG(ERROR, "Failed to resolve host %s\n", fosr->config.host);
		ret = 1;
		goto out_free;
	}

	fd = socket(res->ai_family, res->ai_socktype, 0);
	if (fd < 0) {
		MSG(ERROR, "Failed to create socket\n");
		ret = 1;
		goto out_free;
	}

	/* Protocol version */
	submission_bufffer[0] = 1;

	/* Reporter ID */
	reporter_id = htons(fosr->config.reporter_id);
	memcpy(&submission_bufffer[1], &reporter_id, sizeof(fosr->config.reporter_id));

	/* Metrics */
	submission_bufffer[3] = (uint8_t)metrics->soc;
	submission_bufffer[4] = metrics->charging ? 0xff : 0x00;
	submission_bufffer[5] = (uint8_t)metrics->temperature;

	if (sendto(fd, submission_bufffer, sizeof(submission_bufffer), 0, res->ai_addr, res->ai_addrlen) < 0) {
		/* Don't be spammy, we have retry */
		MSG(DEBUG, "Failed to send UDP packet\n");
		ret = 1;
		goto out_free;
	}

out_free:
	if (fd >= 0) {
		close(fd);
	}

	if (res) {
		freeaddrinfo(res);
	}

	return ret;
}