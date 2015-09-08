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
#include <app_control.h>
#include <efl_extension.h>

#include "log.h"
#include "util.h"
#include "apps/apps_conf.h"
#include "apps/effect.h"
#include "apps/item.h"
#include "apps/item_info.h"
#include "apps/layout.h"
#include "apps/apps_main.h"
#include "apps/page.h"
#include "apps/scroller.h"
#include "apps/scroller_info.h"
#include "wms.h"


#define BOX_EDJE EDJEDIR"/apps_box.edj"
#define BOX_GROUP_NAME "box"
#if 0 /* Do not need buttons */
#define BOX_TOP_BTN_GROUP_NAME "top_button"
#define BOX_BOTTOM_BTN_GROUP_NAME "bottom_button"
#endif

#define PRIVATE_DATA_KEY_SCROLLER_MOUSE_DOWN "p_sc_m_dn"
#define PRIVATE_DATA_KEY_SCROLLER_DRAG "p_sc_dr"
#define PRIVATE_DATA_KEY_IS_SCROLLING "p_is_scr"
#define PRIVATE_DATA_KEY_SCROLLER_REVERSE_LIST "p_sc_re_lt"
#define PRIVATE_DATA_KEY_SCROLLER_POP_ALL_TIMER "p_sc_p_tm"
#define PRIVATE_DATA_KEY_SCROLLER_PUSH_ALL_TIMER "p_p_tm"
#define PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER "p_sc_tmer"
#define PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ITH "p_sc_ith"

#define PRIVATE_DATA_KEY_ITEM_INFO_LIST_APPENDED "p_iil_and"
#define PRIVATE_DATA_KEY_ITEM_INFO_LIST_INDEX "p_iil_idx"
#define PRIVATE_DATA_KEY_ITEM_INFO_LIST_TIMER "p_iil_tm"



static Eina_Bool _rotary_cb(void *data, Evas_Object *obj, Eext_Rotary_Event_Info *rotary_info)
{
	_APPS_D("Rotary callback is called");
	Evas_Object *scroller = obj;

	retv_if(!scroller, ECORE_CALLBACK_PASS_ON);

	_APPS_D("Detent detected, obj[%p], direction[%d], timeStamp[%u]", obj, rotary_info->direction, rotary_info->time_stamp);
	if (rotary_info->direction == EEXT_ROTARY_DIRECTION_CLOCKWISE) {
		apps_scroller_bring_in_region_by_vector(scroller, 1);
	} else {
		apps_scroller_bring_in_region_by_vector(scroller, -1);
	}

	return ECORE_CALLBACK_PASS_ON;
}



static void _init_rotary(Evas_Object *scroller)
{
	_APPS_D("Initialize the rotary event");
	eext_rotary_object_event_callback_add(scroller, _rotary_cb, NULL);
}



static void _destroy_rotary(Evas_Object *scroller)
{
	_APPS_D("Finish the rotary event");
	eext_rotary_object_event_callback_del(scroller, _rotary_cb);
}



static void _mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ei = event_info;
	ret_if(!ei);

	_APPS_D("Mouse is down [%d,%d]", ei->output.x, ei->output.y);
	evas_object_data_set(obj, PRIVATE_DATA_KEY_SCROLLER_MOUSE_DOWN, (void *) 1);
}



static void _mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Up *ei = event_info;
	ret_if(!ei);

	_APPS_D("Mouse is up [%d,%d]", ei->output.x, ei->output.y);
	evas_object_data_del(obj, PRIVATE_DATA_KEY_SCROLLER_MOUSE_DOWN);

	if (evas_object_data_get(obj, PRIVATE_DATA_KEY_SCROLLER_DRAG)) return;
	evas_object_data_del(obj, PRIVATE_DATA_KEY_IS_SCROLLING);
}



static void _anim_start_cb(void *data, Evas_Object *scroller, void *event_info)
{
	_APPS_D("start the scroller animation");
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_IS_SCROLLING, (void *) 1);

	Evas_Object *item = item_get_press_item(scroller);
	if(item) {
		if(item_is_longpressed(item)) {
			_APPS_D("longressed");
		}
		else {
			item_is_released(item);
		}
	}
}



static void _anim_stop_cb(void *data, Evas_Object *scroller, void *event_info)
{
	_APPS_D("stop the scroller animation");
	if (evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_MOUSE_DOWN)) return;
	evas_object_data_del(scroller, PRIVATE_DATA_KEY_IS_SCROLLING);
}



static void _drag_start_cb(void *data, Evas_Object *scroller, void *event_info)
{
	_APPS_D("start to drag the scroller animation");
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_DRAG, (void *) 1);
	/* _drag_start_cb is called even if the scroller is not moved. */
	//evas_object_data_set(scroller, PRIVATE_DATA_KEY_IS_SCROLLING, (void *) 1);

	Evas_Object *item = item_get_press_item(scroller);
	if(item) {
		if(item_is_longpressed(item)) {
			_APPS_D("longressed");
		}
		else {
			item_is_released(item);
		}
	}
}



