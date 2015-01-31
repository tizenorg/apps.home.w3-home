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
#include <efl_assist.h>

#include "log.h"
#include "util.h"
#include "apps/apps_conf.h"
#include "apps/effect.h"
#include "apps/item.h"
#include "apps/item_badge.h"
#include "apps/item_info.h"
#include "apps/layout.h"
#include "apps/apps_main.h"
#include "apps/pkgmgr.h"
#include "apps/scroller.h"
#include "apps/scroller_info.h"
#include "apps/page.h"

#define PRIVATE_DATA_KEY_CALL_X_FOR_MOVING "p_c_x_mv"
#define PRIVATE_DATA_KEY_CALL_Y_FOR_MOVING "p_c_y_mv"
#define PRIVATE_DATA_KEY_ITEM_LONGPRESS_TIMER "p_i_lp_t"
#define PRIVATE_DATA_KEY_ITEM_ANIM_FOR_MOVING "p_it_ani_mv"
#define PRIVATE_DATA_KEY_ITEM_PRESSED "p_i_p"
#define PRIVATE_DATA_KEY_ITEM_DOWN_X "p_i_dx"
#define PRIVATE_DATA_KEY_ITEM_DOWN_Y "p_i_dy"
#define PRIVATE_DATA_KEY_ITEM_X_FOR_MOVING "p_i_x_mv"
#define PRIVATE_DATA_KEY_ITEM_Y_FOR_MOVING "p_i_y_mv"
#define PRIVATE_DATA_KEY_ITEM_X_FOR_ANIM "p_i_x"
#define PRIVATE_DATA_KEY_ITEM_Y_FOR_ANIM "p_i_y"
#define PRIVATE_DATA_KEY_ITEM_W "p_i_w"
#define PRIVATE_DATA_KEY_ITEM_H "p_i_h"
#define PRIVATE_DATA_KEY_ITEM_INNER "p_i_i"
#define PRIVATE_DATA_KEY_SCROLLER_PRESS_ITEM "p_sc_p"
#define PRIVATE_DATA_KEY_SCROLLER_Y "p_sc_y"
#define PRIVATE_DATA_KEY_ITEM_IS_LONGPRESS "p_i_lp"
#define PRIVATE_DATA_KEY_IS_VIRTUAL_ITEM "p_is_vi"
#define PRIVATE_DATA_KEY_ITEM_ANIM_FOR_DIMMING "p_anim_dm"

#define MOVE_THRESHOLD 5
#define CALL_THRESHOLD 50



static void _clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *item = data;
	item_info_s *item_info = NULL;

	_APPS_E("Clicked");

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	int isEdit = apps_layout_is_edited(item_info->layout);
	if(isEdit) {
		_APPS_E("Do not launch because of edit mode");
		return;
	}

	/* Widget has to be launched by aul_launch_app */

	if (!evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_IS_LONGPRESS)) {
		apps_effect_play_sound();

		if (apps_main_get_info()->tts && !item_info->tts) {
			char tmp[NAME_LEN] = {0,};
			snprintf(tmp, sizeof(tmp), _("IDS_SCR_POP_PS_IS_NOT_AVAILABLE_WHILE_SCREEN_READER_IS_ENABLED"), item_info->name);
			char *text = strdup(tmp);
			util_create_toast_popup(item_info->layout, text);
			return;
		}

		if (item_info->open_app) {
			_APPS_D("item(%s) launched to aul_open", item_info->type);
			apps_util_launch(item_info->win, item_info->appid, item_info->name);
		} else {
			_APPS_D("item(%s) launched to aul_launch", item_info->type);
			apps_util_launch_main_operation(item_info->win, item_info->appid, item_info->name);
		}
	} else {
		evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_IS_LONGPRESS, (void*)0);
	}
}



static Evas_Object *_content_set_item_inner(Evas_Object *item)
{
	Evas_Object *item_inner = NULL;
	item_info_s *item_info = NULL;

	item_inner = evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_INNER);
	retv_if(!item_inner, NULL);

	elm_object_part_content_set(item, "item_inner", item_inner);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, NULL);
	apps_scroller_unfreeze(item_info->scroller);

	return item;
}



static Evas_Object *_content_unset_item_inner(Evas_Object *item)
{
	Evas_Object *item_inner = NULL;
	item_info_s *item_info = NULL;

	item_inner = elm_object_part_content_unset(item, "item_inner");
	retv_if(!item_inner, NULL);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_INNER, item_inner);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, NULL);

	apps_scroller_freeze(item_info->scroller);

	return item;
}



