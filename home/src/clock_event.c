/*
 * Samsung API
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Elementary.h>
#include <stdbool.h>
#include <stdio.h>
#include <aul.h>
#include <vconf.h>
#include <efl_assist.h>
#include <dlog.h>

#ifdef ENABLE_MODEM_INDICATOR
#include <tapi_common.h>
#include <ITapiModem.h>
#include <TapiUtility.h>
#endif

#include <pkgmgr-info.h>
#include <package-manager.h>

#include "conf.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "layout.h"
#include "dbus.h"
#include "key.h"
#include "clock_service.h"
#include "power_mode.h"
#include "edit.h"

typedef struct _pkg_event {
	char *pkgname;
	int is_done;
} Pkg_Event;

#define PKGMGR_STR_START "start"
#define PKGMGR_STR_END "end"
#define PKGMGR_STR_OK "ok"
#define PKGMGR_STR_UNINSTALL "uninstall"
#define SAP_BT 0x01

#define VCONFKEY_SAP_CONNECTION_STATUS "memory/private/sap/conn_type"

static struct {
	pkgmgr_client *pkg_client;
	Eina_List *pkg_event_list;

#ifdef ENABLE_MODEM_INDICATOR
	TapiHandle *tapi_handle;
#endif
} s_info = {
	.pkg_client = NULL,
	.pkg_event_list = NULL,

#ifdef ENABLE_MODEM_INDICATOR
	.tapi_handle = NULL,
#endif
};

static Evas_Object *_layout_get(void)
{
	Evas_Object *win = main_get_info()->win;
	retv_if(win == NULL, NULL);

	return evas_object_data_get(win, DATA_KEY_LAYOUT);
}

static void _pkg_event_item_del(Pkg_Event *item_event)
{
	if (item_event != NULL) {
		free(item_event->pkgname);
	}

	free(item_event);
}

static int _is_pkg_event_item_exist(const char *pkgid, int remove_if_exist)
{
	int ret = 0;
	Eina_List *l = NULL;
	Pkg_Event *event_item = NULL;
	retv_if(pkgid == NULL, 0);

	EINA_LIST_FOREACH(s_info.pkg_event_list, l, event_item) {
		if (event_item != NULL) {
			if (strcmp(event_item->pkgname, pkgid) == 0) {
				ret = 1;
				break;
			}
		}
	}

	if (ret == 1 && remove_if_exist == 1) {
		s_info.pkg_event_list = eina_list_remove(s_info.pkg_event_list, event_item);
		_pkg_event_item_del(event_item);
	}

	return ret;
}

static int _pkgmgr_event_cb(int req_id, const char *pkg_type, const char *pkgid,
 const char *key, const char *val, const void *pmsg, void *priv_data)
{
	if (pkgid == NULL) return 0;

	_D("pkg:%s key:%s val:%s", pkgid, key, val);

	if (key != NULL && val != NULL) {
		if (strcasecmp(key, PKGMGR_STR_START) == 0 &&
			strcasecmp(val, PKGMGR_STR_UNINSTALL) == 0) {
			_W("Pkg:%s is being uninstalled", pkgid);

			Pkg_Event *event = calloc(1, sizeof(Pkg_Event));
			if (event != NULL) {
				event->pkgname = strdup(pkgid);
				s_info.pkg_event_list = eina_list_append(s_info.pkg_event_list, event);
			} else {
				_E("failed to create event item");
			}

			return 0;
		} else if (strcasecmp(key, PKGMGR_STR_END) == 0 &&
			strcasecmp(val, PKGMGR_STR_OK) == 0) {
			if (_is_pkg_event_item_exist(pkgid, 1) == 1) {
				_W("Pkg:%s is uninstalled, delete related resource", pkgid);

				clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
				if (clock != NULL && clock->pkgname != NULL) {
					_W("attacheck clock:%s", clock->pkgname);
					if (strcmp(clock->pkgname, pkgid) == 0) {
						_W("clock %s is uninstalled, it will be replaced to default clock");
						clock_service_event_handler(clock, CLOCK_EVENT_APP_PROVIDER_ERROR_FATAL);
					}
				}
				//badge_remove(pkgid);
			}
		}
	}

	return 0;
}

static void _home_editing_cb(void *data, Evas_Object *scroller, void *event_info)
{
	int editing_disabled = (int)data;

	if (editing_disabled == 1) {
		clock_service_request(-1);
	}
}

static key_cb_ret_e _back_key_cb(void *data)
{
	_W("");
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	retv_if(clock == NULL, KEY_CB_RET_CONTINUE);

	clock_view_event_handler(clock, CLOCK_EVENT_DEVICE_BACK_KEY, clock->need_event_relay);

	return KEY_CB_RET_CONTINUE;
}

static void _pm_state_cb(keynode_t * node, void *data)
{
	int val = -1;
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	static int state_prev = VCONFKEY_PM_STATE_LCDOFF;
	ret_if(clock == NULL);

	if(vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
		_E("Failed to get VCONFKEY_PM_STATE");
		return;
	}

	if (state_prev == VCONFKEY_PM_STATE_LCDOFF
		&& val == VCONFKEY_PM_STATE_NORMAL) {
		_D("LCD: off->on");
		clock_view_event_handler(clock, CLOCK_EVENT_DEVICE_LCD_ON, clock->need_event_relay);
	} else if (val == VCONFKEY_PM_STATE_LCDOFF) {
		_D("LCD: on->off");
		clock_view_event_handler(clock, CLOCK_EVENT_DEVICE_LCD_OFF, clock->need_event_relay);
	}

	state_prev = val;
}

static int _pm_state_get(void)
{
	int val = -1;

	if(vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
		_E("Failed to get VCONFKEY_PM_STATE");
		return CLOCK_EVENT_DEVICE_LCD_OFF;
	}

	if (val == VCONFKEY_PM_STATE_NORMAL || val == VCONFKEY_PM_STATE_LCDDIM) {
		return CLOCK_EVENT_DEVICE_LCD_ON;
	} else {
		return CLOCK_EVENT_DEVICE_LCD_OFF;
	}
}

static int _blockingmode_get(void)
{
	int b_mode = 0;

	if(vconf_get_bool(VCONFKEY_SETAPPL_BLOCKMODE_WEARABLE_BOOL, &b_mode) < 0) {
		_E("Failed to get VCONFKEY_SETAPPL_BLOCKMODE_WEARABLE_BOOL:%s", VCONFKEY_SETAPPL_BLOCKMODE_WEARABLE_BOOL);
	}

	_D("b_mode : %d", b_mode);

	if(b_mode) {
		return CLOCK_EVENT_DND_ON;
	} else {
		return CLOCK_EVENT_DND_OFF;
	}
}

static void _blockingmode_cb(keynode_t * node, void *data)
{
	int event = CLOCK_EVENT_SCREEN_READER_OFF;
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	ret_if(clock == NULL);

	event = _blockingmode_get();
	clock_view_event_handler(clock, event, clock->need_event_relay);
}

static int _screen_reader_get(void)
{
	if (clock_util_screen_reader_enabled_get() == 1) {
		return CLOCK_EVENT_SCREEN_READER_ON;
	} else {
		return CLOCK_EVENT_SCREEN_READER_OFF;
	}
}

static void _screen_reader_cb(keynode_t * node, void *data)
{
	int event = CLOCK_EVENT_SCREEN_READER_OFF;
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	ret_if(clock == NULL);

	event = _screen_reader_get();
	clock_view_event_handler(clock, event, clock->need_event_relay);
}

static int _app_status_get(void)
{
	main_s *main_info = main_get_info();
	retv_if(main_info == NULL, CLOCK_EVENT_APP_PAUSE);

	if(main_info->state == APP_STATE_RESUME) {
		return CLOCK_EVENT_APP_RESUME;
	} else if  (main_info->state == APP_STATE_PAUSE) {
		return CLOCK_EVENT_APP_PAUSE;
	}

	return CLOCK_EVENT_APP_PAUSE;
}

#ifdef ENABLE_MODEM_INDICATOR
static int _bt_state_get(void)
{
	int val = -1;

	if(vconf_get_int(VCONFKEY_SAP_CONNECTION_STATUS, &val) < 0) {
		_E("Failed to get VCONFKEY_SAP_CONNECTION_STATUS");
		return CLOCK_EVENT_DEVICE_LCD_OFF;
	}

	if ((val & SAP_BT) == SAP_BT) {
		return CLOCK_EVENT_SAP_ON;
	} else {
		return CLOCK_EVENT_SAP_OFF;
	}
}

static void _bt_state_cb(keynode_t * node, void *data)
{
	int event = CLOCK_EVENT_SAP_OFF;
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	ret_if(clock == NULL);

	event = _bt_state_get();
	clock_view_event_handler(clock, event, clock->need_event_relay);
}

static int _modem_state_get(void)
{
	int ret = 0;
	int current = 0;
	retv_if(s_info.tapi_handle == NULL, CLOCK_EVENT_MODEM_OFF);

	if ((ret = tel_get_property_int(s_info.tapi_handle, TAPI_PROP_MODEM_POWER, &current)) != TAPI_API_SUCCESS) {
		_E("Failed to get modem power property:%d", ret);
	}

	/*
	 * (0=on,1=off,2=err)
	 */
	_D("current:%d", current);

	if (current == 0) {
		return CLOCK_EVENT_MODEM_ON;
	} else {
		return CLOCK_EVENT_MODEM_OFF;
	}
}

