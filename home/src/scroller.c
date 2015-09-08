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
#include <stdbool.h>
#include <stdio.h>
#include <dlog.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
#include <bundle.h>
#include <efl_assist.h>
#include <vconf.h>
#include <efl_extension.h>
#include <widget_viewer_evas.h>

#include "bg.h"
#include "util.h"
#include "wms.h"
#include "conf.h"
#include "power_mode.h"
#include "key.h"
#include "dbus.h"
#include "layout.h"
#include "layout_info.h"
#include "widget.h"
#include "log.h"
#include "main.h"
#include "minictrl.h"
#include "page_info.h"
#include "page.h"
#include "pkgmgr.h"
#include "scroller_info.h"
#include "scroller.h"
#include "index.h"
#include "db.h"

#define BOX_EDJE EDJEDIR"/box_layout.edj"
#define BOX_GROUP_NAME "box"

#define PRIVATE_DATA_KEY_HIDE_INDEX_TIMER "p_h_ix_tm"
#define PRIVATE_DATA_KEY_SHOW_END "p_s_e"
#define PRIVATE_DATA_KEY_IS_DUMMY "p_is_dum"
#define PRIVATE_DATA_KEY_SCROLLER_FREEZED "p_sc_is_fr"
#define PRIVATE_DATA_KEY_SCROLLER_PRESSED_PAGE "p_sc_pr_pg"
#define PRIVATE_DATA_KEY_SCROLLER_PUSH_ALL_TIMER "p_p_tm"
#define PRIVATE_DATA_KEY_SCROLLER_LIST "p_sc_l"
#define PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER "p_sc_tmer"
#define PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM "p_sc_anim"
#define PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB "p_sc_job"
#define PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE "p_sc_page"
#define PRIVATE_DATA_KEY_SCROLLER_HIDE_INDEX_TIMER "p_sc_ht"
#define PRIVATE_DATA_KEY_SCROLLER_BEFORE_FUNC "pdk_sc_bf_fn"
#define PRIVATE_DATA_KEY_SCROLLER_BEFORE_FUNC_DATA "pdk_sc_bf_fn_d"
#define PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC "pdk_sc_af_fn"
#define PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC_DATA "pdk_sc_af_fn_d"



static void _mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Ecore_Timer *timer = NULL;
	Evas_Object *layout = data;
	Evas_Object *scroller = obj;
	scroller_info_s *scroller_info = NULL;

	ret_if(!layout);

	home_dbus_scroll_booster_signal_send(200);

	timer = evas_object_data_del(layout, PRIVATE_DATA_KEY_HIDE_INDEX_TIMER);
	if (timer) {
		ecore_timer_del(timer);
		timer = NULL;
	}

	elm_object_signal_emit(layout, "show", "index");

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	scroller_info->mouse_down = 1;
}



static void _mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *scroller = obj;
	scroller_info_s *scroller_info = NULL;

#if 0 //TBD
	home_dbus_scroll_booster_signal_send(0);
#endif

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	scroller_info->mouse_down = 0;
}



HAPI Evas_Object *scroller_get_focused_page(Evas_Object *scroller)
{
	Evas_Object *obj = NULL;
	Evas_Object *cur_page = NULL;
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	int x, w;
	int half_line, whole_line, pad;

	retv_if(!scroller, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	pad = (scroller_info->root_width - scroller_info->page_width) / 2;
	half_line = scroller_info->root_width / 2;
	whole_line = scroller_info->root_width;

	list = elm_box_children_get(scroller_info->box);
	if (!list) return NULL;

	EINA_LIST_FREE(list, obj) {
		continue_if(!obj);
		if (cur_page) continue;

		page_info = evas_object_data_get(obj, DATA_KEY_PAGE_INFO);
		continue_if(!page_info);

		if (!page_info->appended) {
			_D("Page(%p) is not appended yet", obj);
			continue;
		}

		evas_object_geometry_get(obj, &x, NULL, &w, NULL);
		if (!w) continue;
		if (x + w > half_line && x <= whole_line - pad) {
			cur_page = obj;
		} else if (x <= half_line && x >= pad) {
			cur_page = obj;
		}
	}

	return cur_page;
}



static int _set_scroller_reverse_by_page(Evas_Object *scroller, Evas_Object *target_page)
{
	Evas_Object *focus_page = NULL;
	Evas_Object *tmp = NULL;
	Eina_List *list = NULL;
	const Eina_List *l;
	scroller_info_s *scroller_info = NULL;
	int after_focus = 0;

	retv_if(!scroller, 0);

	focus_page = scroller_get_focused_page(scroller);
	if (!focus_page) return 0;

	/* Center */
	if (focus_page == target_page) return 0;

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, 0);

	list = elm_box_children_get(scroller_info->box);
	if (!list) return 0;

	EINA_LIST_FOREACH(list, l, tmp) {
		/* if the target is focused, then it is not after_focus */
		if (tmp == target_page) break;
		if (tmp == focus_page) after_focus = 1;
	}
	eina_list_free(list);

	/* Right side */
	if (after_focus) return 1;

	/* Left side */
	return -1;
}



static void _elm_box_pack_start(Evas_Object *scroller, Evas_Object *page)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);
	ret_if(!page);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

#if 0 /* EFL Private feature */
	elm_scroller_origin_reverse_set(scroller, EINA_TRUE, EINA_TRUE);
#endif

	_D("pack_start a page(%p) into the scroller(%p), origin_reverse(1)", page, scroller);
	elm_box_pack_start(scroller_info->box, page);
	/* recalculate : child box with pages -> parent box */
	elm_box_recalculate(scroller_info->box);
	elm_box_recalculate(scroller_info->box_layout);
}



static void _elm_box_pack_end(Evas_Object *scroller, Evas_Object *page)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);
	ret_if(!page);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

#if 0 /* EFL Private feature */
	elm_scroller_origin_reverse_set(scroller, EINA_FALSE, EINA_FALSE);
#endif

	_D("pack_end a page(%p) into the scroller(%p), origin_reverse(0)", page, scroller);
	elm_box_pack_end(scroller_info->box, page);
	/* recalculate : child box with pages -> parent box */
	elm_box_recalculate(scroller_info->box);
	elm_box_recalculate(scroller_info->box_layout);
}



static void _elm_box_pack_before(Evas_Object *scroller, Evas_Object *page, Evas_Object *before)
{
	scroller_info_s *scroller_info = NULL;
	int reverse_factor;

	ret_if(!scroller);
	ret_if(!page);
	ret_if(!before);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	reverse_factor = _set_scroller_reverse_by_page(scroller, before);
	switch (reverse_factor) {
	case -1:
	case 0:
#if 0 /* EFL Private feature */
		elm_scroller_origin_reverse_set(scroller, EINA_TRUE, EINA_TRUE);
#endif
		_D("pack_before a page(%p) before a page(%p) into the scroller(%p), origin_reverse(1)", page, before, scroller);
		break;
	case 1:
#if 0 /* EFL Private feature */
		elm_scroller_origin_reverse_set(scroller, EINA_FALSE, EINA_FALSE);
#endif
		_D("pack_before a page(%p) before a page(%p) into the scroller(%p), origin_reverse(0)", page, before, scroller);
		break;
	default:
		_E("Cannot reach here : %d", reverse_factor);
		break;
	}

	elm_box_pack_before(scroller_info->box, page, before);
	/* recalculate : child box with pages -> parent box */
	elm_box_recalculate(scroller_info->box);
	elm_box_recalculate(scroller_info->box_layout);
}



static void _elm_box_pack_after(Evas_Object *scroller, Evas_Object *page, Evas_Object *after)
{
	scroller_info_s *scroller_info = NULL;
	int reverse_factor;

	ret_if(!scroller);
	ret_if(!page);
	ret_if(!after);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	reverse_factor = _set_scroller_reverse_by_page(scroller, after);
	switch (reverse_factor) {
	case -1:
#if 0 /* EFL Private feature */
		elm_scroller_origin_reverse_set(scroller, EINA_TRUE, EINA_TRUE);
#endif
		_D("pack_after a page(%p) after a page(%p) into the scroller(%p), origin_reverse(1)", page, after, scroller);
		break;
	case 0:
	case 1:
#if 0 /* EFL Private feature */
		elm_scroller_origin_reverse_set(scroller, EINA_FALSE, EINA_FALSE);
#endif
		_D("pack_after a page(%p) after a page(%p) into the scroller(%p), origin_reverse(0)", page, after, scroller);
		break;
	default:
		_E("Cannot reach here : %d", reverse_factor);
		break;
	}

	elm_box_pack_after(scroller_info->box, page, after);
	/* recalculate : child box with pages -> parent box */
	elm_box_recalculate(scroller_info->box);
	elm_box_recalculate(scroller_info->box_layout);
}