static void _drag_stop_cb(void *data, Evas_Object *scroller, void *event_info)
{
	_APPS_D("stop to drag the scroller animation");
	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_DRAG);
}



static void _scroll_cb(void *data, Evas_Object *scroller, void *event_info)
{
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_IS_SCROLLING, (void *) 1);
}



static apps_error_e _resume_result_cb(void *data)
{
	Evas_Object *scroller = data;
	retv_if(!scroller, APPS_ERROR_INVALID_PARAMETER);

	_APPS_D("Activate the rotary events for apps");

	eext_rotary_object_event_activated_set(scroller, EINA_TRUE);

	return APPS_ERROR_NONE;
}



HAPI void apps_scroller_destroy(Evas_Object *layout)
{
	Evas_Object *scroller = NULL;
	Evas_Object *page = NULL;
	Eina_List *box_list = NULL;
	scroller_info_s *scroller_info = NULL;

	ret_if(!layout);

	scroller = elm_object_part_content_unset(layout, "scroller");
	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	_destroy_rotary(scroller);

	apps_main_unregister_cb(scroller_info->instance_info, APPS_APP_STATE_RESUME, _resume_result_cb);

	box_list = elm_box_children_get(scroller_info->box);
	ret_if(!box_list);

	/* FIXME : We don't need to remove items? */
	EINA_LIST_FREE(box_list, page) {
		if (!page) break;
		apps_page_destroy(page);
	}

	evas_object_del(scroller_info->box);
	evas_object_del(scroller_info->box_layout);

	evas_object_event_callback_del(scroller, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb);
	evas_object_event_callback_del(scroller, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb);
	evas_object_smart_callback_del(scroller, "scroll,anim,start", _anim_start_cb);
	evas_object_smart_callback_del(scroller, "scroll,anim,stop", _anim_stop_cb);
	evas_object_smart_callback_del(scroller, "scroll,drag,start", _drag_start_cb);
	evas_object_smart_callback_del(scroller, "scroll,drag,stop", _drag_stop_cb);
	evas_object_smart_callback_del(scroller, "scroll", _scroll_cb);

	free(scroller_info);
	evas_object_del(scroller);
}



Eina_Bool _init_timer_cb(void *data)
{
	Evas_Object *scroller = data;
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;
	item_info_s *item_info = NULL;
	int index, count;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);

	list = evas_object_data_get(scroller_info->layout, DATA_KEY_LIST);
	retv_if(!list, ECORE_CALLBACK_CANCEL);

	index = (int) evas_object_data_get(scroller, DATA_KEY_LIST_INDEX);
	count = eina_list_count(list);
	if (index == count) goto OUT;

	item_info = eina_list_nth(list, index);
	goto_if(!item_info, OUT);

	item_info->item = item_create(scroller, item_info);
	item_info->ordering = index;
	apps_scroller_append_item(scroller, item_info->item);

	index ++;
	if (index == count) goto OUT;
	evas_object_data_set(scroller, DATA_KEY_LIST_INDEX, (void *) index);

	return ECORE_CALLBACK_RENEW;

OUT:
	_APPS_D("Loading apps is done");

	/* FIXME : Bring into the top */

	evas_object_data_del(scroller, DATA_KEY_LIST_INDEX);
	evas_object_data_del(scroller, DATA_KEY_TIMER);

	index = 0;

	wms_change_apps_order(W_HOME_WMS_BACKUP);

	return ECORE_CALLBACK_CANCEL;
}



#if 0 /* Do not need buttons */
static void _top_btn_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *top_btn_layout = data;
	ret_if(!top_btn_layout);

	Evas_Event_Mouse_Down *ei = event_info;
	ret_if(!ei);

	_APPS_D("Mouse is down [%d,%d]", ei->output.x, ei->output.y);

	Evas_Object *btn_text_layout = elm_object_part_content_get(top_btn_layout, "button,txt");
	ret_if(!btn_text_layout);

	elm_object_signal_emit(btn_text_layout, "press,button", "button");
}



static void _top_btn_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *top_btn_layout = data;
	ret_if(!top_btn_layout);

	Evas_Event_Mouse_Up *ei = event_info;
	ret_if(!ei);

	_APPS_D("Mouse is up [%d,%d]", ei->output.x, ei->output.y);

	Evas_Object *btn_text_layout = elm_object_part_content_get(top_btn_layout, "button,txt");
	ret_if(!btn_text_layout);

	elm_object_signal_emit(btn_text_layout, "release,button", "button");
}

