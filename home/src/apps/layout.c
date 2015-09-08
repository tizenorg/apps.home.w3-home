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
#include <Ecore_X.h>
#include <app_control.h>
#include <efl_assist.h>

#include "log.h"
#include "util.h"
#include "key.h"
#include "main.h"
#include "layout_info.h"
#include "tutorial.h"
#include "apps/apps_conf.h"
#include "apps/db.h"
#include "apps/item_badge.h"
#include "apps/item_info.h"
#include "apps/layout.h"
#include "apps/list.h"
#include "apps/apps_main.h"
#include "apps/scroller.h"
#include "apps/scroller_info.h"
#include "apps/item.h"


#define PRIVATE_DATA_KEY_CHECKER_TYPE "ck_tp"
#define PRIVATE_DATA_KEY_DOWN_X "dw_x"
#define PRIVATE_DATA_KEY_DOWN_Y "dw_y"
#define PRIVATE_DATA_KEY_TOP_CHECKER "top_ck"
#define PRIVATE_DATA_KEY_BOTTOM_CHECKER "bt_ck"
#define PRIVATE_DATA_KEY_LAYOUT_IS_EDITED "p_ly_ed"

#define MOVE_UP -2
#define MOVE_DOWN 2



static void _bg_down_cb(void *data, Evas_Object *obj, const char* emission, const char* source)
{
	_APPS_D("BG is pressed");
}



static void _bg_up_cb(void *data, Evas_Object *obj, const char* emission, const char* source)
{
	_APPS_D("BG is released");
}


static key_cb_ret_e _back_key_cb(void *data)
{
	_APPS_D("Back key cb");
	Evas_Object *win = data;
	Evas_Object *popup = NULL;

	popup = evas_object_data_del(win, DATA_KEY_CHECK_POPUP);
	if (popup) {
		evas_object_del(popup);
	}

	Eina_Bool bVisible = evas_object_visible_get(win);
	_APPS_D("apps status:[%d]", bVisible);
	if(bVisible) {
		if (APPS_ERROR_FAIL == apps_layout_show(win, EINA_FALSE)) {
			_APPS_E("Cannot show tray");
		}
		return KEY_CB_RET_STOP;
	}
	else {
		return KEY_CB_RET_CONTINUE;
	}
}


static apps_error_e _pause_result_cb(void *data)
{
	instance_info_s *info = data;

	retv_if(!info, APPS_ERROR_INVALID_PARAMETER);
	retv_if(!info->layout, APPS_ERROR_FAIL);

	Evas_Object *popup = evas_object_data_get(info->layout, DATA_KEY_POPUP);
	if(popup) {
		evas_object_del(popup);
	}

	popup = evas_object_data_del(info->win, DATA_KEY_CHECK_POPUP);
	if (popup) {
		evas_object_del(popup);
	}

	evas_object_data_set(info->layout, DATA_KEY_POPUP, NULL);
	evas_object_data_set(info->layout, DATA_KEY_LAYOUT_IS_PAUSED, (void *) 1);

	return APPS_ERROR_NONE;
}



static apps_error_e _resume_result_cb(void *data)
{
	instance_info_s *info = data;

	retv_if(!info, APPS_ERROR_INVALID_PARAMETER);
	retv_if(!info->layout, APPS_ERROR_FAIL);

	evas_object_data_set(info->layout, DATA_KEY_LAYOUT_IS_PAUSED, NULL);

	return APPS_ERROR_NONE;
}



static apps_error_e _reset_result_cb(void *data)
{
	instance_info_s *info = data;
	Evas_Object *layout = NULL;

	retv_if(!info, APPS_ERROR_INVALID_PARAMETER);

	layout = evas_object_data_get(info->win, DATA_KEY_LAYOUT);
	retv_if(!layout, APPS_ERROR_FAIL);
	evas_object_data_set(layout, DATA_KEY_LAYOUT_IS_PAUSED, NULL);

	return APPS_ERROR_NONE;
}



static Eina_Bool _move_timer_cb(void *data)
{
	Evas_Object *scroller = data;
	int checker_type = 0;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);
	checker_type = (int)evas_object_data_get(scroller, PRIVATE_DATA_KEY_CHECKER_TYPE);

	apps_scroller_unfreeze(scroller);
	apps_scroller_bring_in_region_by_vector(scroller, checker_type);
	apps_scroller_freeze(scroller);

	return ECORE_CALLBACK_RENEW;
}