static void _on_noti_modem_power(TapiHandle *handle, const char *noti_id, void *data, void *user_data)
{
	int *status = data;
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	ret_if(clock == NULL);

	_W("status:%x of event:%s receive", TAPI_NOTI_MODEM_POWER);

	if (status == 0) {
		clock_view_event_handler(clock, CLOCK_EVENT_MODEM_ON, clock->need_event_relay);
	} else {
		clock_view_event_handler(clock, CLOCK_EVENT_MODEM_OFF, clock->need_event_relay);
	}
}
#endif

static int _event_app_dead_cb(int pid)
{
	clock_h clock = NULL;

	clock = clock_manager_clock_get(CLOCK_ATTACHED);
	if (clock != NULL) {
		if (clock->pid == pid) {
			_E("clock %s is closed unexpectedly", clock->pkgname);
			clock_service_event_handler(clock, CLOCK_EVENT_APP_PROVIDER_ERROR);
		}
	}

	return 0;
}

static w_home_error_e _event_app_language_changed_cb(void *data)
{
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	retv_if(clock == NULL, W_HOME_ERROR_FAIL);

	clock_view_event_handler(clock, CLOCK_EVENT_APP_LANGUAGE_CHANGED, clock->need_event_relay);

	return W_HOME_ERROR_NONE;
}