#define ANIM_RATE 3
#define ANIM_RATE_SPARE ANIM_RATE - 1
static Eina_Bool _anim_move_item_to_empty_position(void *data)
{
	Evas_Object *item = data;
	item_info_s *item_info = NULL;
	Evas_Object *scroller = NULL;
	Evas_Object *item_inner = evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_INNER);
	int cur_x, cur_y, end_x, end_y;
	int vec_x, vec_y;

	goto_if(!data, ERROR);
	if(!item_inner) {
		_APPS_D("item_inner was not unset");
		goto ERROR;
	}

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	goto_if(!item_info, ERROR);
	scroller = item_info->scroller;

	evas_object_geometry_get(item_inner, &cur_x, &cur_y, NULL, NULL);
	end_x = (int)evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_X_FOR_ANIM);
	end_y = (int)evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_ANIM);

	if (cur_y == end_y && cur_x == end_x) {
		goto_if (NULL == _content_set_item_inner(item), ERROR);
		/* unfreeze the scroller after setting the content */
		if (scroller) apps_scroller_unfreeze(scroller);
		evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_X_FOR_ANIM);
		evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_ANIM);
		evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_ANIM_FOR_MOVING);

		if (apps_layout_is_edited(item_info->layout) && !apps_main_get_info()->longpress_edit_disable) {
			apps_layout_unedit(item_info->layout);
		}

		return ECORE_CALLBACK_CANCEL;
	}

	vec_y = (end_y - cur_y)/ANIM_RATE;
	if (0 == vec_y) {
		if (end_y - cur_y < 0) vec_y = -1;
		else if (end_y - cur_y > 0) vec_y = 1;
	}
	cur_y += vec_y;

	vec_x = (end_x - cur_x)/ANIM_RATE;
	if (0 == vec_x) {
		if (end_x - cur_x < 0) vec_x = -1;
		else if (end_x - cur_x > 0) vec_x = 1;
	}
	cur_x += vec_x;

	evas_object_move(item_inner, cur_x, cur_y);

	return ECORE_CALLBACK_RENEW;

ERROR:
	if (item) {
		if (scroller) apps_scroller_unfreeze(scroller);

		evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_IS_LONGPRESS, (void *) 0);

		evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_ANIM_FOR_MOVING);
		evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_X_FOR_ANIM);
		evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_ANIM);

	}
	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _longpress_timer_cb(void *data)
{
	Evas_Object *item = NULL;
	item_info_s *item_info = NULL;
	Ecore_Timer *timer;

	item = data;
	retv_if(!item, ECORE_CALLBACK_CANCEL);

	_APPS_D("longpress start");

	apps_effect_play_vibration();

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, ECORE_CALLBACK_CANCEL);

	if (!apps_layout_is_edited(item_info->layout)) {
		apps_layout_edit(item_info->layout);
	}

	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_IS_LONGPRESS, (void *) 1);

	timer = evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_LONGPRESS_TIMER);
	if (timer) ecore_timer_del(timer);
	else return ECORE_CALLBACK_CANCEL;

	retv_if(!evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_PRESSED), ECORE_CALLBACK_CANCEL);
	retv_if(!_content_unset_item_inner(item), ECORE_CALLBACK_CANCEL);

	elm_object_signal_emit(item_info->item_inner, "name,hide", "item_inner");
	elm_object_signal_emit(item_info->layout, "show", "checker");

	return ECORE_CALLBACK_CANCEL;

}



static Eina_Bool _move_timer_cb(void *data)
{
	Evas_Object *item = data;
	Evas_Object *pressed_item = NULL;
	item_info_s *item_info = NULL;
	int idx_above_item = -1;
	int idx_below_item = -1;
	int x, y, w, h, sy;

	retv_if(!item, ECORE_CALLBACK_CANCEL);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, ECORE_CALLBACK_CANCEL);

	idx_below_item = apps_scroller_seek_item_position(item_info->scroller, item);
	retv_if(-1 == idx_below_item, ECORE_CALLBACK_CANCEL);

	pressed_item = evas_object_data_get(item_info->scroller, PRIVATE_DATA_KEY_SCROLLER_PRESS_ITEM);
	retv_if(!pressed_item, ECORE_CALLBACK_CANCEL);

	idx_above_item = apps_scroller_seek_item_position(item_info->scroller, pressed_item);
	retv_if(-1 == idx_above_item, ECORE_CALLBACK_CANCEL);

	evas_object_geometry_get(item_info->center, &x, &y, &w, &h);
	x = x + (w /2);
	y = y + (h /2);

	evas_object_data_set(pressed_item, PRIVATE_DATA_KEY_ITEM_X_FOR_ANIM, (void *) x);
	evas_object_data_set(pressed_item, PRIVATE_DATA_KEY_ITEM_Y_FOR_ANIM, (void *) y);

	elm_scroller_region_get(item_info->scroller, NULL, &sy, NULL, NULL);
	sy += y;
	evas_object_data_set(item_info->scroller, PRIVATE_DATA_KEY_SCROLLER_Y, (void *) sy);

	if (idx_above_item < idx_below_item) {
		apps_scroller_move_item_prev(item_info->scroller, pressed_item, item, pressed_item);
	} else {
		apps_scroller_move_item_next(item_info->scroller, item, pressed_item, pressed_item);
	}

	_APPS_D("Move an item(%s, %d) to (%d)", item_info->name, idx_below_item, idx_above_item);
	evas_object_data_del(item_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);

	return ECORE_CALLBACK_CANCEL;
}