static void _elm_box_unpack(Evas_Object *scroller, Evas_Object *page)
{
	scroller_info_s *scroller_info = NULL;
	int reverse_factor;

	ret_if(!scroller);
	ret_if(!page);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	ret_if(!scroller_info->box);

	int is_page_exist = 0;
	Evas_Object *tmp = NULL;
	Eina_List *list = elm_box_children_get(scroller_info->box);
	EINA_LIST_FREE(list, tmp) {
		continue_if(!tmp);

		if (page == tmp) {
			is_page_exist = 1;
		}
	}

	if (is_page_exist == 0) {
		_D("No page to unpack");
		return;
	}

	reverse_factor = _set_scroller_reverse_by_page(scroller, page);
	switch (reverse_factor) {
	case -1:
#if 0 /* EFL Private feature */
		elm_scroller_origin_reverse_set(scroller, EINA_TRUE, EINA_TRUE);
#endif
		_D("unpack a page(%p) from the scroller(%p), origin_reverse(1)", page, scroller);
		break;
	case 0:
	case 1:
#if 0 /* EFL Private feature */
		elm_scroller_origin_reverse_set(scroller, EINA_FALSE, EINA_FALSE);
#endif
		_D("unpack a page(%p) from the scroller(%p), origin_reverse(0)", page, scroller);
		break;
	default:
		_E("Cannot reach here : %d", reverse_factor);
		break;
	}

	elm_object_focus_set(page, EINA_FALSE);
	elm_box_unpack(scroller_info->box, page);
	/* recalculate : child box with pages -> parent box */
	elm_box_recalculate(scroller_info->box);
	elm_box_recalculate(scroller_info->box_layout);
}



static Eina_Bool _hide_index_timer_cb(void *data)
{
	Evas_Object *scroller = data;
	scroller_info_s *scroller_info = NULL;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);

	layout_hide_index(scroller_info->layout);
	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_HIDE_INDEX_TIMER);

	return ECORE_CALLBACK_CANCEL;
}



static Evas_Object *_select_singular_index(Evas_Object *scroller)
{
	Evas_Object *cur_page;
	scroller_info_s *scroller_info = NULL;
	page_direction_e direction;

	retv_if(!scroller, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	cur_page = scroller_get_focused_page(scroller);
	retv_if(!cur_page, NULL);

	direction = scroller_get_current_page_direction(scroller);
	switch (direction) {
	case PAGE_DIRECTION_LEFT:
		if (scroller_info->index[PAGE_DIRECTION_LEFT]) {
			index_bring_in_page(scroller_info->index[PAGE_DIRECTION_LEFT], cur_page);
		}
		return scroller_info->index[PAGE_DIRECTION_LEFT];
	case PAGE_DIRECTION_CENTER:
		if (scroller_info->index[PAGE_DIRECTION_LEFT]) {
			index_bring_in_page(scroller_info->index[PAGE_DIRECTION_LEFT], cur_page);
		}
		if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
			index_bring_in_page(scroller_info->index[PAGE_DIRECTION_RIGHT], cur_page);
		}
		break;
	case PAGE_DIRECTION_RIGHT:
		if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
			index_bring_in_page(scroller_info->index[PAGE_DIRECTION_RIGHT], cur_page);
		}
		return scroller_info->index[PAGE_DIRECTION_RIGHT];
	default:
		_E("Cannot reach here");
		break;
	}

	return NULL;
}



#define HIDE_TIME 1.0f
static Evas_Object *_select_plural_index(Evas_Object *scroller)
{
	Evas_Object *cur_page = NULL;
	Ecore_Timer *hide_timer = NULL;
	scroller_info_s *scroller_info = NULL;
	page_direction_e direction;

	retv_if(!scroller, NULL);

	hide_timer = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_HIDE_INDEX_TIMER);
	if (hide_timer) {
		ecore_timer_del(hide_timer);
		hide_timer = NULL;
	}

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	cur_page = scroller_get_focused_page(scroller);
	retv_if(!cur_page, NULL);

	direction = scroller_get_current_page_direction(scroller);
	switch (direction) {
	case PAGE_DIRECTION_LEFT:
		layout_show_left_index(scroller_info->layout);
		if (scroller_info->index[PAGE_DIRECTION_LEFT]) {
			index_bring_in_page(scroller_info->index[PAGE_DIRECTION_LEFT], cur_page);
		}
		hide_timer = ecore_timer_add(HIDE_TIME, _hide_index_timer_cb, scroller);
		if (hide_timer) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_HIDE_INDEX_TIMER, hide_timer);
		else _E("Cannot add a hide_timer");
		return scroller_info->index[PAGE_DIRECTION_LEFT];
	case PAGE_DIRECTION_CENTER:
		if (scroller_info->index[PAGE_DIRECTION_LEFT]) {
			index_bring_in_page(scroller_info->index[PAGE_DIRECTION_LEFT], cur_page);
		}
		if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
			index_bring_in_page(scroller_info->index[PAGE_DIRECTION_RIGHT], cur_page);
		}
		layout_hide_index(scroller_info->layout);
		break;
	case PAGE_DIRECTION_RIGHT:
		layout_show_right_index(scroller_info->layout);
		if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
			index_bring_in_page(scroller_info->index[PAGE_DIRECTION_RIGHT], cur_page);
		}
		hide_timer = ecore_timer_add(HIDE_TIME, _hide_index_timer_cb, scroller);
		if (hide_timer) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_HIDE_INDEX_TIMER, hide_timer);
		else _E("Cannot add a hide_timer");
		return scroller_info->index[PAGE_DIRECTION_RIGHT];
	default:
		break;
	}

	return NULL;
}



static void _anim_start_cb(void *data, Evas_Object *scroller, void *event_info)
{
	_D("start the scroller(%p) animation", scroller);
}