#define APPS_W_TASKMGR_PKGNAME "org.tizen.w-taskmanager"
static void _top_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	_APPS_D("Clicked");

	Evas_Object *layout = (Evas_Object*)data;
	ret_if(!layout);

	if(apps_layout_is_edited(layout)) {
		_APPS_D("Edit mode");
		return;
	}

	apps_effect_play_sound();

	int isttsmode = apps_item_info_is_support_tts(APPS_W_TASKMGR_PKGNAME);
	if(apps_main_get_info()->tts && !isttsmode) {
		char tmp[NAME_LEN] = {0,};
		snprintf(tmp, sizeof(tmp), _("IDS_SCR_POP_PS_IS_NOT_AVAILABLE_WHILE_SCREEN_READER_IS_ENABLED"), _("IDS_ST_OPT_RECENT_APPS_ABB"));
		char *text = strdup(tmp);
		util_create_toast_popup(layout, text);
	}
	else {
		util_launch_app(APPS_W_TASKMGR_PKGNAME, NULL, NULL);
	}
}



#define APPS_SAMSUNGAPPS_MAIN_CATEGORY_LIST "samsungapps://MainCategoryList/"
static void _bottom_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	_APPS_D("Clicked");

	Evas_Object *layout = (Evas_Object*)data;
	ret_if(!layout);

	if(apps_layout_is_edited(layout)) {
		_APPS_D("Edit mode");
		return;
	}

	apps_effect_play_sound();
	wms_launch_gear_manager(data, APPS_SAMSUNGAPPS_MAIN_CATEGORY_LIST);
}
#endif



static char *_access_info_cb(void *data, Evas_Object *obj)
{
	retv_if(!data, NULL);

	char *tmp = strdup(data);
	retv_if(!tmp, NULL);

	return tmp;
}



HAPI Evas_Object *apps_scroller_create(Evas_Object *layout)
{
	Evas_Object *win = NULL;
	Evas_Object *scroller = NULL;
	Evas_Object *box_layout = NULL;
	Evas_Object *box = NULL;
	Ecore_Timer *timer = NULL;
	instance_info_s *instance_info = NULL;
	scroller_info_s *scroller_info = NULL;
	int height;

	win = evas_object_data_get(layout, DATA_KEY_WIN);
	retv_if(!win, NULL);

	instance_info = evas_object_data_get(win, DATA_KEY_INSTANCE_INFO);
	retv_if(!instance_info, NULL);

	Evas_Object *layout_focus = elm_object_part_content_get(layout, "focus");
	retv_if(!layout_focus, NULL);

	scroller = elm_scroller_add(layout);
	retv_if(!scroller, NULL);

	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_FALSE);
	elm_scroller_page_size_set(scroller, instance_info->root_w, ITEM_HEIGHT * apps_main_get_info()->scale);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, SCROLLER_POLICY_VERTICAL);
	elm_scroller_page_scroll_limit_set(scroller, SCROLLER_PAGE_LIMIT_HORIZONTAL, SCROLLER_PAGE_LIMIT_VERTICAL);
	elm_scroller_content_min_limit(scroller, EINA_FALSE, EINA_FALSE);
	elm_scroller_single_direction_set(scroller, ELM_SCROLLER_SINGLE_DIRECTION_HARD);

	elm_object_style_set(scroller, "effect");
	elm_object_part_content_set(layout, "scroller", scroller);

	height = (double) instance_info->root_h * apps_main_get_info()->scale;
	_APPS_D("Scroller width : %d, height : %d", instance_info->root_w, height);

	evas_object_size_hint_min_set(scroller, instance_info->root_w, height);
	evas_object_size_hint_max_set(scroller, instance_info->root_w, height);
	evas_object_show(scroller);

	evas_object_event_callback_add(scroller, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, NULL);
	evas_object_event_callback_add(scroller, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, NULL);
	evas_object_smart_callback_add(scroller, "scroll,anim,start", _anim_start_cb, NULL);
	evas_object_smart_callback_add(scroller, "scroll,anim,stop", _anim_stop_cb, NULL);
	evas_object_smart_callback_add(scroller, "scroll,drag,start", _drag_start_cb, NULL);
	evas_object_smart_callback_add(scroller, "scroll,drag,stop", _drag_stop_cb, NULL);
	evas_object_smart_callback_add(scroller, "scroll", _scroll_cb, NULL);

	/* Use the layout between box and scroller because of alignment of a page in the box. */
	box_layout = elm_layout_add(scroller);
	goto_if(!box_layout, ERROR);

	elm_layout_file_set(box_layout, BOX_EDJE, BOX_GROUP_NAME);
	evas_object_show(box_layout);
	elm_object_content_set(scroller, box_layout);

	box = elm_box_add(box_layout);
	goto_if(!box, ERROR);

	elm_box_horizontal_set(box, SCROLLER_HORIZONTAL);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(box);
	elm_object_part_content_set(box_layout, "box", box);

	timer = ecore_timer_add(0.01f, _init_timer_cb, scroller);
	if (!timer) {
		_APPS_E("Cannot add a timer");
		apps_scroller_destroy(layout);
		return NULL;
	}
	evas_object_data_set(scroller, DATA_KEY_TIMER, timer);

	scroller_info = calloc(1, sizeof(scroller_info_s));
	if (!scroller_info) {
		_APPS_E("Cannot calloc for the scroller_info");
		apps_scroller_destroy(layout);
		return NULL;
	}

	scroller_info->win = win;
	scroller_info->instance_info = instance_info;
	scroller_info->layout = layout;
	scroller_info->box_layout = box_layout;
	scroller_info->box = box;
	scroller_info->layout_focus = layout_focus;
	scroller_info->list = NULL;
	scroller_info->list_index = 0;

	evas_object_data_set(scroller, DATA_KEY_SCROLLER_INFO, scroller_info);

	_init_rotary(scroller);
	if (APPS_ERROR_NONE != apps_main_register_cb(instance_info, APPS_APP_STATE_RESUME, _resume_result_cb, scroller)) {
		_APPS_E("Cannot register the pause callback");
	}

	return scroller;