#define TIME_MOVE_ITEM 0.1f
static void _upper_start_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *item = data;
	Ecore_Timer *timer = NULL;
	item_info_s *item_info = NULL;

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	_APPS_D("Upper start : %s", item_info->name);

	timer = evas_object_data_del(item_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);
	if (timer) ecore_timer_del(timer);

	timer = ecore_timer_add(TIME_MOVE_ITEM, _move_timer_cb, item);
	if (timer) evas_object_data_set(item_info->scroller, DATA_KEY_EVENT_UPPER_TIMER, timer);
	else _APPS_E("Cannot add a timer");
}



static void _upper_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *item = data;
	item_info_s *item_info = NULL;

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);
}



static void _upper_end_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *item = data;
	item_info_s *item_info = NULL;
	Ecore_Timer *timer = NULL;

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	_APPS_D("Upper end : %s", item_info->name);

	timer = evas_object_data_del(item_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);
	if (timer) ecore_timer_del(timer);
}



static void _down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *item = NULL;
	Evas_Object *item_inner = NULL;
	Evas_Event_Mouse_Down *ei = event_info;
	int x = ei->output.x;
	int y = ei->output.y;
	int center_x, center_y, center_w, center_h;
	int sy;
	Ecore_Timer *timer = NULL;
	Ecore_Animator *move_anim = NULL;
	Ecore_Animator *dim_anim = NULL;
	item_info_s *item_info = NULL;

	item = data;
	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	int pos_x, pos_y, width, height;
	evas_object_geometry_get(item, &pos_x, &pos_y, &width, &height);
	_APPS_D("Down (%s,%p) (%d, %d), item pos(%d,%d,%d,%d)", item_info->name, obj, x, y, pos_x, pos_y, width, height);

	if (apps_scroller_is_scrolling(item_info->scroller)) return;

	dim_anim = evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_ANIM_FOR_DIMMING);
	if (dim_anim) return;

	move_anim = evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_ANIM_FOR_MOVING);
	if (move_anim) return;

	item_inner = elm_object_part_content_get(item, "item_inner");
	if (!item_inner) {
		item_inner = evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_INNER);
		ret_if(!item_inner);
		elm_object_part_content_set(item, "item_inner", item_inner);
	}

	evas_object_data_set(item_info->scroller, PRIVATE_DATA_KEY_SCROLLER_PRESS_ITEM, item);

	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_PRESSED, (void *) 1);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_DOWN_X, (void *) x);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_DOWN_Y, (void *) y);

	evas_object_data_set(item, PRIVATE_DATA_KEY_CALL_X_FOR_MOVING, (void *)x);
	evas_object_data_set(item, PRIVATE_DATA_KEY_CALL_Y_FOR_MOVING, (void *)y);

	evas_object_geometry_get(item_info->center, &center_x, &center_y, &center_w, &center_h);
	center_x += (center_w /2);
	center_y += (center_h /2);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_X_FOR_ANIM, (void *) center_x);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_ANIM, (void *) center_y);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_X_FOR_MOVING, (void *) center_x);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_MOVING, (void *) center_y);

	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_W, (void *) center_w);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_H, (void *) center_h);

	elm_scroller_region_get(item_info->scroller, NULL, &sy, NULL, NULL);
	sy += center_y;
	evas_object_data_set(item_info->scroller, PRIVATE_DATA_KEY_SCROLLER_Y, (void *) sy);

	item_is_pressed(item);

	if(apps_main_get_info()->longpress_edit_disable) {
		_APPS_E("longpress edit mode disable");
		int isEdit = apps_layout_is_edited(item_info->layout);
		if(isEdit) {
			timer = ecore_timer_add(LONGPRESS_TIME, _longpress_timer_cb, item);
			if (timer) evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_LONGPRESS_TIMER, timer);
			else _APPS_E("Cannot add a timer");
		}
	}
	else {
		timer = ecore_timer_add(LONGPRESS_TIME, _longpress_timer_cb, item);
		if (timer) evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_LONGPRESS_TIMER, timer);
		else _APPS_E("Cannot add a timer");
	}
}