static void _anim_stop_cb(void *data, Evas_Object *scroller, void *event_info)
{
	scroller_info_s *scroller_info = NULL;
	Evas_Object *page = NULL;
	Evas_Coord x, w;

	_D("stop the scroller(%p) animation", scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	elm_scroller_region_get(scroller, &x, NULL, &w, NULL);
	if (0 == x % scroller_info->page_width) {
		/* If TTS is off, unpack or pack page_inners */
		if (main_get_info()->is_tts) {
			page = scroller_get_focused_page(scroller);
			page_focus(page);
		}
	}
}



static void _drag_start_cb(void *data, Evas_Object *scroller, void *event_info)
{
	_D("start to drag the scroller(%p)", scroller);
}



static void _drag_stop_cb(void *data, Evas_Object *scroller, void *event_info)
{
	_D("stop to drag the scroller(%p) animation", scroller);
}



static void _scroll_cb(void *data, Evas_Object *scroller, void *event_info)
{
	double loop_time = 0.0;
	static double last_boosted_time = 0.0;
	scroller_info_s *scroller_info = NULL;
	layout_info_s *layout_info = NULL;
	int x, w;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	loop_time = ecore_loop_time_get();
	if (last_boosted_time == 0.0) {
		last_boosted_time = loop_time;
	} else {
		if ((loop_time - last_boosted_time) >= 2.5) {
#if 0 // TBD
			home_dbus_scroll_booster_signal_send(2000);
#endif
			last_boosted_time = loop_time;
		}
	}

	/* 1. Focus */
	elm_scroller_region_get(scroller, &x, NULL, &w, NULL);
	if (0 == x % scroller_info->page_width) {
		/* Page is on the edge */
		scroller_info->scrolling = 0;
		if (main_get_info()->is_tts) {
			scroller_restore_inner_focus(scroller);
		}
	} else {
		/* Page is not on the edge */
		if (!scroller_info->scrolling) {
			scroller_info->scrolling = 1;
			if (main_get_info()->is_tts) {
				scroller_backup_inner_focus(scroller);
			}
		}
	}

	/* 2. Index */
	layout_info = evas_object_data_get(scroller_info->layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	/* When an user is reordering pages now, index has not to be updated */
	if (!scroller_info->scroll_index) return;

	switch (scroller_info->index_number) {
	case SCROLLER_INDEX_SINGULAR:
		_select_singular_index(scroller);
		break;
	case SCROLLER_INDEX_PLURAL:
		_select_plural_index(scroller);
		break;
	default:
		_E("Cannot reach here");
		break;
	}
}



static w_home_error_e _resume_result_cb(void *data)
{
	Evas_Object *scroller = data;
	scroller_info_s *scroller_info = NULL;

	retv_if(!scroller, W_HOME_ERROR_INVALID_PARAMETER);

	_D("Activate the rotary events for Home");

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);
	retv_if(!scroller_info->box, W_HOME_ERROR_FAIL);

	eext_rotary_object_event_activated_set(scroller, EINA_TRUE);

	return W_HOME_ERROR_NONE;
}



static Eina_Bool _rotary_cb(void *data, Evas_Object *obj, Eext_Rotary_Event_Info *rotary_info)
{
	Evas_Object *scroller = obj;
	Evas_Object *cur = NULL;
	Evas_Object *next = NULL;

	retv_if(!scroller, ECORE_CALLBACK_PASS_ON);

	cur = scroller_get_focused_page(scroller);
	retv_if(!cur, ECORE_CALLBACK_PASS_ON);

	_D("Detent detected, obj[%p], direction[%d], timeStamp[%u]", obj, rotary_info->direction, rotary_info->time_stamp);
	if (rotary_info->direction == EEXT_ROTARY_DIRECTION_CLOCKWISE) {
		next = scroller_get_right_page(scroller, cur);
	} else {
		next = scroller_get_left_page(scroller, cur);
	}
	if (next) {
		scroller_bring_in_page(scroller, next, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT);
	}

	return ECORE_CALLBACK_PASS_ON;
}



static void _init_rotary(Evas_Object *scroller)
{
	ret_if(!scroller);

	_D("lnitialize the rotary event");

	eext_rotary_object_event_callback_add(scroller, _rotary_cb, NULL);
	if (W_HOME_ERROR_NONE != main_register_cb(APP_STATE_RESUME, _resume_result_cb, scroller)) {
		_E("Cannot register the pause callback");
	}
	scroller_unfreeze(scroller);
}



static void _destroy_rotary(Evas_Object *scroller)
{
	ret_if(!scroller);

	_D("Finish the rotary event");

	eext_rotary_object_event_callback_del(scroller, _rotary_cb);
	main_unregister_cb(APP_STATE_RESUME, _resume_result_cb);
	scroller_unfreeze(scroller);
}



HAPI void scroller_destroy(Evas_Object *parent)
{
	Evas_Object *scroller = NULL;
	scroller_info_s *scroller_info = NULL;

	ret_if(!parent);

	scroller = elm_object_part_content_unset(parent, "scroller");
	ret_if(!scroller);

	_destroy_rotary(scroller);

	evas_object_data_del(parent, DATA_KEY_SCROLLER);
	scroller_info = evas_object_data_del(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	evas_object_data_del(scroller, DATA_KEY_INDEX);

	if (scroller_info->index_update_timer) {
		ecore_timer_del(scroller_info->index_update_timer);
		scroller_info->index_update_timer = NULL;
	}

	evas_object_del(scroller_info->box);
	evas_object_del(scroller_info->box_layout);

	free(scroller_info);
	evas_object_del(scroller);
}



HAPI void scroller_focus(Evas_Object *scroller)
{
	Evas_Object *page = NULL;

	ret_if(!scroller);

	page = scroller_get_focused_page(scroller);
	if (!page) return;

	page_focus(page);
}



HAPI void scroller_highlight(Evas_Object *scroller)
{
	Evas_Object *page = NULL;
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	ret_if(!scroller_info->box);
	scroller_info->enable_highlight = 1;

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	EINA_LIST_FREE(list, page) {
		page_highlight(page);
	}
}



HAPI void scroller_unhighlight(Evas_Object *scroller)
{
	Evas_Object *page = NULL;
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	ret_if(!scroller_info->box);
	scroller_info->enable_highlight = 0;

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	EINA_LIST_FREE(list, page) {
		page_unhighlight(page);
	}
}



HAPI Evas_Object *scroller_create(Evas_Object *layout, Evas_Object *parent, int page_width, int page_height, scroller_index_e index_number)
{
	Evas_Object *scroller = NULL;
	Evas_Object *box_layout = NULL;
	Evas_Object *box = NULL;
	scroller_info_s *scroller_info = NULL;

	retv_if(!parent, NULL);

	scroller = elm_scroller_add(parent);
	retv_if(!scroller, NULL);

	scroller_info = calloc(1, sizeof(scroller_info_s));
	if (!scroller_info) {
		_E("Cannot calloc for scroller_info");
		evas_object_del(scroller);
		return NULL;
	}
	evas_object_data_set(scroller, DATA_KEY_SCROLLER_INFO, scroller_info);
	evas_object_data_set(parent, DATA_KEY_SCROLLER, scroller);
	evas_object_data_set(scroller, DATA_KEY_INDEX, NULL);

	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_FALSE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_scroller_page_scroll_limit_set(scroller, 1, 1);
	elm_scroller_content_min_limit(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_single_direction_set(scroller, ELM_SCROLLER_SINGLE_DIRECTION_HARD);
	elm_scroller_page_size_set(scroller, page_width, page_height);
	evas_object_smart_callback_add(scroller, "scroll", _scroll_cb, NULL);

	elm_object_style_set(scroller, "effect");
	evas_object_show(scroller);
	elm_object_scroll_lock_y_set(scroller, EINA_TRUE);
	elm_object_part_content_set(parent, "scroller", scroller);

	evas_object_event_callback_add(scroller, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, layout);
	evas_object_event_callback_add(scroller, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, layout);
	evas_object_smart_callback_add(scroller, "scroll,anim,start", _anim_start_cb, NULL);
	evas_object_smart_callback_add(scroller, "scroll,anim,stop", _anim_stop_cb, NULL);
	evas_object_smart_callback_add(scroller, "scroll,drag,start", _drag_start_cb, NULL);
	evas_object_smart_callback_add(scroller, "scroll,drag,stop", _drag_stop_cb, NULL);

	/* Use the parent between box and scroller because of alignment of a page in the box. */
	box_layout = elm_box_add(scroller);
	if (!box_layout) {
		_E("Cannot create box_layout");
		evas_object_del(scroller);
		return NULL;
	}
	elm_box_horizontal_set(box_layout, EINA_TRUE);
	/* The weight of box_layout has to be set as EVAS_HINT_EXPAND on y-axis. */
	evas_object_size_hint_weight_set(box_layout, 0.0, EVAS_HINT_EXPAND);
	evas_object_show(box_layout);

	elm_object_content_set(scroller, box_layout);

	box = elm_box_add(box_layout);
	if (!box) {
		_E("Cannot create box");
		evas_object_del(box_layout);
		evas_object_del(scroller);
		return NULL;
	}

	elm_box_horizontal_set(box, EINA_TRUE);
	/* The alignment of box has to be set as 0.0, 0.5 for pages */
	elm_box_align_set(box, 0.0, 0.5);
	evas_object_size_hint_weight_set(box, 0.0, EVAS_HINT_EXPAND);
	evas_object_show(box);

	elm_box_pack_end(box_layout, box);

	scroller_info->root_width = main_get_info()->root_w;
	scroller_info->root_height = main_get_info()->root_h;
	scroller_info->page_width = page_width;
	scroller_info->page_height = page_height;
	scroller_info->index_number = index_number;

	scroller_info->layout = layout;
	scroller_info->parent = parent;
	scroller_info->box_layout = box_layout;
	scroller_info->box = box;

	scroller_info->scroll_focus = 1;
	scroller_info->scroll_index = 1;

	_init_rotary(scroller);

	return scroller;
}



HAPI w_home_error_e scroller_push_page_before(Evas_Object *scroller, Evas_Object *page, Evas_Object *before)
{
	Evas_Object *tmp = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	int center_check = 0;

	retv_if(!scroller, W_HOME_ERROR_FAIL);
	retv_if(!page, W_HOME_ERROR_FAIL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, W_HOME_ERROR_FAIL);

	_elm_box_unpack(scroller, page);
	if (before) _elm_box_pack_before(scroller, page, before);
	else _elm_box_pack_end(scroller, page);

	list = elm_box_children_get(scroller_info->box);

	EINA_LIST_FOREACH_SAFE(list, l, ln, tmp) {
		continue_if(!tmp);

		if (scroller_info->center == tmp) center_check = 1;
		if (page == tmp) {
			if (center_check) page_info->direction = PAGE_DIRECTION_RIGHT;
			else page_info->direction = PAGE_DIRECTION_LEFT;
			break;
		}
	}
	eina_list_free(list);

	if (scroller_info->index[PAGE_DIRECTION_LEFT]) {
		index_update(scroller_info->index[PAGE_DIRECTION_LEFT], scroller, INDEX_BRING_IN_NONE);
	}

	if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
		index_update(scroller_info->index[PAGE_DIRECTION_RIGHT], scroller, INDEX_BRING_IN_NONE);
	}

	return W_HOME_ERROR_NONE;
}



HAPI w_home_error_e scroller_push_page_after(Evas_Object *scroller, Evas_Object *page, Evas_Object *after)
{
	Evas_Object *tmp = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	int center_check = 0;

	retv_if(!scroller, W_HOME_ERROR_FAIL);
	retv_if(!page, W_HOME_ERROR_FAIL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, W_HOME_ERROR_FAIL);

	_elm_box_unpack(scroller, page);
	if (after) _elm_box_pack_after(scroller, page, after);
	else _elm_box_pack_start(scroller, page);

	list = elm_box_children_get(scroller_info->box);

	EINA_LIST_FOREACH_SAFE(list, l, ln, tmp) {
		continue_if(!tmp);

		if (scroller_info->center == tmp) center_check = 1;
		if (page == tmp) {
			if (center_check) page_info->direction = PAGE_DIRECTION_RIGHT;
			else page_info->direction = PAGE_DIRECTION_LEFT;
			break;
		}
	}
	eina_list_free(list);

	if (scroller_info->index[PAGE_DIRECTION_LEFT]) {
		index_update(scroller_info->index[PAGE_DIRECTION_LEFT], scroller, INDEX_BRING_IN_NONE);
	}

	if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
		index_update(scroller_info->index[PAGE_DIRECTION_RIGHT], scroller, INDEX_BRING_IN_NONE);
	}

	return W_HOME_ERROR_NONE;
}



static Eina_Bool _index_update_cb(void *data)
{
	Evas_Object *scroller = data;
	scroller_info_s *scroller_info = NULL;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);

	scroller_info->index_update_timer = NULL;

	if (scroller_info->index[PAGE_DIRECTION_LEFT]) {
		index_update(scroller_info->index[PAGE_DIRECTION_LEFT], scroller, INDEX_BRING_IN_AFTER);
	}

	if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
		index_update(scroller_info->index[PAGE_DIRECTION_RIGHT], scroller, INDEX_BRING_IN_AFTER);
	}

	_D("Index is updated");

	return ECORE_CALLBACK_CANCEL;
}