ERROR:
	if(box_layout) evas_object_del(box_layout);
	if(scroller) evas_object_del(scroller);
#if 0 /* Do not need buttons */
	if(top_btn_layout) evas_object_del(top_btn_layout);
	if(bottom_btn_layout) evas_object_del(bottom_btn_layout);
#endif

	return NULL;
}



HAPI int apps_scroller_count_page(Evas_Object *scroller)
{
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;
	int count = 0;

	retv_if(!scroller, 0);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, 0);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, 0);

	count = eina_list_count(list);
	eina_list_free(list);

	return count;
}



HAPI int apps_scroller_count_item(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	Evas_Object *page = NULL;
	int count = 0;

	retv_if(!scroller, -1);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, -1);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, -1);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		count += apps_page_count_item(page);
	}

	eina_list_free(list);

	return count;
}



HAPI Eina_Bool apps_scroller_is_scrolling(Evas_Object *scroller)
{
	return evas_object_data_get(scroller, PRIVATE_DATA_KEY_IS_SCROLLING)? EINA_TRUE:EINA_FALSE;
}



HAPI void apps_scroller_freeze(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	elm_object_scroll_freeze_push(scroller_info->box);
}



HAPI void apps_scroller_unfreeze(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	while (elm_object_scroll_freeze_get(scroller_info->box)) {
		elm_object_scroll_freeze_pop(scroller_info->box);
	}
}



static Eina_Bool _bring_in_timer_cb(void *data)
{
	Evas_Object *scroller = data;
	int i = 0;

	i = (int) evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ITH);
	elm_scroller_page_bring_in(scroller, i, 0);
	evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER);
	return ECORE_CALLBACK_CANCEL;
}



HAPI void apps_scroller_bring_in(Evas_Object *scroller, int page_no)
{
	Ecore_Timer *timer = NULL;

	ret_if(!scroller);

	/* 1. Remove the old timer */
	timer = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER);
	if (timer) ecore_timer_del(timer);

	/* 2. Append the new timer */
	timer = ecore_timer_add(0.01f, _bring_in_timer_cb, scroller);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER, timer);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ITH, (void *) page_no);
}



HAPI void apps_scroller_bring_in_page(Evas_Object *scroller, Evas_Object *page)
{
	Evas_Object *tmp = NULL;
	Eina_List *list = NULL;
	Ecore_Timer *timer = NULL;
	scroller_info_s *scroller_info = NULL;
	const Eina_List *l;
	const Eina_List *n;
	int i = -1;

	ret_if(!scroller);
	ret_if(!page);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	EINA_LIST_FOREACH_SAFE(list, l, n, tmp) {
		i++;
		break_if(!tmp);
		if (tmp != page) continue;

		/* 1. Remove the old timer */
		timer = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER);
		if (timer) ecore_timer_del(timer);

		/* 2. Append the new timer */
		timer = ecore_timer_add(0.01f, _bring_in_timer_cb, scroller);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_TIMER, timer);
		evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_BRING_IN_ITH, (void *) i);
		break;
	}
	eina_list_free(list);
}



HAPI void apps_scroller_region_show(Evas_Object *scroller, int x, int y)
{
	scroller_info_s *scroller_info = NULL;
	instance_info_s *instance_info = NULL;
	int w, h;

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	instance_info = scroller_info->instance_info;
	ret_if(!instance_info);

	evas_object_geometry_get(scroller, NULL, NULL, &w, &h);
	elm_scroller_region_show(scroller, x, y, w, h);
}



HAPI void apps_scroller_append_item(Evas_Object *scroller, Evas_Object *item)
{
	Evas_Object *last_page = NULL;
	Eina_List *list, *last;
	scroller_info_s *scroller_info = NULL;
	item_info_s *item_info = NULL;
	int items_per_page = 0;

	ret_if(!scroller);
	ret_if(!item);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	list = elm_box_children_get(scroller_info->box);
	if (list) {
		last = eina_list_last(list);
		goto_if(!last, ERROR);

		last_page = eina_list_data_get(last);
		goto_if(!last_page, ERROR);

		eina_list_free(list);

		items_per_page = apps_page_count_item(last_page);
	} else {
		/* If there is no list, then we have to create a page */
		items_per_page = APPS_PER_PAGE;
	}

	if (items_per_page < APPS_PER_PAGE) {
		apps_page_pack_item(last_page, item);
	} else {
		Evas_Object *page = apps_page_create(scroller, last_page, NULL);
		ret_if(!page);
		elm_box_pack_end(scroller_info->box, page);
		apps_page_pack_item(page, item);
	}

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	scroller_info->list = eina_list_append(scroller_info->list, item_info);

	return;

ERROR:
	_APPS_E("Find an error on appending an item");
	if (list) eina_list_free(list);
}



