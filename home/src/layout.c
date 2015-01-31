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
#include <efl_assist.h>
#include <bundle.h>
#include <dlog.h>

#include "util.h"
#include "conf.h"
#include "log.h"
#include "main.h"
#include "db.h"
#include "edit.h"
#include "edit_info.h"
#include "effect.h"
#include "key.h"
#include "layout.h"
#include "layout_info.h"
#include "dynamicbox_evas.h"
#include "page_info.h"
#include "scroller_info.h"
#include "scroller.h"
#include "index.h"
#include "util.h"
#include "xml.h"
#include "clock_service.h"
#include "power_mode.h"
#include "apps/apps_main.h"
#include "gesture.h"

#define PRIVATE_DATA_KEY_LAYOUT_DOWN_X "p_l_x"
#define PRIVATE_DATA_KEY_LAYOUT_DOWN_Y "p_l_y"
#define PRIVATE_DATA_KEY_LAYOUT_PRESSED "p_l_ps"
#define PRIVATE_DATA_KEY_LAYOUT_TIMER "p_l_t"
#define PRIVATE_DATA_KEY_CHECKER_TYPE "ck_tp"
#define PRIVATE_DATA_KEY_DOWN_X "dw_x"
#define PRIVATE_DATA_KEY_DOWN_Y "dw_y"
#define PRIVATE_DATA_KEY_LAYOUT_G_DOWN_Y "gdw_y"
#define PRIVATE_DATA_KEY_LAYOUT_G_FKICKUP_DONE "gdw_f_d"
#define PRIVATE_DATA_KEY_LEFT_CHECKER "top_ck"
#define PRIVATE_DATA_KEY_RIGHT_CHECKER "bt_ck"
#define PRIVATE_DATA_KEY_READY_TO_BEZEL_DOWN "pd_rt_bd"
#define PRIVATE_DATA_KEY_CENTER_LONG_PRESSED "pd_c_l_p"

#define MOVE_LEFT -1
#define MOVE_RIGHT 1

#define THRESHOLD_BEZEL_UP_H 60
#define THRESHOLD_BEZEL_UP_END_D_H 25
#define THRESHOLD_BEZEL_UP_MOVE_D_H 25
#define THRESHOLD_MOMENTUM_FLICK_Y 450
#define LONGPRESS_GRAY_ZONE_W 50
#define LONGPRESS_GRAY_ZONE_H 100

static w_home_error_e _pause_result_cb(void *data)
{
	Evas_Object *layout = data;
	Evas_Object *page = NULL;
	layout_info_s *layout_info = NULL;

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, W_HOME_ERROR_FAIL);

	page = scroller_get_focused_page(layout_info->scroller);
	retv_if(!page, W_HOME_ERROR_FAIL);
	page_focus(page);

	/* If TTS is on, use focus */
	if (main_get_info()->is_tts) {
		elm_object_tree_focus_allow_set(layout, EINA_FALSE);
		_D("tree_focus_allow_set layout(%p) as FALSE", layout);
	}

	return W_HOME_ERROR_NONE;
}



static w_home_error_e _resume_result_cb(void *data)
{
	Evas_Object *layout = data;
	layout_info_s *layout_info = NULL;

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, W_HOME_ERROR_FAIL);

	if (layout_info->tutorial) {
		/* If TTS is on, use focus */
		if (main_get_info()->is_tts) {
			elm_object_tree_focus_allow_set(layout, EINA_FALSE);
			_D("tree_focus_allow_set layout(%p) as FALSE", layout);
		}
	} else {
		Evas_Object *layout = NULL;
		Evas_Object *scroller = NULL;

		layout = evas_object_data_get(main_get_info()->win, DATA_KEY_LAYOUT);
		if (!layout) {
			return W_HOME_ERROR_FAIL;
		}

		/* If TTS is on, use focus */
		if (main_get_info()->is_tts) {
			elm_object_tree_focus_allow_set(layout, EINA_TRUE);
			_D("tree_focus_allow_set layout(%p) as TRUE", layout);
		}

		scroller = evas_object_data_get(layout, DATA_KEY_SCROLLER);
		if (!scroller) {
			return W_HOME_ERROR_FAIL;
		}
	}

	return W_HOME_ERROR_NONE;
}



