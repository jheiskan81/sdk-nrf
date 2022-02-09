/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define LOG_MODULE_NAME net_lwm2m_obj_loc_assistance
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include <net/lwm2m_client_utils.h>
#include <modem/location.h>

struct lwm2m_ctx *lwm2m_app_ctx_ptr_get(void);

#define LOCATION_ASSIST_VERSION_MAJOR 1
#define LOCATION_ASSIST_VERSION_MINOR 0

/* Location Assistance resource IDs */
#define LOCATION_ASSIST_ASSIST_TYPE			0
#define LOCATION_ASSIST_AGPS_MASK			1
#define LOCATION_ASSIST_PGPS_PRED_COUNT			2
#define LOCATION_ASSIST_PGPS_PRED_INTERVAL		3
#define LOCATION_ASSIST_PGPS_START_GPS_DAY		4
#define LOCATION_ASSIST_PGPS_START_GPS_TIME_OF_DAY	5
#define LOCATION_ASSIST_ASSIST_DATA			6
#define LOCATION_ASSIST_RESULT_CODE			7
#define LOCATION_ASSIST_LATITUDE			8
#define LOCATION_ASSIST_LONGITUDE			9
#define LOCATION_ASSIST_ALTITUDE			10
#define LOCATION_ASSIST_ACCURACY			11

#define LOCATION_ASSIST_MAX_ID				12

#define LOCATION_ASSIST_NEEDS_UTC			BIT(0)
#define LOCATION_ASSIST_NEEDS_EPHEMERIES		BIT(1)
#define LOCATION_ASSIST_NEEDS_ALMANAC			BIT(2)
#define LOCATION_ASSIST_NEEDS_KLOBUCHAR			BIT(3)
#define LOCATION_ASSIST_NEEDS_NEQUICK			BIT(4)
#define LOCATION_ASSIST_NEEDS_TOW			BIT(5)
#define LOCATION_ASSIST_NEEDS_CLOCK			BIT(6)
#define LOCATION_ASSIST_NEEDS_LOCATION			BIT(7)
#define LOCATION_ASSIST_NEEDS_INTEGRITY			BIT(8)

static int32_t assist_type;
static int32_t agps_mask;
static int32_t pgps_pred_count;
static int32_t pgps_pred_interval;
static int32_t pgps_start_gps_day;
static int32_t pgps_start_gps_time_of_day;
/* Assistance data */
static char assist_buf[4096];


static int32_t result;
static float latitude;
static float longitude;
static float altitude;
static float accuracy;


