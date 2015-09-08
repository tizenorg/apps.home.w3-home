/*
 * Samsung API
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Elementary.h>
#include <Evas.h>
#include <stdbool.h>
#include <vconf.h>
#include <bundle.h>
#include <app.h>
#include <aul.h>
#include <efl_assist.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
#include <system_settings.h>
#include <dlog.h>

#include "conf.h"
#include "layout.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "page_info.h"
#include "scroller_info.h"
#include "page.h"
#include "scroller.h"
#include "key.h"
#include "effect.h"
#include "minictrl.h"
#include "clock_service.h"
#include "power_mode.h"
#include "dbus.h"
#include "tutorial.h"

#define PRIVATE_DATA_KEY_VISIBLE "p_d_k_v"

#ifdef RUN_ON_DEVICE

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
static Evas_Object *_hidden_layout_get(Evas_Object *page)
{
	retv_if(page == NULL, NULL);
	Evas_Object *item = page_get_item(page);
	retv_if(item == NULL, NULL);
	Evas_Object *layout = elm_object_part_content_get(item, "hidden_item");

	return layout;
}
#endif

static int _is_flight_mode(void)
{
	int ret = 0;
	bool m_mode = false;

	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, &m_mode);
	if(ret < 0) {
		_E("Failed to get SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE:%s %d", SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE, ret);
	}

	_D("m_mode : %d", m_mode);

	return m_mode;
}

static void _text_update(Evas_Object *layout)
{
	Evas_Object *button = NULL;
	ret_if(layout == NULL);

	button = elm_object_part_content_get(layout, "button.1");
	if (button != NULL) {
		elm_object_part_text_set(button, "contents", _("IDS_CST_OPT_FWD_CALLS_TO_GEAR_ABB"));
	}

	button = elm_object_part_content_get(layout, "button.2");
	if (button != NULL) {
		elm_object_part_text_set(button, "name", _("IDS_ST_MBODY_DO_NOT_DISTURB_ABB"));
	}
}

static void _status_update(Evas_Object *layout)
{
	ret_if(layout == NULL);

	Evas_Object *button1 = elm_object_part_content_get(layout, "button.1");
	if (button1 != NULL) {
		if (_is_flight_mode() == 1) {
			elm_object_signal_emit(button1, "icon,disable", "prog");
		} else {
			elm_object_signal_emit(button1, "icon,enable", "prog");
		}
	}

	_text_update(layout);
}

static void _vconf_cb(keynode_t *node, void *data)
{
	_status_update((Evas_Object*)data);
}

static void _system_settings_cb(system_settings_key_e key, void *data)
{
	_status_update((Evas_Object *)data);
}

#define CALLSETTING_APP  "org.tizen.w-call-setting"
#define CALLSETTING_KEY  "type"
#define CALLSETTING_VALUE  "fwd_call"
static void _icon_1_clicked_cb(void *cbdata, Evas_Object *obj, void *event_info)
{
	_D("");

	effect_play_sound();

	if (_is_flight_mode() == 1) {
#if 0
		Evas_Object *win = main_get_info()->win;
		if (win != NULL) {
			util_create_toast_popup(win,
				"IDS_ST_TPOP_UNABLE_TO_TURN_ON_MOBILE_DATA_WHILE_FLIGHT_MODE_ENABLED_DISABLE_FLIGHT_MODE_AND_TRY_AGAIN_ABB");
		}
#endif
		return;
	}

	int ret = 0;
	app_control_h ac = NULL;
	if ((ret = app_control_create(&ac)) != APP_CONTROL_ERROR_NONE) {
		_E("Failed to create app control:%d", ret);
	}
	ret_if(ac == NULL);

	app_control_set_operation(ac, APP_CONTROL_OPERATION_VIEW);
	app_control_set_app_id(ac, CALLSETTING_APP);
	app_control_add_extra_data(ac, CALLSETTING_KEY, CALLSETTING_VALUE);
	home_dbus_cpu_booster_signal_send();
	if ((ret = app_control_send_launch_request(ac, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
		_E("Failed to launch %s(%d)", CALLSETTING_APP, ret);
	}
	app_control_destroy(ac);
}

static char *_button_1_access_info_cb(void *data, Evas_Object *obj)
{
	int index = (int)data;
	char *info = NULL;

	if (index == 1) {
		info = _("IDS_CST_OPT_FWD_CALLS_TO_GEAR_ABB");
	}

	if (info != NULL) {
		return strdup(info);
	}

	return NULL;
}

static Evas_Object *_view_create(Evas_Object *page)
{
	Evas_Object *focus = NULL;
	Eina_Bool ret = EINA_TRUE;
	Evas_Object *item = page_get_item(page);
	retv_if(item == NULL, NULL);

	Evas_Object *layout = elm_layout_add(item);
	retv_if(layout == NULL, NULL);
	ret = elm_layout_file_set(layout, PAGE_CLOCK_EDJE_FILE, "blockingmode");
	retv_if(ret == EINA_FALSE, NULL);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(layout);

	Evas_Object *button1 = elm_layout_add(layout);
	if (button1 != NULL) {
		ret = elm_layout_file_set(button1, PAGE_CLOCK_EDJE_FILE, "hidden_item_call_fwd");
		if (ret == EINA_TRUE) {
			evas_object_size_hint_weight_set(button1, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

			focus = elm_button_add(button1);
			if (focus != NULL) {
				elm_object_style_set(focus, "focus");
				elm_access_info_cb_set(focus, ELM_ACCESS_INFO, _button_1_access_info_cb, (void*)1);
				evas_object_smart_callback_add(focus, "clicked", _icon_1_clicked_cb, page);
				elm_access_object_unregister(focus);
				elm_object_part_content_set(button1, "focus", focus);
			}

			elm_object_part_content_set(layout, "button.1", button1);
			evas_object_show(button1);
		} else {
			_E("Failed to set file");
			evas_object_del(button1);
		}
	}

	if (clock_service_event_state_get(CLOCK_EVENT_SIM) == CLOCK_EVENT_SIM_NOT_INSERTED) {
		elm_object_signal_emit(layout, "button.1.hide", "prog");
	}

	return layout;
}

HAPI Evas_Object *clock_view_hidden_add(Evas_Object *page)
{
	Evas_Object *view = NULL;
	retv_if(page == NULL, NULL);

	view = _view_create(page);
	retv_if(view == NULL, NULL);

	_status_update(view);

	if (vconf_ignore_key_changed(VCONFKEY_TELEPHONY_MODEM_ALWAYS_ON_STATE,
		_vconf_cb) < 0) {
		_E("Failed to ignore the VCONFKEY_TELEPHONY_MODEM_ALWAYS_ON_STATE callback");
	}
	if (vconf_notify_key_changed(VCONFKEY_TELEPHONY_MODEM_ALWAYS_ON_STATE,
		_vconf_cb, view) < 0) {
		_E("Failed to get notification from VCONFKEY_TELEPHONY_MODEM_ALWAYS_ON_STATE");
	}
	if (vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
		_vconf_cb) < 0) {
		_E("Failed to ignore the VCONFKEY_TELEPHONY_SIM_SLOT callback");
	}
	if (vconf_notify_key_changed(VCONFKEY_TELEPHONY_SIM_SLOT,
		_vconf_cb, view) < 0) {
		_E("Failed to get notification from VCONFKEY_TELEPHONY_SIM_SLOT");
	}
	if (system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE) < 0) {
		_E("Failed to ignore the SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE callback");
	}
	if (system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE,
		_system_settings_cb, view) < 0) {
		_E("Failed to get notification from SYSTEM_SETTINGS_KEY_NETWORK_FLIGHT_MODE");
	}

	return view;
}
#endif

HAPI void clock_view_hidden_event_handler(Evas_Object *page, int event)
{
#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
#ifdef RUN_ON_DEVICE
	ret_if(page == NULL);
	Evas_Object *layout = _hidden_layout_get(page);
	ret_if(layout == NULL);
	Evas_Object *focus = NULL;
	Evas_Object *button = NULL;

	switch (event) {
		case CLOCK_EVENT_VIEW_HIDDEN_SHOW:
			if (clock_service_event_state_get(CLOCK_EVENT_SCREEN_READER) == CLOCK_EVENT_SCREEN_READER_ON) {
				if (tutorial_is_exist() == 0) {
					button = elm_object_part_content_get(layout, "button.1");
					if (button != NULL) {
						focus = elm_object_part_content_get(button, "focus");
						if (focus != NULL) {
							elm_access_object_register(focus, layout);
						}
					}
					button = elm_object_part_content_get(layout, "button.2");
					if (button != NULL) {
						focus = elm_object_part_content_get(button, "focus");
						if (focus != NULL) {
							elm_access_object_register(focus, layout);
						}
					}
				}
			}
			evas_object_data_set(layout, PRIVATE_DATA_KEY_VISIBLE, (void*)1);
			break;
		case CLOCK_EVENT_VIEW_HIDDEN_HIDE:
			button = elm_object_part_content_get(layout, "button.1");
			if (button != NULL) {
				focus = elm_object_part_content_get(button, "focus");
				if (focus != NULL) {
					elm_access_object_unregister(focus);
				}
			}
			button = elm_object_part_content_get(layout, "button.2");
			if (button != NULL) {
				focus = elm_object_part_content_get(button, "focus");
				if (focus != NULL) {
					elm_access_object_unregister(focus);
				}
			}

			evas_object_data_del(layout, PRIVATE_DATA_KEY_VISIBLE);
			break;
		case CLOCK_EVENT_APP_LANGUAGE_CHANGED:
			_status_update(layout);
			break;
		case CLOCK_EVENT_POWER_ENHANCED_MODE_ON:
			button = elm_object_part_content_get(layout, "button.1");
			if (button != NULL) {
				//elm_object_signal_emit(button, "gray,enable", "prog");
			}
			button = elm_object_part_content_get(layout, "button.2");
			if (button != NULL) {
				//elm_object_signal_emit(button, "gray,enable", "prog");
			}
			break;
		case CLOCK_EVENT_POWER_ENHANCED_MODE_OFF:
			button = elm_object_part_content_get(layout, "button.1");
			if (button != NULL) {
				//elm_object_signal_emit(button, "gray,disable", "prog");
			}
			button = elm_object_part_content_get(layout, "button.2");
			if (button != NULL) {
				//elm_object_signal_emit(button, "gray,disable", "prog");
			}
			break;
		case CLOCK_EVENT_SIM_NOT_INSERTED:
			elm_object_signal_emit(layout, "button.1.hide", "prog");
			break;
		case CLOCK_EVENT_SIM_INSERTED:
			elm_object_signal_emit(layout, "button.1.show", "prog");
			break;
	}
#endif
#endif
}
