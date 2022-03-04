/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/lwm2m_client_utils_location.h>

#include <stdio.h>

static int log_gps_agps_request_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	struct gps_agps_request_event *event = cast_gps_agps_request_event(eh);
	
	EVENT_MANAGER_LOG(eh, "got agps request event");
	return 0;
}

EVENT_TYPE_DEFINE(gps_agps_request_event, false, log_gps_agps_request_event, NULL);

static int log_cell_location_request_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	EVENT_MANAGER_LOG(eh, "got cell location request event");
	return 0;
}

EVENT_TYPE_DEFINE(cell_location_request_event, false, log_cell_location_request_event, NULL);

static int log_cell_location_inform_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	EVENT_MANAGER_LOG(eh, "got cell location inform event");
	return 0;
}

EVENT_TYPE_DEFINE(cell_location_inform_event, false, log_cell_location_inform_event, NULL);