/*
 * Calculate resource instances as follows:
 * start with LOCATION_ASSIST_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT	(LOCATION_ASSIST_MAX_ID)

static struct lwm2m_engine_obj location_assistance;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(LOCATION_ASSIST_ASSIST_TYPE, R, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_AGPS_MASK, R_OPT, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_PGPS_PRED_COUNT, R_OPT, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_PGPS_PRED_INTERVAL, R_OPT, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_PGPS_START_GPS_DAY, R_OPT, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_PGPS_START_GPS_TIME_OF_DAY, R_OPT, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_ASSIST_DATA, W, OPAQUE),
	OBJ_FIELD_DATA(LOCATION_ASSIST_RESULT_CODE, W, S32),
	OBJ_FIELD_DATA(LOCATION_ASSIST_LATITUDE, W, FLOAT),
	OBJ_FIELD_DATA(LOCATION_ASSIST_LONGITUDE, W, FLOAT),
	OBJ_FIELD_DATA(LOCATION_ASSIST_ALTITUDE, W, FLOAT),
	OBJ_FIELD_DATA(LOCATION_ASSIST_ACCURACY, W, FLOAT),
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[LOCATION_ASSIST_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_INSTANCE_COUNT];

static void *get_assist_buf(uint16_t obj_inst_id, uint16_t res_id,
			      uint16_t res_inst_id, size_t *data_len)
{
	*data_len = sizeof(assist_buf);
	return assist_buf;
}

static int location_assist_write_cb(uint16_t obj_inst_id, uint16_t res_id,
			    uint16_t res_inst_id, uint8_t *data, uint16_t data_len,
			    bool last_block, size_t total_size)
{
	LOG_INF("Writing assistance data %u", data_len);
	int err = 0;

	err = location_agps_data_process(data, data_len);
	if (err) {
		LOG_WRN("Unable to process A-GPS data, error: %d", err);
	} else {
		LOG_INF("A-GPS data processed");
	}
	return err;
}

static struct lwm2m_engine_obj_inst *location_assist_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	init_res_instance(res_inst, ARRAY_SIZE(res_inst));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_ASSIST_TYPE, res, i, res_inst, j,
			  &assist_type, sizeof(assist_type));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_AGPS_MASK, res, i, res_inst, j,
			  &agps_mask, sizeof(agps_mask));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_PGPS_PRED_COUNT, res, i, res_inst, j,
			  &pgps_pred_count, sizeof(pgps_pred_count));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_PGPS_PRED_INTERVAL, res, i, res_inst, j,
			  &pgps_pred_interval, sizeof(pgps_pred_interval));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_PGPS_START_GPS_DAY, res, i, res_inst, j,
			  &pgps_start_gps_day, sizeof(pgps_start_gps_day));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_PGPS_START_GPS_TIME_OF_DAY, res, i, res_inst, j,
			  &pgps_start_gps_time_of_day, sizeof(pgps_start_gps_time_of_day));
	INIT_OBJ_RES_OPT(LOCATION_ASSIST_ASSIST_DATA, res, i, res_inst, j, 1, false, true, NULL,
			 get_assist_buf, NULL, location_assist_write_cb, NULL);
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_RESULT_CODE, res, i, res_inst, j,
			  &result, sizeof(result));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_LATITUDE, res, i, res_inst, j,
			  &latitude, sizeof(latitude));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_LONGITUDE, res, i, res_inst, j,
			  &longitude, sizeof(longitude));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_ALTITUDE, res, i, res_inst, j,
			  &altitude, sizeof(altitude));
	INIT_OBJ_RES_DATA(LOCATION_ASSIST_ACCURACY, res, i, res_inst, j,
			  &accuracy, sizeof(accuracy));

	inst.resources = res;
	inst.resource_count = i;
	LOG_INF("Created a location assistance object: %d", obj_inst_id);
	return &inst;
}

static int lwm2m_location_assist_init(const struct device *dev)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	location_assistance.obj_id = LOCATION_ASSIST_OBJECT_ID;
	location_assistance.version_major = LOCATION_ASSIST_VERSION_MAJOR;
	location_assistance.version_minor = LOCATION_ASSIST_VERSION_MINOR;
	location_assistance.is_core = false;
	location_assistance.fields = fields;
	location_assistance.field_count = ARRAY_SIZE(fields);
	location_assistance.max_instance_count = 1U;
	location_assistance.create_cb = location_assist_create;
	lwm2m_register_obj(&location_assistance);

	ret = lwm2m_create_obj_inst(LOCATION_ASSIST_OBJECT_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_ERR("Create location assist object error: %d", ret);
	}

	return 0;
}

int lwm2m_location_assist_request(struct nrf_modem_gnss_agps_data_frame *agps)
{
	LOG_INF("Requesting A-GPS data, ephe 0x%08x, alm 0x%08x, flags 0x%02x",
		agps->sv_mask_ephe, agps->sv_mask_alm, agps->data_flags);

	uint32_t mask = 0;

	if (agps->data_flags & NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_UTC;
	}
	if (agps->sv_mask_ephe) {
		mask |= LOCATION_ASSIST_NEEDS_EPHEMERIES;
	}
	if (agps->sv_mask_alm) {
		mask |= LOCATION_ASSIST_NEEDS_ALMANAC;
	}
	if (agps->data_flags & NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) {

		mask |= LOCATION_ASSIST_NEEDS_KLOBUCHAR;
	}
	if (agps->data_flags & NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_NEQUICK;
	}
	if (agps->data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_TOW;
		mask |= LOCATION_ASSIST_NEEDS_CLOCK;
	}
	if (agps->data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_LOCATION;
	}
	if (agps->data_flags & NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_INTEGRITY;
	}
	/* Store mask to object resource */
	lwm2m_engine_set_s32(LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, LOCATION_ASSIST_AGPS_MASK),
			     mask);
	lwm2m_engine_set_s32(LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, LOCATION_ASSIST_ASSIST_TYPE),
			     5);

	char const *send_path[7] = {
		LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, LOCATION_ASSIST_ASSIST_TYPE),
		LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, LOCATION_ASSIST_AGPS_MASK),
		LWM2M_PATH(4, 0, 8), /* ECI */
		LWM2M_PATH(4, 0, 9), /* MNC */
		LWM2M_PATH(4, 0, 10), /* MCC */
		LWM2M_PATH(4, 0, 2), /* RSRP */
		LWM2M_PATH(4, 0, 12) /* LAC */
	};
	lwm2m_engine_send(lwm2m_app_ctx_ptr_get(), send_path, 7,
		      true);

	/* Send Request to server */
	return 0;
}

SYS_INIT(lwm2m_location_assist_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
