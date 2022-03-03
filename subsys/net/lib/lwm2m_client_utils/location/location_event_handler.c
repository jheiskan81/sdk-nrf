/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/gps.h>

#include <net/lwm2m_path.h>
#include <net/lwm2m_client_utils_location.h>
#include <net/lwm2m_client_utils.h>

#define MODULE lwm2m_location_event_handler

static struct lwm2m_ctx *client_ctx;

#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
#define CELL_LOCATION_PATHS 6
#else
#define CELL_LOCATION_PATHS 5
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_LWM2M_LOG_LEVEL);

static bool handle_agps_request(const struct gps_agps_request_event *event)
{
	uint32_t mask = 0;

	if (event->agps_req.utc) {
		mask |= LOCATION_ASSIST_NEEDS_UTC;
	}
	if (event->agps_req.sv_mask_ephe) {
		mask |= LOCATION_ASSIST_NEEDS_EPHEMERIES;
	}
	if (event->agps_req.sv_mask_alm) {
		mask |= LOCATION_ASSIST_NEEDS_ALMANAC;
	}
	if (event->agps_req.klobuchar) {

		mask |= LOCATION_ASSIST_NEEDS_KLOBUCHAR;
	}
	if (event->agps_req.nequick) {
		mask |= LOCATION_ASSIST_NEEDS_NEQUICK;
	}
	if (event->agps_req.system_time_tow) {
		mask |= LOCATION_ASSIST_NEEDS_TOW;
		mask |= LOCATION_ASSIST_NEEDS_CLOCK;
	}
	if (event->agps_req.position) {
		mask |= LOCATION_ASSIST_NEEDS_LOCATION;
	}
	if (event->agps_req.integrity) {
		mask |= LOCATION_ASSIST_NEEDS_INTEGRITY;
	}

	location_assist_agps_request_set(mask);

	char const *send_path[7] = {
	LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, LOCATION_ASSIST_ASSIST_TYPE),
	LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, LOCATION_ASSIST_AGPS_MASK),
	LWM2M_PATH(4, 0, 8), /* ECI */
	LWM2M_PATH(4, 0, 9), /* MNC */
	LWM2M_PATH(4, 0, 10), /* MCC */
	LWM2M_PATH(4, 0, 2), /* RSRP */
	LWM2M_PATH(4, 0, 12) /* LAC */
	};

	/* Send Request to server */
	lwm2m_engine_send(client_ctx, send_path, 7,
				true);
}

static bool handle_cell_location_request()
{
	LOG_INF("Got cell location request event");

	location_assist_cell_request_set();

	char const *send_path[CELL_LOCATION_PATHS] = {
	LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, LOCATION_ASSIST_ASSIST_TYPE),
	LWM2M_PATH(4, 0, 8), /* ECI */
	LWM2M_PATH(4, 0, 9), /* MNC */
	LWM2M_PATH(4, 0, 10), /* MCC */
	LWM2M_PATH(4, 0, 12), /* LAC */
#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
	LWM2M_PATH(ECID_SIGNAL_MEASUREMENT_INFO_OBJECT_ID)
#endif
	};

	/* Send Request to server */
	lwm2m_engine_send(client_ctx, send_path, CELL_LOCATION_PATHS,
				true);
}

static bool event_handler(const struct event_header *eh)
{
	if (client_ctx == 0) {
		return false;
	}

	if (is_gps_agps_request_event(eh)) {
		LOG_INF("Got agps request event");
		struct gps_agps_request_event *event = cast_gps_agps_request_event(eh);
		handle_agps_request(event);
		return true;
	} else if(is_cell_location_request_event(eh)) {
		handle_cell_location_request();
		return true;
	}

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, gps_agps_request_event);
EVENT_SUBSCRIBE(MODULE, cell_location_request_event);

int location_event_handler_init(struct lwm2m_ctx *ctx)
{
	int err = 0;

	if(client_ctx == 0) {
		client_ctx = ctx;
	} else {
		LOG_ERR("Already initialized");
		err = -EALREADY;
	}

	return err;
}
