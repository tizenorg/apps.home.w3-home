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
#include <vconf.h>
#include <bundle.h>
#include <efl_assist.h>
#include <dlog.h>

#include "log.h"
#include "util.h"
#include "main.h"
#include "layout_info.h"
#include "page_info.h"
#include "scroller_info.h"
#include "scroller.h"
#include "layout.h"
#include "clock_service.h"
#include "db.h"
#include "dbus.h"
#include "tutorial.h"
#include "apps/apps_main.h"
#include "power_mode.h"

#define COOLDOWN_STATUS_RELEASE "Release"
#define COOLDOWN_STATUS_LIMITATION "LimitAction"
#define COOLDOWN_STATUS_WARNING "WarningAction"

#define POWER_MODE_POWER_ENHANCED_MODE 1
#define POWER_MODE_COOLDOWN_MODE 2

static struct {
	int is_ui_ready;
	int ps_mode;
	int cooldown_mode;
	int applied_mode;

	Eina_List *cbs_list[POWER_MODE_EVENT_MAX];
} s_info = {
	.is_ui_ready = 0,
	.ps_mode = 0,
	.cooldown_mode = -1,
	.applied_mode = -1,

	.cbs_list = {NULL, },
};

typedef struct {
	void (*result_cb)(void *, void *);
	void *result_data;
} powermode_cb_s;