static void _call_object(Evas_Object *item, int cur_x, int cur_y)
{
	Evas_Object *scroller = NULL;
	Evas_Object *parent = NULL;
	Evas_Object *tmp = NULL;
	item_info_s *item_info = NULL;
	Eina_List *obj_list = NULL;
	const Eina_List *l, *ln;

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);
	scroller = item_info->scroller;

	evas_object_data_set(item_info->win, DATA_KEY_IS_ORDER_CHANGE, (void*)1);

	obj_list = evas_tree_objects_at_xy_get(item_info->instance_info->e, NULL, cur_x, cur_y);
	ret_if(!obj_list);

	EINA_LIST_FOREACH_SAFE(obj_list, l, ln, tmp) {
		parent = tmp;
		while (parent) {
			if (evas_object_data_get(parent, DATA_KEY_EVENT_UPPER_IS_ON)) {
				Evas_Object *old_obj = evas_object_data_get(scroller, DATA_KEY_EVENT_UPPER_ITEM);
				if (old_obj != parent) {
					evas_object_smart_callback_call(old_obj, "upper_end", NULL);
					evas_object_smart_callback_call(parent, "upper_start", NULL);
					evas_object_data_set(scroller, DATA_KEY_EVENT_UPPER_ITEM, parent);
				} else {
					evas_object_smart_callback_call(parent, "upper", NULL);
				}
				return;
			}

			parent = evas_object_smart_parent_get(parent);
			if (parent == scroller) {
				Evas_Object *old_obj = evas_object_data_get(scroller, DATA_KEY_EVENT_UPPER_ITEM);
				if (old_obj != scroller) evas_object_smart_callback_call(old_obj, "upper_end", NULL);
				evas_object_data_set(scroller, DATA_KEY_EVENT_UPPER_ITEM, parent);
				break;
			}
		}
	}

	eina_list_free(obj_list);
}



static void _move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Move *ei = event_info;
	Evas_Object *item = data;
	Evas_Object *item_inner = NULL;
	Ecore_Timer *timer = NULL;
	item_info_s *item_info = NULL;

	int down_x, down_y, center_x, center_y;
	int cur_x, cur_y, vec_x, vec_y;
	int move_x, move_y;

	ret_if(!item);
	if (!evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_PRESSED)) {
		return;
	}

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	cur_x = ei->cur.output.x;
	cur_y = ei->cur.output.y;

	down_x = (int) evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_DOWN_X);
	down_y = (int) evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_DOWN_Y);

	center_x = (int) evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_X_FOR_MOVING);
	center_y = (int) evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_MOVING);

	move_x = (int) evas_object_data_get(item, PRIVATE_DATA_KEY_CALL_X_FOR_MOVING);
	move_y = (int) evas_object_data_get(item, PRIVATE_DATA_KEY_CALL_Y_FOR_MOVING);

	vec_x = cur_x - down_x;
	vec_y = cur_y - down_y;

	center_x += vec_x;
	center_y += vec_y;

	timer = evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_LONGPRESS_TIMER);
	if (timer && (abs(vec_x) >= 20 || abs(vec_y) >= 20)) {
		evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_LONGPRESS_TIMER);
		ecore_timer_del(timer);
		return;
	}
	item_inner = evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_INNER);
	if (!item_inner) return;

	evas_object_move(item_inner, center_x, center_y);

	vec_x = cur_x - move_x;
	vec_y = cur_y - move_y;

	if (abs(vec_x) >= CALL_THRESHOLD || abs(vec_y) >= CALL_THRESHOLD) {
		_call_object(item, cur_x, cur_y);
		evas_object_data_set(item, PRIVATE_DATA_KEY_CALL_X_FOR_MOVING, (void *)cur_x);
		evas_object_data_set(item, PRIVATE_DATA_KEY_CALL_Y_FOR_MOVING, (void *)cur_y);
	}
}