static w_home_error_e _reset_result_cb(void *data)
{
	return W_HOME_ERROR_NONE;
}



static Eina_Bool _longpress_timer_cb(void *data)
{
	int no_effect = 0;
	Evas_Object *layout = data;
	Evas_Object *proxy_page = NULL;
	Evas_Object *effect_page = NULL;
	layout_info_s *layout_info = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	edit_info_s *edit_info = NULL;

	retv_if(!layout, ECORE_CALLBACK_CANCEL);
	evas_object_data_del(layout, PRIVATE_DATA_KEY_LAYOUT_TIMER);

	_D("Enter the edit mode");

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, ECORE_CALLBACK_CANCEL);
	retv_if(!layout_info->pressed_page, ECORE_CALLBACK_CANCEL);

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);

	page_info = evas_object_data_get(layout_info->pressed_page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, ECORE_CALLBACK_CANCEL);

	if (!page_info->layout_longpress) {
		_D("long-press is not supported on this page");
		return ECORE_CALLBACK_CANCEL;
	}

	switch (page_info->direction) {
	case PAGE_DIRECTION_LEFT:
		_D("There is already a pressed item");
		evas_object_data_set(layout_info->pressed_page, DATA_KEY_PAGE_ONHOLD_COUNT, (void*)1);
		retv_if(!edit_create_layout(layout, EDIT_MODE_LEFT), ECORE_CALLBACK_CANCEL);

		edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
		retv_if(!edit_info, ECORE_CALLBACK_CANCEL);

		proxy_page = evas_object_data_get(layout_info->pressed_page, DATA_KEY_PROXY_PAGE);
		retv_if(!proxy_page, ECORE_CALLBACK_CANCEL);

		effect_page = edit_create_minify_effect_page(proxy_page);
		retv_if(!effect_page, ECORE_CALLBACK_CANCEL);

		scroller_region_show_center_of(edit_info->scroller, proxy_page, SCROLLER_FREEZE_OFF, NULL, NULL, edit_minify_effect_page, effect_page);
		break;
	case PAGE_DIRECTION_CENTER:
		if (util_feature_enabled_get(FEATURE_CLOCK_SELECTOR) == 1) {
			if (clock_service_clock_selector_launch() > 0) {
				evas_object_data_set(layout_info->pressed_page, DATA_KEY_PAGE_ONHOLD_COUNT, (void*)1);
				evas_object_data_set(layout, PRIVATE_DATA_KEY_CENTER_LONG_PRESSED, (void*)1);
				scroller_freeze(layout_info->scroller);
			}
		} else {
			no_effect = 1;
		}
		break;
	case PAGE_DIRECTION_RIGHT:
		if (main_get_info()->is_tts) return ECORE_CALLBACK_CANCEL;
		if (layout_info->pressed_item) {
			_D("There is already a pressed item");
			evas_object_dynamicbox_force_mouse_up(layout_info->pressed_item);
		}
		retv_if(!edit_create_layout(layout, EDIT_MODE_RIGHT), ECORE_CALLBACK_CANCEL);

		edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
		retv_if(!edit_info, ECORE_CALLBACK_CANCEL);

		proxy_page = evas_object_data_get(layout_info->pressed_page, DATA_KEY_PROXY_PAGE);
		retv_if(!proxy_page, ECORE_CALLBACK_CANCEL);

		effect_page = edit_create_minify_effect_page(proxy_page);
		retv_if(!effect_page, ECORE_CALLBACK_CANCEL);

		scroller_region_show_center_of(edit_info->scroller, proxy_page, SCROLLER_FREEZE_OFF, NULL, NULL, edit_minify_effect_page, effect_page);

		if (layout_info->pressed_page == scroller_info->plus_page) {
			evas_object_data_set(layout_info->pressed_item, DATA_KEY_IS_LONGPRESS, (void *)1);
		}
		break;
	default:
		_D("Cannot reach here");
		break;
	}

	if (no_effect == 0) {
		effect_play_vibration();
	}

	return ECORE_CALLBACK_CANCEL;
}



