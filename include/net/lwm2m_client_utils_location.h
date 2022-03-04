/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file lwm2m_client_utils_location.h
 * @defgroup lwm2m_client_utils_location LwM2M LOCATION
 * @ingroup lwm2m_client_utils
 * @{
 * @brief API for the LwM2M based LOCATION
 */

#ifndef LWM2M_CLIENT_UTILS_LOCATION_H__
#define LWM2M_CLIENT_UTILS_LOCATION_H__

#include <zephyr.h>
#include <drivers/gps.h>
#include <net/lwm2m.h>
#include <event_manager.h>
#include <event_manager_profiler_tracer.h>

#define LOCATION_ASSIST_NEEDS_UTC			BIT(0)
#define LOCATION_ASSIST_NEEDS_EPHEMERIES		BIT(1)
#define LOCATION_ASSIST_NEEDS_ALMANAC			BIT(2)
#define LOCATION_ASSIST_NEEDS_KLOBUCHAR			BIT(3)
#define LOCATION_ASSIST_NEEDS_NEQUICK			BIT(4)
#define LOCATION_ASSIST_NEEDS_TOW			BIT(5)
#define LOCATION_ASSIST_NEEDS_CLOCK			BIT(6)
#define LOCATION_ASSIST_NEEDS_LOCATION			BIT(7)
#define LOCATION_ASSIST_NEEDS_INTEGRITY			BIT(8)

struct gps_agps_request_event {
	struct event_header header;

	struct gps_agps_request agps_req;
};

EVENT_TYPE_DECLARE(gps_agps_request_event);

struct cell_location_request_event {
	struct event_header header;
};

EVENT_TYPE_DECLARE(cell_location_request_event);

struct cell_location_inform_event {
	struct event_header header;
};

EVENT_TYPE_DECLARE(cell_location_inform_event);

int location_event_handler_init(struct lwm2m_ctx *ctx);

#endif /* LWM2M_CLIENT_UTILS_LOCATION_H__ */