HAPI w_home_error_e scroller_push_page(Evas_Object *scroller, Evas_Object *page, scroller_push_type_e scroller_type)
{
	Evas_Object *tmp = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;

	retv_if(!scroller, W_HOME_ERROR_FAIL);
	retv_if(!page, W_HOME_ERROR_FAIL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);

	if (scroller_info->index_update_timer) {
		ecore_timer_del(scroller_info->index_update_timer);
		scroller_info->index_update_timer = NULL;
	}

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, W_HOME_ERROR_FAIL);

	switch (scroller_type) {
	case SCROLLER_PUSH_TYPE_FIRST:
	case SCROLLER_PUSH_TYPE_CENTER_LEFT:
	case SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_LEFT:
		page_info->direction = PAGE_DIRECTION_LEFT;
		break;
	case SCROLLER_PUSH_TYPE_CENTER:
		page_info->direction = PAGE_DIRECTION_CENTER;
		break;
	case SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_RIGHT:
	case SCROLLER_PUSH_TYPE_CENTER_RIGHT:
	case SCROLLER_PUSH_TYPE_LAST:
	default:
		page_info->direction = PAGE_DIRECTION_RIGHT;
		break;
	}

	if (!scroller_info->center) {
		switch (scroller_type) {
		case SCROLLER_PUSH_TYPE_CENTER_LEFT:
		case SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_LEFT:
		case SCROLLER_PUSH_TYPE_CENTER_RIGHT:
		case SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_RIGHT:
			scroller_type = SCROLLER_PUSH_TYPE_LAST;
			break;
		case SCROLLER_PUSH_TYPE_CENTER:
			scroller_type = SCROLLER_PUSH_TYPE_LAST;
			scroller_info->center = page;
			break;
		default:
			break;
		}
	}

	switch (scroller_type) {
	case SCROLLER_PUSH_TYPE_FIRST:
		_elm_box_unpack(scroller, page);
		_elm_box_pack_start(scroller, page);
		break;
	case SCROLLER_PUSH_TYPE_CENTER_LEFT:
		_elm_box_unpack(scroller, page);

		tmp = (scroller_info->center_neighbor_left && scroller_info->center_neighbor_left != page) ?
			scroller_info->center_neighbor_left : scroller_info->center;
		_elm_box_pack_before(scroller, page, tmp);
		break;
	case SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_LEFT:
		if (scroller_info->center_neighbor_left) {
			_elm_box_unpack(scroller, scroller_info->center_neighbor_left);
		}
		_elm_box_pack_before(scroller, page, scroller_info->center);
		scroller_info->center_neighbor_left = page;
		break;
	case SCROLLER_PUSH_TYPE_CENTER:
		_elm_box_pack_after(scroller, page, scroller_info->center);
		_elm_box_unpack(scroller, scroller_info->center);

		if (evas_object_data_del(scroller_info->center, PRIVATE_DATA_KEY_IS_DUMMY) != NULL) {
			page_destroy(scroller_info->center);
		}
		scroller_info->center = page;
		break;
	case SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_RIGHT:
		if (scroller_info->center_neighbor_right) {
			_elm_box_unpack(scroller, scroller_info->center_neighbor_right);
		}
		_elm_box_pack_after(scroller, page, scroller_info->center);
		scroller_info->center_neighbor_right = page;
		break;
	case SCROLLER_PUSH_TYPE_CENTER_RIGHT:
		_elm_box_unpack(scroller, page);
		tmp = scroller_info->center_neighbor_right && scroller_info->center_neighbor_right != page ?
			scroller_info->center_neighbor_right : scroller_info->center;
		_elm_box_pack_after(scroller, page, tmp);
		break;
	case SCROLLER_PUSH_TYPE_LAST:
	default:
		_elm_box_unpack(scroller, page);
		_elm_box_pack_end(scroller, page);
		break;
	}

	scroller_info->index_update_timer = ecore_timer_add(INDEX_UPDATE_TIME, _index_update_cb, scroller);
	if (!scroller_info->index_update_timer) {
		_E("Cannot add an index update timer");
	}

	return W_HOME_ERROR_NONE;
}



HAPI Evas_Object *scroller_pop_page(Evas_Object *scroller, Evas_Object *page)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	Evas_Object *tmp_page = NULL;
	page_info_s *page_info = NULL;
	int count = 0;

	retv_if(!scroller, NULL);
	retv_if(!page, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	if (scroller_info->index_update_timer) {
		ecore_timer_del(scroller_info->index_update_timer);
		scroller_info->index_update_timer = NULL;
	}

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(NULL == list, NULL);

	if (scroller_info->center == page) {
		_D("The center will be removed");
		scroller_info->center = NULL;
	}

	if (scroller_info->center_neighbor_left == page) {
		_D("The neighbor left will be removed");
		scroller_info->center_neighbor_left = NULL;
	}

	if (scroller_info->center_neighbor_right == page) {
		_D("The neighbor right will be removed");
		scroller_info->center_neighbor_right = NULL;
	}

	count = eina_list_count(list);
	EINA_LIST_FOREACH_SAFE(list, l, ln, tmp_page) {
		if (page != tmp_page) continue;
		_elm_box_unpack(scroller, page);

		scroller_info->index_update_timer = ecore_timer_add(INDEX_UPDATE_TIME, _index_update_cb, scroller);
		if (!scroller_info->index_update_timer) {
			_E("Cannot add an index update timer");
		}

		if (1 == count) {
			_D("There is only one page");
			eina_list_free(list);
			return page;
		}

		eina_list_free(list);
		return page;
	}

	eina_list_free(list);

	return NULL;
}



static void _change_favorites_order_cb(keynode_t *node, void *data)
{
	int value = -1;

	_D("Change favorites order");

	/* check Emergency Mode */
	if(emergency_mode_enabled_get()) {
		_E("emergency mode enabled");
		return;
	}

	// 0 : init, 1 : backup request, 2 : restore request, 3: write done
	if(vconf_get_int(VCONF_KEY_WMS_FAVORITES_ORDER, &value) < 0) {
		_E("Failed to get VCONF_KEY_WMS_FAVORITES_ORDER");
		return;
	}

	_D("Change favorites order vconf:[%d]", value);

	wms_change_favorite_order(value);
}



static Eina_Bool _push_all_page_cb(void *data)
{
	Evas_Object *scroller = data;
	Eina_List *page_info_list = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;

	void (*after_func)(void *);
	void *after_func_data = NULL;
	static int i = 0;
	int count = 0;

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);

	page_info_list = evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_LIST);
	if (!page_info_list) {
		_W("page_info_list is NULL");
		goto END;
	}

	page_info = eina_list_nth(page_info_list, i);
	if (!page_info) {
		if (!i) {
			_W("page_info_list is zero");
		}
		goto END;
	}
	i++;

	page_info->item = widget_create(scroller, page_info->id, page_info->subid, page_info->period);
	if (!page_info->item) {
		_E("cannot create a widget");
		return ECORE_CALLBACK_RENEW;
	}
	widget_viewer_evas_disable_loading(page_info->item);
	evas_object_resize(page_info->item, scroller_info->page_width, scroller_info->page_height);
	evas_object_size_hint_min_set(page_info->item, scroller_info->page_width, scroller_info->page_height);
	evas_object_show(page_info->item);

	page_info->page = page_create(scroller
							, page_info->item
							, page_info->id, page_info->subid
							, scroller_info->page_width, scroller_info->page_height
							, PAGE_CHANGEABLE_BG_ON, PAGE_REMOVABLE_ON);
	if (!page_info->page) {
		evas_object_del(page_info->item);
		_E("cannot create a page");
		goto END;
	}
	widget_add_callback(page_info->item, page_info->page);

	page_set_effect(page_info->page, page_effect_none, page_effect_none);
	scroller_push_page(scroller, page_info->page, SCROLLER_PUSH_TYPE_LAST);

	return ECORE_CALLBACK_RENEW;

END:
	/* If there are 7 pages, do not append the plus page */
	count = scroller_count_direction(scroller, PAGE_DIRECTION_RIGHT);
	if (count < MAX_WIDGET
		&& !main_get_info()->is_tts
		&& page_create_plus_page(scroller))
	{
		page_set_effect(scroller_info->plus_page, page_effect_none, page_effect_none);
		scroller_push_page(scroller, scroller_info->plus_page, SCROLLER_PUSH_TYPE_LAST);
	}

	after_func = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC);
	after_func_data = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC_DATA);

	if (after_func) {
		after_func(after_func_data);
	}

	_W("It's done to push all the pages(%d)", i);
	i = 0;

	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_PUSH_ALL_TIMER);
	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_LIST);

	if (!main_get_booting_state()) {
		_W("First boot");
		main_inc_booting_state();
		_change_favorites_order_cb(NULL, NULL);

		/* wms vconf has to be dealt after pushing pages */
		if (vconf_notify_key_changed(VCONF_KEY_WMS_FAVORITES_ORDER, _change_favorites_order_cb, NULL) < 0) {
			_E("Failed to register the changed_apps_order callback");
		}
	}

	/* Key register */
	key_register();

	if (main_get_info()->is_tts) {
		_W("TTS Mode : Do not save orders of widgets");
		return ECORE_CALLBACK_CANCEL;
	}

	/* Widgets can be removed before launching w-home */
	/* But, we do not need to refresh the DB */
	count = scroller_count_direction(scroller, PAGE_DIRECTION_RIGHT);
	if (count <= 1) {
		_E("Right page is only one or zero");
	}

	wms_change_favorite_order(W_HOME_WMS_BACKUP);

	return ECORE_CALLBACK_CANCEL;
}



/* This dummy will be destroyed by the clock */
static w_home_error_e _dummy_center(Evas_Object *scroller)
{
	Evas_Object *page = NULL;
	Evas_Object *center = NULL;
	scroller_info_s *scroller_info = NULL;

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);

	center = evas_object_rectangle_add(main_get_info()->e);
	retv_if(!center, W_HOME_ERROR_FAIL);

	page = page_create(scroller
						, center
						, NULL, NULL
						, scroller_info->page_width, scroller_info->page_height
						, PAGE_CHANGEABLE_BG_OFF, PAGE_REMOVABLE_OFF);
	if (!page) {
		_E("Cannot create a page");
		evas_object_del(center);
		return W_HOME_ERROR_FAIL;
	}

	evas_object_data_set(page, PRIVATE_DATA_KEY_IS_DUMMY, (void*)1);
	page_set_effect(scroller_info->plus_page, page_effect_none, page_effect_none);
	scroller_push_page(scroller, page, SCROLLER_PUSH_TYPE_CENTER);

	return W_HOME_ERROR_NONE;
}



