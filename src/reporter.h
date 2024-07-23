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

    uint16_t reporter_id;
};

enum fosr_submission_reason {
    FOSR_SUBMISSION_REASON_SOC = 1 << 0,
    FOSR_SUBMISSION_REASON_CHARGING = 1 << 1,
    FOSR_SUBMISSION_REASON_INTERVAL = 1 << 2,
};

struct fosr {
    struct ubus_context ctx;

    struct sysinfo sysinfo;

    struct fosr_config config;

    struct {
        struct fosr_metrics current;
        struct fosr_metrics last_submitted;
    } metrics;


    unsigned long last_interval_submission;

    struct uloop_timeout work_timeout;
};