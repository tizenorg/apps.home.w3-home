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
#include <Evas.h>
#include <stdbool.h>
#include <vconf.h>
#include <bundle.h>
#include <app.h>
#include <aul.h>
#include <efl_assist.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
#include <minicontrol-handler.h>
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

#define ICON_TYPE_NONE 0
#define ICON_TYPE_MODEM_OFF 1
#define ICON_TYPE_DND_ON 2
#define ICON_TYPE_CALLFW 3

#define ENABLE_AUTO_CALL_FWD
#define PRIVATE_DATA_KEY_INDICATOR_ICON_TYPE "p_d_k_iit"

static void _view_type_set(Evas_Object *view, int type)
{
	ret_if(view == NULL);

	evas_object_data_set(view, PRIVATE_DATA_KEY_INDICATOR_ICON_TYPE, (void*)type);
}

static int _view_type_get(Evas_Object *view)
{
	retv_if(view == NULL, ICON_TYPE_NONE);

	return (int)evas_object_data_get(view, PRIVATE_DATA_KEY_INDICATOR_ICON_TYPE);
}

#define MODEM_SETTING_APPID "org.tizen.clocksetting.network"
static void _modem_off_icon_clicked_cb(void *cbdata, Evas_Object *obj, void *event_info)
{
	_D("");

	int ret = 0;

	app_control_h ac = NULL;
	if ((ret = app_control_create(&ac)) != APP_CONTROL_ERROR_NONE) {
		_E("Failed to create app control:%d", ret);
	}
	ret_if(ac == NULL);

	app_control_set_operation(ac, APP_CONTROL_OPERATION_VIEW);
	app_control_set_app_id(ac, MODEM_SETTING_APPID);
	if ((ret = app_control_send_launch_request(ac, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
		_E("Failed to launch %s(%d)", MODEM_SETTING_APPID, ret);
	}
	app_control_destroy(ac);

	effect_play_sound();
}

static Evas_Object *_view_modem_icon(Evas_Object *parent, int type)
{

	int ret = 0;
	Evas_Object *layout = NULL;
	Evas_Object *focus = NULL;

	if (type == ICON_TYPE_MODEM_OFF) {
		layout = elm_layout_add(parent);
		retv_if(layout == NULL, NULL);
		ret = elm_layout_file_set(layout, PAGE_CLOCK_EDJE_FILE, "modem_off_icon");
		retv_if(ret == EINA_FALSE, NULL);
		evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

		focus = elm_button_add(layout);
		if (focus != NULL) {
			elm_object_style_set(focus, "focus");
			evas_object_smart_callback_add(focus, "clicked", _modem_off_icon_clicked_cb, NULL);
			elm_object_part_content_set(layout, "focus", focus);
		}

		evas_object_show(layout);
	}

	return layout;
}

static int _view_modem_icon_type_get(void)
{
	int sap = clock_service_event_state_get(CLOCK_EVENT_SAP);
	int modem = clock_service_event_state_get(CLOCK_EVENT_MODEM);

	_D("sap:0x%x modem:0x%x", sap, modem);

	if (sap == CLOCK_EVENT_SAP_OFF && modem == CLOCK_EVENT_MODEM_OFF) {
		return ICON_TYPE_MODEM_OFF;
	} else if (sap == CLOCK_EVENT_SAP_ON || modem == CLOCK_EVENT_MODEM_ON) {
		return ICON_TYPE_NONE;
	}

	return ICON_TYPE_NONE;
}

#define BLOCKINGMODE_APP  "org.tizen.clocksetting.blockingmode"
#define BLOCKINGMODE_KEY  "viewtype"
static void _dnd_clicked_cb(void *cbdata, Evas_Object *obj, void *event_info)
{
#if 0
	int ret = 0;
	const char *value = "disable";
	_D("blockingmode");

 	app_control_h ac = NULL;
 	if ((ret = app_control_create(&ac)) != APP_CONTROL_ERROR_NONE) {
 		_E("Failed to create app control:%d", ret);
 	}
	ret_if(ac == NULL);

	app_control_set_operation(ac, APP_CONTROL_OPERATION_VIEW);
	app_control_set_app_id(ac, BLOCKINGMODE_APP);
	app_control_add_extra_data(ac, BLOCKINGMODE_KEY, value);
	home_dbus_cpu_booster_signal_send();
	if ((ret = app_control_send_launch_request(ac, NULL, NULL)) != APP_CONTROL_ERROR_NONE) {
		_E("Failed to launch %s(%d)", BLOCKINGMODE_APP, ret);
	}
	app_control_destroy(ac);
#endif

	if (util_feature_enabled_get(FEATURE_CLOCK_HIDDEN_BUTTON) == 0) {
		return ;
	}

	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	ret_if(!clock);
	ret_if(!clock->view);

	/* Hide indicator */
	evas_object_smart_callback_call(clock->view, CLOCK_SMART_SIGNAL_VIEW_REQUEST, (void*)CLOCK_VIEW_REQUEST_DRAWER_SHOW);

	effect_play_sound();
}

static char *_dnd_access_info_cb(void *data, Evas_Object *obj)
{
	int index = (int)data;
	char *info = NULL;

	if (index == 1) {
		info = _("IDS_ST_MBODY_DO_NOT_DISTURB_ABB");
	}

	if (info != NULL) {
		return strdup(info);
	}

	return NULL;
}

static char *_dnd_access_state_cb(void *data, Evas_Object *obj)
{
	char *state = NULL;
	int is_enabled = 0;

	if (clock_service_event_state_get(CLOCK_EVENT_DND) == CLOCK_EVENT_DND_ON) {
		is_enabled = 1;
	}

	if (is_enabled == 1) {
		state = _("IDS_COM_BODY_ON_M_STATUS");
	} else {
		state = _("IDS_COM_BODY_OFF_M_STATUS");
	}

	if (state) {
		return strdup(state);
	}

	return NULL;
}

static Evas_Object *_view_dnd_icon(Evas_Object *parent, int type)
{

	int ret = 0;
	Evas_Object *layout = NULL;
	Evas_Object *focus = NULL;

	if (type == ICON_TYPE_DND_ON) {
		layout = elm_layout_add(parent);
		retv_if(layout == NULL, NULL);
		ret = elm_layout_file_set(layout, PAGE_CLOCK_EDJE_FILE, "dnd_icon");
		retv_if(ret == EINA_FALSE, NULL);
		evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

		focus = elm_button_add(layout);
		if (focus != NULL) {
			elm_object_style_set(focus, "focus");
			elm_access_info_cb_set(focus, ELM_ACCESS_INFO, _dnd_access_info_cb, (void*)1);
			elm_access_info_cb_set(focus, ELM_ACCESS_STATE, _dnd_access_state_cb, (void*)1);
			evas_object_smart_callback_add(focus, "clicked", _dnd_clicked_cb, NULL);
			elm_object_part_content_set(layout, "focus", focus);
		}

		evas_object_show(layout);
	}

	return layout;
}

static int _view_dnd_icon_type_get(void)
{
	int dnd = clock_service_event_state_get(CLOCK_EVENT_DND);

	_D("dnd:0x%x", dnd);

	if (dnd == CLOCK_EVENT_DND_ON) {
		return ICON_TYPE_DND_ON;
	}

	return ICON_TYPE_NONE;
}

#define VCONFKEY_MODEM_ON_OFF_ENABLED "file/private/weconn/modem_onoff_control"
static Evas_Object *_view_create_by_type(Evas_Object *page, int type)
{
	Evas_Object *view = NULL;
	Evas_Object *item = page_get_item(page);
	retv_if(item == NULL, NULL);

	_D("%x", type);

	int is_feature_enabled = 0;

	switch (type) {
		case ICON_TYPE_MODEM_OFF:
			if (vconf_get_int(VCONFKEY_MODEM_ON_OFF_ENABLED, &is_feature_enabled) != 0) {
				_E("Failed to get %s", VCONFKEY_MODEM_ON_OFF_ENABLED);
			}

			if (is_feature_enabled == 1) {
				_E("modem on/off feature isn't enabled");
				view = _view_modem_icon(item, ICON_TYPE_MODEM_OFF);
			} else {
				view = NULL;
			}
			break;
		case ICON_TYPE_DND_ON:
			view = _view_dnd_icon(item, ICON_TYPE_DND_ON);
			break;
	}

	_view_type_set(view, type);

	return view;
}

static void _view_remove_by_type(Evas_Object *page, int type)
{
	Evas_Object *slot1 = NULL;
	Evas_Object *slot2 = NULL;
	Evas_Object *item = page_get_item(page);
	ret_if(item == NULL);

	slot1 = elm_object_part_content_get(item, "indicator.1");
	slot2 = elm_object_part_content_get(item, "indicator.2");

	if (_view_type_get(slot1) == type) {
		elm_object_part_content_unset(item, "indicator.1");
		evas_object_del(slot1);
		elm_object_part_content_unset(item, "indicator.2");
		elm_object_part_content_set(item, "indicator.1", slot2);
	} else if (_view_type_get(slot2) == type) {
		elm_object_part_content_unset(item, "indicator.2");
		evas_object_del(slot2);
	}
}

static void _view_add_by_type(Evas_Object *page, int type)
{
	Evas_Object *slot1 = NULL;
	Evas_Object *slot2 = NULL;
	Evas_Object *item = page_get_item(page);
	ret_if(item == NULL);

	slot1 = elm_object_part_content_get(item, "indicator.1");
	if (_view_type_get(slot1) == type) {
		return;
	}
	slot2 = elm_object_part_content_get(item, "indicator.2");
	if (_view_type_get(slot2) == type) {
		return;
	}

	//view create
	Evas_Object *view = _view_create_by_type(page, type);
	if (view != NULL) {
		if (slot1 == NULL) {
			elm_object_part_content_set(item, "indicator.1", view);
		} else if (slot1 != NULL && slot2 == NULL) {
			elm_object_part_content_set(item, "indicator.2", view);
		} else {
			_E("it can't be..");
		}
	}
}

HAPI void clock_view_indicator_add(Evas_Object *page)
{
	ret_if(page == NULL);

	_view_add_by_type(page, _view_modem_icon_type_get());
	_view_add_by_type(page, _view_dnd_icon_type_get());
#ifdef ENABLE_AUTO_CALL_FWD
	_view_add_by_type(page, ICON_TYPE_CALLFW);
#endif
}

HAPI void clock_view_indicator_event_handler(Evas_Object *page, int event)
{
	ret_if(page == NULL);

	_D("%p %x", page, event);

	if (((event & CLOCK_EVENT_CATEGORY(CLOCK_EVENT_SAP)) == CLOCK_EVENT_CATEGORY(CLOCK_EVENT_SAP)) ||
	    ((event & CLOCK_EVENT_CATEGORY(CLOCK_EVENT_MODEM)) == CLOCK_EVENT_CATEGORY(CLOCK_EVENT_MODEM))) {
		if (_view_modem_icon_type_get() == ICON_TYPE_MODEM_OFF) {
			_view_add_by_type(page, ICON_TYPE_MODEM_OFF);
		} else {
			_view_remove_by_type(page, ICON_TYPE_MODEM_OFF);
		}
	} else if (event & CLOCK_EVENT_CATEGORY(CLOCK_EVENT_DND)) {
		if (_view_dnd_icon_type_get() == ICON_TYPE_DND_ON) {
			_view_add_by_type(page, ICON_TYPE_DND_ON);
		} else {
			_view_remove_by_type(page, ICON_TYPE_DND_ON);
		}
	}
}