static void _up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *item = data;
	Evas_Object *old_obj;
	Evas_Event_Mouse_Up *ei = event_info;
	Ecore_Timer *timer = NULL;
	item_info_s *item_info = NULL;
	Ecore_Animator *move_anim = NULL;
	Ecore_Animator *dim_anim = NULL;

	int x = ei->output.x;
	int y = ei->output.y;
	int down_sy, sy, sh;
	int center_x, center_y;

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	int pos_x, pos_y, width, height;
	evas_object_geometry_get(item, &pos_x, &pos_y, &width, &height);
	_APPS_D("Up (%s,%p) (%d, %d), item pos(%d,%d,%d,%d)", item_info->name, obj, x, y, pos_x, pos_y, width, height);

	item_is_released(item);

	//after apps_scroller_move_item_prev(), up_cb is comming again why???
	if(evas_object_data_get(item, PRIVATE_DATA_KEY_IS_VIRTUAL_ITEM)) {
		evas_object_data_del(item, PRIVATE_DATA_KEY_IS_VIRTUAL_ITEM);
		return;
	}

	dim_anim = evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_ANIM_FOR_DIMMING);
	if (dim_anim) {
		ecore_animator_del(dim_anim);
		dim_anim = NULL;
	}

	move_anim = evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_ANIM_FOR_MOVING);
	if (move_anim) return;

	evas_object_data_del(item_info->scroller, PRIVATE_DATA_KEY_SCROLLER_PRESS_ITEM);
	evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_PRESSED);
	evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_DOWN_X);
	evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_DOWN_Y);
	center_x = (int) evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_X_FOR_MOVING);
	center_y = (int) evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_MOVING);
	evas_object_data_del(item, PRIVATE_DATA_KEY_CALL_X_FOR_MOVING);
	evas_object_data_del(item, PRIVATE_DATA_KEY_CALL_Y_FOR_MOVING);


	evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_W);
	evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_H);

	old_obj = evas_object_data_del(item_info->scroller, DATA_KEY_EVENT_UPPER_ITEM);
	if (old_obj) {
		evas_object_smart_callback_call(old_obj, "upper_end", NULL);
	}

	timer = evas_object_data_del(item_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);
	if (timer) ecore_timer_del(timer);

	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_IS_LONGPRESS, (void *) 0);
	timer = evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_LONGPRESS_TIMER);
	if (timer) {
		ecore_timer_del(timer);
		timer = NULL;
		return;
	}

	elm_scroller_region_get(item_info->scroller, NULL, &sy, NULL, &sh);
	down_sy = (int)evas_object_data_del(item_info->scroller, PRIVATE_DATA_KEY_SCROLLER_Y);

	if (down_sy <= sy){
		evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_ANIM, (void *)-200);
	} else if (down_sy >= (sy + sh)) {
		evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_ANIM, (void *)1000);
	} else {
		int cur_x, cur_y, cur_w, cur_h;
		evas_object_geometry_get(item_info->center, &cur_x, &cur_y, &cur_w, &cur_h);
		cur_x += (cur_w /2);
		cur_y += (cur_h /2);
		if (cur_x != center_x || cur_y != center_y) {
			_APPS_D("cur_x,cur_y: [%d, %d]", cur_x, cur_y);
			evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_X_FOR_ANIM, (void *)cur_x);
			evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_ANIM, (void *)cur_y);
		}
	}

	//Check the virtual item
	Evas_Object *virtual_item = apps_page_get_item_at(item_info->page, 0);
	if(virtual_item != item)
	{
		int bIsVirtual = (int)evas_object_data_get(virtual_item, DATA_KEY_IS_VIRTUAL_ITEM);
		if(bIsVirtual)
		{
			item_info_s *virtual_item_info = evas_object_data_get(virtual_item, DATA_KEY_ITEM_INFO);
			int cur_x, cur_y, cur_w, cur_h;
			evas_object_geometry_get(virtual_item_info->center, &cur_x, &cur_y, &cur_w, &cur_h);
			cur_x += (cur_w /2);
			cur_y += (cur_h /2);
			_APPS_D("cur_x,cur_y: [%d, %d]", cur_x, cur_y);
			evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_X_FOR_ANIM, (void *)cur_x);
			evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_Y_FOR_ANIM, (void *)cur_y);
			evas_object_data_set(item, PRIVATE_DATA_KEY_IS_VIRTUAL_ITEM, (void *)EINA_TRUE);

			apps_scroller_move_item_prev(item_info->scroller, virtual_item, item, virtual_item);
		}
	}
#if 0
	move_anim = evas_object_data_del(item, PRIVATE_DATA_KEY_ITEM_ANIM_FOR_MOVING);
	if (move_anim) {
		ecore_animator_del(move_anim);
		move_anim = NULL;
	}
#endif
	move_anim = ecore_animator_add(_anim_move_item_to_empty_position, item);
	ret_if(!move_anim);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_ANIM_FOR_MOVING, move_anim);

	elm_object_signal_emit(item_info->item_inner, "name,show", "item_inner");
	elm_object_signal_emit(item_info->layout, "hide", "checker");
}