HAPI void apps_scroller_remove_item(Evas_Object *scroller, Evas_Object *item)
{
	Evas_Object *page = NULL;
	Eina_List *list;
	const Eina_List *l, *ln;
	scroller_info_s *scroller_info = NULL;
	item_info_s *item_info = NULL;

	ret_if(!scroller);
	ret_if(!item);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		if (APPS_ERROR_NONE == apps_page_unpack_item(page, item)) {
			break;
		}
	}

	eina_list_free(list);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	scroller_info->list = eina_list_remove(scroller_info->list, item_info);
}



HAPI apps_error_e apps_scroller_pack_nth(Evas_Object *scroller, Evas_Object *item, int ordering)
{
	Evas_Object *page = NULL;
	Evas_Object *last_page = NULL;
	Evas_Object *tmp = NULL;
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	Eina_List *last = NULL;
	int cur_count, more_page_count;
	register int i;

	retv_if(!scroller, APPS_ERROR_INVALID_PARAMETER);
	retv_if(!item, APPS_ERROR_INVALID_PARAMETER);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, APPS_ERROR_FAIL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, APPS_ERROR_FAIL);

	cur_count = eina_list_count(list);
	more_page_count = (ordering / APPS_PER_PAGE + 1) - cur_count;

	if (more_page_count > 0) {
		last = eina_list_last(list);
		if (last) last_page = eina_list_data_get(last);
	}

	for (i = 0; i < more_page_count; i++) {
		Evas_Object *new_page = apps_page_create(scroller, last_page, NULL);
		retv_if(!new_page, APPS_ERROR_FAIL);
		last_page = new_page;
	}

	page = eina_list_nth(list, ordering / APPS_PER_PAGE);
	retv_if(!page, APPS_ERROR_FAIL);

	tmp = apps_page_unpack_nth(page, ordering % APPS_PER_PAGE);
	if (tmp) {
		item_info_s *item_info = evas_object_data_get(tmp, DATA_KEY_ITEM_INFO);
		if (item_info && item_info->appid) {
			_APPS_D("Destroy an item[%s]", item_info->appid);
		}
	}
	apps_page_pack_nth(page, item, ordering % APPS_PER_PAGE);

	return APPS_ERROR_NONE;
}



static Eina_Bool _append_items_timer_cb(void *data)
{
	Evas_Object *scroller = data;
	Eina_List *list = NULL;
	item_info_s *item_info = NULL;
	int index, count;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);

	list = evas_object_data_get(scroller, PRIVATE_DATA_KEY_ITEM_INFO_LIST_APPENDED);
	retv_if(!list, ECORE_CALLBACK_CANCEL);

	index = (int) evas_object_data_get(scroller, PRIVATE_DATA_KEY_ITEM_INFO_LIST_INDEX);
	count = eina_list_count(list);
	if (index == count) goto OUT;

	item_info = eina_list_nth(list, index);
	goto_if(!item_info, OUT);

	item_info->item = item_create(scroller, item_info);
	item_info->ordering = index;
	apps_scroller_append_item(scroller, item_info->item);

	index ++;
	if (index == count) goto OUT;
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_ITEM_INFO_LIST_INDEX, (void *) index);

	return ECORE_CALLBACK_RENEW;

OUT:
	_APPS_D("Loading apps is done");

	/* FIXME : Scroller into the top */

	evas_object_data_del(scroller, PRIVATE_DATA_KEY_ITEM_INFO_LIST_TIMER);
	evas_object_data_del(scroller, PRIVATE_DATA_KEY_ITEM_INFO_LIST_INDEX);
	evas_object_data_del(scroller, PRIVATE_DATA_KEY_ITEM_INFO_LIST_APPENDED);

	index = 0;

	return ECORE_CALLBACK_CANCEL;
}



HAPI void apps_scroller_append_list(Evas_Object *scroller, Eina_List *list)
{
	Ecore_Timer *timer = NULL;

	ret_if(!scroller);
	ret_if(!list);

	timer = evas_object_data_del(scroller, PRIVATE_DATA_KEY_ITEM_INFO_LIST_TIMER);
	if (timer) ecore_timer_del(timer);

	timer = ecore_timer_add(0.01f, _append_items_timer_cb, scroller);
	ret_if(!timer);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_ITEM_INFO_LIST_TIMER, timer);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_ITEM_INFO_LIST_INDEX, NULL);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_ITEM_INFO_LIST_APPENDED, list);
}