static void _down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ei = event_info;
	Evas_Object *layout = obj;
	Ecore_Timer *timer = NULL;

	int is_gray_zone = 0;
	int x = ei->output.x;
	int y = ei->output.y;

	_D("Mouse is down on the layout");

	timer = evas_object_data_del(layout, PRIVATE_DATA_KEY_LAYOUT_TIMER);
	if (timer) ecore_timer_del(timer);

	evas_object_data_set(layout, PRIVATE_DATA_KEY_LAYOUT_PRESSED, (void *) 1);
	evas_object_data_set(layout, PRIVATE_DATA_KEY_LAYOUT_DOWN_X, (void *) x);
	evas_object_data_set(layout, PRIVATE_DATA_KEY_LAYOUT_DOWN_Y, (void *) y);

	double longpress_time = LONGPRESS_TIME;
	layout_info_s *layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	if (layout_info != NULL) {
		Evas_Object *page = scroller_get_focused_page(layout_info->scroller);
		if (page != NULL) {
			page_info_s *page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
			if (page_info != NULL) {
				if (page_info->direction == PAGE_DIRECTION_CENTER) {
					longpress_time = 0.4f;
				}
			}
		}
	}

	if ((x <= LONGPRESS_GRAY_ZONE_W) || (x >= (main_get_info()->root_w - LONGPRESS_GRAY_ZONE_W))) {
		is_gray_zone = 1;
	}
	if ((y <= LONGPRESS_GRAY_ZONE_H) || (y >= (main_get_info()->root_h - LONGPRESS_GRAY_ZONE_H))) {
		is_gray_zone = 1;
	}
	if (is_gray_zone == 1) {
		_W("we don't add a longpress timer, it's too close to home key");
		return;
	}

	timer = ecore_timer_add(longpress_time, _longpress_timer_cb, layout);
	if (timer) evas_object_data_set(layout, PRIVATE_DATA_KEY_LAYOUT_TIMER, timer);
	else _E("Cannot add a timer");
}



static void _move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Move *ei = event_info;
	Evas_Object *layout = obj;
	Ecore_Timer *timer = NULL;

	int down_x, down_y, vec_x, vec_y;
	int cur_x = ei->cur.output.x;
	int cur_y = ei->cur.output.y;

	if (!evas_object_data_get(layout, PRIVATE_DATA_KEY_LAYOUT_PRESSED)) return;

	down_x = (int) evas_object_data_get(layout, PRIVATE_DATA_KEY_LAYOUT_DOWN_X);
	down_y = (int) evas_object_data_get(layout, PRIVATE_DATA_KEY_LAYOUT_DOWN_Y);

	vec_x = cur_x - down_x;
	vec_y = cur_y - down_y;

	timer = evas_object_data_get(layout, PRIVATE_DATA_KEY_LAYOUT_TIMER);
	if (timer && (abs(vec_x) >= LONGPRESS_THRESHOLD || abs(vec_y) >= LONGPRESS_THRESHOLD)) {
		evas_object_data_del(layout, PRIVATE_DATA_KEY_LAYOUT_TIMER);
		ecore_timer_del(timer);
	}
}



static void _up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *layout = obj;
	Ecore_Timer *timer = NULL;
	layout_info_s *layout_info = NULL;

	_D("Mouse is up on the layout");

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	timer = evas_object_data_del(layout, PRIVATE_DATA_KEY_LAYOUT_TIMER);
	if (timer) ecore_timer_del(timer);

	if (evas_object_data_del(layout, PRIVATE_DATA_KEY_CENTER_LONG_PRESSED) != NULL) {
		if (layout_info->scroller) {
			scroller_unfreeze(layout_info->scroller);
			scroller_bring_in_by_push_type(layout_info->scroller, SCROLLER_PUSH_TYPE_CENTER, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_ANIMATOR);
		}
	}
}