HAPI int power_mode_register_cb(
	int type,
	void (*result_cb)(void *, void *), void *result_data)
{
	retv_if(result_cb == NULL, W_HOME_ERROR_INVALID_PARAMETER);

	powermode_cb_s *cb = calloc(1, sizeof(powermode_cb_s));
	retv_if(cb == NULL, W_HOME_ERROR_FAIL);

	cb->result_cb = result_cb;
	cb->result_data = result_data;

	s_info.cbs_list[type] = eina_list_prepend(s_info.cbs_list[type], cb);
	retv_if(s_info.cbs_list[type] == NULL, W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}

HAPI void power_mode_unregister_cb(
	int type,
	void (*result_cb)(void *, void *))
{
	const Eina_List *l;
	const Eina_List *n;
	powermode_cb_s *cb;
	EINA_LIST_FOREACH_SAFE(s_info.cbs_list[type], l, n, cb) {
		continue_if(cb == NULL);
		if (result_cb != cb->result_cb) continue;
		s_info.cbs_list[type] = eina_list_remove(s_info.cbs_list[type], cb);
		free(cb);
		return;
	}
}

static void _execute_cbs(int type, void *event_info)
{
	const Eina_List *l = NULL;
	const Eina_List *n = NULL;
	powermode_cb_s *cb = NULL;
	EINA_LIST_FOREACH_SAFE(s_info.cbs_list[type], l, n, cb) {
		continue_if(cb == NULL);
		continue_if(cb->result_cb == NULL);

		cb->result_cb(cb->result_data, event_info);
	}
}

static Evas_Object *_layout_get(void) {
	Evas_Object *win = main_get_info()->win;
	Evas_Object *layout = NULL;

	if (win != NULL) {
		layout = evas_object_data_get(win, DATA_KEY_LAYOUT);
	}

	return layout;
}

static Evas_Object *_scroller_get(void) {
	Evas_Object *win = main_get_info()->win;
	Evas_Object *layout = NULL;
	Evas_Object *scroller = NULL;

	if (win != NULL) {
		layout = evas_object_data_get(win, DATA_KEY_LAYOUT);
		if (layout != NULL) {
			scroller = elm_object_part_content_get(layout, "scroller");
		}
	}

	return scroller;
}

static Evas_Object *_tutorial_get(void) {
	Evas_Object *layout = _layout_get();
	layout_info_s *layout_info = NULL;
	Evas_Object *tutorial = NULL;

	if (layout != NULL) {
		layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
		if(layout_info != NULL) {
			tutorial = layout_info->tutorial;
		}
	}

	return tutorial;
}

static int _psmode_get(void)
{
	int ps_mode = SETTING_PSMODE_NORMAL;

	if(vconf_get_int(VCONFKEY_SETAPPL_PSMODE, &ps_mode) < 0) {
		_E("Failed to get VCONFKEY_SETAPPL_PSMODE");
	}

	return ps_mode;
}

static void _del_list(void *data)
{
	Eina_List *page_info_list = data;
	ret_if(!page_info_list);
	page_info_list_destroy(page_info_list);
}

/*
 * Emergency Mode
 */
static void _psmode_apply(int mode)
{
	Evas_Object *scroller = _scroller_get();
	ret_if(scroller == NULL);

	_D("changing home to emergency mode to:%d", mode);

	if (mode == SETTING_PSMODE_NORMAL) {
		Eina_List *page_info_list = NULL;

		_execute_cbs(POWER_MODE_ENHANCED_OFF, NULL);

		clock_service_request(CLOCK_SERVICE_MODE_NORMAL);

		page_info_list = db_write_list();
		if (page_info_list) {
			scroller_push_pages(scroller, page_info_list, _del_list, page_info_list);
		} else {
			scroller_push_pages(scroller, NULL, NULL, NULL);
		}

		if(tutorial_is_first_boot()) tutorial_create(_layout_get());
	} else if (mode == SETTING_PSMODE_WEARABLE_ENHANCED) {
		_execute_cbs(POWER_MODE_ENHANCED_ON, NULL);

		if(_tutorial_get()) tutorial_destroy(_tutorial_get());

		if (apps_main_is_visible() == EINA_TRUE) {
			_D("need to hide apps");
			apps_main_launch(APPS_LAUNCH_HIDE);
		}
		layout_set_idle(_layout_get());

		clock_service_request(CLOCK_SERVICE_MODE_EMERGENCY);
		scroller_region_show_by_push_type(scroller, SCROLLER_PUSH_TYPE_CENTER, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT);

		scroller_pop_pages(scroller, PAGE_DIRECTION_RIGHT);
		elm_win_activate(main_get_info()->win);
	}
}

/*
 * CoolDown Mode
 */
static int _cooldown_mode_status_to_mode(const char *status)
{
	if (strcmp(status, COOLDOWN_STATUS_RELEASE) == 0) {
		return COOLDOWN_MODE_RELEASE;
	} else if (strcmp(status, COOLDOWN_STATUS_LIMITATION) == 0) {
		return COOLDOWN_MODE_LIMITATION;
	} else if (strcmp(status, COOLDOWN_STATUS_WARNING) == 0) {
		return COOLDOWN_MODE_WARNING;
	}

	return COOLDOWN_MODE_RELEASE;
}

static int _cooldown_mode_get_from_dbus(void)
{
	int mode = COOLDOWN_MODE_RELEASE;

	char *cooldown_mode = home_dbus_cooldown_status_get();
	_W("cooldown status:%s", cooldown_mode);
	if (cooldown_mode != NULL) {
		mode = _cooldown_mode_status_to_mode(cooldown_mode);
		free(cooldown_mode);
	}

	return mode;
}

static int _cooldown_mode_get(void)
{
	if (s_info.cooldown_mode == -1) {
		s_info.cooldown_mode = _cooldown_mode_get_from_dbus();
	}

	return s_info.cooldown_mode;
}

static void _cooldown_mode_apply(int mode)
{
	Evas_Object *scroller = _scroller_get();
	ret_if(scroller == NULL);

	_D("changing home to cooldown mode:%d", mode);

	if (mode == COOLDOWN_MODE_RELEASE) {
		_execute_cbs(POWER_MODE_COOLDOWN_OFF, NULL);

		clock_service_request(CLOCK_SERVICE_MODE_NORMAL);
		scroller_unfreeze(scroller);

		if(tutorial_is_first_boot()) tutorial_create(_layout_get());
	} else if (mode == COOLDOWN_MODE_LIMITATION) {
		_execute_cbs(POWER_MODE_COOLDOWN_ON, NULL);

		if(_tutorial_get()) tutorial_destroy(_tutorial_get());

		if (apps_main_is_visible() == EINA_TRUE) {
			_D("need to hide apps");
			apps_main_launch(APPS_LAUNCH_HIDE);
		}
		layout_set_idle(_layout_get());

		clock_service_request(CLOCK_SERVICE_MODE_COOLDOWN);
		scroller_region_show_by_push_type(scroller, SCROLLER_PUSH_TYPE_CENTER, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT);
		elm_win_activate(main_get_info()->win);
		scroller_freeze(scroller);
	}
}

#define POWER_MODE_POWER_ENHANCED_MODE 1
#define POWER_MODE_COOLDOWN_MODE 2

static void _power_mode_apply(void)
{
	int ps_mode = SETTING_PSMODE_NORMAL;
	int cooldown_mode = COOLDOWN_MODE_RELEASE;

	ps_mode = _psmode_get();
	s_info.ps_mode = ps_mode;

	cooldown_mode = _cooldown_mode_get();
	s_info.cooldown_mode = cooldown_mode;

	_D("ps mode:%d cooldown mode:%d", ps_mode, cooldown_mode);

	if (s_info.is_ui_ready == 0) {
		_E("ui isn't ready yet, just update power mode flag");
		return ;
	}

	if (ps_mode != SETTING_PSMODE_NORMAL && cooldown_mode != COOLDOWN_MODE_RELEASE) {
		_cooldown_mode_apply(cooldown_mode);
		s_info.applied_mode = POWER_MODE_COOLDOWN_MODE;

	} else if (ps_mode != SETTING_PSMODE_NORMAL && cooldown_mode == COOLDOWN_MODE_RELEASE) {
		_psmode_apply(ps_mode);
		s_info.applied_mode = POWER_MODE_POWER_ENHANCED_MODE;

	} else if (ps_mode == SETTING_PSMODE_NORMAL && cooldown_mode != COOLDOWN_MODE_RELEASE) {
		_cooldown_mode_apply(cooldown_mode);
		s_info.applied_mode = POWER_MODE_COOLDOWN_MODE;

	} else {
		if (s_info.applied_mode == POWER_MODE_POWER_ENHANCED_MODE) {
			_psmode_apply(ps_mode);
		} else if (s_info.applied_mode == POWER_MODE_COOLDOWN_MODE) {
			_cooldown_mode_apply(cooldown_mode);
		} else {
			_E("no power mode applied");
		}
		s_info.applied_mode = -1;
	}
}

static void _psmode_changed_cb(keynode_t *node, void *data)
{
	_D("");
	_power_mode_apply();
}

static void _cooldown_mode_changed_cb(void *user_data, void *event_info)
{
	_D("");
	int mode = COOLDOWN_MODE_RELEASE;
	_W("cooldown status:%s", event_info);
	if (event_info != NULL) {
		mode = _cooldown_mode_status_to_mode(event_info);
		s_info.cooldown_mode = mode;
	}

	_power_mode_apply();
}

/*!
 * constructor/deconstructor
 */
HAPI void power_mode_init(void)
{
	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_PSMODE, _psmode_changed_cb, NULL) < 0) {
		_E("Failed to register the VCONFKEY_SETAPPL_PSMODE callback");
	}

	if (home_dbus_register_cb(DBUS_EVENT_COOLDOWN_STATE_CHANGED, _cooldown_mode_changed_cb, NULL) != W_HOME_ERROR_NONE) {
		_E("Failed to register cooldown status changed cb");
	}
}

HAPI void power_mode_ui_init(void)
{
	s_info.is_ui_ready = 1;

	_power_mode_apply();
}

HAPI void power_mode_fini(void)
{
	if (vconf_ignore_key_changed(VCONFKEY_SETAPPL_PSMODE, _psmode_changed_cb) < 0) {
		_E("Failed to ignore the VCONFKEY_SETAPPL_PSMODE callback");
	}

	home_dbus_unregister_cb(DBUS_EVENT_COOLDOWN_STATE_CHANGED, _cooldown_mode_changed_cb);

	s_info.is_ui_ready = 0;
}

HAPI int emergency_mode_enabled_get(void)
{
	int mode = _psmode_get();

	if (mode == SETTING_PSMODE_WEARABLE_ENHANCED) {
		return 1;
	}

	return 0;
}

HAPI int cooldown_mode_enabled_get(void)
{
	if (_cooldown_mode_get() == COOLDOWN_MODE_LIMITATION) {
		return 1;
	}

	return 0;
}

HAPI int cooldown_mode_warning_get(void)
{
	if (_cooldown_mode_get() == COOLDOWN_MODE_WARNING) {
		return 1;
	}

	return 0;
}