HAPI w_home_error_e scroller_push_pages(Evas_Object *scroller, Eina_List *page_info_list, void (*after_func)(void *), void *data)
{
	Ecore_Timer *timer = NULL;
	scroller_info_s *scroller_info = NULL;

	retv_if(!scroller, W_HOME_ERROR_INVALID_PARAMETER);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);

	timer = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_PUSH_ALL_TIMER);
	if (timer) {
		_D("There is already a timer for popping all items.");
		ecore_timer_del(timer);
		timer = NULL;
	}

	if (!scroller_info->center && W_HOME_ERROR_NONE != _dummy_center(scroller)) {
		_E("Cannot create a dummy clock");
	}

	/* We have to prevent pushing pages doubly */
	/* We don't need to care about LEFT pages according to UIG */
	scroller_pop_pages(scroller, PAGE_DIRECTION_RIGHT);

	timer = ecore_timer_add(0.01f, _push_all_page_cb, scroller);
	if (!timer) {
		_E("Cannot add a timer");
		return W_HOME_ERROR_FAIL;
	}
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_PUSH_ALL_TIMER, timer);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_LIST, page_info_list);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC, after_func);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC_DATA, data);

	return W_HOME_ERROR_NONE;
}



HAPI void scroller_pop_pages(Evas_Object *scroller, page_direction_e direction)
{
	Evas_Object *page = NULL;
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;

	ret_if(!scroller);

	_W("pop all the pages");

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	switch (direction) {
	case PAGE_DIRECTION_LEFT:
		scroller_info->center_neighbor_left = NULL;
		break;
	case PAGE_DIRECTION_CENTER:
		scroller_info->center = NULL;
		break;
	case PAGE_DIRECTION_RIGHT:
		scroller_info->center_neighbor_right = NULL;
		page_destroy_plus_page(scroller);
		break;
	case PAGE_DIRECTION_ANY:
		scroller_info->center_neighbor_left = NULL;
		scroller_info->center = NULL;
		scroller_info->center_neighbor_right = NULL;
		page_destroy_plus_page(scroller);
		break;
	default:
		_E("Cannot reach here");
		break;
	}

	list = elm_box_children_get(scroller_info->box);
	if (!list) return;

	EINA_LIST_FREE(list, page) {
		continue_if(!page);

		page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		continue_if(!page_info);

		if (PAGE_DIRECTION_ANY != direction
			&& direction != page_info->direction)
		{
			continue;
		}

		if (PAGE_DIRECTION_RIGHT == page_info->direction) {
			evas_object_del(page_info->item);
		}

		scroller_pop_page(scroller, page);
		page_destroy(page);
	}
}



HAPI int scroller_count(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	int count;

	retv_if(NULL == scroller, 0);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, 0);

	list = elm_box_children_get(scroller_info->box);
	if (NULL == list) return 0;

	count = eina_list_count(list);
	eina_list_free(list);

	return count;
}



HAPI int scroller_count_direction(Evas_Object *scroller, page_direction_e direction)
{
	Evas_Object *page = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	Eina_List *list = NULL;
	int count = 0;

	retv_if(NULL == scroller, 0);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, 0);

	list = elm_box_children_get(scroller_info->box);
	if (NULL == list) return 0;

	EINA_LIST_FREE(list, page) {
		continue_if(!page);
		page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		continue_if(!page_info);

		if (page_info->direction == direction) count++;
	}

	return count;
}



HAPI Eina_Bool scroller_is_scrolling(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	retv_if(!scroller, EINA_FALSE);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, EINA_FALSE);

	return scroller_info->scrolling? EINA_TRUE:EINA_FALSE;
}



HAPI void scroller_freeze(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;
	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	elm_object_scroll_freeze_push(scroller_info->box);
}



HAPI void scroller_unfreeze(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(NULL == scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	while (elm_object_scroll_freeze_get(scroller_info->box)) {
		elm_object_scroll_freeze_pop(scroller_info->box);
	}
}



static inline int _get_index_in_list(Evas_Object *box, Evas_Object *page)
{
	Evas_Object *tmp = NULL;
	Eina_List *list = NULL;

	list = elm_box_children_get(box);
	retv_if(!list, 0);

	int exist = 1;
	int i = 0;

	EINA_LIST_FREE(list, tmp) {
		continue_if(!tmp);
		if (tmp == page)
			exist = 0;
		i += exist;
	}

	if (exist) {
		i = 0;
	}
	return i;
}



static inline void _page_bring_in(Evas_Object *scroller, int index, scroller_freeze_e is_freezed)
{
	Evas_Object *page = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	int cur_freezed = 0;

	page = scroller_get_page_at(scroller, index);
	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	cur_freezed = elm_object_scroll_freeze_get(scroller_info->box);

	if (is_freezed) scroller_unfreeze(scroller);
#if 0 /* EFL Private feature */
	elm_scroller_origin_reverse_set(scroller, EINA_FALSE, EINA_FALSE);
#endif
	_D("Bring in now : %d(%p)", index, page);
	elm_scroller_page_bring_in(scroller, index, 0);
	if (is_freezed && cur_freezed) scroller_freeze(scroller);
	if (scroller_info->index[page_info->direction]) {
		index_bring_in_page(scroller_info->index[page_info->direction], page);
	}
}



static inline void _page_show(Evas_Object *scroller, int index, scroller_freeze_e is_freezed)
{
	Evas_Object *page = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	int cur_freezed = 0;

	page = scroller_get_page_at(scroller, index);
	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	cur_freezed = elm_object_scroll_freeze_get(scroller_info->box);

	if (is_freezed) scroller_unfreeze(scroller);
#if 0 /* EFL Private feature */
	elm_scroller_origin_reverse_set(scroller, EINA_FALSE, EINA_FALSE);
#endif
	_D("Page show now : %d(%p)", index, page);
	elm_scroller_page_show(scroller, index, 0);
	if (is_freezed && cur_freezed) scroller_freeze(scroller);
	if (scroller_info->index[page_info->direction]) {
		index_bring_in_page(scroller_info->index[page_info->direction], page);
	}
}



static Eina_Bool _bring_in_anim_cb(void *data)
{
	Evas_Object *scroller = data;
	int i = 0;
	int is_freezed = 0;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);

	i = (int) evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE);
	is_freezed = (int) evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED);

	_page_bring_in(scroller, i, is_freezed);

	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM);
	return ECORE_CALLBACK_CANCEL;
}



static void _bring_in_job_cb(void *data)
{
	Evas_Object *scroller = data;
	int i = 0;
	int is_freezed = 0;

	ret_if(!scroller);

	i = (int) evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE);
	is_freezed = (int) evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED);

	_page_bring_in(scroller, i, is_freezed);

	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB);
}



HAPI void scroller_bring_in_by_push_type(Evas_Object *scroller, scroller_push_type_e push_type, scroller_freeze_e freeze, scroller_bring_type_e bring_type)
{
	Evas_Object *page = NULL;
	Ecore_Animator *anim = NULL;
	Ecore_Job *job = NULL;
	scroller_info_s *scroller_info = NULL;
	int i = 0;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	switch (push_type) {
	case SCROLLER_PUSH_TYPE_CENTER:
		page = scroller_info->center;
		break;
	case SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_LEFT:
		page = scroller_info->center_neighbor_left;
		if (page == NULL) {
			page = scroller_get_left_page(scroller, scroller_info->center);
		}
		break;
	case SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_RIGHT:
		page = scroller_info->center_neighbor_right;
		if (page == NULL) {
			page = scroller_get_right_page(scroller, scroller_info->center);
		}
		break;
	default:
		_E("Unsupported push-type:%d", push_type);
		break;
	}
	ret_if(!page);
	i = _get_index_in_list(scroller_info->box, page);

	/* 1. Remove the old action */
	job = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB);
	if (job) ecore_job_del(job);

	anim = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM);
	if (anim) ecore_animator_del(anim);

	/* 2. Append the new timer */
	switch (bring_type) {
	case SCROLLER_BRING_TYPE_INSTANT_SHOW:
		_page_show(scroller, i, freeze);
		break;
	case SCROLLER_BRING_TYPE_INSTANT:
		_page_bring_in(scroller, i, freeze);
		break;
	case SCROLLER_BRING_TYPE_JOB:
		job = ecore_job_add(_bring_in_job_cb, scroller);
		if (job) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB, job);
		else _E("Cannot add a job");
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE, (void *) i);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
		break;
	case SCROLLER_BRING_TYPE_ANIMATOR:
		anim = ecore_animator_add(_bring_in_anim_cb, scroller);
		if (anim) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM, anim);
		else _E("Cannot add an animator");
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE, (void *) i);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
		break;
	default:
		_E("Unsupported bring_type:%d", bring_type);
		break;
	}
}