#define TIME_MOVE_SCROLLER 1.0f
static void _upper_start_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *checker = obj;
	Evas_Object *layout = data;
	Evas_Object *scroller = NULL;
	Ecore_Timer *timer = NULL;
	int checker_type = 0;

	_APPS_D("checker upper start");

	ret_if(!checker);
	ret_if(!layout);
	scroller = evas_object_data_get(layout, DATA_KEY_SCROLLER);
	ret_if(!scroller);
	checker_type = (int)evas_object_data_get(checker, PRIVATE_DATA_KEY_CHECKER_TYPE);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_CHECKER_TYPE, (void *)checker_type);

	timer = evas_object_data_del(scroller, DATA_KEY_EVENT_UPPER_TIMER);
	if (timer) ecore_timer_del(timer);

	timer = ecore_timer_add(TIME_MOVE_SCROLLER, _move_timer_cb, scroller);
	if (timer) evas_object_data_set(scroller, DATA_KEY_EVENT_UPPER_TIMER, timer);
	else _APPS_E("Cannot add a timer");
}



static void _upper_end_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *checker = obj;
	Evas_Object *layout = data;
	Evas_Object *scroller = NULL;
	Ecore_Timer *timer = NULL;

	_APPS_D("checker upper end");

	ret_if(!checker);
	ret_if(!layout);
	scroller = evas_object_data_get(layout, DATA_KEY_SCROLLER);
	ret_if(!scroller);

	timer = evas_object_data_del(scroller, DATA_KEY_EVENT_UPPER_TIMER);
	if (timer) {
		ecore_timer_del(timer);
	}
	evas_object_data_del(scroller, PRIVATE_DATA_KEY_CHECKER_TYPE);
}