static int _power_mode_state_get(void)
{
	if (emergency_mode_enabled_get() == 1) {
		return CLOCK_EVENT_POWER_ENHANCED_MODE_ON;
	} else {
		return CLOCK_EVENT_POWER_ENHANCED_MODE_OFF;
	}
}

static void _enhanced_power_mode_on_cb(void *user_data, void *event_info)
{
	_D("");
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	ret_if(clock == NULL);

	clock_view_event_handler(clock, CLOCK_EVENT_POWER_ENHANCED_MODE_ON, clock->need_event_relay);
}

static void _enhanced_power_mode_off_cb(void *user_data, void *event_info)
{
	_D("");
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	ret_if(clock == NULL);

	clock_view_event_handler(clock, CLOCK_EVENT_POWER_ENHANCED_MODE_OFF, clock->need_event_relay);
}

static int _simcard_state_get(void) {
	int ret = 0;
	int sim_status = VCONFKEY_TELEPHONY_SIM_UNKNOWN;

	ret = vconf_get_int(VCONFKEY_TELEPHONY_SIM_SLOT, &sim_status);
	if (ret == 0 && sim_status == VCONFKEY_TELEPHONY_SIM_INSERTED) {
		return CLOCK_EVENT_SIM_INSERTED;
	} else {
		_E("Failed to get simcard status");
	}

	return CLOCK_EVENT_SIM_NOT_INSERTED;
}

static void _simcard_state_cb(keynode_t * node, void *data)
{
	int event = CLOCK_EVENT_SIM_INSERTED;
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	ret_if(clock == NULL);

	event = _simcard_state_get();

	clock_view_event_handler(clock, event, clock->need_event_relay);
}

HAPI void clock_service_event_app_dead_cb(int pid)
{
	_event_app_dead_cb(pid);
	clock_shortcut_app_dead_cb(pid);
}

HAPI void clock_service_event_register(void)
{
	int ret = 0;
	Evas_Object *layout = _layout_get();
	if (layout != NULL) {
		evas_object_smart_callback_add(layout, LAYOUT_SMART_SIGNAL_EDIT_OFF,
			_home_editing_cb, (void *)1);
	}

	main_register_cb(APP_STATE_LANGUAGE_CHANGED, _event_app_language_changed_cb, NULL);

	key_register_cb(KEY_TYPE_BACK, _back_key_cb, NULL);

	power_mode_register_cb(POWER_MODE_ENHANCED_ON, _enhanced_power_mode_on_cb, NULL);
	power_mode_register_cb(POWER_MODE_ENHANCED_OFF, _enhanced_power_mode_off_cb, NULL);

	if(vconf_notify_key_changed(VCONFKEY_PM_STATE, _pm_state_cb, NULL) < 0) {
		_E("Failed to register the pm status callback");
	}
	if(vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, _screen_reader_cb, NULL) < 0) {
		_E("Failed to register the screen reader status callback");
	}