HAPI void scroller_bring_in(Evas_Object *scroller, int i, scroller_freeze_e freeze, scroller_bring_type_e bring_type)
{
	Ecore_Animator *anim = NULL;
	Ecore_Job *job = NULL;

	ret_if(!scroller);

	/* 1. Remove the old action */
	job = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB);
	if (job) ecore_job_del(job);

	anim = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM);
	if (anim) ecore_animator_del(anim);

	/* 2. Append the new timer */
	switch (bring_type) {
	case SCROLLER_BRING_TYPE_INSTANT_SHOW:
		_page_show(scroller, i, freeze);
		break;
	case SCROLLER_BRING_TYPE_INSTANT:
		_page_bring_in(scroller, i, freeze);
		break;
	case SCROLLER_BRING_TYPE_JOB:
		job = ecore_job_add(_bring_in_job_cb, scroller);
		if (job) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB, job);
		else _E("Cannot add a job");
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE, (void *) i);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
		break;
	case SCROLLER_BRING_TYPE_ANIMATOR:
		anim = ecore_animator_add(_bring_in_anim_cb, scroller);
		if (anim) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM, anim);
		else _E("Cannot add an animator");
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE, (void *) i);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
		break;
	default:
		_E("Unsupported bring_type:%d", bring_type);
		break;
	}
}



HAPI void scroller_bring_in_page(Evas_Object *scroller, Evas_Object *page, scroller_freeze_e freeze, scroller_bring_type_e bring_type)
{
	scroller_info_s *scroller_info = NULL;
	Ecore_Animator *anim = NULL;
	Ecore_Job *job = NULL;
	int i = 0;

	ret_if(!scroller);
	ret_if(!page);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	i = _get_index_in_list(scroller_info->box, page);

	/* 1. Remove the old action */
	job = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB);
	if (job) ecore_job_del(job);

	anim = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM);
	if (anim) ecore_animator_del(anim);

	/* 2. Append the new timer */
	switch (bring_type) {
	case SCROLLER_BRING_TYPE_INSTANT_SHOW:
		_page_show(scroller, i, freeze);
		break;
	case SCROLLER_BRING_TYPE_INSTANT:
		_page_bring_in(scroller, i, freeze);
		break;
	case SCROLLER_BRING_TYPE_JOB:
		job = ecore_job_add(_bring_in_job_cb, scroller);
		if (job) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB, job);
		else _E("Cannot add a job");
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE, (void *) i);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
		break;
	case SCROLLER_BRING_TYPE_ANIMATOR:
		anim = ecore_animator_add(_bring_in_anim_cb, scroller);
		if (anim) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM, anim);
		else _E("Cannot add an animator");
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE, (void *) i);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
		break;
	default:
		_E("Unsupported bring_type:%d", bring_type);
		break;
	}

}



static Eina_Bool _bring_in_center_timer_cb(void *data)
{
	Evas_Object *scroller = data;
	Evas_Object *page = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	void (*before_func)(void *);
	void (*after_func)(void *);
	void *func_data = NULL;

	int page_w = 0;
	int total_x = 0;

	/* 1. Before function */
	before_func = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BEFORE_FUNC);
	if (before_func) {
		func_data = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BEFORE_FUNC_DATA);
		before_func(func_data);
		return ECORE_CALLBACK_RENEW;
	}

	/* 2. Bring in */
	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);

	page = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_PRESSED_PAGE);
	if (page) {
		Evas_Object *tmp_page = NULL;
		Eina_List *list = NULL;
		const Eina_List *l, *ln;
		int is_freezed = (int) evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED);
		int sw, sh;
		int cur_freezed = 0;

		list = elm_box_children_get(scroller_info->box);
		retv_if(!list, ECORE_CALLBACK_CANCEL);

		cur_freezed = elm_object_scroll_freeze_get(scroller_info->box);

		EINA_LIST_FOREACH_SAFE(list, l, ln, tmp_page) {
			continue_if(!tmp_page);
			if (tmp_page == page) break;
			evas_object_geometry_get(tmp_page, NULL, NULL, &page_w, NULL);
			total_x += page_w;
		}
		eina_list_free(list);
		evas_object_geometry_get(scroller, NULL, NULL, &sw, &sh);

		if (is_freezed) scroller_unfreeze(scroller);
		elm_scroller_region_bring_in(scroller, total_x, 0, sw, sh);
		if (is_freezed && cur_freezed) scroller_freeze(scroller);

		page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		retv_if(!page_info, ECORE_CALLBACK_RENEW);

		if (scroller_info->index[page_info->direction]) {
			index_bring_in_page(scroller_info->index[page_info->direction], page);
		}

		return ECORE_CALLBACK_RENEW;
	}

	/* 3. After function */
	after_func = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC);
	if (after_func) {
		func_data = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC_DATA);
		after_func(func_data);
	}

	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER);
	return ECORE_CALLBACK_CANCEL;
}



HAPI void scroller_bring_in_center_of(Evas_Object *scroller
		, Evas_Object *page
		, scroller_freeze_e freeze
		, void (*before_func)(void *), void *before_data
		, void (*after_func)(void *), void *after_data)
{
	scroller_info_s *scroller_info = NULL;
	Ecore_Timer *timer = NULL;

	ret_if(!scroller);
	ret_if(!page);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_PRESSED_PAGE, page);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BEFORE_FUNC, before_func);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BEFORE_FUNC_DATA, before_data);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC, after_func);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC_DATA, after_data);

	/* 1. Remove the old timer */
	timer = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER);
	if (timer) {
		ecore_timer_del(timer);
		timer = NULL;
	}

	/* 2. Append the new timer */
	timer = ecore_timer_add(0.01f, _bring_in_center_timer_cb, scroller);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER, timer);
}



static Eina_Bool _region_show_center_timer_cb(void *data)
{
	Evas_Object *scroller = data;
	Evas_Object *page = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	void (*before_func)(void *);
	void (*after_func)(void *);
	void *func_data = NULL;

	int page_w = 0;
	int total_x = 0;

	/* 1. Before function */
	before_func = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BEFORE_FUNC);
	if (before_func) {
		func_data = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BEFORE_FUNC_DATA);
		before_func(func_data);
		return ECORE_CALLBACK_RENEW;
	}

	/* 2. Bring in */
	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);

	page = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_PRESSED_PAGE);
	if (page) {
		Evas_Object *tmp_page = NULL;
		Eina_List *list = NULL;
		const Eina_List *l, *ln;
		int is_freezed = (int) evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED);
		int cur_freezed = 0;
		int sw, sh;

		list = elm_box_children_get(scroller_info->box);
		retv_if(!list, ECORE_CALLBACK_CANCEL);

		cur_freezed = elm_object_scroll_freeze_get(scroller_info->box);

		EINA_LIST_FOREACH_SAFE(list, l, ln, tmp_page) {
			continue_if(!tmp_page);
			if (tmp_page == page) break;
			evas_object_geometry_get(tmp_page, NULL, NULL, &page_w, NULL);
			total_x += page_w;
		}
		eina_list_free(list);
		evas_object_geometry_get(scroller, NULL, NULL, &sw, &sh);

		if (is_freezed) scroller_unfreeze(scroller);
		_D("Region show now : %d", total_x);
		elm_scroller_region_show(scroller, total_x, 0, sw, sh);
		if (is_freezed && cur_freezed) scroller_freeze(scroller);

		page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		retv_if(!page_info, ECORE_CALLBACK_RENEW);

		if (scroller_info->index[page_info->direction]) {
			index_bring_in_page(scroller_info->index[page_info->direction], page);
		}

		return ECORE_CALLBACK_RENEW;
	}

	/* 3. After function */
	after_func = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC);
	if (after_func) {
		func_data = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC_DATA);
		after_func(func_data);
	}

	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER);
	return ECORE_CALLBACK_CANCEL;
}



HAPI void scroller_region_show_center_of(Evas_Object *scroller
		, Evas_Object *page
		, scroller_freeze_e freeze
		, void (*before_func)(void *), void *before_data
		, void (*after_func)(void *), void *after_data)
{
	Ecore_Timer *timer = NULL;

	ret_if(!scroller);
	ret_if(!page);

	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_PRESSED_PAGE, page);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BEFORE_FUNC, before_func);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BEFORE_FUNC_DATA, before_data);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC, after_func);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_AFTER_FUNC_DATA, after_data);

	/* 1. Remove the old timer */
	timer = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER);
	if (timer) {
		ecore_timer_del(timer);
		timer = NULL;
	}

	/* 2. Append the new timer */
	timer = ecore_timer_add(0.01f, _region_show_center_timer_cb, scroller);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER, timer);
}



static inline void _page_region_show(Evas_Object *scroller, int index, scroller_freeze_e is_freezed)
{
	Evas_Object *page = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	int cur_freezed = 0;

	page = scroller_get_page_at(scroller, index);
	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	elm_object_tree_focus_allow_set(scroller, EINA_TRUE);
	page_focus(page);

	cur_freezed = elm_object_scroll_freeze_get(scroller_info->box);

	if (is_freezed) scroller_unfreeze(scroller);
#if 0 /* EFL Private feature */
	elm_scroller_origin_reverse_set(scroller, EINA_FALSE, EINA_FALSE);
#endif
	_D("Page show now : %d(%p)", index, page);
	elm_scroller_page_show(scroller, index, 0);
	if (is_freezed && cur_freezed) scroller_freeze(scroller);

	if (scroller_info->index[page_info->direction]) {
		index_bring_in_page(scroller_info->index[page_info->direction], page);
	}
}



static void _region_show_job_cb(void *data)
{
	Evas_Object *scroller = data;
	int i = 0;
	int is_freezed = 0;

	ret_if(!scroller);

	i = (int) evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE);
	is_freezed = (int) evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED);

	_page_region_show(scroller, i, is_freezed);

	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB);
}