static void _bezel_up_cb(void *data)
{
	Evas_Object *layout = data;
	Evas_Object *scroller = NULL;
	Evas_Object *focused_page = NULL;

	_D("Bezel up");

	ret_if(!layout);

	scroller = evas_object_data_get(layout, DATA_KEY_SCROLLER);
	ret_if(!scroller);

	if (PAGE_DIRECTION_CENTER != scroller_get_current_page_direction(scroller)) {
		return;
	}

	if (util_feature_enabled_get(FEATURE_APPS) == 0) {
		elm_object_signal_emit(layout, "bottom,show", "layout");
		return;
	}

	if (util_feature_enabled_get(FEATURE_APPS_BY_BEZEL_UP) == 0) {
		return;
	}

	if (scroller_is_scrolling(scroller)) {
		return;
	}

	if (layout_is_edit_mode(layout)) {
		return;
	}

	focused_page = scroller_get_focused_page(scroller);
	if (focused_page != NULL) {
		evas_object_data_set(focused_page, DATA_KEY_PAGE_ONHOLD_COUNT, (void*)1);
	}

	apps_main_show_count_add();
	apps_main_launch(APPS_LAUNCH_SHOW);
}



HAPI void layout_add_mouse_cb(Evas_Object *layout)
{
	evas_object_event_callback_add(layout, EVAS_CALLBACK_MOUSE_DOWN, _down_cb, NULL);
	evas_object_event_callback_add(layout, EVAS_CALLBACK_MOUSE_MOVE, _move_cb, NULL);
	evas_object_event_callback_add(layout, EVAS_CALLBACK_MOUSE_UP, _up_cb, NULL);
}



HAPI void layout_del_mouse_cb(Evas_Object *layout)
{
	evas_object_event_callback_del(layout, EVAS_CALLBACK_MOUSE_DOWN, _down_cb);
	evas_object_event_callback_del(layout, EVAS_CALLBACK_MOUSE_MOVE, _move_cb);
	evas_object_event_callback_del(layout, EVAS_CALLBACK_MOUSE_UP, _up_cb);
}



static Eina_Bool _move_timer_cb(void *data)
{
	Evas_Object *scroller = data;
	Evas_Object *cur_page = NULL;
	Evas_Object *move_page = NULL;
	scroller_info_s *scroller_info = NULL;
	int checker_type = 0;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);
	checker_type = (int)evas_object_data_get(scroller, PRIVATE_DATA_KEY_CHECKER_TYPE);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);

	cur_page = scroller_get_focused_page(scroller);
	if (MOVE_LEFT == checker_type) {
		move_page = scroller_get_left_page(scroller, cur_page);
		if (move_page && scroller_info->center != move_page) {
			elm_object_signal_emit(scroller_info->layout, "left,show", "layout");
			edit_push_page_before(scroller, cur_page, move_page);
		}
	} else {
		move_page = scroller_get_right_page(scroller, cur_page);
		if (move_page && scroller_info->plus_page != move_page) {
			elm_object_signal_emit(scroller_info->layout, "right,show", "layout");
			edit_push_page_after(scroller, cur_page, move_page);
		}
	}

	return ECORE_CALLBACK_RENEW;
}



