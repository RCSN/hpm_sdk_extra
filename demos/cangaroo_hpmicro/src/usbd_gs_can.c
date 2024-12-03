#include "usbd_core.h"
#include "usbd_gs_can.h"
#include "gs_usb.h"

#define CAN_CLOCK_SPEED 48000000
#define NUM_CAN_CHANNEL 1

const struct gs_device_bt_const_extended CAN_btconst_ext = {
	.feature =
		GS_CAN_FEATURE_LISTEN_ONLY   |
		GS_CAN_FEATURE_LOOP_BACK |
		GS_CAN_FEATURE_IDENTIFY |
		GS_CAN_FEATURE_USER_ID |
		GS_CAN_FEATURE_FD |
        GS_CAN_FEATURE_BT_CONST_EXT 
	,
	.fclk_can = CAN_CLOCK_SPEED,
	.btc = {
		.tseg1_min = 1,
		.tseg1_max = 16,
		.tseg2_min = 1,
		.tseg2_max = 8,
		.sjw_max = 4,
		.brp_min = 1,
		.brp_max = 1024,
		.brp_inc = 1,
	},
	.dbtc = {
		.tseg1_min = 1,
		.tseg1_max = 16,
		.tseg2_min = 1,
		.tseg2_max = 8,
		.sjw_max = 4,
		.brp_min = 1,
		.brp_max = 1024,
		.brp_inc = 1,
	},
};

#ifdef TERM_Pin
enum gs_can_termination_state set_term(unsigned int channel, enum gs_can_termination_state state);
enum gs_can_termination_state get_term(unsigned int channel);

#else
static inline enum gs_can_termination_state set_term(unsigned int channel, enum gs_can_termination_state state)
{
    (void)channel;
    (void)state;
    return GS_CAN_TERMINATION_UNSUPPORTED;
}

static inline enum gs_can_termination_state get_term(unsigned int channel)
{
    (void)channel;
    return GS_CAN_TERMINATION_UNSUPPORTED;
}
#endif

const struct gs_device_bt_const CAN_btconst = {
	.feature =
		GS_CAN_FEATURE_LISTEN_ONLY |
		GS_CAN_FEATURE_LOOP_BACK |
		GS_CAN_FEATURE_ONE_SHOT |
		GS_CAN_FEATURE_HW_TIMESTAMP |
		GS_CAN_FEATURE_IDENTIFY |
        GS_CAN_FEATURE_FD       |
		GS_CAN_FEATURE_PAD_PKTS_TO_MAX_PKT_SIZE
#ifdef TERM_Pin
		| GS_CAN_FEATURE_TERMINATION
#endif
	,
	.fclk_can = CAN_CLOCK_SPEED,
	.btc = {
		.tseg1_min = 1,
		.tseg1_max = 16,
		.tseg2_min = 1,
		.tseg2_max = 8,
		.sjw_max = 4,
		.brp_min = 1,
		.brp_max = 1024,
		.brp_inc = 1,
	},
};

// device info
static const struct gs_device_config USBD_GS_CAN_dconf = {
    .icount = NUM_CAN_CHANNEL - 1,
    .sw_version = 2,
    .hw_version = 1,
};