HAPI void apps_scroller_remove_list(Evas_Object *scroller, Eina_List *list)
{
	const Eina_List *l, *ln;
	item_info_s *item_info = NULL;

	ret_if(!scroller);
	ret_if(!list);

	EINA_LIST_FOREACH_SAFE(list, l, ln, item_info) {
		continue_if(!item_info);
		if (item_info->item) {
			apps_scroller_remove_item(scroller, item_info->item);
			item_destroy(item_info->item);
		}
		item_info->item = NULL;
	}
}



HAPI void apps_scroller_trim(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *cur_list = NULL;
	Eina_List *page_list = NULL;
	Eina_List *item_list = NULL;
	const Eina_List *l, *ln;

	Evas_Object *page = NULL;
	Evas_Object *item = NULL;

	int i, count = 0, index = 0, pull = 0;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	cur_list = elm_box_children_get(scroller_info->box);
	ret_if(!cur_list);

	// 1. Unpack all untrimmed items
	EINA_LIST_FOREACH_SAFE(cur_list, l, ln, page) {
		continue_if(!page);

		if(!pull) {
			count = apps_page_count_item(page);
			if (APPS_PER_PAGE == count) continue;
		}

		pull = 1;

		for (i = 0; i < APPS_PER_PAGE; i++) {
			item = apps_page_unpack_nth(page, i);
			if (!item) continue;
			item_list = eina_list_append(item_list, item);
		}

		page_list = eina_list_append(page_list, page);
	}
	eina_list_free(cur_list);
	count = eina_list_count(item_list);

	// 2. Pack all items
	EINA_LIST_FOREACH_SAFE(page_list, l, ln, page) {
		for (i = 0; item_list && i < APPS_PER_PAGE; i++) {
			item = eina_list_nth(item_list, index);
			index++;
			continue_if(!item);

			break_if(APPS_ERROR_NONE != apps_page_pack_nth(page, item, i));
		}
	}

	eina_list_free(item_list);

	// 3. Remove empty page
	EINA_LIST_FREE(page_list, page) {
		if (apps_page_count_item(page) == 0) {
			apps_page_destroy(page);
		}
	}
}



HAPI void apps_scroller_change_language(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	Evas_Object *page = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	elm_access_info_cb_set(scroller_info->layout_focus, ELM_ACCESS_INFO, _access_info_cb, _("IDS_IDLE_BODY_APPS"));
#if 0 /* Do not need buttons */
	elm_access_info_cb_set(scroller_info->top_focus, ELM_ACCESS_INFO, _access_info_cb, _("IDS_ST_OPT_RECENT_APPS_ABB"));
	elm_access_info_cb_set(scroller_info->bottom_focus, ELM_ACCESS_INFO, _access_info_cb, _("IDS_HS_BUTTON_GET_MORE_APPLICATIONS_ABB2"));
#endif

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);
		apps_page_change_language(page);
	}

	eina_list_free(list);
}



/* return a page that has the item */
HAPI Evas_Object *apps_scroller_has_item(Evas_Object *scroller, Evas_Object *item)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	Evas_Object *page = NULL;

	retv_if(!scroller, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, NULL);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);
		if (apps_page_has_item(page, item)) {
			eina_list_free(list);
			return page;
		}
	}

	eina_list_free(list);
	return NULL;
}



HAPI Evas_Object *apps_scroller_move_item_prev(Evas_Object *scroller, Evas_Object *from_item, Evas_Object *to_item, Evas_Object *append_item)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	Eina_List *l;
	Evas_Object *page = NULL;
	Evas_Object *tmp_page = NULL;
	Evas_Object *tmp_next;
	item_info_s *from_info = NULL;
	item_info_s *to_info = NULL;

	retv_if(!scroller, NULL);
	retv_if(!append_item, NULL);
	retv_if(!from_item, NULL);
	retv_if(!to_item, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, NULL);

	/* 1. Remove the nodes before from_info->page */
	from_info = evas_object_data_get(from_item, DATA_KEY_ITEM_INFO);
	retv_if(!from_info, NULL);

	EINA_LIST_FOREACH(list, l, page) {
		if (page != from_info->page) {
			list = eina_list_remove_list(list, l);
		} else break;
	}

	/* 2. Remove the nodes after to_info->page */
	list = eina_list_reverse(list);
	retv_if(!list, NULL);

	to_info = evas_object_data_get(to_item, DATA_KEY_ITEM_INFO);
	retv_if(!to_info, NULL);

	EINA_LIST_FOREACH(list, l, page) {
		if (page != to_info->page) {
			list = eina_list_remove_list(list, l);
		} else break;
	}

	tmp_next = append_item;
	/* 3. Move items */
	EINA_LIST_FOREACH(list, l, page) {
		Evas_Object *tmp_from = NULL;
		Evas_Object *tmp_to = NULL;

		continue_if(!page);

		if (apps_page_has_item(page, from_item)) tmp_from = from_item;
		else tmp_from = NULL;

		if (apps_page_has_item(page, to_item)) tmp_to = to_item;
		else tmp_to = NULL;

		tmp_next = apps_page_move_item_prev(page, tmp_from, tmp_to, tmp_next);
		tmp_page = page;
	}
	if (tmp_next) apps_page_pack_item(tmp_page, tmp_next);

	eina_list_free(list);

	return APPS_ERROR_NONE;
}