#define TIME_MOVE_SCROLLER 1.0f
static void _upper_start_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *layout = data;
	Evas_Object *checker = obj;
	Ecore_Timer *timer = NULL;

	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;
	int checker_type = 0;

	_D("Upper start for the checker");

	ret_if(!layout);
	ret_if(!checker);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);

	checker_type = (int) evas_object_data_get(checker, PRIVATE_DATA_KEY_CHECKER_TYPE);
	evas_object_data_set(edit_info->scroller, PRIVATE_DATA_KEY_CHECKER_TYPE, (void *)checker_type);

	timer = evas_object_data_del(edit_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);
	if (timer) ecore_timer_del(timer);

	timer = ecore_timer_add(TIME_MOVE_SCROLLER, _move_timer_cb, edit_info->scroller);
	if (timer) evas_object_data_set(edit_info->scroller, DATA_KEY_EVENT_UPPER_TIMER, timer);
	else _E("Cannot add a timer");
}



static void _upper_end_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *layout = data;
	Evas_Object *checker = obj;
	Ecore_Timer *timer = NULL;

	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;

	_D("Upper end for the checker");

	ret_if(!checker);
	ret_if(!layout);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);

	timer = evas_object_data_del(edit_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);
	if (timer) ecore_timer_del(timer);
	evas_object_data_del(edit_info->scroller, PRIVATE_DATA_KEY_CHECKER_TYPE);
}



static Evas_Event_Flags _flick_start_cb(void *data, void *event_info)
{
	int gesture_down_y = 0;
	Evas_Object *layout = data;
	Elm_Gesture_Line_Info *ei = (Elm_Gesture_Line_Info *)event_info;
	retv_if(!layout, EVAS_EVENT_FLAG_NONE);
	retv_if(!ei, EVAS_EVENT_FLAG_NONE);

	gesture_down_y = (int) evas_object_data_get(layout, PRIVATE_DATA_KEY_LAYOUT_G_DOWN_Y);

	if (gesture_down_y <= THRESHOLD_BEZEL_UP_H
		&& ei->momentum.my >= THRESHOLD_MOMENTUM_FLICK_Y) {
		evas_object_data_set(layout, PRIVATE_DATA_KEY_READY_TO_BEZEL_DOWN, (void *) 1);
	}

	return EVAS_EVENT_FLAG_NONE;
}



static Evas_Event_Flags _flick_move_cb(void *data, void *event_info)
{
	int gesture_down_y = 0;
	Evas_Object *layout = data;
	Elm_Gesture_Line_Info *ei = (Elm_Gesture_Line_Info *)event_info;
	retv_if(!layout, EVAS_EVENT_FLAG_NONE);
	retv_if(!ei, EVAS_EVENT_FLAG_NONE);

	int is_flickup_done = (int) evas_object_data_get(layout, PRIVATE_DATA_KEY_LAYOUT_G_FKICKUP_DONE);
	gesture_down_y = (int) evas_object_data_get(layout, PRIVATE_DATA_KEY_LAYOUT_G_DOWN_Y);
	int vector_y = ei->momentum.y2 - ei->momentum.y1;
	int distance_x = abs( ei->momentum.x1 - ei->momentum.x2);
	int distance_y = abs(vector_y);

#if 0 //DEBUG
	_D("gesture_down_y:%d", gesture_down_y);
	_D("ei->momentum.my:%d", ei->momentum.my);
	_D("is_flickup_done:%d", is_flickup_done);
	_D("vector_y:%d", vector_y);
	_D("distance_x:%d", distance_x);
	_D("distance_y:%d", distance_y);
#endif

	if (vector_y < 0 &&
		(ei->momentum.my <= -THRESHOLD_MOMENTUM_FLICK_Y || distance_y >= THRESHOLD_BEZEL_UP_MOVE_D_H) &&
		distance_x < distance_y) {
		if (is_flickup_done == 0) {
			if (gesture_down_y >= (main_get_info()->root_h - THRESHOLD_BEZEL_UP_H)) {
				gesture_execute_cbs(BEZEL_UP);
			}
			evas_object_smart_callback_call(layout, LAYOUT_SMART_SIGNAL_FLICK_UP, layout);
			evas_object_data_set(layout, PRIVATE_DATA_KEY_LAYOUT_G_FKICKUP_DONE, (void *) 1);
		}
	}

	return EVAS_EVENT_FLAG_NONE;
}