static Eina_Bool _region_show_anim_cb(void *data)
{
	Evas_Object *scroller = data;
	int i = 0;
	int is_freezed = 0;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);

	i = (int) evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE);
	is_freezed = (int) evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED);

	_page_region_show(scroller, i, is_freezed);

	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM);
	return ECORE_CALLBACK_CANCEL;
}



HAPI void scroller_region_show_by_push_type(Evas_Object *scroller, scroller_push_type_e push_type, scroller_freeze_e freeze, scroller_bring_type_e bring_type)
{
	Evas_Object *page = NULL;
	Ecore_Animator *anim = NULL;
	Ecore_Job *job = NULL;
	scroller_info_s *scroller_info = NULL;
	int i = 0;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	switch (push_type) {
	case SCROLLER_PUSH_TYPE_CENTER:
		page = scroller_info->center;
		break;
	case SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_LEFT:
		page = scroller_info->center_neighbor_left;
		break;
	case SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_RIGHT:
		page = scroller_info->center_neighbor_right;
		break;
	default:
		_E("Unsupported push-type:%d", push_type);
		break;
	}
	ret_if(!page);

	i = _get_index_in_list(scroller_info->box, page);

	/* 1. Remove the old action */
	job = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB);
	if (job) ecore_job_del(job);

	anim = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM);
	if (anim) ecore_animator_del(anim);

	/* 2. Append the new timer */
	switch (bring_type) {
	case SCROLLER_BRING_TYPE_INSTANT_SHOW:
		_page_show(scroller, i, freeze);
		break;
	case SCROLLER_BRING_TYPE_INSTANT:
		_page_region_show(scroller, i, freeze);
		break;
	case SCROLLER_BRING_TYPE_JOB:
		job = ecore_job_add(_region_show_job_cb, scroller);
		if (job) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB, job);
		else _E("Cannot add a job");
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE, (void *) i);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
		break;
	case SCROLLER_BRING_TYPE_ANIMATOR:
		anim = ecore_animator_add(_region_show_anim_cb, scroller);
		if (anim) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM, anim);
		else _E("Cannot add an animator");
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE, (void *) i);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
		break;
	default:
		_E("Unsupported bring_type:%d", bring_type);
		break;
	}
}



HAPI void scroller_region_show_page(Evas_Object *scroller, Evas_Object *page, scroller_freeze_e freeze, scroller_bring_type_e bring_type)
{
	Ecore_Animator *anim = NULL;
	Ecore_Job *job = NULL;
	scroller_info_s *scroller_info = NULL;
	int i = 0;


	ret_if(!scroller);
	ret_if(!page);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	i = _get_index_in_list(scroller_info->box, page);

	/* 1. Remove the old action */
	job = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB);
	if (job) ecore_job_del(job);

	anim = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM);
	if (anim) ecore_animator_del(anim);

	/* 2. Append the new timer */
	switch (bring_type) {
	case SCROLLER_BRING_TYPE_INSTANT_SHOW:
		_page_show(scroller, i, freeze);
		break;
	case SCROLLER_BRING_TYPE_INSTANT:
		_page_region_show(scroller, i, freeze);
		break;
	case SCROLLER_BRING_TYPE_JOB:
		job = ecore_job_add(_region_show_job_cb, scroller);
		if (job) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_JOB, job);
		else _E("Cannot add a job");
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE, (void *) i);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
		break;
	case SCROLLER_BRING_TYPE_ANIMATOR:
		anim = ecore_animator_add(_region_show_anim_cb, scroller);
		if (anim) evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ANIM, anim);
		else _E("Cannot add an animator");
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_PAGE, (void *) i);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_FREEZED, (void *) freeze);
		break;
	default:
		_E("Unsupported bring_type:%d", bring_type);
		break;
	}
}



static Evas_Object *_pop_page_in_list(Eina_List **list, const char *id, const char *subid)
{
	Evas_Object *page = NULL;
	const Eina_List *l, *ln;
	page_info_s *page_info = NULL;

	retv_if(!list, NULL);
	retv_if(!id, NULL);

	EINA_LIST_FOREACH_SAFE(*list, l, ln, page) {
		continue_if(!page);

		page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		continue_if(!page_info);
		if (!page_info->id) continue;

		if (strcmp(page_info->id, id)) continue;

		if (!subid && !page_info->subid) {
			*list = eina_list_remove(*list, page);
			return page;
		}

		if (subid
			&& page_info->subid
			&& !strcmp(subid, page_info->subid))
		{
			*list = eina_list_remove(*list, page);
			return page;
		}
	}

	return NULL;
}



static page_info_s *_get_page_info_in_list(page_info_s *scroller_page_info, Eina_List *page_info_list)
{
	const Eina_List *l, *ln;
	page_info_s *page_info = NULL;

	retv_if(!scroller_page_info, NULL);
	retv_if(!scroller_page_info->id, NULL);
	retv_if(!page_info_list, NULL);

	EINA_LIST_FOREACH_SAFE(page_info_list, l, ln, page_info) {
		continue_if(!page_info);
		continue_if(!page_info->id);

		if (!strcmp(scroller_page_info->id, page_info->id)) {
			if (((scroller_page_info->subid && page_info->subid)
				 && !strcmp(scroller_page_info->subid, page_info->subid))
				 || (!scroller_page_info->subid && !page_info->subid))
			{
				return page_info;
			}
		}
	}
	return NULL;
}



static Evas_Object *_get_page_in_list(page_info_s *page_info, Eina_List *list)
{
	const Eina_List *l, *ln;
	Evas_Object *page = NULL;
	page_info_s *scroller_page_info = NULL;

	retv_if(!page_info, NULL);
	retv_if(!page_info->id, NULL);
	retv_if(!list, NULL);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);

		scroller_page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		continue_if(!scroller_page_info);
		if (!scroller_page_info->id) continue;

		if (!strcmp(scroller_page_info->id, page_info->id)) {
			if (((scroller_page_info->subid && page_info->subid)
				 && !strcmp(scroller_page_info->subid, page_info->subid))
				 || (!scroller_page_info->subid && !page_info->subid))
			{
				return scroller_page_info->page;
			}
		}
	}
	return NULL;
}



HAPI void scroller_read_favorites_list(Evas_Object *scroller, Eina_List *page_info_list)
{
	Evas_Object *page = NULL;
	Evas_Object *scroller_page = NULL;
	Evas_Object *before_page = NULL;

	Eina_List *list = NULL;
	Eina_List *cloned_page_info_list = NULL;
	const Eina_List *l, *ln;

	scroller_info_s *scroller_info = NULL;
	page_info_s *scroller_page_info = NULL;
	page_info_s *page_info = NULL;
	page_info_s *clone_info = NULL;

	ret_if(!scroller);
	ret_if(!page_info_list);

	cloned_page_info_list = eina_list_clone(page_info_list);
	ret_if(!cloned_page_info_list);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	before_page = scroller_info->center;

	/* 1. Remove widgets those are not in the page_info_list */
	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);

		scroller_page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		continue_if(!scroller_page_info);
		if (!scroller_page_info->id) continue;

		if (scroller_page_info->direction != PAGE_DIRECTION_RIGHT) continue;

		clone_info = _get_page_info_in_list(scroller_page_info, cloned_page_info_list);
		if (clone_info) {
			_D("Keep this widget : %s(%p)", scroller_page_info->id, page);
			cloned_page_info_list = eina_list_remove(cloned_page_info_list, clone_info);
			continue;
		} else {
			_D("Do not keep this widget : %s(%p)", scroller_page_info->id, page);
			list = eina_list_remove(list, scroller_page_info->page);
			page_destroy(scroller_page_info->page);
		}
	}

	/* 2. Create widgets those are not in the scroller */
	EINA_LIST_FOREACH_SAFE(page_info_list, l, ln, page_info) {
		continue_if(!page_info);
		continue_if(!page_info->id);

		scroller_page = _get_page_in_list(page_info, list);
		if (scroller_page) {
			_D("Reorder this widget : %s(%p)", page_info->id, scroller_page);
			_elm_box_unpack(scroller, scroller_page);
			_elm_box_pack_after(scroller, scroller_page, before_page);
			before_page = scroller_page;
			continue;
		} else {
			/* Page create */
			page_info->item = widget_create(scroller, page_info->id, page_info->subid, page_info->period);
			ret_if (!page_info->item);
			widget_viewer_evas_disable_loading(page_info->item);
			evas_object_resize(page_info->item, scroller_info->page_width, scroller_info->page_height);
			evas_object_size_hint_min_set(page_info->item, scroller_info->page_width, scroller_info->page_height);
			evas_object_show(page_info->item);

			page_info->page = page_create(scroller
									, page_info->item
									, page_info->id, page_info->subid
									, scroller_info->page_width, scroller_info->page_height
									, PAGE_CHANGEABLE_BG_ON, PAGE_REMOVABLE_ON);
			if (!page_info->page) {
				_E("Cannot create a page");
				evas_object_del(page_info->item);
				return;
			}
			widget_add_callback(page_info->item, page_info->page);

			page_set_effect(page_info->page, page_effect_none, page_effect_none);
			scroller_push_page(scroller, page_info->page, SCROLLER_PUSH_TYPE_LAST);

			before_page = page_info->page;

			_D("Create this widget : %s(%p)", page_info->id, page_info->page);
		}
	}

	eina_list_free(list);
	eina_list_free(cloned_page_info_list);

	page_arrange_plus_page(scroller, 0);
}