HAPI Evas_Object *apps_scroller_move_item_next(Evas_Object *scroller, Evas_Object *from_item, Evas_Object *to_item, Evas_Object *insert_item)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	Eina_List *l;
	Evas_Object *page = NULL;
	Evas_Object *tmp_page = NULL;
	Evas_Object *tmp_next;
	item_info_s *from_info = NULL;
	item_info_s *to_info = NULL;

	retv_if(!scroller, NULL);
	retv_if(!insert_item, NULL);
	retv_if(!from_item, NULL);
	retv_if(!to_item, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, NULL);

	from_info = evas_object_data_get(from_item, DATA_KEY_ITEM_INFO);
	retv_if(!from_info, NULL);

	/* 1. Remove the nodes before from_info->page */
	from_info = evas_object_data_get(from_item, DATA_KEY_ITEM_INFO);
	retv_if(!from_info, NULL);

	EINA_LIST_FOREACH(list, l, page) {
		if (page != from_info->page) {
			list = eina_list_remove_list(list, l);
		} else break;
	}

	/* 2. Remove the nodes after to_info->page */
	list = eina_list_reverse(list);
	retv_if(!list, NULL);

	to_info = evas_object_data_get(to_item, DATA_KEY_ITEM_INFO);
	retv_if(!to_info, NULL);

	EINA_LIST_FOREACH(list, l, page) {
		if (page != to_info->page) {
			list = eina_list_remove_list(list, l);
		} else break;
	}

	list = eina_list_reverse(list);
	retv_if(!list, NULL);

	tmp_next = insert_item;
	/* 3. Move items */
	EINA_LIST_FOREACH(list, l, page) {
		Evas_Object *tmp_from = NULL;
		Evas_Object *tmp_to = NULL;

		continue_if(!page);

		if (apps_page_has_item(page, from_item)) tmp_from = from_item;
		else tmp_from = NULL;

		if (apps_page_has_item(page, to_item)) tmp_to = to_item;
		else tmp_to = NULL;

		tmp_next = apps_page_move_item_next(page, tmp_from, tmp_to, tmp_next);
		tmp_page = page;
	}
	if (tmp_next) apps_page_pack_item(tmp_page, tmp_next);

	eina_list_free(list);

	return APPS_ERROR_NONE;
}



HAPI int apps_scroller_seek_item_position(Evas_Object *scroller, Evas_Object *item)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	Evas_Object *page = NULL;
	int position = 0;

	retv_if(!scroller, -1);
	retv_if(!item, -1);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, -1);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, -1);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		int idx;
		continue_if(!page);
		idx = apps_page_seek_item_position(page, item);
		if (idx < 0) {
			position += APPS_PER_PAGE;
		} else if (idx >= 0 && idx < APPS_PER_PAGE) {
			position += idx;
			break;
		} else {
			_APPS_E("Is this a valid value? idx [%d]", idx);
		}
	}

	eina_list_free(list);
	return position;
}



HAPI void apps_scroller_bring_in_region_by_vector(Evas_Object *scroller, int vector)
{
	_APPS_D("Apps scroller bring in region by vector: %d", vector);
	int position = 0;
	int count = 0;
	int max = 0;

	ELM_SCROLLER_REGION_GET(scroller, &position);
	vector = vector * SCROLL_DISTANCE;
	position += vector;

	count = apps_scroller_count_page(scroller);
	max = (count - PAGE_IN_VIEW) * SCROLL_DISTANCE + SCROLL_PAD;

	if (position < 0) position = 1;
	else if (position > max) position = max - 1;

	ELM_SCROLLER_REGION_BRING_IN(scroller, position);
}


static item_info_s *_find_item_from_list(Eina_List *list, const char *appid)
{
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;
	item_info_s *item_info = NULL;
	EINA_LIST_FOREACH_SAFE(list, l, l_next, item_info) {
		if(!strcmp(item_info->appid, appid)) {
			return item_info;
		}
	}
	return NULL;
}