HAPI void item_destroy(Evas_Object *item)
{
	Evas_Object *icon = NULL;
	item_info_s *item_info = NULL;

	ret_if(!item);

	item_badge_destroy(item);

	item_info = evas_object_data_del(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	if (APPS_ERROR_NONE != apps_pkgmgr_item_list_remove_item(item_info->pkgid, item_info->appid, item)) {
		_APPS_E("Cannot remove item in the item_list");
	}

	icon = elm_object_part_content_unset(item_info->item_inner, "icon");
	if (icon) evas_object_del(icon);
	if (item_info->center) {
		evas_object_data_del(item_info->center, DATA_KEY_EVENT_UPPER_IS_ON);
		evas_object_del(item_info->center);
	}
	evas_object_del(item_info->item_inner);
	evas_object_del(item);
}



static char *_access_info_cb(void *data, Evas_Object *obj)
{
	Evas_Object *item = data;
	item_info_s *item_info = NULL;
	char *tmp = NULL;

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, NULL);

	tmp = strdup(item_info->name);
	retv_if(!tmp, NULL);
	return tmp;
}



static char *_access_item_badge_cb(void *data, Evas_Object *obj)
{
	Evas_Object *item = data;
	char *tmp = NULL;
	char number[BUFSZE];

	int nBadgeCount = item_badge_count(item);
	if(nBadgeCount == 1) {
		snprintf(number, sizeof(number), _("IDS_TTS_BODY_1_NEW_ITEM"));
	}
	else if(nBadgeCount > 1) {
		snprintf(number, sizeof(number), _("IDS_TTS_BODY_PD_NEW_ITEMS"), nBadgeCount);
	}
	else {
		return NULL;
	}

	tmp = strdup(number);
	retv_if(!tmp, NULL);

	return tmp;
}