#ifdef ENABLE_MODEM_INDICATOR
	if(vconf_notify_key_changed(VCONFKEY_SAP_CONNECTION_STATUS, _bt_state_cb, NULL) < 0) {
		_E("Failed to register sap status callback");
	}
	TapiHandle *handle = tel_init(NULL);
	if (handle != NULL) {
		ret =  tel_register_noti_event(handle, TAPI_NOTI_MODEM_POWER, _on_noti_modem_power, NULL);
		if (ret != TAPI_API_SUCCESS) {
			_E("event register failed(%d)", ret);
		}
		s_info.tapi_handle = handle;
	} else {
		_E("Failed to create tapi handle");
	}
#endif
	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_BLOCKMODE_WEARABLE_BOOL,
		_blockingmode_cb, NULL) < 0) {
		_E("Failed to get notification from VCONFKEY_SETAPPL_BLOCKMODE_WEARABLE_BOOL");
	}

	pkgmgr_client *client = pkgmgr_client_new(PC_LISTENING);
	if (client != NULL) {
		if ((ret = pkgmgr_client_listen_status(client, _pkgmgr_event_cb, NULL)) != PKGMGR_R_OK) {
			_E("Failed to listen pkgmgr event:%d", ret);
		}
		s_info.pkg_client = client;
	} else {
		_E("Failed to create package manager client");
	}
	if(vconf_notify_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT, _simcard_state_cb, NULL) < 0) {
		_E("Failed to register simcard status callback");
	}
}

HAPI void clock_service_event_deregister(void)
{
	Evas_Object *layout = _layout_get();
	if (layout != NULL) {
		evas_object_smart_callback_del(layout, LAYOUT_SMART_SIGNAL_EDIT_OFF,
			_home_editing_cb);
	}

	main_unregister_cb(APP_STATE_LANGUAGE_CHANGED, _event_app_language_changed_cb);

	key_unregister_cb(KEY_TYPE_BACK, _back_key_cb);

	power_mode_unregister_cb(POWER_MODE_ENHANCED_ON, _enhanced_power_mode_on_cb);
	power_mode_unregister_cb(POWER_MODE_ENHANCED_OFF, _enhanced_power_mode_off_cb);

	if(vconf_ignore_key_changed(VCONFKEY_PM_STATE, _pm_state_cb) < 0) {
		_E("Failed to ignore the pm status callback");
	}
	if(vconf_ignore_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, _screen_reader_cb) < 0) {
		_E("Failed to ignore the screen reader callback");
	}
#ifdef ENABLE_MODEM_INDICATOR
	if(vconf_ignore_key_changed(VCONFKEY_SAP_CONNECTION_STATUS, _bt_state_cb) < 0) {
		_E("Failed to ignore the sap status callback");
	}
	if (s_info.tapi_handle != NULL) {
		tel_deinit(s_info.tapi_handle);
		s_info.tapi_handle = NULL;
	}
#endif
	if (vconf_ignore_key_changed(VCONFKEY_SETAPPL_BLOCKMODE_WEARABLE_BOOL,
		_blockingmode_cb) < 0) {
		_E("Failed to ignore the VCONFKEY_SETAPPL_BLOCKMODE_WEARABLE_BOOL callback");
	}
	if(vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT, _simcard_state_cb) < 0) {
		_E("Failed to ignore the simcard status callback");
	}

	int ret = 0;
	Pkg_Event *event_item = NULL;
	if (s_info.pkg_client != NULL) {
		if ((ret = pkgmgr_client_free(s_info.pkg_client)) != PKGMGR_R_OK) {
			_E("Failed to free pkgmgr client:%d", ret);
		}
		s_info.pkg_client = NULL;
	}
	EINA_LIST_FREE(s_info.pkg_event_list, event_item) {
		_pkg_event_item_del(event_item);
	}
	s_info.pkg_event_list = NULL;
}

HAPI int clock_service_event_state_get(int event_source)
{
	switch (event_source) {
		case CLOCK_EVENT_SCREEN_READER:
			return _screen_reader_get();
		case CLOCK_EVENT_APP:
			return _app_status_get();
		case CLOCK_EVENT_DEVICE_LCD:
			return _pm_state_get();
#ifdef ENABLE_MODEM_INDICATOR
		case CLOCK_EVENT_MODEM:
			return _modem_state_get();
		case CLOCK_EVENT_SAP:
			return _bt_state_get();
#endif
		case CLOCK_EVENT_DND:
			return _blockingmode_get();
		case CLOCK_EVENT_POWER:
			return _power_mode_state_get();
		case CLOCK_EVENT_SIM:
			return _simcard_state_get();
	}

	return event_source;
}