static int gs_can_vendor_request_handler(uint8_t busid, struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    struct gs_device_termination_state term_state;

    printf("GS CAN Class request: "
                "bRequest 0x%02x\r\n",
                setup->bRequest);

    /*
	 * For all "per device" USB control messages
	 * (GS_USB_BREQ_HOST_FORMAT and GS_USB_BREQ_DEVICE_CONFIG) the
	 * Linux gs_usb driver uses a req->wValue = 1.
	 *
	 * All other control messages are "per channel" and specify the
	 * channel number in req->wValue. So check req->wValue for valid
	 * CAN channel.
	 *
	 */
    // if (!(setup->bRequest == GS_USB_BREQ_HOST_FORMAT ||
    // 	  setup->bRequest == GS_USB_BREQ_DEVICE_CONFIG)) {
    // 	channel = USBD_GS_CAN_GetChannel(hcan, setup->wValue);
    // 	if (!channel) {
    // 		return -1;
    // 	}
    // }

#ifndef CONFIG_CANFD
    switch (setup->bRequest) {
    case GS_USB_BREQ_DATA_BITTIMING:
    case GS_USB_BREQ_BT_CONST_EXT:
        return -1;
    }
#endif

    switch (setup->bRequest) {
    case GS_USB_BREQ_HOST_FORMAT:
        /* The firmware on the original USB2CAN by Geschwister Schneider
        * Technologie Entwicklungs- und Vertriebs UG exchanges all data
        * between the host and the device in host byte order. This is done
        * with the struct gs_host_config::byte_order member, which is sent
        * first to indicate the desired byte order.
        *
        * The widely used open source firmware candleLight doesn't support
        * this feature and exchanges the data in little endian byte order.
        */
        //sizeof(struct gs_host_config);
        break;
    case GS_USB_BREQ_BITTIMING: {
        struct gs_device_bittiming timing;

        memcpy(&timing, *data, sizeof(struct gs_device_bittiming));

        // if (!can_check_bittiming_ok(&CAN_btconst.btc, timing))
        //     goto out_fail;

        // can_set_bittiming(channel, timing);
    }

    break;
    case GS_USB_BREQ_MODE: {
        struct gs_device_mode mode;

        memcpy(&mode, *data, sizeof(struct gs_device_mode));

        // if (mode->mode == GS_CAN_MODE_RESET) {
        //     can_disable(channel);
        //     led_set_mode(&channel->leds, LED_MODE_OFF);
        // } else if (mode->mode == GS_CAN_MODE_START) {
        //     hcan->timestamps_enabled = (mode->flags & GS_CAN_MODE_HW_TIMESTAMP) != 0;
        //     hcan->pad_pkts_to_max_pkt_size = (mode->flags & GS_CAN_MODE_PAD_PKTS_TO_MAX_PKT_SIZE) != 0;

        //     can_enable(channel, mode->flags);

        //     led_set_mode(&channel->leds, LED_MODE_NORMAL);
        // }
    } break;
    case GS_USB_BREQ_BT_CONST:
        *len = sizeof(CAN_btconst);
        memcpy(*data, &CAN_btconst, *len);
        break;
    case GS_USB_BREQ_DEVICE_CONFIG:
        *len = sizeof(USBD_GS_CAN_dconf);
        memcpy(*data, &USBD_GS_CAN_dconf, *len);
        break;
    case GS_USB_BREQ_TIMESTAMP:
        // *len = sizeof(hcan->sof_timestamp_us);
        // memcpy(*data, &hcan->sof_timestamp_us, *len);
        break;
    case GS_USB_BREQ_IDENTIFY: {
        struct gs_identify_mode imode;

        memcpy(&imode, *data, sizeof(struct gs_identify_mode));
        // if (imode->mode) {
        //     led_run_sequence(&channel->leds, led_identify_seq, -1);
        // } else {
        //     led_set_mode(&channel->leds, can_is_enabled(channel) ? LED_MODE_NORMAL : LED_MODE_OFF);
        // }
    } break;
    case GS_USB_BREQ_DATA_BITTIMING: {
        struct gs_device_bittiming timing;
        memcpy(&timing, *data, sizeof(struct gs_device_bittiming));

        // if (!can_check_bittiming_ok(&CAN_btconst_ext.dbtc, timing))
        //     return -1;

        // can_set_data_bittiming(channel, timing);
    } break;
    case GS_USB_BREQ_BT_CONST_EXT:
        *len = sizeof(CAN_btconst_ext);
        memcpy(*data, &CAN_btconst_ext, *len);
        break;
    case GS_USB_BREQ_SET_TERMINATION: {
        if (get_term(setup->wValue) != GS_CAN_TERMINATION_UNSUPPORTED) {
            struct gs_device_termination_state term_state;

            memcpy(&term_state, *data, sizeof(struct gs_device_termination_state));

            if (set_term(setup->wValue, term_state.state) == GS_CAN_TERMINATION_UNSUPPORTED) {
                return -1;
            }
        } else {
            return -1;
        }
    } break;
    case GS_USB_BREQ_GET_TERMINATION: {
        enum gs_can_termination_state state;

        state = get_term(setup->wValue);
        if (state == GS_CAN_TERMINATION_UNSUPPORTED) {
            return -1;
        }

        term_state.state = state;
        *len = sizeof(term_state);
        memcpy(*data, &term_state, *len);
        break;
    }
    default:
        USB_LOG_WRN("Unhandled GS CAN Class bRequest 0x%02x\r\n", setup->bRequest);
        return -1;
    }

    return 0;
}

static void gs_can_notify_handler(uint8_t busid, uint8_t event, void *arg)
{
    switch (event) {
    case USBD_EVENT_RESET:

        break;
    case USBD_EVENT_CONFIGURED:

        break;

    default:
        break;
    }
}

struct usbd_interface *usbd_gs_can_init_intf(uint8_t busid, struct usbd_interface *intf)
{
    intf->class_interface_handler = NULL;
    intf->class_endpoint_handler = NULL;
    intf->vendor_handler = gs_can_vendor_request_handler;
    intf->notify_handler = gs_can_notify_handler;

    return intf;
}