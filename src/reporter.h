#pragma once

#include <stdlib.h>
#include <stdint.h>

#include <sys/sysinfo.h>

#include <libubus.h>

struct fosr_metrics {
    uint16_t soc;
    uint8_t charging;
    double temperature;
};

struct fosr_config {
    const char *host;
    uint16_t port;
    int32_t interval;
};


struct fosr {
    struct ubus_context ctx;

    struct sysinfo sysinfo;

    struct fosr_config config;

    struct fosr_metrics metrics;
    uint8_t metrics_updated;

    uint8_t pending_submission;
    unsigned long last_submission;

    struct uloop_timeout work_timeout;
};