static void _gesture_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ei = event_info;
	Evas_Object *layout = obj;

	int y = ei->output.y;

	_D("Mouse is down on the gesture layer:%d", y);

	evas_object_data_del(layout, PRIVATE_DATA_KEY_LAYOUT_G_FKICKUP_DONE);
	evas_object_data_set(layout, PRIVATE_DATA_KEY_LAYOUT_G_DOWN_Y, (void *) y);

}



static Evas_Event_Flags _flick_end_cb(void *data, void *event_info)
{
	int gesture_down_y = 0;
	Evas_Object *layout = data;
	Elm_Gesture_Line_Info *ei = (Elm_Gesture_Line_Info *)event_info;
	retv_if(!layout, EVAS_EVENT_FLAG_NONE);
	retv_if(!ei, EVAS_EVENT_FLAG_NONE);

	if (evas_object_data_del(layout, PRIVATE_DATA_KEY_READY_TO_BEZEL_DOWN)) {
		gesture_execute_cbs(BEZEL_DOWN);
	}

	int is_flickup_done = (int) evas_object_data_get(layout, PRIVATE_DATA_KEY_LAYOUT_G_FKICKUP_DONE);
	gesture_down_y = (int) evas_object_data_get(layout, PRIVATE_DATA_KEY_LAYOUT_G_DOWN_Y);
	int vector_y = ei->momentum.y2 - ei->momentum.y1;
	int distance_x = abs( ei->momentum.x1 - ei->momentum.x2);
	int distance_y = abs(vector_y);

#if 0 //DEBUG
	_D("gesture_down_y:%d", gesture_down_y);
	_D("ei->momentum.my:%d", ei->momentum.my);
	_D("is_flickup_done:%d", is_flickup_done);
	_D("vector_y:%d", vector_y);
	_D("distance_x:%d", distance_x);
	_D("distance_y:%d", distance_y);
#endif

	if (vector_y < 0 &&
		distance_y >= THRESHOLD_BEZEL_UP_END_D_H && //cannot use momentum
		distance_x < distance_y) {
		if (is_flickup_done == 0) {
			if (gesture_down_y >= (main_get_info()->root_h - THRESHOLD_BEZEL_UP_H)) {
				gesture_execute_cbs(BEZEL_UP);
			}
			evas_object_smart_callback_call(layout, LAYOUT_SMART_SIGNAL_FLICK_UP, layout);
			evas_object_data_set(layout, PRIVATE_DATA_KEY_LAYOUT_G_FKICKUP_DONE, (void *) 1);
		}
	}

	return EVAS_EVENT_FLAG_NONE;
}



static void _attach_gesture_layer(Evas_Object *layout)
{
	Evas_Object *gesture_layer = NULL;
	ret_if(!layout);

	gesture_layer = elm_gesture_layer_add(layout);
	ret_if(!gesture_layer);

	evas_object_event_callback_add(layout, EVAS_CALLBACK_MOUSE_DOWN, _gesture_down_cb, NULL);

	elm_gesture_layer_attach(gesture_layer, layout);
	elm_gesture_layer_cb_set(gesture_layer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_START, _flick_start_cb, layout);
	elm_gesture_layer_cb_set(gesture_layer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, _flick_end_cb, layout);
	elm_gesture_layer_cb_set(gesture_layer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_MOVE, _flick_move_cb, layout);
}