#define ITEM_EDJE_FILE EDJEDIR"/apps_item.edj"
HAPI Evas_Object *item_create(Evas_Object *scroller, item_info_s *item_info)
{
	Evas_Object *item = NULL;
	Evas_Object *item_inner = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *center = NULL;
	Evas_Object *focus = NULL;

	scroller_info_s *scroller_info = NULL;
	instance_info_s *instance_info = NULL;

	double scale = elm_config_scale_get();
	int w = (int)(ITEM_WIDTH * scale);
	int h = (int)(ITEM_HEIGHT * scale);
	int w_ef, h_ef;

	retv_if(!scroller, NULL);
	retv_if(!item_info, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);
	instance_info = scroller_info->instance_info;

	/* Item outer */
	item = elm_layout_add(scroller);
	retv_if(NULL == item, NULL);
	elm_layout_file_set(item, ITEM_EDJE_FILE, "item");
	evas_object_repeat_events_set(item, EINA_TRUE);
	evas_object_size_hint_weight_set(item, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(item);

	/* Item inner */
	item_inner = elm_layout_add(scroller);
	goto_if(NULL == item_inner, ERROR);
	elm_layout_file_set(item_inner, ITEM_EDJE_FILE, "item_inner");
	evas_object_repeat_events_set(item_inner, EINA_TRUE);
	evas_object_size_hint_weight_set(item_inner, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(item_inner);
	elm_object_part_content_set(item, "item_inner", item_inner);

	/* Center */
	center = elm_layout_add(scroller);
	retv_if(NULL == center, NULL);
	elm_layout_file_set(center, ITEM_EDJE_FILE, "center");
	evas_object_size_hint_min_set(center, w, h);
	evas_object_show(center);
	evas_object_smart_callback_add(center, "upper_start", _upper_start_cb, item);
	evas_object_smart_callback_add(center, "upper", _upper_cb, item);
	evas_object_smart_callback_add(center, "upper_end", _upper_end_cb, item);
//	evas_object_event_callback_add(center, EVAS_CALLBACK_MOUSE_DOWN, _center_down_cb, item);
//	evas_object_event_callback_add(center, EVAS_CALLBACK_MOUSE_UP, _center_up_cb, item);
	evas_object_data_set(center, DATA_KEY_EVENT_UPPER_IS_ON, (void *) 1);

	/* Icon & Text */
	icon = evas_object_image_add(instance_info->e);
	goto_if(NULL == icon, ERROR);
	evas_object_repeat_events_set(icon, EINA_TRUE);
	evas_object_image_file_set(icon, item_info->icon, NULL);
	evas_object_image_filled_set(icon, EINA_TRUE);
	evas_object_show(icon);

	//Apply image effect
	ea_effect_h *ea_effect_stroke = ea_image_effect_create();
	ea_image_effect_add_outer_shadow(ea_effect_stroke, 0, 1.0, 1.5, 0x4C000000);
	ea_object_image_effect_set(icon, ea_effect_stroke);
	ea_image_effect_destroy(ea_effect_stroke);

	ea_effect_h *ea_effect = ea_image_effect_create();
	ea_image_effect_add_outer_shadow(ea_effect, -90, 4.0, 1.5, 0x4C000000);
	ea_object_image_effect_set(icon, ea_effect);
	ea_image_effect_destroy(ea_effect);

	/* We have to check the icon size after ea_effect */
	evas_object_image_size_get(icon, &w_ef, &h_ef);
	_APPS_D("Icon size after ea_effect (%d:%d)", w_ef, h_ef);

	elm_object_part_content_set(item_inner, "icon", icon);
	elm_object_part_text_set(item_inner, "name", item_info->name);

	focus = elm_button_add(item);
	retv_if(NULL == focus, NULL);

	elm_object_style_set(focus, "focus");
	elm_object_part_content_set(item, "focus", focus);
	elm_access_info_cb_set(focus, ELM_ACCESS_INFO, _access_info_cb, item);
	elm_access_info_cb_set(focus, ELM_ACCESS_TYPE, NULL, NULL);
	elm_access_info_cb_set(focus, ELM_ACCESS_STATE, _access_item_badge_cb, item);
	elm_object_focus_allow_set(focus, EINA_FALSE);
	evas_object_smart_callback_add(focus, "clicked", _clicked_cb, item);
	evas_object_event_callback_add(focus, EVAS_CALLBACK_MOUSE_DOWN, _down_cb, item);
	evas_object_event_callback_add(focus, EVAS_CALLBACK_MOUSE_MOVE, _move_cb, item);
	evas_object_event_callback_add(focus, EVAS_CALLBACK_MOUSE_UP, _up_cb, item);

	/* Item info */
	item_info->win = scroller_info->win;
	item_info->layout = scroller_info->layout;
	item_info->instance_info = instance_info;

	item_info->scroller = scroller;
	item_info->box = scroller_info->box;

	item_info->item = item;
	item_info->item_inner = item_inner;
	item_info->center = center;

	evas_object_data_set(item, DATA_KEY_ITEM_INFO, item_info);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_IS_LONGPRESS, (void *) 0);

	if (APPS_ERROR_NONE != item_badge_init(item)) _APPS_E("Cannot register badge");
	if (APPS_ERROR_NONE != apps_pkgmgr_item_list_append_item(item_info->pkgid, item_info->appid, item)) {
		_APPS_E("Cannot append item into the item_list");
	}

	return item;

ERROR:
	item_destroy(item);
	return NULL;
}



HAPI Evas_Object *item_virtual_create(Evas_Object *scroller)
{
	Evas_Object *item = NULL;
	Evas_Object *item_inner = NULL;
	Evas_Object *center = NULL;

	scroller_info_s *scroller_info = NULL;
	instance_info_s *instance_info = NULL;

	double scale = elm_config_scale_get();
	int w = (int)(ITEM_WIDTH * scale);
	int h = (int)(ITEM_HEIGHT * scale);

	retv_if(!scroller, NULL);

	item_info_s *item_info = calloc(1, sizeof(item_info_s));
	retv_if(!item_info, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	if (!scroller_info) {
		_E("cannot get the scroller_info");
		free(item_info);
		return NULL;
	}
	instance_info = scroller_info->instance_info;

	/* Item outer */
	item = elm_layout_add(scroller);
	if (NULL == item) {
		_E("cannot add a layout");
		free(item_info);
		return NULL;
	}
	elm_layout_file_set(item, ITEM_EDJE_FILE, "item");
	evas_object_repeat_events_set(item, EINA_TRUE);
	evas_object_size_hint_weight_set(item, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(item);
	evas_object_data_set(item, DATA_KEY_ITEM_INFO, item_info);

	/* Item inner */
	item_inner = elm_layout_add(scroller);
	goto_if(NULL == item_inner, ERROR);
	elm_layout_file_set(item_inner, ITEM_EDJE_FILE, "item_inner");
	evas_object_repeat_events_set(item_inner, EINA_TRUE);
	evas_object_size_hint_weight_set(item_inner, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(item_inner);
	elm_object_part_content_set(item, "item_inner", item_inner);

	/* Center */
	center = elm_layout_add(scroller);
	goto_if(NULL == center, ERROR);
	elm_layout_file_set(center, ITEM_EDJE_FILE, "center");
	evas_object_size_hint_min_set(center, w, h);
	evas_object_show(center);
	evas_object_smart_callback_add(center, "upper_start", _upper_start_cb, item);
	evas_object_smart_callback_add(center, "upper", _upper_cb, item);
	evas_object_smart_callback_add(center, "upper_end", _upper_end_cb, item);
//	evas_object_event_callback_add(center, EVAS_CALLBACK_MOUSE_DOWN, _center_down_cb, item);
//	evas_object_event_callback_add(center, EVAS_CALLBACK_MOUSE_UP, _center_up_cb, item);
	evas_object_data_set(center, DATA_KEY_EVENT_UPPER_IS_ON, (void *) 1);

	/* Item info */
	item_info->icon = NULL;
	item_info->name = NULL;

	item_info->win = scroller_info->win;
	item_info->layout = scroller_info->layout;
	item_info->instance_info = instance_info;

	item_info->scroller = scroller;
	item_info->box = scroller_info->box;

	item_info->item = item;
	item_info->item_inner = item_inner;
	item_info->center = center;

	return item;

ERROR:
	item_virtual_destroy(item);
	return NULL;
}



HAPI void item_virtual_destroy(Evas_Object *item)
{
	item_info_s *item_info = NULL;

	ret_if(!item);

	item_info = evas_object_data_del(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	if (item_info->center) {
		evas_object_data_del(item_info->center, DATA_KEY_EVENT_UPPER_IS_ON);
		evas_object_del(item_info->center);
	}
	evas_object_del(item_info->item_inner);
	evas_object_del(item);
	free(item_info);
}



HAPI void item_change_language(Evas_Object *item)
{
	item_info_s *item_info = NULL;

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);
	ret_if(!item_info->item_inner);

	elm_object_part_text_set(item_info->item_inner, "name", item_info->name);
}



HAPI void item_edit(Evas_Object *item)
{
	item_info_s *item_info = NULL;

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);
	ret_if(!item_info->item_inner);

//	elm_object_signal_emit(item_info->item, "edit", "item");
//	elm_object_signal_emit(item_info->item_inner, "edit", "item_inner");
}



HAPI void item_unedit(Evas_Object *item)
{
	item_info_s *item_info = NULL;

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);
	ret_if(!item_info->item_inner);

//	elm_object_signal_emit(item_info->item, "unedit", "item");
//	elm_object_signal_emit(item_info->item_inner, "unedit", "item_inner");
}



HAPI Evas_Object* item_get_press_item(Evas_Object *scroller)
{
	retv_if(!scroller, NULL);
	return (Evas_Object*) evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_PRESS_ITEM);
}



HAPI int item_is_longpressed(Evas_Object *item)
{
	retv_if(!item, 0);
	return (int) evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_IS_LONGPRESS);
}