static Evas_Object *_create_checker(Evas_Object *layout, int type)
{
	Evas_Object *checker;

	retv_if(!layout, NULL);

	checker = elm_button_add(layout);
	evas_object_size_hint_weight_set(checker, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(checker, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_color_set(checker, 0, 0, 0, 0);
	if (MOVE_UP == type) {
		elm_object_part_content_set(layout, "top_checker", checker);
		evas_object_data_set(checker, PRIVATE_DATA_KEY_CHECKER_TYPE, (void *)MOVE_UP);
	} else if (MOVE_DOWN == type) {
		elm_object_part_content_set(layout, "bottom_checker", checker);
		evas_object_data_set(checker, PRIVATE_DATA_KEY_CHECKER_TYPE, (void *)MOVE_DOWN);
	}
	evas_object_data_set(checker, DATA_KEY_EVENT_UPPER_IS_ON, (void *) 1);

	evas_object_smart_callback_add(checker, "upper_start", _upper_start_cb, layout);
	evas_object_smart_callback_add(checker, "upper_end", _upper_end_cb, layout);
	evas_object_show(checker);

	return checker;
}



static void _destroy_checker(Evas_Object *checker)
{
	evas_object_data_del(checker, DATA_KEY_EVENT_UPPER_IS_ON);
	evas_object_data_del(checker, PRIVATE_DATA_KEY_CHECKER_TYPE);
	evas_object_del(checker);
}



static Eina_Bool _push_items_cb(void *layout)
{
	Evas_Object *scroller = NULL;
	Eina_List *item_info_list = NULL;
	instance_info_s *info = NULL;
	int count = 0;

	retv_if(!layout, ECORE_CALLBACK_CANCEL);

	info = evas_object_data_get(layout, DATA_KEY_INSTANCE_INFO);
	retv_if(!info, ECORE_CALLBACK_CANCEL);

	/* scroller */
	scroller = apps_scroller_create(layout);
	retv_if(!scroller, ECORE_CALLBACK_CANCEL);
	evas_object_data_set(layout, DATA_KEY_SCROLLER, scroller);
	evas_object_show(scroller);

	elm_object_part_content_set(layout, "scroller", scroller);

	if (apps_main_get_info()->first_boot) {
		item_info_list = apps_item_info_list_create(ITEM_INFO_LIST_TYPE_XML);
		apps_db_read_list(item_info_list);
		apps_db_sync();
		item_info_list = apps_db_write_list();
	} else {
		apps_db_sync();
		item_info_list = apps_db_write_list();
	}
	apps_main_get_info()->first_boot = 0;

	/* Retry to write a list */
	if (item_info_list) {
		count = eina_list_count(item_info_list);
	}
	if (!item_info_list || !count) {
		_APPS_E("Cannot read the DB, normally");
		item_info_list = apps_item_info_list_create(ITEM_INFO_LIST_TYPE_XML);
	}
	retv_if(!item_info_list, ECORE_CALLBACK_CANCEL);

	count = eina_list_count(item_info_list);
	if (0 == count) {
		_APPS_D("There are no apps");
		return ECORE_CALLBACK_CANCEL;
	}

	evas_object_data_set(layout, DATA_KEY_LIST, item_info_list);
	evas_object_data_del(layout, DATA_KEY_IDLE_TIMER);

	return ECORE_CALLBACK_CANCEL;
}


static char *_access_info_cb(void *data, Evas_Object *obj)
{
	retv_if(!data, NULL);

	char *tmp = strdup(data);
	retv_if(!tmp, NULL);

	return tmp;
}


HAPI Evas_Object *apps_layout_create(Evas_Object *win, const char *file, const char *group)
{
	Evas_Object *layout;
	Ecore_Idler *idle_timer = NULL;
	instance_info_s *info = NULL;

	retv_if(!win, NULL);

	info = evas_object_data_get(win, DATA_KEY_INSTANCE_INFO);
	retv_if(!info, NULL);

	layout = apps_layout_load_edj(win, file, group);
	retv_if(!layout, NULL);

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_resize(layout, info->root_w, info->root_h);
	evas_object_show(layout);

	//box focus
	Evas_Object *focus = elm_button_add(layout);
	retv_if(!focus, NULL);

	elm_object_style_set(focus, "focus");
	elm_object_part_content_set(layout, "focus", focus);
	elm_access_info_cb_set(focus, ELM_ACCESS_INFO, _access_info_cb, _("IDS_IDLE_BODY_APPS"));
	elm_access_info_cb_set(focus, ELM_ACCESS_TYPE, NULL, NULL);
	elm_object_focus_allow_set(focus, EINA_FALSE);

	evas_object_data_set(layout, DATA_KEY_WIN, win);
	evas_object_data_set(layout, DATA_KEY_INSTANCE_INFO, info);
	evas_object_data_set(layout, DATA_KEY_LAYOUT_FOCUS_ON, NULL);
	evas_object_data_set(layout, DATA_KEY_LAYOUT_IS_PAUSED, (void *) 1);
	evas_object_data_set(layout, DATA_KEY_POPUP, NULL);

	elm_object_signal_callback_add(layout,
		"bg,down", "bg", _bg_down_cb, NULL);
	elm_object_signal_callback_add(layout,
		"bg,up", "bg", _bg_up_cb, NULL);

	if (APPS_ERROR_NONE != apps_main_register_cb(info, APPS_APP_STATE_PAUSE, _pause_result_cb, info)) {
		_APPS_E("Cannot register the pause callback");
	}

	if (APPS_ERROR_NONE != apps_main_register_cb(info, APPS_APP_STATE_RESUME, _resume_result_cb, info)) {
		_APPS_E("Cannot register the pause callback");
	}

	if (APPS_ERROR_NONE != apps_main_register_cb(info, APPS_APP_STATE_RESET, _reset_result_cb, info)) {
		_APPS_E("Cannot register the pause callback");
	}

	item_badge_register_changed_cb(layout);

	idle_timer = ecore_idler_add(_push_items_cb, layout);
	if (NULL == idle_timer) {
		_APPS_E("Cannot push items");
	} else evas_object_data_set(layout, DATA_KEY_IDLE_TIMER, idle_timer);

	return layout;
}



HAPI void apps_layout_destroy(Evas_Object *layout)
{
	Ecore_Idler *idle_timer = NULL;
	Evas_Object *scroller = NULL;
	Eina_List *list = NULL;
	instance_info_s *info = NULL;

	ret_if(!layout);

	idle_timer = evas_object_data_del(layout, DATA_KEY_IDLE_TIMER);
	if (idle_timer) {
		ecore_idler_del(idle_timer);
	}

	scroller = evas_object_data_del(layout, DATA_KEY_SCROLLER);
	if (scroller) {
		apps_scroller_destroy(layout);
	}

	list = evas_object_data_del(layout, DATA_KEY_LIST);
	if (list) {
		apps_item_info_list_destroy(list);
	}

	item_badge_unregister_changed_cb();

	info = evas_object_data_del(layout, DATA_KEY_INSTANCE_INFO);
	if (info) {
		apps_main_unregister_cb(info, APPS_APP_STATE_PAUSE, _pause_result_cb);
		apps_main_unregister_cb(info, APPS_APP_STATE_RESUME, _resume_result_cb);
		apps_main_unregister_cb(info, APPS_APP_STATE_RESET, _reset_result_cb);
	}

	edje_object_signal_callback_del(_EDJ(layout),
		"bg,down", "bg", _bg_down_cb);
	edje_object_signal_callback_del(_EDJ(layout),
		"bg,up", "bg", _bg_up_cb);

	evas_object_data_del(layout, DATA_KEY_WIN);
	evas_object_data_del(layout, DATA_KEY_LAYOUT_FOCUS_ON);
	evas_object_data_del(layout, DATA_KEY_LAYOUT_IS_PAUSED);

	apps_layout_unload_edj(layout);
}


static Eina_Bool _show_anim_cb(void *data)
{
	Evas_Object *win = data;

	retv_if(NULL == data, ECORE_CALLBACK_CANCEL);

	elm_win_activate(win);
	evas_object_show(win);

	layout_info_s *layout_info = evas_object_data_get(main_get_info()->layout, DATA_KEY_LAYOUT_INFO);
	retv_if(NULL == layout_info, ECORE_CALLBACK_CANCEL);

	Evas_Object *layout = evas_object_data_get(win, DATA_KEY_LAYOUT);
	retv_if(NULL == layout, ECORE_CALLBACK_CANCEL);

	Evas_Object *scroller = evas_object_data_get(layout, DATA_KEY_SCROLLER);
	retv_if(NULL == scroller, ECORE_CALLBACK_CANCEL);

	scroller_info_s *scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(NULL == scroller_info, ECORE_CALLBACK_CANCEL);

	if (main_get_info()->is_tts
		&& !layout_info->tutorial) {

		//box focus
		Evas_Object *focus = elm_object_part_content_get(layout, "focus");
		if(focus) {
			elm_access_highlight_set(focus);
		}
	}

	elm_object_signal_emit(scroller, "show", "");

	return ECORE_CALLBACK_CANCEL;
}



HAPI apps_error_e apps_layout_show(Evas_Object *win, Eina_Bool show)
{
	Evas_Object *layout = NULL;
	Ecore_Timer *timer = NULL;

	retv_if(NULL == win, APPS_ERROR_FAIL);

	layout = evas_object_data_get(win, DATA_KEY_LAYOUT);
	retv_if(NULL == layout, APPS_ERROR_FAIL);

	timer = evas_object_data_del(layout, DATA_KEY_LAYOUT_HIDE_TIMER);
	if (timer) ecore_timer_del(timer);

	_APPS_D("(%p) %s the tray", win, show? "Show":"Hide");

	if (show) {
		Ecore_Idler *animator = ecore_idler_add(_show_anim_cb, win);
		retv_if(NULL == animator, APPS_ERROR_FAIL);

		if (W_HOME_ERROR_NONE != key_register_cb(KEY_TYPE_BACK, _back_key_cb, win)) {
			_APPS_E("Cannot register the key callback");
		}
	} else {
		if(apps_main_get_info()->longpress_edit_disable && apps_layout_is_edited(layout)) {
			apps_layout_unedit(layout);
		}

		evas_object_hide(win);
		key_unregister_cb(KEY_TYPE_BACK, _back_key_cb);

		Evas_Object *scroller = evas_object_data_get(layout, DATA_KEY_SCROLLER);
		retv_if(NULL == scroller, APPS_ERROR_FAIL);
		apps_scroller_region_show(scroller, 0, 0);
	}

	return APPS_ERROR_NONE;
}



HAPI Evas_Object* apps_layout_load_edj(Evas_Object *parent, const char *edjname, const char *grpname)
{
	Evas_Object *layout = NULL;

	retv_if(NULL == parent, NULL);

	layout = elm_layout_add(parent);
	retv_if(NULL == layout, NULL);
	retv_if(EINA_FALSE == elm_layout_file_set(layout, edjname, grpname), NULL);

	Evas_Object *edje_object = _EDJ(layout);
	if (edje_object != NULL) {
		evas_object_data_set(edje_object, DATA_KEY_EVAS_OBJECT, layout);
		evas_object_data_set(layout, DATA_KEY_EDJE_OBJECT, edje_object);
	}
	evas_object_show(layout);

	return layout;
}



HAPI void apps_layout_unload_edj(Evas_Object *layout)
{
	Evas_Object *edje_object = NULL;

	ret_if(NULL == layout);

	edje_object = evas_object_data_del(layout, DATA_KEY_EDJE_OBJECT);
	if (edje_object) evas_object_data_del(edje_object, DATA_KEY_EVAS_OBJECT);
	else _APPS_E("Cannot get edje object");

	evas_object_del(layout);
}



HAPI void apps_layout_rotate(Evas_Object *layout, int is_rotated)
{
	const char *edje_signal;

	ret_if(NULL == layout);

	if (is_rotated) { // ROTATE
		edje_signal = STR_SIGNAL_EMIT_SIGNAL_ROTATE;
	} else { // UNROTATE
		edje_signal = STR_SIGNAL_EMIT_SIGNAL_UNROTATE;
	}

	elm_object_signal_emit(layout, edje_signal, "tray");
}



HAPI void apps_layout_focus_on(Evas_Object *layout)
{
	ret_if(NULL == layout);
	evas_object_data_set(layout, DATA_KEY_LAYOUT_FOCUS_ON, (void *) 1);
	elm_object_tree_focus_allow_set(layout, EINA_TRUE);
}



HAPI void apps_layout_focus_off(Evas_Object *layout)
{
	ret_if(NULL == layout);
	evas_object_data_set(layout, DATA_KEY_LAYOUT_FOCUS_ON, (void *) 0);
	elm_object_tree_focus_allow_set(layout, EINA_FALSE);
}



HAPI void apps_layout_block(Evas_Object *layout)
{
	ret_if(NULL == layout);
	elm_object_signal_emit(layout, "block", "layout");
}



HAPI void apps_layout_unblock(Evas_Object *layout)
{
	ret_if(NULL == layout);
	elm_object_signal_emit(layout, "unblock", "layout");
}



HAPI int apps_layout_is_edited(Evas_Object *layout)
{
	retv_if(!layout, 0);
	return (int) evas_object_data_get(layout, PRIVATE_DATA_KEY_LAYOUT_IS_EDITED);
}



HAPI void apps_layout_edit(Evas_Object *layout)
{
	Evas_Object *scroller = NULL;
	Evas_Object *checker = NULL;

	ret_if(!layout);

	evas_object_data_set(layout, PRIVATE_DATA_KEY_LAYOUT_IS_EDITED, (void *) 1);
	/* checker : it checkes whether or not scrolling the scroller */
	checker = _create_checker(layout, MOVE_UP);
	evas_object_data_set(layout, PRIVATE_DATA_KEY_TOP_CHECKER, checker);

	checker = _create_checker(layout, MOVE_DOWN);
	evas_object_data_set(layout, PRIVATE_DATA_KEY_BOTTOM_CHECKER, checker);

	scroller = evas_object_data_get(layout, DATA_KEY_SCROLLER);
	apps_scroller_edit(scroller);
}


static Eina_Bool _update_list_cb(void *data)
{
	Evas_Object *scroller = data;
	scroller_info_s *scroller_info = NULL;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);

	apps_scroller_write_list(scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);
	apps_db_read_list(scroller_info->list);

	/* Currently, update every times after going out from the edit mode */
	apps_main_get_info()->updated = 1;

	return ECORE_CALLBACK_CANCEL;
}



HAPI void apps_layout_unedit(Evas_Object *layout)
{
	Ecore_Timer *unedit_timer = NULL;
	Evas_Object *scroller = NULL;
	Evas_Object *checker = NULL;
	Evas_Object *reserve_item = NULL;
	item_info_s *item_info = NULL;

	ret_if(!layout);

	evas_object_data_del(layout, PRIVATE_DATA_KEY_LAYOUT_IS_EDITED);

	checker = evas_object_data_del(layout, PRIVATE_DATA_KEY_TOP_CHECKER);
	if (checker) {
		_destroy_checker(checker);
	}

	checker = evas_object_data_del(layout, PRIVATE_DATA_KEY_BOTTOM_CHECKER);
	if (checker) {
		_destroy_checker(checker);
	}
	scroller = evas_object_data_get(layout, DATA_KEY_SCROLLER);
	ret_if(!scroller);

	apps_scroller_unedit(scroller);

	char *appid = (char*)evas_object_data_get(scroller, DATA_KEY_ITEM_UNINSTALL_RESERVED);
	if(appid) {
		reserve_item = apps_scroller_get_item_by_appid(scroller, appid);
		ret_if(!reserve_item);

		item_info = evas_object_data_get(reserve_item, DATA_KEY_ITEM_INFO);

		apps_scroller_remove_item(scroller, reserve_item);
		apps_db_remove_item(appid);
		item_destroy(reserve_item);

		if (item_info) {
			_APPS_SD("appid[%s], name[%s]", item_info->appid, item_info->name);
			item_badge_remove(item_info->appid);
			item_badge_remove(item_info->pkgid);
			apps_item_info_destroy(item_info);
		}
	}
	apps_scroller_trim(scroller);
	evas_object_data_set(scroller, DATA_KEY_ITEM_UNINSTALL_RESERVED, (void*)NULL);

	unedit_timer = ecore_timer_add(0.5, _update_list_cb, scroller);
	if (NULL == unedit_timer) {
		_E("Cannot update list");
	}
}



// End of file
