#include <stdint.h>
#include <libubus.h>

#include "log.h"
#include "reporter.h"
#include "ubus.h"
#include "reporter.h"

#define UBUS_TIMEOUT 1000

static struct blob_buf b;

static void fosr_ubus_battery_status_update_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
	struct fosr *fosr = req->priv;
	struct fosr_metrics metrics = {0};

	enum {
		UBUS_BATTERY_STATUS_SOC,
		UBUS_BATTERY_STATUS_CHARGING,
		UBUS_BATTERY_STATUS_TEMPERATURE,
		__UBUS_BATTERY_STATUS_MAX
	};

	static struct blobmsg_policy policy[__UBUS_BATTERY_STATUS_MAX] = {
		[UBUS_BATTERY_STATUS_SOC] = { .name = "soc", .type = BLOBMSG_TYPE_INT16 },
		[UBUS_BATTERY_STATUS_CHARGING] = { .name = "charging", .type = BLOBMSG_TYPE_INT8 },
		[UBUS_BATTERY_STATUS_TEMPERATURE] = { .name = "temperature", .type = BLOBMSG_TYPE_DOUBLE },
	};

	struct blob_attr *tb[__UBUS_BATTERY_STATUS_MAX];

	MSG(DEBUG, "Battery status update len=%d\n", blob_len(msg));

	blobmsg_parse(policy, __UBUS_BATTERY_STATUS_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[UBUS_BATTERY_STATUS_SOC]) {
		MSG(ERROR, "Failed to parse battery status soc\n");
	}

	if (!tb[UBUS_BATTERY_STATUS_CHARGING]) {
		MSG(ERROR, "Failed to parse battery status charging\n");
	}

	if (!tb[UBUS_BATTERY_STATUS_TEMPERATURE]) {
		MSG(ERROR, "Failed to parse battery status temperature\n");
	}

	if (!tb[UBUS_BATTERY_STATUS_SOC] || !tb[UBUS_BATTERY_STATUS_CHARGING] || !tb[UBUS_BATTERY_STATUS_TEMPERATURE]) {
		MSG(ERROR, "Failed to parse battery status\n");
		return;
	}

	metrics.soc = blobmsg_get_u16(tb[UBUS_BATTERY_STATUS_SOC]);
	metrics.charging = blobmsg_get_u8(tb[UBUS_BATTERY_STATUS_CHARGING]);
	metrics.temperature = blobmsg_get_double(tb[UBUS_BATTERY_STATUS_TEMPERATURE]);

	MSG(DEBUG, "Battery status: soc=%u, charging=%u, temperature=%.2f\n", metrics.soc, metrics.charging, metrics.temperature);

	if (metrics.charging != fosr->metrics.charging) {
		MSG(INFO, "Charging status changed: charging=%u\n", metrics.charging);
		fosr->metrics_updated = 1;
	}

	if (!metrics.charging && metrics.soc != fosr->metrics.soc) {
		MSG(INFO, "SOC changed on battery: soc=%u\n", metrics.soc);
		fosr->metrics_updated = 1;
	}

	memcpy(&fosr->metrics, &metrics, sizeof(metrics));
}

int fosr_ubus_battery_status_update(struct fosr *fosr)
{
	uint32_t ubus_id;

	if (ubus_lookup_id(&fosr->ctx, "battery", &ubus_id)) {
		MSG(ERROR, "Failed to lookup ubus id for battery\n");
		return -1;
	}

	MSG(DEBUG, "Battery ubus_id: %u\n", ubus_id);
	
	blob_buf_init(&b, 0);
	return ubus_invoke(&fosr->ctx, ubus_id, "info", b.head, fosr_ubus_battery_status_update_cb, fosr, UBUS_TIMEOUT);
}

int fosr_ubus_connect(struct fosr *fosr)
{
	int ret;

	blob_buf_init(&b, 0);

	ret = ubus_connect_ctx(&fosr->ctx, NULL);
	if (ret) {
		MSG(ERROR, "Failed to connect to ubus: %s\n", ubus_strerror(ret));
		goto out_free;
	}

out_free:
	return ret;
}