static Eina_Bool _anim_item_pressed_cb(void *data)
{
	item_info_s *item_info = data;
	retv_if(!item_info, ECORE_CALLBACK_CANCEL);
	retv_if(!item_info->scroller, ECORE_CALLBACK_CANCEL);
	retv_if(!item_info->item_inner, ECORE_CALLBACK_CANCEL);

	if (apps_scroller_is_scrolling(item_info->scroller)) {
		_APPS_E("apps_scroller_is_scrolling");
		return ECORE_CALLBACK_CANCEL;
	}

	_APPS_W("item is pressed");

	elm_object_signal_emit(item_info->item_inner, "item,pressed", "item_inner");
	edje_message_signal_process();

	evas_object_data_del(item_info->item, PRIVATE_DATA_KEY_ITEM_ANIM_FOR_DIMMING);

	return ECORE_CALLBACK_CANCEL;
}

HAPI void item_is_pressed(Evas_Object *item)
{
	ret_if(!item);

	item_info_s *item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	Ecore_Animator *dim_anim = ecore_animator_add(_anim_item_pressed_cb, item_info);
	ret_if(!dim_anim);
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_ANIM_FOR_DIMMING, dim_anim);
}


static Eina_Bool _anim_item_released_cb(void *data)
{
	item_info_s *item_info = data;
	retv_if(!item_info, ECORE_CALLBACK_CANCEL);
	retv_if(!item_info->item_inner, ECORE_CALLBACK_CANCEL);

	_APPS_W("item is released");

	elm_object_signal_emit(item_info->item_inner, "item,released", "item_inner");
	edje_message_signal_process();

	return ECORE_CALLBACK_CANCEL;
}


HAPI void item_is_released(Evas_Object *item)
{
	ret_if(!item);

	item_info_s *item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	Ecore_Animator *dim_anim = ecore_animator_add(_anim_item_released_cb, item_info);
	ret_if(!dim_anim);
}

// End of this file
