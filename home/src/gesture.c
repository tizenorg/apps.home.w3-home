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

#include <Evas.h>
#include <stdio.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Elementary.h>
#include <utilX.h>
#include <dd-display.h>
#include <bundle.h>
#include <efl_assist.h>
#include <dlog.h>

#include "util.h"
#include "main.h"
#include "log.h"
#include "gesture.h"
#include "effect.h"
#include "page_info.h"
#include "scroller_info.h"
#include "scroller.h"
#include "layout.h"
#include "layout_info.h"
#include "tutorial.h"
#include "dbus.h"
#include "apps/apps_main.h"

#define LCD_ON_REASON_GESTURE "gesture"
#define VCONFKEY_SIMPLE_CLOCK_STATUS "db/setting/simpleclock_mode"
#define VCONFKEY_RESERVED_APPS_STATUS "memory/starter/reserved_apps_status"

#define CB_LIST_MAX 3

static Eina_List *cbs_list[CB_LIST_MAX];

typedef struct {
	void (*result_cb)(void *);
	void *result_data;
} gesture_cb_s;

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

static Eina_Bool _apps_hide_idler_cb(void *data)
{
	apps_main_launch(APPS_LAUNCH_HIDE);

	return ECORE_CALLBACK_CANCEL;
}

static void _clock_show(void)
{
	int is_clock_displayed = 0;
	Eina_Bool is_apps_displayed = (apps_main_is_visible() == EINA_TRUE) ? 1 : 0;

	Evas_Object *scroller = _scroller_get();
	ret_if(scroller == NULL);
	Evas_Object *layout = _layout_get();
	ret_if(layout == NULL);

	if (tutorial_is_exist() == 1) {
		return ;
	}

	/* delete edit/addviewer layout */
	layout_set_idle(layout);

	if (is_clock_displayed == 0) {
		scroller_bring_in_by_push_type(scroller, SCROLLER_PUSH_TYPE_CENTER, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT_SHOW);
	}

	int w, h;
	Evas *e;
	Ecore_Evas *ee;
	Ecore_X_GC gc;
	ee = ecore_evas_ecore_evas_get(evas_object_evas_get(main_get_info()->win));
	e = ecore_evas_get(ee);
	evas_output_viewport_get(e, NULL, NULL, &w, &h);
	evas_obscured_clear(e);
	evas_damage_rectangle_add(e, 0, 0, w, h);
	ecore_evas_manual_render(ee);

	gc = ecore_x_gc_new(0, ECORE_X_GC_VALUE_MASK_FOREGROUND, NULL);
	ecore_x_pixmap_paste(elm_win_xwindow_get(main_get_info()->win)
		, ecore_x_window_root_first_get(), gc, 0, 0, main_get_info()->root_w, main_get_info()->root_h, 0, 0);
	ecore_x_sync();

#if 0 /* EFL private features */
	gesture_home_window_effect_set(0);
#endif
	elm_win_raise(main_get_info()->win);

	/* hiding apps */
	if (is_apps_displayed == 1) {
		_D("need to hide apps");
		ecore_idler_add(_apps_hide_idler_cb, NULL);
	}
}

#define ALPM_MANAGER_STATUS_SHOW "show"
#define ALPM_MANAGER_STATUS_HIDE "hide"
static void _alpm_manager_status_changed_cb(void *user_data, void *event_info)
{
	char *status = event_info;
	ret_if(status == NULL);

	_W("alpm status:%s", status);

	if (strcmp(status, ALPM_MANAGER_STATUS_SHOW) == 0) {
		// do nothing
	} else if (strcmp(status, ALPM_MANAGER_STATUS_SIMPLE_HIDE) == 0) {
		_clock_show();
		_W("clock show");
	}
}



HAPI void home_gesture_init(void)
{
	if (home_dbus_register_cb(DBUS_EVENT_ALPM_MANAGER_STATE_CHANGED, _alpm_manager_status_changed_cb, NULL) != W_HOME_ERROR_NONE) {
		_E("Failed to register alpm manager status changed cb");
	}
}

HAPI void home_gesture_fini(void)
{
	home_dbus_unregister_cb(DBUS_EVENT_ALPM_MANAGER_STATE_CHANGED, _alpm_manager_status_changed_cb);
}



HAPI w_home_error_e gesture_register_cb(
	int mode,
	void (*result_cb)(void *), void *result_data)
{
	retv_if(NULL == result_cb, W_HOME_ERROR_INVALID_PARAMETER);

	gesture_cb_s *cb = calloc(1, sizeof(gesture_cb_s));
	retv_if(!cb, W_HOME_ERROR_FAIL);

	cb->result_cb = result_cb;
	cb->result_data = result_data;

	cbs_list[mode] = eina_list_append(cbs_list[mode], cb);
	retv_if(NULL == cbs_list[mode], W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}



HAPI void gesture_unregister_cb(
	int mode,
	void (*result_cb)(void *))
{
	const Eina_List *l;
	const Eina_List *n;
	gesture_cb_s *cb;
	EINA_LIST_FOREACH_SAFE(cbs_list[mode], l, n, cb) {
		continue_if(NULL == cb);
		if (result_cb != cb->result_cb) {
			continue;
		}
		cbs_list[mode] = eina_list_remove(cbs_list[mode], cb);
		free(cb);
		return;
	}
}



extern void gesture_execute_cbs(int mode)
{
	const Eina_List *l;
	const Eina_List *n;
	gesture_cb_s *cb;
	EINA_LIST_FOREACH_SAFE(cbs_list[mode], l, n, cb) {
		continue_if(n);
		cb->result_cb(cb->result_data);
	}
}