static Evas_Object *_create_checker(Evas_Object *layout, int type)
{
	Evas_Object *checker;

	retv_if(!layout, NULL);

	checker = elm_button_add(layout);
	evas_object_size_hint_weight_set(checker, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(checker, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_color_set(checker, 0, 0, 0, 0);
	if (MOVE_LEFT == type) {
		elm_object_part_content_set(layout, "left_checker", checker);
		evas_object_data_set(checker, PRIVATE_DATA_KEY_CHECKER_TYPE, (void *)MOVE_LEFT);
	} else if (MOVE_RIGHT == type) {
		elm_object_part_content_set(layout, "right_checker", checker);
		evas_object_data_set(checker, PRIVATE_DATA_KEY_CHECKER_TYPE, (void *)MOVE_RIGHT);
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



#define FILE_LAYOUT_EDJ EDJEDIR"/layout.edj"
#define GROUP_LAYOUT "layout"
HAPI Evas_Object *layout_create(Evas_Object *win)
{
	Evas_Object *layout = NULL;
	Evas_Object *checker = NULL;
	Evas_Object *scroller = NULL;
	layout_info_s *layout_info = NULL;
	scroller_info_s *scroller_info = NULL;
	Eina_Bool ret;

	retv_if(!win, NULL);

	layout = elm_layout_add(win);
	retv_if(NULL == layout, NULL);

	layout_info = calloc(1, sizeof(layout_info_s));
	if (!layout_info) {
		_E("Cannot calloc for layout_info");
		evas_object_del(layout);
		return NULL;
	}
	evas_object_data_set(layout, DATA_KEY_LAYOUT_INFO, layout_info);
	layout_info->win = win;

	ret = elm_layout_file_set(layout, FILE_LAYOUT_EDJ, GROUP_LAYOUT);
	if (EINA_FALSE == ret) {
		_E("cannot set the file into the layout");
		free(layout_info);
		evas_object_del(layout);
		return NULL;
	}

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(layout, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_resize(layout, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_show(layout);

	evas_object_data_set(win, DATA_KEY_LAYOUT, layout);
	evas_object_data_set(layout, DATA_KEY_WIN, win);

	if (W_HOME_ERROR_NONE != main_register_cb(APP_STATE_PAUSE, _pause_result_cb, layout)) {
		_E("Cannot register the pause callback");
	}

	if (W_HOME_ERROR_NONE != main_register_cb(APP_STATE_RESUME, _resume_result_cb, layout)) {
		_E("Cannot register the resume callback");
	}

	if (W_HOME_ERROR_NONE != main_register_cb(APP_STATE_RESET, _reset_result_cb, layout)) {
		_E("Cannot register the reset callback");
	}

	if (W_HOME_ERROR_NONE != gesture_register_cb(BEZEL_UP, _bezel_up_cb, layout)) {
		_E("Cannot register the gesture callback");
	}

	layout_add_mouse_cb(layout);

	checker = _create_checker(layout, MOVE_LEFT);
	evas_object_data_set(layout, PRIVATE_DATA_KEY_LEFT_CHECKER, checker);

	checker = _create_checker(layout, MOVE_RIGHT);
	evas_object_data_set(layout, PRIVATE_DATA_KEY_RIGHT_CHECKER, checker);

	scroller = scroller_create(layout, layout, main_get_info()->root_w, main_get_info()->root_h, SCROLLER_INDEX_PLURAL);
	if (!scroller) {
		_E("Cannot create scroller");
		free(layout_info);
		evas_object_del(layout);
		return NULL;
	}
	elm_object_part_content_set(layout, "scroller", scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	if (!scroller_info) {
		_E("Cannot create scroller");
		scroller_destroy(layout);
		free(layout_info);
		evas_object_del(layout);
		return NULL;
	}
	layout_info->scroller = scroller;

	/* Scroller has to unpack all page_inners on scroll */
	scroller_info->unpack_page_inners_on_scroll = 1;
	scroller_info->scroll_effect = 1;

	scroller_info->index[PAGE_DIRECTION_LEFT] = index_create(layout, scroller, PAGE_DIRECTION_LEFT);
	if (!scroller_info->index[PAGE_DIRECTION_LEFT]) _E("Cannot create the left index");
	else elm_object_part_content_set(layout, "left_index", scroller_info->index[PAGE_DIRECTION_LEFT]);

	scroller_info->index[PAGE_DIRECTION_RIGHT] = index_create(layout, scroller, PAGE_DIRECTION_RIGHT);
	if (!scroller_info->index[PAGE_DIRECTION_RIGHT]) _E("Cannot create the right index");
	else elm_object_part_content_set(layout, "right_index", scroller_info->index[PAGE_DIRECTION_RIGHT]);
	layout_hide_index(layout);

	_attach_gesture_layer(layout);

	return layout;
}



HAPI void layout_destroy(Evas_Object *win)
{
	Evas_Object *layout = NULL;
	Evas_Object *checker = NULL;
	Evas_Object *scroller = NULL;
	layout_info_s *layout_info = NULL;
	scroller_info_s *scroller_info = NULL;
	Eina_List *page_info_list = NULL;

	ret_if(win);

	layout = evas_object_data_del(win, DATA_KEY_LAYOUT);
	ret_if(!layout);

	scroller = evas_object_data_del(layout, DATA_KEY_SCROLLER);
	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	if (scroller_info->index[PAGE_DIRECTION_LEFT]) {
		index_destroy(scroller_info->index[PAGE_DIRECTION_LEFT]);
	}

	if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
		index_destroy(scroller_info->index[PAGE_DIRECTION_RIGHT]);
	}

	scroller_pop_pages(scroller, PAGE_DIRECTION_ANY);
	scroller_destroy(layout);

	checker = evas_object_data_del(layout, PRIVATE_DATA_KEY_LEFT_CHECKER);
	if (checker) {
		_destroy_checker(checker);
	}

	checker = evas_object_data_del(layout, PRIVATE_DATA_KEY_RIGHT_CHECKER);
	if (checker) {
		_destroy_checker(checker);
	}

	main_unregister_cb(APP_STATE_PAUSE, _pause_result_cb);
	main_unregister_cb(APP_STATE_RESUME, _resume_result_cb);
	main_unregister_cb(APP_STATE_RESET, _reset_result_cb);
	gesture_unregister_cb(BEZEL_UP, _bezel_up_cb);

	evas_object_data_del(layout, DATA_KEY_WIN);

	layout_info = evas_object_data_del(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	page_info_list_destroy(page_info_list);
	free(layout_info);

	evas_object_del(layout);
}



HAPI void layout_show_left_index(Evas_Object *layout)
{
	ret_if(!layout);
	elm_object_signal_emit(layout, "show", "left_index");
	elm_object_signal_emit(layout, "hide", "right_index");
}



HAPI void layout_show_right_index(Evas_Object *layout)
{
	ret_if(!layout);
	elm_object_signal_emit(layout, "show", "right_index");
	elm_object_signal_emit(layout, "hide", "left_index");
}



HAPI void layout_hide_index(Evas_Object *layout)
{
	ret_if(!layout);
	elm_object_signal_emit(layout, "hide", "right_index");
	elm_object_signal_emit(layout, "hide", "left_index");
}



HAPI int layout_is_edit_mode(Evas_Object *layout)
{
	layout_info_s *layout_info = NULL;

	retv_if(!layout, 0);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, 0);

	return layout_info->edit? 1 : 0;
}



HAPI void layout_set_idle(Evas_Object *layout)
{
	layout_info_s *layout_info = NULL;

	ret_if(!layout);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	if (evas_object_data_get(layout, DATA_KEY_ADD_VIEWER) != NULL) {
		_D("destroy a addviewer");
		edit_destroy_add_viewer(layout);
	}

	if (layout_info->edit != NULL) {
		_D("destroy a editing layout");
		edit_destroy_layout(layout);
	}
}



HAPI int layout_is_idle(Evas_Object *layout)
{
	layout_info_s *layout_info = NULL;

	retv_if(!layout, 1);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, 1);

	if (evas_object_data_get(layout, DATA_KEY_ADD_VIEWER) != NULL) {
		_W("Addview is exist");
		return 0;
	}

	if (layout_info->edit != NULL) {
		_W("editing is in progress");
		return 0;
	}

	return 1;
}
// End of file
