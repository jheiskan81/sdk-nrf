/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Ensure 'strnlen' is available even with -std=c99. If
 * _POSIX_C_SOURCE was defined we will get a warning when referencing
 * 'strnlen'. If this proves to cause trouble we could easily
 * re-implement strnlen instead, perhaps with a different name, as it
 * is such a simple function.
 */
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include <string.h>

#include <zephyr/kernel.h>
#include <pm_config.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/mcumgr_client.h>
#include <dfu/dfu_target.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(dfu_target_smp, CONFIG_DFU_TARGET_LOG_LEVEL);

#define MCUBOOT_HEADER_MAGIC 0x96f3b83d


struct upload_progress {
	size_t image_size;
	size_t offset;
        uint32_t image_num;
        int smp_status;
};

static struct upload_progress upload_state;
static struct mcumgr_image_data *image_info;
static int image_count;

static void img_state_res_fn(int status, struct mcumgr_image_state image_state)
{
	if (status) {
		LOG_ERR("Image state response command fail, err:%d", status);
		image_count = 0;
	} else {
		image_count = image_state.image_list_lenght;
		image_info = image_state.image_list;
	}
}

static void img_upload_res_fn(int status, size_t offset)
{
    if (status) {
	LOG_ERR("Image Upload command fail err:%d", status);
    } else {
	upload_state.offset = offset;
    }
}

static void client_gr_image_fn(struct mcumgr_client_image_gr_rsp *resp)
{
    upload_state.smp_status = resp->status;
    switch (resp->cmd) {
    case MGMT_IMAGE_STATE_RSP:
	img_state_res_fn(resp->status, resp->image_state);
	break;

    case MGMT_IMAGE_UPLOAD_RSP:
	img_upload_res_fn(resp->status, resp->image_uppload_offset);
	break;

    /** Secondary image erase response */
    case MGMT_IMAGE_ERASE_RSP:
	LOG_INF("Image Erase command status:%d", resp->status);
	break;
    }
}

bool dfu_target_smp_identify(const void *const buf)
{
	/* MCUBoot headers starts with 4 byte magic word */
	return *((const uint32_t *)buf) == MCUBOOT_HEADER_MAGIC;
}

int dfu_target_smp_init(size_t file_size, int img_num, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
        if (img_num < 0) {
                return -ENOENT;
        }

        upload_state.image_size = file_size;
        upload_state.offset = 0;
        upload_state.image_num = (uint32_t)img_num;
	mcumgr_client_image_upload_init(file_size);
        mcumgr_client_image_callback_register(client_gr_image_fn);

	return 0;
}

int dfu_target_smp_offset_get(size_t *out)
{
	*out = upload_state.offset;
	return 0;
}

int dfu_target_smp_write(const void *const buf, size_t len)
{
        int err = 0;

        err = mcumgr_client_image_upload(buf, len);
        if (err) {
                return err;
        }

        return upload_state.smp_status;
}

int dfu_target_smp_done(bool successful)
{
	int err = 0;

	if (successful) {
		/* Read Image list */
		LOG_INF("MCUBoot SMP image download ready.");
		err = mcumgr_client_image_state_read();
		mcumgr_client_image_list_print();
	} else {
		/* Erase Image */
		LOG_INF("MCUBoot SMP image upgrade aborted %d", upload_state.image_num);
		err = mcumgr_client_image_erase(upload_state.image_num);

	}
	upload_state.image_size = 0;
	upload_state.offset = 0;

	if (err) {
		return err;
	}

	return upload_state.smp_status;
}

int dfu_target_smp_schedule_update(int img_num)
{
        int err = 0;

        /*  Test functionality here */
        if (!image_info || !image_count  || (image_count -1) < img_num) {
                return -ENOENT;
        }

        LOG_INF("MCUBoot image-%d upgrade scheduled. "
			"Reset device to apply", img_num);

        err = mcumgr_client_image_state_write(image_info[img_num].hash, 32, false);
        if (err) {
                return err;
        }
	mcumgr_client_image_list_print();

        return upload_state.smp_status;

}

int dfu_target_smp_reset(void)
{
	int err;

        upload_state.image_size = 0;
        upload_state.offset = 0;

	err = mcumgr_client_image_erase(upload_state.image_num);
	if (err) {
		LOG_INF("SMP reset: fail: %d", err);
		return err;
	}
	LOG_INF("SMP reset: status %d", upload_state.smp_status);
	return upload_state.smp_status;
}


static void client_gr_os_fn(struct mcumgr_client_os_gr_rsp *resp)
{

	switch (resp->cmd) {
	case MGMT_OS_ECHO_RSP:
		break;

	/** Image Upload response*/
	case MGMT_OS_RESET_RSP:
		upload_state.smp_status = resp->status;
		LOG_INF("OS Reset command status:%d", resp->status);
		break;
	}
}

int dfu_target_smp_reset_command(void)
{
	int err;
	int count = 0;

	mcumgr_client_os_callback_register(client_gr_os_fn);
	err = mcumgr_client_os_reset();
	if (err) {
                return err;
        }

        if (upload_state.smp_status) {
		return upload_state.smp_status;
	}

	while (1) {
		if (count++ > 10) {
			LOG_ERR("Image not swapped");
			return -ENOENT;
		}
		k_sleep(K_SECONDS(1));
		LOG_INF("Wait image list");
		err = mcumgr_client_image_state_read();

		if (err) {
			return err;
		}

		if (upload_state.smp_status) {
			return upload_state.smp_status;
		}

		if (!image_info || !image_count) {
			LOG_ERR("IMG count %d", image_count);
			return -ENOENT;
		}
		if (!image_info[0].confirmed) {
			break;
		}
	}

	mcumgr_client_image_list_print();
	LOG_INF("Confirm Image");

        err = mcumgr_client_image_state_write(NULL, 0, true);
	if (err) {
                return err;
        }

	if (upload_state.smp_status) {
		LOG_INF("Confirm faul err:%d", upload_state.smp_status);
		return upload_state.smp_status;
	}
	mcumgr_client_image_list_print();

        return 0;
}