/* Caution : Do not create & destroy an item */
HAPI void scroller_read_list(Evas_Object *scroller, Eina_List *page_info_list)
{
	Evas_Object *page = NULL;
	Evas_Object *before_page = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;

	ret_if(!scroller);
	ret_if(!page_info_list);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	before_page = scroller_info->center;

	EINA_LIST_FOREACH_SAFE(page_info_list, l, ln, page_info) {
		continue_if(!page_info);
		continue_if(!page_info->id);

		if (page_info->direction != PAGE_DIRECTION_RIGHT) continue;

		page = _pop_page_in_list(&list, page_info->id, page_info->subid);
		if (page) {
			_elm_box_unpack(scroller, page);
			_elm_box_pack_after(scroller, page, before_page);
			before_page = page;
		}
	}

	eina_list_free(list);
}



HAPI Eina_List *scroller_write_list(Evas_Object *scroller)
{
	Evas_Object *page = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	page_info_s *dup_page_info = NULL;
	Eina_List *list = NULL;
	Eina_List *page_info_list = NULL;
	const Eina_List *l, *ln;

	retv_if(!scroller, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, NULL);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);

		page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		if (!page_info) continue;
		if (!page_info->id) continue;
		if (PAGE_DIRECTION_RIGHT != page_info->direction) continue;

		dup_page_info = page_info_dup(page_info);
		continue_if(!dup_page_info);

		page_info_list = eina_list_append(page_info_list, dup_page_info);
	}
	eina_list_free(list);

	return page_info_list;
}



HAPI Evas_Object *scroller_move_page_prev(Evas_Object *scroller, Evas_Object *from_page, Evas_Object *to_page, Evas_Object *append_page)
{
	scroller_info_s *scroller_info = NULL;

	retv_if(!scroller, NULL);
	retv_if(!from_page, NULL);
	retv_if(!to_page, NULL);
	retv_if(!append_page, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	_elm_box_unpack(scroller, from_page);
	_elm_box_pack_after(scroller, append_page, to_page);

	return from_page;
}



HAPI Evas_Object *scroller_move_page_next(Evas_Object *scroller, Evas_Object *from_page, Evas_Object *to_page, Evas_Object *insert_page)
{
	scroller_info_s *scroller_info = NULL;

	retv_if(!scroller, NULL);
	retv_if(!from_page, NULL);
	retv_if(!to_page, NULL);
	retv_if(!insert_page, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	_elm_box_unpack(scroller, to_page);
	_elm_box_pack_before(scroller, insert_page, from_page);

	return to_page;
}



HAPI int scroller_seek_page_position(Evas_Object *scroller, Evas_Object *page)
{
	Evas_Object *tmp = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	scroller_info_s *scroller_info = NULL;
	int position = 0;

	retv_if(!scroller, -1);
	retv_if(!page, -1);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, -1);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, -1);

	EINA_LIST_FOREACH_SAFE(list, l, ln, tmp) {
		continue_if(!tmp);
		if (page == tmp) {
			eina_list_free(list);
			return position;
		}
		position++;
	}

	eina_list_free(list);
	return -1;
}



HAPI Evas_Object *scroller_get_page_at(Evas_Object *scroller, int idx)
{
	Evas_Object *page = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	scroller_info_s *scroller_info = NULL;
	int position = 0;

	retv_if(!scroller, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(NULL == list, NULL);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		if (idx == position) {
			eina_list_free(list);
			return page;
		}
		position ++;
	}
	eina_list_free(list);
	return NULL;
}



HAPI int scroller_get_current_page_direction(Evas_Object *scroller)
{
	Evas_Object *cur_page = NULL;
	page_info_s *page_info = NULL;

	retv_if(!scroller, -1);

	cur_page = scroller_get_focused_page(scroller);
	retv_if(!cur_page, -1);

	page_info = evas_object_data_get(cur_page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, -1);

	return page_info->direction;
}



HAPI Evas_Object *scroller_get_left_page(Evas_Object *scroller, Evas_Object *page)
{
	Evas_Object *before_page = NULL;
	Evas_Object *tmp = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	scroller_info_s *scroller_info = NULL;

	retv_if(!scroller, NULL);
	retv_if(!page, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, NULL);

	EINA_LIST_FOREACH_SAFE(list, l, ln, tmp) {
		continue_if(!tmp);

		if (page == tmp) break;
		before_page = tmp;
	}
	eina_list_free(list);
	return before_page;
}



HAPI Evas_Object *scroller_get_right_page(Evas_Object *scroller, Evas_Object *page)
{
	Evas_Object *after_page = NULL;
	Evas_Object *tmp = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	scroller_info_s *scroller_info = NULL;

	retv_if(!scroller, NULL);
	retv_if(!page, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, NULL);

	EINA_LIST_FOREACH_SAFE(list, l, ln, tmp) {
		continue_if(!tmp);

		if (page == tmp) {
			after_page = eina_list_data_get(ln);
			break;
		}
	}
	eina_list_free(list);
	return after_page;
}



HAPI void scroller_region_show_page_without_timer(Evas_Object *scroller, Evas_Object *page)
{
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	int i = 0;
	ret_if(scroller == NULL);
	ret_if(page == NULL);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	i = _get_index_in_list(scroller_info->box, page);

#if 0 /* EFL Private feature */
	elm_scroller_origin_reverse_set(scroller, EINA_FALSE, EINA_FALSE);
#endif
	_D("Page show now : %d(%p)", i, page);
	elm_scroller_page_show(scroller, i, 0);
	if (scroller_info->index[page_info->direction]) {
		index_bring_in_page(scroller_info->index[page_info->direction], page);
	}
}



HAPI void scroller_enable_focus_on_scroll(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	_D("Enable the focus on scroll");

	elm_object_tree_focus_allow_set(scroller, EINA_TRUE);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	scroller_info->scroll_focus = 1;
}



HAPI void scroller_disable_focus_on_scroll(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	_D("Disable the focus on scroll");

	elm_object_tree_focus_allow_set(scroller, EINA_FALSE);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	scroller_info->scroll_focus = 0;
}



HAPI void scroller_enable_index_on_scroll(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	_D("Enable the index on scroll");

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	scroller_info->scroll_index = 1;
}



HAPI void scroller_disable_index_on_scroll(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	_D("Disable the index on scroll");

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	scroller_info->scroll_index = 0;
}



HAPI void scroller_enable_effect_on_scroll(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	_D("Enable the effect on scroll");

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	scroller_info->scroll_effect = 1;
}



HAPI void scroller_disable_effect_on_scroll(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	_D("Disable the effect on scroll");

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	scroller_info->scroll_effect = 0;
}



HAPI void scroller_enable_cover_on_scroll(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	scroller_info->scroll_cover = 1;
}



HAPI void scroller_disable_cover_on_scroll(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	scroller_info->scroll_cover = 0;
}



HAPI void scroller_backup_inner_focus(Evas_Object *scroller)
{
	Evas_Object *prev_page = NULL;
	Evas_Object *cur_page = NULL;
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	ret_if(!scroller_info->box);

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	EINA_LIST_FREE(list, cur_page) {
		continue_if(!cur_page);

		if (prev_page) {
			page_backup_inner_focus(cur_page, prev_page, NULL);
			page_backup_inner_focus(prev_page, NULL, cur_page);
		}

		prev_page = cur_page;
	}
}



HAPI void scroller_restore_inner_focus(Evas_Object *scroller)
{
	Evas_Object *page = NULL;
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	ret_if(!scroller_info->box);

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	EINA_LIST_FREE(list, page) {
		continue_if(!page);
		page_restore_inner_focus(page);
	}
}



/* This function just reorders not making a notification panel or widget */
/* CRITICAL : You have to use this function only for notification reordering */
HAPI void scroller_reorder_with_list(Evas_Object *scroller, Eina_List *list, page_direction_e page_direction)
{
	Evas_Object *tmp_page = NULL;
	Evas_Object *focus_page = NULL;
	Evas_Object *after = NULL;
	Eina_List *reverse_list = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;

	ret_if(!scroller);
	ret_if(!list);

	reverse_list = eina_list_reverse_clone(list);
	ret_if(!reverse_list);

	focus_page = scroller_get_focused_page(scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	ret_if(!scroller_info->box);

	if (scroller_info->center_neighbor_right) {
		after = scroller_info->center_neighbor_right;
	} else {
		after = scroller_info->center;
	}
	ret_if(!after);

	EINA_LIST_FREE(reverse_list, tmp_page) {
		page_info = evas_object_data_get(tmp_page, DATA_KEY_PAGE_INFO);
		continue_if(!page_info);

		switch (page_info->direction) {
		case PAGE_DIRECTION_LEFT:
			_D("Reorder a page(%p) with pack_start", tmp_page);
			elm_box_unpack(scroller_info->box, tmp_page);
			elm_box_pack_start(scroller_info->box, tmp_page);
			break;
		case PAGE_DIRECTION_RIGHT:
			_D("Reorder a page(%p) with pack_after", tmp_page);
			elm_box_unpack(scroller_info->box, tmp_page);
			elm_box_pack_after(scroller_info->box, tmp_page, after);
			break;
		default:
			_E("Cannot reach here");
			break;
		}
	}

	elm_box_recalculate(scroller_info->box);
	elm_box_recalculate(scroller_info->box_layout);

	if (focus_page) scroller_region_show_page(scroller, focus_page, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT);
	else _E("Cannot get the focused page");

	scroller_info->index_update_timer = ecore_timer_add(INDEX_UPDATE_TIME, _index_update_cb, scroller);
	if (!scroller_info->index_update_timer) {
		_E("Cannot add an index update timer");
	}
}



// End of this file