/* item_info_list must have all the items */
HAPI apps_error_e apps_scroller_read_list(Evas_Object *scroller, Eina_List *item_info_list)
{
	scroller_info_s *scroller_info = NULL;
	item_info_s *item_info, *cur_item_info;
	Eina_List *cur_list = NULL;
	Eina_List *cur_item_list = NULL;

	Evas_Object *page = NULL;
	Evas_Object *cur_item = NULL;

	int i = 0;

	retv_if(!scroller, APPS_ERROR_INVALID_PARAMETER);
	retv_if(!item_info_list, APPS_ERROR_INVALID_PARAMETER);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, APPS_ERROR_FAIL);

	cur_list = elm_box_children_get(scroller_info->box);
	retv_if(!cur_list, APPS_ERROR_FAIL);

	/* Unpack all the items those don't match with item_info_list */
	EINA_LIST_FREE(cur_list, page) {
		continue_if(!page);

		for (i = 0; i < APPS_PER_PAGE; i++) {

			cur_item = apps_page_get_item_at(page, i);
			if (!cur_item) continue;

			cur_item_info = evas_object_data_get(cur_item, DATA_KEY_ITEM_INFO);
			retv_if(!cur_item_info, APPS_ERROR_FAIL);
			retv_if(!cur_item_info->appid, APPS_ERROR_FAIL);

			apps_scroller_remove_item(scroller, cur_item);
			_APPS_D("remove item[%s]", cur_item_info->appid);
			item_destroy(cur_item);
			cur_item_list = eina_list_append(cur_item_list, cur_item_info);
		}
		apps_page_destroy(page);
	}

	//Append apps in item_info_list
	EINA_LIST_FREE(item_info_list, item_info) {
		continue_if(!item_info);
		_APPS_D("Pack an item[%s] into [%d]", item_info->appid, item_info->ordering);

		cur_item_info = _find_item_from_list(cur_item_list, item_info->appid);
		if(cur_item_info) {
			cur_item_list = eina_list_remove(cur_item_list, cur_item_info);

			item_info->item = item_create(scroller, cur_item_info);
		}
		else {
			item_info->item = item_create(scroller, item_info);
		}

		apps_scroller_append_item(scroller, item_info->item);
	}

	//if cur_item_list is not NULL. append items
	EINA_LIST_FREE(cur_item_list, cur_item_info) {
		continue_if(!cur_item_info);
		_APPS_D("Pack an item[%s] into [%d] tts [%d]", cur_item_info->appid, cur_item_info->ordering, cur_item_info->tts);

		cur_item_info->item = item_create(scroller, cur_item_info);
		apps_scroller_append_item(scroller, cur_item_info->item);
	}

	return APPS_ERROR_NONE;
}


HAPI void apps_scroller_write_list(Evas_Object *scroller)
{
	Evas_Object *page = NULL;
	Evas_Object *item = NULL;
	scroller_info_s *scroller_info = NULL;
	item_info_s *item_info = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;

	int i = 0;
	int ordering = 0;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);

		for (i = 0; i < APPS_PER_PAGE; i++) {
			item = apps_page_get_item_at(page, i);
			if (!item) continue;

			item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
			continue_if(!item_info);

			item_info->ordering = ordering;
			_APPS_SD("[%s]'s ordering : %d", item_info->appid, item_info->ordering);
			ordering++;
		}
	}

	eina_list_free(list);
}



HAPI Evas_Object *apps_scroller_get_item_by_pkgid(Evas_Object *scroller, const char *pkgid)
{
	Evas_Object *page = NULL;
	Evas_Object *item = NULL;
	scroller_info_s *scroller_info = NULL;
	item_info_s *item_info = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	int i = 0;

	retv_if(!scroller, NULL);
	retv_if(!pkgid, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, NULL);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);

		for (i = 0; i < APPS_PER_PAGE; i++) {
			item = apps_page_get_item_at(page, i);
			if (!item) continue;

			item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
			continue_if(!item_info);
			continue_if(!item_info->pkgid);

			if (strcmp(item_info->pkgid, pkgid)) continue;
			_APPS_SD("Get [%s]'s item object", item_info->pkgid);
			return item;
		}
	}

	eina_list_free(list);
	return NULL;
}



HAPI Evas_Object *apps_scroller_get_item_by_appid(Evas_Object *scroller, const char *appid)
{
	Evas_Object *page = NULL;
	Evas_Object *item = NULL;
	scroller_info_s *scroller_info = NULL;
	item_info_s *item_info = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	int i = 0;

	retv_if(!scroller, NULL);
	retv_if(!appid, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, NULL);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);

		for (i = 0; i < APPS_PER_PAGE; i++) {
			item = apps_page_get_item_at(page, i);
			if (!item) continue;

			item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
			continue_if(!item_info);
			continue_if(!item_info->appid);

			if (strcmp(item_info->appid, appid)) continue;
			_APPS_SD("Get [%s]'s item object", item_info->appid);
			return item;
		}
	}

	eina_list_free(list);
	return NULL;
}



HAPI void apps_scroller_edit(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	Evas_Object *page = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	elm_object_signal_emit(scroller_info->layout, "show,zoom", "scroller");

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);
		apps_page_edit(page);
	}
	eina_list_free(list);
}



HAPI void apps_scroller_unedit(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	Evas_Object *page = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	elm_object_signal_emit(scroller_info->layout, "reset,zoom", "scroller");

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);
		apps_page_unedit(page);
	}
	eina_list_free(list);
}



// End of this file
