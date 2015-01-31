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

#include <dlog.h>
#include <bundle.h>
#include <efl_assist.h>
#include <unicode/unum.h>
#include <unicode/ustring.h>
#include <vconf.h>

#include "util.h"
#include "add-viewer.h"
#include "conf.h"
#include "db.h"
#include "wms.h"
#include "page.h"
#include "page_info.h"
#include "edit_info.h"
#include "effect.h"
#include "key.h"
#include "layout_info.h"
#include "layout.h"
#include "dbox.h"
#include "dynamicbox_evas.h"
#include "log.h"
#include "main.h"
#include "scroller_info.h"
#include "scroller.h"
#include "edit.h"
#include "gesture.h"
#include "index.h"
#include "noti_broker.h"

#define PRIVATE_DATA_KEY_EDIT_DISABLE_REORDERING "p_e_l"
#define PRIVATE_DATA_KEY_EDIT_FOCUS_OBJECT "p_fo"
#define PRIVATE_DATA_KEY_EDIT_UNFOCUSABLE "p_uf"
#define PRIVATE_DATA_KEY_PAGE_LONGPRESS_TIMER "p_p_lp_t"
#define PRIVATE_DATA_KEY_PAGE_ANIM_FOR_DEL "p_it_ani_d"
#define PRIVATE_DATA_KEY_PAGE_ANIM_FOR_MOVING "p_it_ani_mv"
#define PRIVATE_DATA_KEY_PAGE_PRESSED "p_i_p"
#define PRIVATE_DATA_KEY_PAGE_DOWN_X "p_i_dx"
#define PRIVATE_DATA_KEY_PAGE_DOWN_Y "p_i_dy"
#define PRIVATE_DATA_KEY_PAGE_X_FOR_MOVING "p_i_x_mv"
#define PRIVATE_DATA_KEY_PAGE_Y_FOR_MOVING "p_i_y_mv"
#define PRIVATE_DATA_KEY_PAGE_X_FOR_ANIM "p_i_x"
#define PRIVATE_DATA_KEY_PAGE_Y_FOR_ANIM "p_i_y"
#define PRIVATE_DATA_KEY_PAGE_W "p_i_w"
#define PRIVATE_DATA_KEY_PAGE_H "p_i_h"
#define PRIVATE_DATA_KEY_PAGE_INNER "p_pg_in"
#define PRIVATE_DATA_KEY_PROXY_BG "p_pr_bg"
#define PRIVATE_DATA_KEY_SCROLLER_ANIM_FOR_MOVING "p_sc_an_mv"
#define PRIVATE_DATA_KEY_SCROLLER_PAGE_FOR_MOVING "p_sc_pg_mv"
#define PRIVATE_DATA_KEY_SCROLLER_RECT_FOR_MOVING "p_sc_r_mv"
#define PRIVATE_DATA_KEY_SCROLLER_PRESS_PAGE "p_sc_p"
#define PRIVATE_DATA_KEY_SCROLLER_X "p_sc_x"
#define PRIVATE_DATA_KEY_EDIT_SYNC_IS_DONE "pdke_s_d"
#define PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED "pdke_il"
#define PRIVATE_DATA_KEY_EDIT_ITEM_IS_MOVED "pdke_im"
#define PRIVATE_DATA_KEY_DIVIDE_FACTOR "pdk_df"
#define PRIVATE_DATA_KEY_ANIM_FOR_MINIFY "p_a_mi"
#define PRIVATE_DATA_KEY_ANIM_FOR_ENLARGE "p_a_en"
#define PRIVATE_DATA_KEY_EDIT_DO_NOT_SUPPORT_ENLARGE_EFFECT "pdk_dnsee"

#define MOVE_THRESHOLD 5
#define SLIPPED_LENGTH 40

static void _destroy_proxy_bg(Evas_Object *clip_bg);
static Evas_Object *_create_proxy_bg(Evas_Object *item);



static char *_access_tab_to_move_cb(void *data, Evas_Object *obj)
{
	char *tmp;

	tmp = strdup(_("IDS_COM_BODY_TAP_AND_HOLD_A_WIDGET_TO_MOVE_IT_ABB"));
	retv_if(!tmp, NULL);

	return tmp;
}



static char *_access_plus_button_name_cb(void *data, Evas_Object *obj)
{
	char *tmp;

	tmp = strdup(_("IDS_HS_BODY_ADD_WIDGET"));
	retv_if(!tmp, NULL);

	return tmp;
}



/* Caution : Do not create & destroy an item */
static void _scroller_read_page_list(Evas_Object *scroller, Eina_List *page_info_list)
{
	Evas_Object *before_page = NULL;
	const Eina_List *l, *ln;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;

	ret_if(!scroller);
	ret_if(!page_info_list);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	before_page = scroller_info->center;

	EINA_LIST_FOREACH_SAFE(page_info_list, l, ln, page_info) {
		continue_if(!page_info);
		continue_if(!page_info->page);

		elm_box_unpack(scroller_info->box, page_info->page);
		elm_box_pack_after(scroller_info->box, page_info->page, before_page);
		before_page = page_info->page;
	}
}



static Eina_List *_scroller_write_page_list(Evas_Object *edit_scroller)
{
	Evas_Object *page = NULL;
	Evas_Object *tmp = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	page_info_s *page_info = NULL;
	page_info_s *dup_page_info = NULL;
	Eina_List *list = NULL;
	Eina_List *page_info_list = NULL;
	const Eina_List *l, *ln;

	retv_if(!edit_scroller, NULL);

	edit_scroller_info = evas_object_data_get(edit_scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!edit_scroller_info, NULL);

	list = elm_box_children_get(edit_scroller_info->box);
	retv_if(!list, NULL);

	EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
		continue_if(!page);

		page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		if (!page_info) continue;
		if (!page_info->id) continue;
		if (PAGE_DIRECTION_RIGHT != page_info->direction) continue;

		dup_page_info = page_info_dup(page_info);
		continue_if(!dup_page_info);

		tmp = evas_object_data_get(dup_page_info->page, DATA_KEY_REAL_PAGE);
		if (tmp) dup_page_info->page = tmp;
		page_info_list = eina_list_append(page_info_list, dup_page_info);
	}
	eina_list_free(list);

	return page_info_list;
}



static Evas_Object *_content_set_page_inner(Evas_Object *proxy_page)
{
	Evas_Object *page_inner = NULL;
	page_info_s *page_info = NULL;

	page_inner = evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_INNER);
	retv_if(!page_inner, NULL);

	elm_object_part_content_set(proxy_page, "inner", page_inner);

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	scroller_unfreeze(page_info->scroller);

	return proxy_page;
}



static Evas_Object *_content_unset_page_inner(Evas_Object *proxy_page)
{
	Evas_Object *page_inner = NULL;
	page_info_s *page_info = NULL;

	page_inner = elm_object_part_content_unset(proxy_page, "inner");
	retv_if(!page_inner, NULL);
	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_INNER, page_inner);

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	elm_object_signal_emit(page_inner, "on,zoom", "inner");

	scroller_freeze(page_info->scroller);

	return proxy_page;
}



/* This function SHOULD be called related to edit_destroy_layout */
static void _sync_from_edit_to_normal(void *layout)
{
	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;
	Eina_List *page_info_list = NULL;

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);

	page_info_list = _scroller_write_page_list(edit_info->scroller);
	if (page_info_list) {
		db_read_list(page_info_list);
		_scroller_read_page_list(layout_info->scroller, page_info_list);
		page_info_list_destroy(page_info_list);
	} else {
		db_remove_all_item();
	}

	evas_object_data_set(layout, PRIVATE_DATA_KEY_EDIT_SYNC_IS_DONE, (void *) 1);
}



static void _clicked_noti_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *proxy_page = data;
	Evas_Object *effect_page = NULL;
	Evas_Object *page = NULL;
	Evas_Object *scroller = NULL;
	Evas_Object *layout = NULL;

	layout_info_s *layout_info = NULL;
	page_info_s *page_info = NULL;

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	if (evas_object_data_del(page_info->scroller, PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED)) return;

	layout = page_info->layout;
	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	scroller = layout_info->scroller;

	effect_page = edit_create_enlarge_effect_page(proxy_page);
	if (effect_page) edit_enlarge_effect_page(effect_page);

	page = evas_object_data_get(proxy_page, DATA_KEY_REAL_PAGE);
	ret_if(!page);
	scroller_region_show_center_of(scroller, page, SCROLLER_FREEZE_OFF, NULL, NULL, NULL, NULL);
}



static void _clicked_center_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *proxy_page = data;
	Evas_Object *effect_page = NULL;
	Evas_Object *page = NULL;
	Evas_Object *scroller = NULL;
	Evas_Object *layout = NULL;

	layout_info_s *layout_info = NULL;
	page_info_s *page_info = NULL;

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	if (evas_object_data_get(page_info->scroller, PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED)) return;

	layout = page_info->layout;
	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	scroller = layout_info->scroller;

	effect_page = edit_create_enlarge_effect_page(proxy_page);
	if (effect_page) edit_enlarge_effect_page(effect_page);

	page = evas_object_data_get(proxy_page, DATA_KEY_REAL_PAGE);
	ret_if(!page);
	scroller_region_show_center_of(scroller, page, SCROLLER_FREEZE_OFF, NULL, NULL, NULL, NULL);
}



static void _clicked_dbox_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *proxy_page = data;
	Evas_Object *effect_page = NULL;
	Evas_Object *page = NULL;
	Evas_Object *scroller = NULL;
	Evas_Object *layout = NULL;

	layout_info_s *layout_info = NULL;
	page_info_s *page_info = NULL;

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	if (evas_object_data_get(page_info->scroller, PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED)) return;

	layout = page_info->layout;
	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	scroller = layout_info->scroller;

	effect_page = edit_create_enlarge_effect_page(proxy_page);
	if (effect_page) edit_enlarge_effect_page(effect_page);

	page = evas_object_data_get(proxy_page, DATA_KEY_REAL_PAGE);
	ret_if(!page);
	scroller_region_show_center_of(scroller, page, SCROLLER_FREEZE_OFF, _sync_from_edit_to_normal, layout, NULL, NULL);
}



#define ANIM_RATE 5
#define ANIM_RATE_SPARE ANIM_RATE - 1
static Eina_Bool _anim_move_page_to_empty_position(void *data)
{
	Evas_Object *proxy_page = data;
	page_info_s *page_info = NULL;
	Evas_Object *scroller = NULL;
	Evas_Object *page_inner = evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_INNER);
	int cur_x, cur_y, end_x, end_y;
	int vec_x, vec_y;

	goto_if(!data, ERROR);
	if(!page_inner) {
		_D("page_inner was not unset");
		goto ERROR;
	}

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	goto_if(!page_info, ERROR);
	scroller = page_info->scroller;

	evas_object_geometry_get(page_inner, &cur_x, &cur_y, NULL, NULL);
	end_x = (int)evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_X_FOR_ANIM);
	end_y = (int)evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_Y_FOR_ANIM);

	if (cur_y == end_y && cur_x == end_x) {
		goto_if (NULL == _content_set_page_inner(proxy_page), ERROR);
		/* unfreeze the scroller after setting the content */
		if (scroller) scroller_unfreeze(scroller);

		evas_object_data_del(scroller, PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED);
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_X_FOR_ANIM);
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_Y_FOR_ANIM);
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_MOVING);
		edit_change_focus(scroller, proxy_page);

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

	evas_object_move(page_inner, cur_x, cur_y);

	return ECORE_CALLBACK_RENEW;

ERROR:
	if (proxy_page) {
		if (scroller) scroller_unfreeze(scroller);
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_MOVING);
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_X_FOR_ANIM);
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_Y_FOR_ANIM);

	}
	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _longpress_timer_cb(void *data)
{
	Evas_Object *proxy_page = NULL;
	page_info_s *proxy_page_info = NULL;
	Ecore_Timer *timer;

	proxy_page = data;
	retv_if(!proxy_page, ECORE_CALLBACK_CANCEL);

	proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	retv_if(!proxy_page_info, ECORE_CALLBACK_CANCEL);

	timer = evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_LONGPRESS_TIMER);
	if (timer) ecore_timer_del(timer);
	else return ECORE_CALLBACK_CANCEL;

	evas_object_data_set(proxy_page_info->scroller, PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED, (void *) 1);

	if (evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_EDIT_DISABLE_REORDERING)) {
		_D("long-press is not supported on this page");
		scroller_freeze(proxy_page_info->scroller);
		util_create_toast_popup(proxy_page_info->scroller, _("IDS_HS_TPOP_CANNOT_REORDER_NOTIFICATION_BOARD_ITEMS"));
		return ECORE_CALLBACK_CANCEL;
	}

	_D("longpress start for proxy_page(%p)", proxy_page);

	page_unfocus(proxy_page);
	scroller_disable_focus_on_scroll(proxy_page_info->scroller);
	scroller_disable_index_on_scroll(proxy_page_info->scroller);

	retv_if(!evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_PRESSED), ECORE_CALLBACK_CANCEL);
	retv_if(!_content_unset_page_inner(proxy_page), ECORE_CALLBACK_CANCEL);

	elm_object_signal_emit(proxy_page_info->layout, "show", "checker");
	scroller_bring_in_center_of(proxy_page_info->scroller, proxy_page, SCROLLER_FREEZE_ON, NULL, NULL, NULL, NULL);
	effect_play_vibration();

	return ECORE_CALLBACK_CANCEL;

}



static void _down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ei = event_info;
	Evas_Object *proxy_page = data;
	Evas_Object *page_current = NULL;
	Evas_Object *page_inner = NULL;
	Ecore_Timer *timer = NULL;
	Ecore_Animator *anim = NULL;
	page_info_s *page_info = NULL;

	int x = ei->output.x;
	int y = ei->output.y;
	int inner_x, inner_y, inner_w, inner_h;
	int sx;

	_D("edit : Down (%d, %d)", x, y);

	ret_if(!proxy_page);

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	if (scroller_is_scrolling(page_info->scroller)) return;

	anim = evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_MOVING);
	if (anim) return;

	page_current = scroller_get_focused_page(page_info->scroller);
	ret_if(!page_current);
	if (proxy_page != page_current) return;

	page_inner = elm_object_part_content_get(proxy_page, "inner");
	if (!page_inner) {
		page_inner = evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_INNER);
		ret_if(!page_inner);
		elm_object_part_content_set(proxy_page, "inner", page_inner);
	}

	evas_object_data_set(page_info->scroller, PRIVATE_DATA_KEY_SCROLLER_PRESS_PAGE, proxy_page);
	evas_object_data_set(page_info->scroller, DATA_KEY_EVENT_UPPER_PAGE, proxy_page);

	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_PRESSED, (void *) 1);
	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_DOWN_X, (void *) x);
	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_DOWN_Y, (void *) y);

	evas_object_geometry_get(page_inner, &inner_x, &inner_y, &inner_w, &inner_h);

	/* this data key is used the destination in animation of item's set.
	If the destination of animation can be changed, you set the changed value  in _up_cb or other function */
	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_X_FOR_ANIM, (void *) inner_x);
	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_Y_FOR_ANIM, (void *) inner_y);

	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_X_FOR_MOVING, (void *) inner_x);
	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_Y_FOR_MOVING, (void *) inner_y);

	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_W, (void *) inner_w);
	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_H, (void *) inner_h);

	elm_scroller_region_get(page_info->scroller, &sx, NULL, NULL, NULL);
	sx += inner_x;
	evas_object_data_set(page_info->scroller, PRIVATE_DATA_KEY_SCROLLER_X, (void *) sx);
	evas_object_data_del(page_info->scroller, PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED);

	timer = ecore_timer_add(LONGPRESS_TIME, _longpress_timer_cb, proxy_page);
	if (timer) evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_LONGPRESS_TIMER, timer);
	else _E("Cannot add a timer");
}



static void _call_object(Evas_Object *proxy_page, int cur_x, int cur_y)
{
	Evas_Object *parent = NULL;
	Evas_Object *tmp = NULL;
	Eina_List *obj_list = NULL;
	page_info_s *page_info = NULL;

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->scroller);

	obj_list = evas_tree_objects_at_xy_get(main_get_info()->e, NULL, cur_x, cur_y);
	ret_if(!obj_list);

	EINA_LIST_FREE(obj_list, tmp) {
		parent = tmp;
		while (parent) {
			if (evas_object_data_get(parent, DATA_KEY_EVENT_UPPER_IS_ON)) {
				Evas_Object *old_obj = evas_object_data_get(page_info->scroller, DATA_KEY_EVENT_UPPER_PAGE);
				if (old_obj != parent) {
					_D("Call upper_end(%p) & upper_start(%p)", old_obj, parent);
					evas_object_smart_callback_call(old_obj, "upper_end", NULL);
					evas_object_smart_callback_call(parent, "upper_start", NULL);
					evas_object_data_set(page_info->scroller, DATA_KEY_EVENT_UPPER_PAGE, parent);
				} else {
					evas_object_smart_callback_call(parent, "upper", proxy_page);
				}

				eina_list_free(obj_list);
				return;
			}

			parent = evas_object_smart_parent_get(parent);
			if (parent == page_info->scroller) {
				evas_object_data_set(page_info->scroller, DATA_KEY_EVENT_UPPER_PAGE, parent);
				break;
			}
		}
	}
}



static void _move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Move *ei = event_info;
	Evas_Object *proxy_page = data;
	Evas_Object *page_inner = NULL;
	Ecore_Timer *timer = NULL;
	page_info_s *page_info = NULL;
	Ecore_Animator *anim = NULL;

	int down_x, down_y, inner_x, inner_y;
	int cur_x, cur_y, vec_x, vec_y;

	ret_if(!proxy_page);
	if (!evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_PRESSED)) {
		return;
	}

	anim = evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_MOVING);
	if (anim) return;

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	cur_x = ei->cur.output.x;
	cur_y = ei->cur.output.y;

	down_x = (int) evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_DOWN_X);
	down_y = (int) evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_DOWN_Y);

	inner_x = (int) evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_X_FOR_MOVING);
	inner_y = (int) evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_Y_FOR_MOVING);

	vec_x = cur_x - down_x;
	vec_y = cur_y - down_y;

	inner_x += vec_x;
	inner_y += vec_y;

	timer = evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_LONGPRESS_TIMER);
	if (timer && (abs(vec_x) >= LONGPRESS_THRESHOLD || abs(vec_y) >= LONGPRESS_THRESHOLD)) {
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_LONGPRESS_TIMER);
		ecore_timer_del(timer);
		return;
	}

	page_inner = evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_INNER);
	if (!page_inner) return;

	evas_object_move(page_inner, inner_x, inner_y);

	_call_object(proxy_page, cur_x, cur_y);
}



static void _up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *proxy_page = data;
	Evas_Object *old_obj;
	Evas_Object *page_current;
	Evas_Event_Mouse_Up *ei = event_info;
	Ecore_Timer *timer = NULL;
	page_info_s *page_info = NULL;
	Ecore_Animator *anim = NULL;

	int x = ei->output.x;
	int y = ei->output.y;

	_D("edit : Up (%p) (%d, %d)", obj, x, y);

	ret_if(!proxy_page);

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	page_current = scroller_get_focused_page(page_info->scroller);
	ret_if(!page_current);
	if (page_current != proxy_page) return;

	scroller_enable_focus_on_scroll(page_info->scroller);
	scroller_enable_index_on_scroll(page_info->scroller);

	if (evas_object_data_del(page_info->scroller, PRIVATE_DATA_KEY_EDIT_ITEM_IS_MOVED)
		&& main_get_info()->is_tts)
	{
		elm_access_say(_("IDS_TTS_BODY_ITEM_MOVED"));
	}

	elm_object_signal_emit(page_info->layout, "hide", "checker");

	evas_object_data_del(page_info->scroller, PRIVATE_DATA_KEY_SCROLLER_PRESS_PAGE);
	evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_PRESSED);
	evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_DOWN_X);
	evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_DOWN_Y);
	evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_X_FOR_MOVING);
	evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_Y_FOR_MOVING);

	evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_W);
	evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_H);
	old_obj = evas_object_data_del(page_info->scroller, DATA_KEY_EVENT_UPPER_PAGE);
	if (old_obj) {
		_D("Call upper_end & upper_start");
		evas_object_smart_callback_call(old_obj, "upper_end", NULL);
	}

	timer = evas_object_data_del(page_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);
	if (timer) ecore_timer_del(timer);

	timer = evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_LONGPRESS_TIMER);
	if (timer) {
		ecore_timer_del(timer);
		timer = NULL;
		return;
	}

	anim = evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_MOVING);
	if (anim) {
		ecore_animator_del(anim);
		anim = NULL;
	}

	if (evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_EDIT_DISABLE_REORDERING)) {
		_D("long-press is not supported on this page");
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_X_FOR_ANIM);
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_Y_FOR_ANIM);
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_MOVING);
		scroller_unfreeze(page_info->scroller);
		edit_change_focus(page_info->scroller, page_current);
		return;
	}

	anim = ecore_animator_add(_anim_move_page_to_empty_position, proxy_page);
	ret_if(NULL == anim);
	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_MOVING, anim);
	if (evas_object_data_get(page_info->scroller, PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED)) {
		elm_object_signal_emit(page_info->page_inner, "reset,zoom", "inner");
	}
}



static key_cb_ret_e _add_viewer_back_key_cb(void *data)
{
	_W("");
	Evas_Object *layout = data;

	retv_if(!layout, KEY_CB_RET_CONTINUE);
	edit_destroy_add_viewer(layout);

	return KEY_CB_RET_STOP;
}



#if 0 /* We use this when back key isn't used anymore*/
static Eina_Bool _delay_destroy_add_viewer_cb(void *data)
{
	Evas_Object *layout = data;

	edit_destroy_add_viewer(layout);
	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _delay_destroy_edit_cb(void *data)
{
	Evas_Object *layout = data;

	edit_destroy_layout(layout);
	return ECORE_CALLBACK_CANCEL;
}



static void _add_viewer_bezel_down_cb(void *data)
{
	Evas_Object *layout = data;
	Ecore_Timer *timer = NULL;

	ret_if(!layout);

	timer = evas_object_data_del(layout, PRIVATE_DATA_KEY_TIMER);
	if (timer) {
		ecore_timer_del(timer);
	}
	timer = ecore_timer_add(0.1f, _delay_destroy_add_viewer_cb, layout);
	evas_object_data_set(layout, PRIVATE_DATA_KEY_TIMER, timer);
}



static void _edit_bezel_down_cb(void *data)
{
	Evas_Object *layout = data;
	Ecore_Timer *timer = NULL;

	ret_if(!layout);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	if (!layout_info->edit) {
		_D("Layout is not edited");
		return;
	}
	timer = evas_object_data_del(layout, PRIVATE_DATA_KEY_TIMER);
	if (timer) {
		ecore_timer_del(timer);
	}
	timer = ecore_timer_add(0.1f, _delay_destroy_layout_cb, layout);
	evas_object_data_set(layout, PRIVATE_DATA_KEY_TIMER, timer);
}
#endif



static key_cb_ret_e _add_viewer_home_key_cb(void *data)
{
	Evas_Object *layout = data;
	layout_info_s *layout_info = NULL;

	retv_if(!layout, KEY_CB_RET_CONTINUE);
	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, KEY_CB_RET_CONTINUE);

	edit_destroy_add_viewer(layout);
	if (layout_info->edit) edit_destroy_layout(layout);

	return KEY_CB_RET_STOP;
}



static char *_access_remove_button_name_cb(void *data, Evas_Object *obj)
{
    char *tmp;

    tmp = strdup(_("IDS_COM_BUTTON_REMOVE_ABB"));
    retv_if(!tmp, NULL);
    return tmp;
}



#define PROXY_ITEM_EDJ EDJEDIR"/edit.edj"
#define PROXY_ITEM_GROUP "proxy_item"
Evas_Object *_create_proxy_item(Evas_Object *edit_scroller, Evas_Object *real_page)
{
	Evas_Object *proxy_item = NULL;
	Evas_Object *proxy_image = NULL;
	page_info_s *page_info = NULL;
	Eina_Bool ret;

	retv_if(!edit_scroller, NULL);
	retv_if(!real_page, NULL);

	page_info = evas_object_data_get(real_page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	/* Proxy Item */
	proxy_item = elm_layout_add(edit_scroller);
	retv_if(!proxy_item, NULL);

	ret = elm_layout_file_set(proxy_item, PROXY_ITEM_EDJ, PROXY_ITEM_GROUP);
	if (EINA_FALSE == ret) {
		_E("cannot set the file into the layout");
		evas_object_del(proxy_item);
		return NULL;
	}
	evas_object_repeat_events_set(proxy_item, EINA_TRUE);

	/* Proxy Image */
	proxy_image = evas_object_image_filled_add(main_get_info()->e);
	if (!proxy_item) {
		_E("Cannot add an image");
		evas_object_del(proxy_image);
		return NULL;
	}

	ret = evas_object_image_source_set(proxy_image, page_info->item);
	if(EINA_FALSE == ret) _E("Cannot set the source into the proxy image");

	evas_object_image_source_visible_set(proxy_image, EINA_FALSE);
	evas_object_image_source_clip_set(proxy_image, EINA_FALSE);

	elm_object_part_content_set(proxy_item, "proxy_item", proxy_image);

	evas_object_repeat_events_set(proxy_image, EINA_TRUE);
	evas_object_size_hint_weight_set(proxy_image, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(proxy_image, ITEM_EDIT_WIDTH, ITEM_EDIT_HEIGHT);
	evas_object_resize(proxy_image, ITEM_EDIT_WIDTH, ITEM_EDIT_HEIGHT);
	evas_object_show(proxy_image);

	_D("Create a proxy item(%p)", proxy_item);

	return proxy_item;
}



static void _destroy_proxy_item(Evas_Object *proxy_item)
{
	Evas_Object *proxy_image = NULL;

	ret_if(!proxy_item);

	proxy_image = elm_object_part_content_unset(proxy_item, "proxy_item");
	if (proxy_image) {
		evas_object_image_source_visible_set(proxy_image, EINA_TRUE);
		evas_object_del(proxy_image);
	}

	_D("Destroy a proxy item(%p)", proxy_item);

	evas_object_del(proxy_item);
}



HAPI void edit_change_focus(Evas_Object *edit_scroller, Evas_Object *page_current)
{
	Evas_Object *page_focused = NULL;
	page_info_s *page_info = NULL;
	page_info_s *focused_page_info = NULL;
	int unfocusable = 0;

	ret_if(!edit_scroller);
	if(!page_current) {
		evas_object_data_set(edit_scroller, PRIVATE_DATA_KEY_EDIT_FOCUS_OBJECT, NULL);
		return;
	}

	page_focused = evas_object_data_get(edit_scroller, PRIVATE_DATA_KEY_EDIT_FOCUS_OBJECT);
	if (page_focused == page_current) return;

	page_info = evas_object_data_get(page_current, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->page_inner);
	ret_if(!page_info->item);

	/* Blocker has to be disabled even if this is unfocusable */
	elm_object_signal_emit(page_info->page_inner, "disable", "blocker");

	evas_object_data_set(edit_scroller, PRIVATE_DATA_KEY_EDIT_FOCUS_OBJECT, page_current);

	if(page_focused) {
		focused_page_info = evas_object_data_get(page_focused, DATA_KEY_PAGE_INFO);
		if (!focused_page_info) goto OUT;
		if (!focused_page_info->page_inner) goto OUT;
		if (!focused_page_info->item) goto OUT;

		unfocusable = (int) evas_object_data_get(page_focused, PRIVATE_DATA_KEY_EDIT_UNFOCUSABLE);
		if (!unfocusable) {
			elm_object_signal_emit(focused_page_info->page_inner, "deselect", "cover");
			elm_object_signal_emit(focused_page_info->page_inner, "deselect", "line");
			elm_object_signal_emit(focused_page_info->page_inner, "hide", "del");
		}
		elm_object_signal_emit(focused_page_info->page_inner, "enable", "blocker");
	}

OUT:
	/* Unfocusable page */
	unfocusable = (int) evas_object_data_get(page_current, PRIVATE_DATA_KEY_EDIT_UNFOCUSABLE);
	if (unfocusable) return;

	/* Focusable page */
	elm_object_signal_emit(page_info->page_inner, "select", "cover");
	elm_object_signal_emit(page_info->page_inner, "select", "line");
	if (page_info->removable) {
		elm_object_signal_emit(page_info->page_inner, "show", "del");
	}
}



static void _remove_dbox(Evas_Object *page)
{
	Evas_Object *proxy_page = NULL;
	Evas_Object *page_current = NULL;
	Evas_Object *focused_page = NULL;

	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	page_info_s *page_info = NULL;

	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	layout_info = evas_object_data_get(page_info->layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);

	edit_scroller_info = evas_object_data_get(edit_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!edit_scroller_info);

	/* Proxy DBox */
	proxy_page = evas_object_data_get(page, DATA_KEY_PROXY_PAGE);
	ret_if(!proxy_page);

	focused_page = evas_object_data_get(edit_info->scroller, PRIVATE_DATA_KEY_EDIT_FOCUS_OBJECT);
	if (proxy_page == focused_page) {
		evas_object_data_set(edit_info->scroller, PRIVATE_DATA_KEY_EDIT_FOCUS_OBJECT, NULL);
	}

	scroller_pop_page(edit_info->scroller, proxy_page);
	edit_destroy_proxy_page(proxy_page);
	edit_arrange_plus_page(layout_info->edit);

	/* Real DBox */
	scroller_pop_page(layout_info->scroller, page);
	/**
	 * @note
	 * Delete a box permanently only if a user deletes it.
	 */
	evas_object_dynamicbox_set_permanent_delete(page_info->item, 1);
	evas_object_del(page_info->item);
	page_destroy(page);
	page_arrange_plus_page(layout_info->scroller, 0);

	page_current = scroller_get_focused_page(edit_info->scroller);
	ret_if(!page_current);

	scroller_region_show_page(edit_info->scroller, page_current, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_ANIMATOR);
	edit_change_focus(edit_info->scroller, page_current);
}



static Eina_Bool _minify_dbox_anim_cb(void *data)
{
	Evas_Object *proxy_page = data;
	int w, h;

	retv_if(!proxy_page, ECORE_CALLBACK_CANCEL);

	evas_object_geometry_get(proxy_page, NULL, NULL, &w, &h);

	w -= SLIPPED_LENGTH;
	evas_object_size_hint_min_set(proxy_page, w, h);

	if (w <= 0) {
		Evas_Object *real_page = evas_object_data_get(proxy_page, DATA_KEY_REAL_PAGE);

		/* This has to be executed before _remove_dbox */
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_DEL);

		if (real_page) _remove_dbox(real_page);
		return ECORE_CALLBACK_CANCEL;
	}

	return ECORE_CALLBACK_RENEW;

}



static void _del_dbox_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *proxy_page = data;
	Evas_Object *page_inner = NULL;
	Ecore_Animator *anim = NULL;

	_D("Del is clicked");

	ret_if(!proxy_page);
	page_inner = elm_object_part_content_unset(proxy_page, "inner");
	ret_if(!page_inner);

	evas_object_hide(page_inner);

	anim = evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_DEL);
	if (anim) return;

	anim = ecore_animator_add(_minify_dbox_anim_cb, proxy_page);
	if (anim) evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_DEL, anim);
	else _E("Cannot add an animator");

	elm_access_say(_("IDS_TTS_BODY_ITEM_REMOVED"));
}



static Evas_Object *_add_dbox_in_normal(Evas_Object *layout, const char *id, const char *subid)
{
	Evas_Object *page = NULL;
	Evas_Object *item = NULL;
	Eina_List *page_info_list = NULL;
	layout_info_s *layout_info = NULL;
	scroller_info_s *scroller_info = NULL;

	retv_if(!layout, NULL);
	retv_if(!id, NULL);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, NULL);

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	/* Real DBox */
	item = dbox_create(layout_info->scroller, id, NULL, DYNAMICBOX_EVAS_DEFAULT_PERIOD);
	retv_if(!item, NULL);
	evas_object_dynamicbox_resume(item);
	evas_object_dynamicbox_disable_loading(item);
	evas_object_resize(item, scroller_info->page_width, scroller_info->page_height);
	evas_object_size_hint_min_set(item, scroller_info->page_width, scroller_info->page_height);
	evas_object_show(item);

	page = page_create(layout_info->scroller
							, item
							, id, NULL
							, scroller_info->page_width, scroller_info->page_height
							, PAGE_CHANGEABLE_BG_ON, PAGE_REMOVABLE_ON);
	goto_if(!page, ERROR);
	dbox_add_callback(item, page);
	page_set_effect(page, page_effect_none, page_effect_none);

	scroller_push_page(layout_info->scroller, page, SCROLLER_PUSH_TYPE_LAST);
	page_arrange_plus_page(layout_info->scroller, 1);

	page_info_list = scroller_write_list(layout_info->scroller);
	if (page_info_list) {
		db_read_list(page_info_list);
		scroller_read_list(layout_info->scroller, page_info_list);
		page_info_list_destroy(page_info_list);
	}

	scroller_region_show_page(layout_info->scroller, page, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_ANIMATOR);

	return page;

ERROR:
	dbox_destroy(item);
	return NULL;

}



static Evas_Object *add_dbox_in_edit(Evas_Object *layout, const char *id, const char *subid)
{
	Evas_Object *proxy_page = NULL;
	Evas_Object *page = NULL;
	Evas_Object *page_inner = NULL;
	Evas_Object *item = NULL;
	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;
	scroller_info_s *scroller_info = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	page_info_s *proxy_page_info = NULL;
	page_info_s *plus_page_info = NULL;

	retv_if(!layout, NULL);
	retv_if(!id, NULL);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, NULL);

	edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
	retv_if(!edit_info, NULL);

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	edit_scroller_info = evas_object_data_get(edit_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!edit_scroller_info, NULL);

	plus_page_info = evas_object_data_get(edit_scroller_info->plus_page, DATA_KEY_PAGE_INFO);
	retv_if(!plus_page_info, NULL);

	/* Real DBox */
	item = dbox_create(layout_info->scroller, id, NULL, DYNAMICBOX_EVAS_DEFAULT_PERIOD);
	goto_if(!item, ERROR);
	evas_object_dynamicbox_resume(item);
	evas_object_dynamicbox_disable_loading(item);
	evas_object_resize(item, scroller_info->page_width, scroller_info->page_height);
	evas_object_size_hint_min_set(item, scroller_info->page_width, scroller_info->page_height);
	evas_object_show(item);

	page = page_create(layout_info->scroller
							, item
							, id, NULL
							, scroller_info->page_width, scroller_info->page_height
							, PAGE_CHANGEABLE_BG_ON, PAGE_REMOVABLE_ON);
	goto_if(!page, ERROR);
	dbox_add_callback(item, page);
	page_set_effect(page, page_effect_none, page_effect_none);

	scroller_push_page_before(layout_info->scroller, page, scroller_info->plus_page);
	page_arrange_plus_page(layout_info->scroller, 1);

	page_inner = elm_object_part_content_unset(page, "inner");
	if (page_inner) evas_object_move(page_inner, 0, 0);
	else _E("Cannot get the page_inner");

	/* Proxy DBox */
	proxy_page = edit_create_proxy_page(edit_info->scroller, page, PAGE_CHANGEABLE_BG_ON);
	goto_if(!proxy_page, ERROR);

	proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	goto_if(!proxy_page_info, ERROR);
	goto_if(!edit_info->plus_page, ERROR);

	elm_object_signal_emit(proxy_page_info->page_inner, "select", "cover_clipper");

	scroller_push_page_before(edit_info->scroller, proxy_page, edit_info->plus_page);
	edit_arrange_plus_page(layout_info->edit);

	evas_object_smart_callback_add(proxy_page_info->focus, "clicked", _clicked_dbox_cb, proxy_page);
	evas_object_smart_callback_add(proxy_page_info->remove_focus, "clicked",  _del_dbox_cb, proxy_page);

	/* Update index */
	if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
		index_update(scroller_info->index[PAGE_DIRECTION_RIGHT], layout_info->scroller, INDEX_BRING_IN_NONE);
	}

	if (edit_scroller_info->index[PAGE_DIRECTION_RIGHT]) {
		index_update(edit_scroller_info->index[PAGE_DIRECTION_RIGHT], edit_info->scroller, INDEX_BRING_IN_NONE);
	}

	index_bring_in_page(edit_scroller_info->index[PAGE_DIRECTION_RIGHT], proxy_page);
	edit_change_focus(edit_info->scroller, proxy_page);
	scroller_region_show_page(edit_info->scroller, proxy_page, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_ANIMATOR);
	elm_object_signal_emit(plus_page_info->page_inner, "deselect", "line");
	elm_object_signal_emit(plus_page_info->page_inner, "deselect", "cover");
	elm_object_signal_emit(plus_page_info->page_inner, "enable", "blocker");

	return page;

ERROR:
	if (page) {
		_remove_dbox(page);
	}
	return NULL;
}



static Eina_Bool _fire_timer_cb(void *data)
{
	Evas_Object *real_page = data;
	retv_if(!real_page, ECORE_CALLBACK_CANCEL);
	noti_broker_event_fire_to_plugin(EVENT_SOURCE_EDITING, EVENT_TYPE_NOTI_DELETE, real_page);
	return ECORE_CALLBACK_CANCEL;
}



static void _remove_noti(Evas_Object *real_page)
{
	Evas_Object *edit_scroller = NULL;
	Evas_Object *proxy_page = NULL;
	Evas_Object *page_current = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	page_info_s *page_info = NULL;
	page_info_s *proxy_page_info = NULL;
	Ecore_Timer *fire_timer = NULL;

	ret_if(!real_page);

	page_info = evas_object_data_get(real_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	proxy_page = evas_object_data_get(real_page, DATA_KEY_PROXY_PAGE);
	ret_if(!proxy_page);

	proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	ret_if(!proxy_page_info);

	edit_scroller = proxy_page_info->scroller;
	ret_if(!edit_scroller);

	edit_scroller_info = evas_object_data_get(edit_scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!edit_scroller_info);

	/* Proxy */
	scroller_pop_page(edit_scroller, proxy_page);
	edit_destroy_proxy_page(proxy_page);

	page_current = scroller_get_focused_page(edit_scroller);
	if (!page_current) {
		_D("No focused page");
		return;
	}

	edit_change_focus(edit_scroller, page_current);

	/* Noti */
	fire_timer = ecore_timer_add(0.1f, _fire_timer_cb, real_page);
	if (!fire_timer) {
		_E("Cannot add a timer");
		noti_broker_event_fire_to_plugin(EVENT_SOURCE_EDITING, EVENT_TYPE_NOTI_DELETE, real_page);
	}
}



static Eina_Bool _minify_noti_anim_cb(void *data)
{
	Evas_Object *proxy_page = data;
	int w, h;

	retv_if(!proxy_page, ECORE_CALLBACK_CANCEL);

	evas_object_geometry_get(proxy_page, NULL, NULL, &w, &h);

	w -= SLIPPED_LENGTH;
	evas_object_size_hint_min_set(proxy_page, w, h);

	if (w <= 0) {
		Evas_Object *real_page = NULL;

		/* This has to be executed before _remove_noti */
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_DEL);

		real_page = evas_object_data_get(proxy_page, DATA_KEY_REAL_PAGE);
		if (real_page) _remove_noti(real_page);

		return ECORE_CALLBACK_CANCEL;
	}

	return ECORE_CALLBACK_RENEW;
}



static void _del_noti_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *proxy_page = data;
	Evas_Object *page_inner = NULL;
	Ecore_Animator *anim = NULL;

	_D("Del is clicked");

	ret_if(!proxy_page);

	page_inner = elm_object_part_content_unset(proxy_page, "inner");
	ret_if(!page_inner);
	evas_object_hide(page_inner);

	anim = evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_DEL);
	if (anim) return;

	anim = ecore_animator_add(_minify_noti_anim_cb, proxy_page);
	if (anim) evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_DEL, anim);
	else _E("Cannot add an animator");
}



#define SLIPPED_LENGTH_BEFORE 15
static Eina_Bool _anim_move_page_before_cb(void *data)
{
	Evas_Object *scroller = data;
	Evas_Object *page = NULL;
	Evas_Object *rect = NULL;
	scroller_info_s *scroller_info = NULL;
	int w, h;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);

	page = evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_PAGE_FOR_MOVING);
	evas_object_geometry_get(page, NULL, NULL, &w, &h);

	w += SLIPPED_LENGTH_BEFORE;
	evas_object_size_hint_min_set(page, w, h);
	if (w >= PAGE_EDIT_WIDTH) {
		evas_object_size_hint_min_set(page, PAGE_EDIT_WIDTH, PAGE_EDIT_HEIGHT);
		evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_PAGE_FOR_MOVING);
		evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_ANIM_FOR_MOVING);
		rect = evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_RECT_FOR_MOVING);

		if (rect) {
			elm_box_unpack(scroller_info->box, rect);
			evas_object_del(rect);
		} else _E("Cannot get the rect");

		if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
			index_update(scroller_info->index[PAGE_DIRECTION_RIGHT], scroller, INDEX_BRING_IN_NONE);
			index_bring_in_page(scroller_info->index[PAGE_DIRECTION_RIGHT], page);
		}
		return ECORE_CALLBACK_CANCEL;
	}
	return ECORE_CALLBACK_RENEW;
}



HAPI w_home_error_e edit_push_page_before(Evas_Object *scroller, Evas_Object *page, Evas_Object *before)
{
	Evas_Object *rect = NULL;
	scroller_info_s *scroller_info = NULL;
	Ecore_Animator *anim = NULL;

	retv_if(!scroller, W_HOME_ERROR_FAIL);
	retv_if(!page, W_HOME_ERROR_FAIL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);

	anim = evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_ANIM_FOR_MOVING);
	if (anim) return W_HOME_ERROR_FAIL;

	rect = evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_RECT_FOR_MOVING);
	if (rect) {
		elm_box_unpack(scroller_info->box, rect);
		evas_object_del(rect);
	}

	rect = evas_object_rectangle_add(main_get_info()->e);
	retv_if(!rect, W_HOME_ERROR_FAIL);
	evas_object_size_hint_min_set(rect, PAGE_EDIT_WIDTH, PAGE_EDIT_HEIGHT);
	evas_object_color_set(rect, 0, 0, 0, 0);
	evas_object_show(rect);

	elm_box_pack_after(scroller_info->box, rect, page);
	elm_box_unpack(scroller_info->box, page);
	if (before) elm_box_pack_before(scroller_info->box, page, before);
	else elm_box_pack_end(scroller_info->box, page);

	evas_object_size_hint_min_set(page, 1, PAGE_EDIT_HEIGHT);

	scroller_bring_in_page(scroller, page, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_ANIMATOR);

	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_PAGE_FOR_MOVING, page);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_RECT_FOR_MOVING, rect);

	anim = ecore_animator_add(_anim_move_page_before_cb, scroller);
	retv_if(!anim, W_HOME_ERROR_FAIL);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_ANIM_FOR_MOVING, anim);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_EDIT_ITEM_IS_MOVED, (void *) 1);

	return W_HOME_ERROR_NONE;
}



#define SLIPPED_LENGTH_AFTER 30
static Eina_Bool _anim_move_page_after_cb(void *data)
{
	Evas_Object *scroller = data;
	Evas_Object *page = NULL;
	Evas_Object *rect = NULL;
	scroller_info_s *scroller_info = NULL;
	int w, h;

	retv_if(!scroller, ECORE_CALLBACK_CANCEL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, ECORE_CALLBACK_CANCEL);

	rect = evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_RECT_FOR_MOVING);
	retv_if(!rect, ECORE_CALLBACK_CANCEL);
	evas_object_geometry_get(rect, NULL, NULL, &w, &h);

	w -= SLIPPED_LENGTH_AFTER;
	evas_object_size_hint_min_set(rect, w, h);
	if (w <= 0) {
		page = evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_PAGE_FOR_MOVING);
		elm_box_unpack(scroller_info->box, rect);
		scroller_bring_in_page(scroller, page, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);
		evas_object_del(rect);

		if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
			index_update(scroller_info->index[PAGE_DIRECTION_RIGHT], scroller, INDEX_BRING_IN_NONE);
			index_bring_in_page(scroller_info->index[PAGE_DIRECTION_RIGHT], page);
		}
		evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_PAGE_FOR_MOVING);
		evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_ANIM_FOR_MOVING);
		evas_object_data_del(scroller, PRIVATE_DATA_KEY_SCROLLER_RECT_FOR_MOVING);
		return ECORE_CALLBACK_CANCEL;
	}
	return ECORE_CALLBACK_RENEW;
}



HAPI w_home_error_e edit_push_page_after(Evas_Object *scroller, Evas_Object *page, Evas_Object *after)
{
	Evas_Object *rect = NULL;
	scroller_info_s *scroller_info = NULL;
	Ecore_Animator *anim = NULL;

	retv_if(!scroller, W_HOME_ERROR_FAIL);
	retv_if(!page, W_HOME_ERROR_FAIL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);

	anim = evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_ANIM_FOR_MOVING);
	if (anim) return W_HOME_ERROR_FAIL;

	rect = evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_RECT_FOR_MOVING);
	if (rect) {
		elm_box_unpack(scroller_info->box, rect);
		evas_object_del(rect);
	}

	rect = evas_object_rectangle_add(main_get_info()->e);
	retv_if(!rect, W_HOME_ERROR_FAIL);
	evas_object_size_hint_min_set(rect, PAGE_EDIT_WIDTH, PAGE_EDIT_HEIGHT);
	evas_object_color_set(rect, 0, 0, 0, 0);
	evas_object_show(rect);

	elm_box_pack_before(scroller_info->box, rect, page);
	elm_box_unpack(scroller_info->box, page);
	if (after) elm_box_pack_after(scroller_info->box, page, after);
	else elm_box_pack_start(scroller_info->box, page);

	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_PAGE_FOR_MOVING, page);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_RECT_FOR_MOVING, rect);

	anim = ecore_animator_add(_anim_move_page_after_cb, scroller);
	retv_if(!anim, W_HOME_ERROR_FAIL);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_SCROLLER_ANIM_FOR_MOVING, anim);
	evas_object_data_set(scroller, PRIVATE_DATA_KEY_EDIT_ITEM_IS_MOVED, (void *) 1);

	return W_HOME_ERROR_NONE;
}



static Eina_Bool _move_timer_cb(void *data)
{
	Evas_Object *proxy_page = data;
	Evas_Object *pressed_page = NULL;
	page_info_s *page_info = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	int idx_above_page = -1;
	int idx_below_page = -1;
	int inner_x, inner_y, sx;

	retv_if(!proxy_page, ECORE_CALLBACK_CANCEL);

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, ECORE_CALLBACK_CANCEL);
	retv_if(!page_info->scroller, ECORE_CALLBACK_CANCEL);
	evas_object_data_del(page_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);

	evas_object_geometry_get(page_info->page_inner, &inner_x, &inner_y, NULL, NULL);
	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_X_FOR_ANIM, (void *) inner_x);
	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_PAGE_Y_FOR_ANIM, (void *) inner_y);

	elm_scroller_region_get(page_info->scroller, &sx, NULL, NULL, NULL);
	sx += inner_x;
	evas_object_data_set(page_info->scroller, PRIVATE_DATA_KEY_SCROLLER_X, (void *)sx);

	idx_below_page = scroller_seek_page_position(page_info->scroller, proxy_page);
	retv_if(-1 == idx_below_page, ECORE_CALLBACK_CANCEL);

	pressed_page = evas_object_data_get(page_info->scroller, PRIVATE_DATA_KEY_SCROLLER_PRESS_PAGE);
	retv_if(!pressed_page, ECORE_CALLBACK_CANCEL);

	idx_above_page = scroller_seek_page_position(page_info->scroller, pressed_page);
	retv_if(-1 == idx_above_page, ECORE_CALLBACK_CANCEL);

	if (idx_above_page < idx_below_page) {
		scroller_move_page_prev(page_info->scroller, pressed_page, proxy_page, pressed_page);
	} else {
		scroller_move_page_next(page_info->scroller, proxy_page, pressed_page, pressed_page);
	}

	scroller_bring_in_center_of(page_info->scroller, pressed_page, SCROLLER_FREEZE_ON, NULL, NULL, NULL, NULL);

	edit_scroller_info = evas_object_data_get(page_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!edit_scroller_info, ECORE_CALLBACK_CANCEL);

	/* Update index */
	retv_if(!edit_scroller_info->index[PAGE_DIRECTION_RIGHT], ECORE_CALLBACK_CANCEL);
	index_update(edit_scroller_info->index[PAGE_DIRECTION_RIGHT], page_info->scroller, INDEX_BRING_IN_NONE);
	index_bring_in_page(edit_scroller_info->index[PAGE_DIRECTION_RIGHT], pressed_page);

	_D("Move an proxy_page(%p, %d) to (%d)", proxy_page, idx_below_page, idx_above_page);

	return ECORE_CALLBACK_CANCEL;
}



#define TIME_MOVE_ITEM 0.1f
static void _upper_start_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *proxy_page = obj;
	Evas_Object *pressed_page = NULL;
	Ecore_Timer *timer = NULL;
	page_info_s *page_info = NULL;

	ret_if(!proxy_page);

	page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	pressed_page = evas_object_data_get(page_info->scroller, PRIVATE_DATA_KEY_SCROLLER_PRESS_PAGE);
	ret_if(!pressed_page);

	if (proxy_page == pressed_page) {
		timer = evas_object_data_del(page_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);
		if (timer) ecore_timer_del(timer);
		return;
	}

	_D("Upper start : %p", proxy_page);

	timer = evas_object_data_del(page_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);
	if (timer) ecore_timer_del(timer);

	timer = ecore_timer_add(TIME_MOVE_ITEM, _move_timer_cb, proxy_page);
	if (timer) evas_object_data_set(page_info->scroller, DATA_KEY_EVENT_UPPER_TIMER, timer);
	else _E("Cannot add a timer");
}



static void _upper_cb(void *data, Evas_Object *obj, void *event_info)
{
	;
}



static void _upper_end_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *page = obj;
	page_info_s *page_info = NULL;
	Ecore_Timer *timer = NULL;

	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	_D("Upper end : %p", page);

	timer = evas_object_data_del(page_info->scroller, DATA_KEY_EVENT_UPPER_TIMER);
	if (timer) ecore_timer_del(timer);
}



Evas_Object *edit_create_proxy_page(Evas_Object *edit_scroller, Evas_Object *real_page, page_changeable_bg_e changable_color)
{
	Evas_Object *proxy_page = NULL;
	Evas_Object *proxy_item = NULL;
	Evas_Object *remove_focus = NULL;
	page_info_s *page_info = NULL;
	page_info_s *proxy_page_info = NULL;

	retv_if(!edit_scroller, NULL);
	retv_if(!real_page, NULL);

	page_info = evas_object_data_get(real_page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	/* Proxy item */
	proxy_item = _create_proxy_item(edit_scroller, real_page);
	retv_if(!proxy_item, NULL);

	/* Proxy Page */
	proxy_page = page_create(edit_scroller
								, proxy_item
								, page_info->id, page_info->subid
								, PAGE_EDIT_WIDTH, PAGE_EDIT_HEIGHT
								, changable_color, page_info->removable);
	if (!proxy_page) {
		_E("Cannot create a page");
		_destroy_proxy_item(proxy_item);
		return NULL;
	}

	proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	if (!proxy_page_info) {
		_E("Cannot get the proxy_page_info");
		page_destroy(proxy_page);
		_destroy_proxy_item(proxy_item);
		return NULL;
	}
	elm_object_signal_emit(proxy_page_info->page_inner, "enable", "blocker");

	remove_focus = elm_button_add(proxy_page_info->page_inner);
	retv_if(!remove_focus, NULL);

	elm_object_style_set(remove_focus, "focus");
	elm_object_part_content_set(proxy_page_info->page_inner, "remove_focus", remove_focus);
	elm_access_info_cb_set(remove_focus, ELM_ACCESS_INFO, _access_remove_button_name_cb, NULL);
	proxy_page_info->remove_focus = remove_focus;

	evas_object_data_set(proxy_page, DATA_KEY_REAL_PAGE, real_page);
	evas_object_data_set(real_page, DATA_KEY_PROXY_PAGE, proxy_page);

	proxy_page_info->direction = page_info->direction;
	if (PAGE_DIRECTION_CENTER == page_info->direction) return proxy_page;

	evas_object_event_callback_add(proxy_page_info->page_inner, EVAS_CALLBACK_MOUSE_DOWN, _down_cb, proxy_page);
	evas_object_event_callback_add(proxy_page_info->page_inner, EVAS_CALLBACK_MOUSE_MOVE, _move_cb, proxy_page);
	evas_object_event_callback_add(proxy_page_info->page_inner, EVAS_CALLBACK_MOUSE_UP, _up_cb, proxy_page);
	evas_object_smart_callback_add(proxy_page, "upper_start", _upper_start_cb, NULL);
	evas_object_smart_callback_add(proxy_page, "upper", _upper_cb, NULL);
	evas_object_smart_callback_add(proxy_page, "upper_end", _upper_end_cb, NULL);

	return proxy_page;
}



void edit_destroy_proxy_page(Evas_Object *proxy_page)
{
	Evas_Object *real_page = NULL;
	page_info_s *proxy_page_info = NULL;
	Ecore_Animator *anim = NULL;

	ret_if(!proxy_page);

	real_page = evas_object_data_del(proxy_page, DATA_KEY_REAL_PAGE);
	if (real_page) evas_object_data_del(real_page, DATA_KEY_PROXY_PAGE);

	anim = evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_ANIM_FOR_MOVING);
	if (anim) {
		ecore_animator_del(anim);
		anim = NULL;
		if (NULL == _content_set_page_inner(proxy_page)) _E("cannot set the page");
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_X_FOR_ANIM);
		evas_object_data_del(proxy_page, PRIVATE_DATA_KEY_PAGE_Y_FOR_ANIM);
	}

	proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	ret_if(!proxy_page_info);

	if (proxy_page_info->remove_focus) {
		evas_object_del(proxy_page_info->remove_focus);
		proxy_page_info->remove_focus = NULL;
	}

	if (proxy_page_info->item) {
		_destroy_proxy_item(proxy_page_info->item);
	}
	page_destroy(proxy_page);
}



static Eina_Bool _enlarge_dbox_anim_cb(void *data)
{
	Evas_Object *effect_page = data;
	Evas_Object *bg_rect = NULL;

	int cur_x, cur_y, cur_w, cur_h;
	int end_x, end_y, end_w, end_h;
	int vec_x, vec_y, vec_w, vec_h;
	int divide_factor;

	goto_if(!effect_page, OUT);

	bg_rect = evas_object_data_get(effect_page, DATA_KEY_BG);
	goto_if(!bg_rect, OUT);

	end_w = main_get_info()->root_w;
	end_h = main_get_info()->root_h;

	end_x = 0;
	end_y = 0;

	evas_object_geometry_get(effect_page, &cur_x, &cur_y, &cur_w, &cur_h);

	divide_factor = (int) evas_object_data_get(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR);
	if (divide_factor > 1) {
		evas_object_data_set(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR, (void *) divide_factor - 1);
	}

	vec_x = (end_x - cur_x) / divide_factor;
	vec_y = (end_y - cur_y) / divide_factor;
	vec_w = (end_w - cur_w) / divide_factor;
	vec_h = (end_h - cur_h) / divide_factor;

	if (!vec_x) vec_x = -1;
	if (!vec_y) vec_y = -1;
	if (!vec_w) vec_w = 1;
	if (!vec_h) vec_h = 1;

	cur_x += vec_x;
	cur_y += vec_y;
	cur_w += vec_w;
	cur_h += vec_h;

	if (cur_x < end_x) {
		goto OUT;
	}

	evas_object_move(effect_page, cur_x, cur_y);
	evas_object_resize(effect_page, cur_w, cur_h);
	evas_object_move(bg_rect, cur_x, cur_y);
	evas_object_resize(bg_rect, cur_w, cur_h);

	return ECORE_CALLBACK_RENEW;

OUT:
	if (effect_page) {
		evas_object_data_del(effect_page, PRIVATE_DATA_KEY_ANIM_FOR_ENLARGE);
		evas_object_data_del(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR);
		evas_object_data_del(effect_page, DATA_KEY_BG);

		edit_destroy_add_viewer(main_get_info()->layout);
		_destroy_proxy_bg(bg_rect);
		evas_object_del(effect_page);
	}
	return ECORE_CALLBACK_CANCEL;
}



#define DIVIDE_FACTOR 5
HAPI void edit_enlarge_effect_dbox(void *effect_page)
{
	Evas_Object *clip_bg = NULL;
	Ecore_Animator *anim = NULL;

	anim = evas_object_data_get(effect_page, PRIVATE_DATA_KEY_ANIM_FOR_ENLARGE);
	if (anim) {
		_D("You've already an animator");
		return;
	}

	evas_object_raise(effect_page);

	anim = ecore_animator_add(_enlarge_dbox_anim_cb, effect_page);
	if (anim) {
		evas_object_data_set(effect_page, PRIVATE_DATA_KEY_ANIM_FOR_ENLARGE, anim);
		evas_object_data_set(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR, (void *) DIVIDE_FACTOR);
	} else {
		clip_bg = evas_object_data_del(effect_page, DATA_KEY_BG);
		if (clip_bg) _destroy_proxy_bg(clip_bg);
		evas_object_del(effect_page);
	}
}



static void _dbox_selected_cb(void *data, Evas_Object *add_viewer, void *event_info)
{
	Evas_Object *layout = data;
	Evas_Object *clip_bg = NULL;
	layout_info_s *layout_info = NULL;
	struct add_viewer_event_info *e = event_info;

	_D("Selected: %s", e->pkg_info.dbox_id);
	ret_if(!layout);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	if (layout_info->edit) {
		evas_object_del(e->pkg_info.image);
		if (!add_dbox_in_edit(layout, e->pkg_info.dbox_id, e->pkg_info.content)) {
			_E("Cannot add dbox in edit mode");
		}
		edit_destroy_add_viewer(layout);
	} else {
		clip_bg = _create_proxy_bg(e->pkg_info.image);
		ret_if(!clip_bg);
		evas_object_data_set(e->pkg_info.image, DATA_KEY_BG, clip_bg);
		edit_enlarge_effect_dbox(e->pkg_info.image);

		if (!_add_dbox_in_normal(layout, e->pkg_info.dbox_id, e->pkg_info.content)) {
			_E("Cannot add dbox in widget scroller");
		}
	}

	/* Only for right layout */
	wms_change_favorite_order(W_HOME_WMS_BACKUP);
}



HAPI Evas_Object *edit_create_add_viewer(Evas_Object *layout)
{
	Evas_Object *add_viewer = NULL;
	layout_info_s *layout_info = NULL;

	retv_if(!layout, NULL);

	if (evas_object_data_get(layout, DATA_KEY_ADD_VIEWER)) {
		_D("There is already the add-viewer");
		return NULL;
	}

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, NULL);

	add_viewer = evas_object_add_viewer_add(layout);
	retv_if(!add_viewer, NULL);

	evas_object_size_hint_weight_set(add_viewer, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(add_viewer, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_resize(add_viewer, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_smart_callback_add(add_viewer, "selected", _dbox_selected_cb, layout);
	evas_object_show(add_viewer);
	elm_object_part_content_set(layout, "add_viewer", add_viewer);
	elm_object_signal_emit(layout, "show", "add_viewer");

	if (layout_info->edit) {
		elm_object_tree_focus_allow_set(layout_info->edit, EINA_FALSE);
	} else {
		elm_object_tree_focus_allow_set(layout_info->scroller, EINA_FALSE);
		layout_del_mouse_cb(layout);
	}
	elm_object_tree_focus_allow_set(add_viewer, EINA_TRUE);

	evas_object_data_set(layout, DATA_KEY_ADD_VIEWER, add_viewer);

	if (W_HOME_ERROR_NONE != key_register_cb(KEY_TYPE_BACK, _add_viewer_back_key_cb, layout)) {
		_E("Cannot register the key callback");
	}
	if (W_HOME_ERROR_NONE != key_register_cb(KEY_TYPE_HOME, _add_viewer_home_key_cb, layout)) {
		_E("Cannot register the key callback");
	}
#if 0
	if (W_HOME_ERROR_NONE != gesture_register_cb(BEZEL_DOWN, _add_viewer_bezel_down_cb, layout)) {
		_E("Cannot register the gesture callback");
	}
#endif

	return add_viewer;
}



static void _del_add_viewer(void *data, Evas_Object *o, const char *emission, const char *source)
{
	Evas_Object *add_viewer = NULL;
	Evas_Object *layout = data;
	layout_info_s *layout_info = NULL;

	ret_if(!layout);
	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	add_viewer = elm_object_part_content_unset(layout, "add_viewer");
	ret_if(!add_viewer);

	if (layout_info->edit) {
		/* If you want to set the EINA_TRUE in this API,
		 you have to control the tree focus set when using content_set or content_unset API
		 in the layout that elm_object_tree_focus_allow_set is EINA_TRUE*/
		elm_object_tree_focus_allow_set(layout_info->edit, EINA_FALSE);
	} else {
		elm_object_tree_focus_allow_set(layout_info->scroller, EINA_TRUE);
		layout_add_mouse_cb(layout);
	}
	elm_object_tree_focus_allow_set(add_viewer, EINA_FALSE);

	evas_object_del(add_viewer);
	elm_object_signal_callback_del(layout, "add_viewer,hide", "add_viewer", _del_add_viewer);
}



HAPI void edit_destroy_add_viewer(Evas_Object *layout)
{
	Evas_Object *add_viewer = NULL;
	layout_info_s *layout_info = NULL;

	ret_if(!layout);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	add_viewer = evas_object_data_del(layout, DATA_KEY_ADD_VIEWER);
	if (!add_viewer) return;

#if 0
	gesture_unregister_cb(BEZEL_DOWN, _add_viewer_bezel_down_cb);
#endif
	key_unregister_cb(KEY_TYPE_BACK, _add_viewer_back_key_cb);
	key_unregister_cb(KEY_TYPE_HOME, _add_viewer_home_key_cb);

	if (layout_info->edit) elm_object_signal_emit(layout, "hide", "add_viewer");
	else elm_object_signal_emit(layout, "hide,instant", "add_viewer");

	edje_object_message_signal_process(elm_layout_edje_get(layout));

	elm_object_signal_callback_add(layout, "add_viewer,hide", "add_viewer", _del_add_viewer, layout);
}



/* You have to free the returned value */
HAPI char *edit_get_count_str_from_icu(int count)
{
	char *p = NULL;
	char *ret_str = NULL;
	char *locale = NULL;
	char res[LOCALE_LEN] = { 0, };

	UErrorCode status = U_ZERO_ERROR;
	UNumberFormat *num_fmt;
	UChar result[20] = { 0, };

	uint32_t number = count;
	int32_t len = (int32_t) (sizeof(result) / sizeof((result)[0]));

	locale = vconf_get_str(VCONFKEY_REGIONFORMAT);
	retv_if(!locale, NULL);

	if(locale[0] != '\0') {
		p = strstr(locale, ".UTF-8");
		if (p) *p = 0;
	}

	num_fmt = unum_open(UNUM_DEFAULT, NULL, -1, locale, NULL, &status);
	unum_format(num_fmt, number, result, len, NULL, &status);
	u_austrcpy(res, result);
	unum_close(num_fmt);

	ret_str = strdup(res);
	free(locale);
	return ret_str;
}



static void _plus_item_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	edit_info_s *edit_info = NULL;
	layout_info_s *layout_info = data;

	_D("Clicked the plus item");

	ret_if(!layout_info);
	ret_if(!layout_info->edit);

	edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
	edit_change_focus(edit_info->scroller, NULL);

	if (!edit_create_add_viewer(edit_info->layout)) {
		_E("Cannot add the add-viewer");
	}

	effect_play_sound();
}



static void _plus_item_down_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *page = data;
	Evas_Object *box = NULL;
	page_info_s *page_info = NULL;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->page_inner);
	ret_if(!page_info->item);

	box = elm_object_part_content_get(page_info->page_inner, "line");
	ret_if(!box);
	evas_object_color_set(box, 255, 255, 255, 100);

	elm_object_signal_emit(page_info->item, "edit,press", "plus");
}



static void _plus_item_up_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *page = data;
	Evas_Object *box = NULL;
	page_info_s *page_info = NULL;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->page_inner);
	ret_if(!page_info->item);

	box = elm_object_part_content_get(page_info->page_inner, "line");
	ret_if(!box);
	evas_object_color_set(box, 255, 255, 255, 255);

	elm_object_signal_emit(page_info->item, "edit,release", "plus");
}



void _destroy_plus_page(Evas_Object *edit)
{
	edit_info_s *edit_info = NULL;
	page_info_s *page_info = NULL;
	scroller_info_s *scroller_info = NULL;

	ret_if(!edit);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);
	ret_if(!edit_info->scroller);
	ret_if(!edit_info->plus_page);

	scroller_info = evas_object_data_get(edit_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	page_info = evas_object_data_get(edit_info->plus_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	evas_object_del(page_info->item);

	evas_object_data_del(edit_info->plus_page, DATA_KEY_REAL_PAGE);
	evas_object_data_del(edit_info->plus_page, PRIVATE_DATA_KEY_EDIT_DO_NOT_SUPPORT_ENLARGE_EFFECT);
	page_destroy(edit_info->plus_page);

	edit_info->plus_page = NULL;
	scroller_info->plus_page = NULL;
}



#define PLUS_ITEM_EDJ EDJEDIR"/page.edj"
#define PLUS_ITEM_GROUP "plus_item"
Evas_Object *_create_plus_page(Evas_Object *edit)
{
	Evas_Object *page = NULL;
	Evas_Object *plus_item = NULL;
	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;
	page_info_s *page_info = NULL;
	scroller_info_s *scroller_info = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	Eina_Bool ret;
	char max_text[TEXT_LEN] = {0, };
	char *count_str = NULL;

	retv_if(!edit, NULL);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	retv_if(!edit_info, NULL);

	layout_info = evas_object_data_get(edit_info->layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, NULL);

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	edit_scroller_info = evas_object_data_get(edit_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!edit_scroller_info, NULL);

	plus_item = elm_layout_add(edit_info->scroller);
	goto_if(!plus_item, ERROR);

	ret = elm_layout_file_set(plus_item, PLUS_ITEM_EDJ, PLUS_ITEM_GROUP);
	goto_if(EINA_FALSE == ret, ERROR);
	evas_object_size_hint_weight_set(plus_item, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(plus_item, ITEM_EDIT_WIDTH, ITEM_EDIT_HEIGHT);
	evas_object_show(plus_item);

	count_str = edit_get_count_str_from_icu(MAX_DBOX);
	if (!count_str) {
		_E("count_str is NULL");
		count_str = calloc(1, LOCALE_LEN);
		retv_if(!count_str, NULL);
		snprintf(count_str, LOCALE_LEN, "%d", MAX_DBOX);
	}

	snprintf(max_text
			, sizeof(max_text)
			, _("IDS_HS_BODY_THE_MAXIMUM_NUMBER_OF_WIDGETS_HPD_HAS_BEEN_REACHED_DELETE_SOME_WIDGETS_AND_TRY_AGAIN_ABB")
			, count_str);
	elm_object_part_text_set(plus_item, "max_text", max_text);
	free(count_str);

	page = page_create(edit_info->scroller,
							plus_item,
							NULL, NULL,
							edit_scroller_info->page_width, edit_scroller_info->page_height,
							PAGE_CHANGEABLE_BG_OFF, PAGE_REMOVABLE_OFF);
	if (!page) {
		_E("Cannot create the page");
		evas_object_del(plus_item);
		return NULL;
	}

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	goto_if(!page_info, ERROR);
	goto_if(!page_info->page_inner, ERROR);

	elm_object_signal_emit(page_info->page_inner, "enable", "blocker");
	elm_object_signal_emit(plus_item, "show,edit", "plus_item");

	elm_access_info_cb_set(page_info->focus, ELM_ACCESS_TYPE, NULL, NULL);
	elm_access_info_cb_set(page_info->focus, ELM_ACCESS_INFO, _access_plus_button_name_cb, NULL);
	elm_object_signal_callback_add(plus_item, "click", "plus_item", _plus_item_clicked_cb, layout_info);
	elm_object_signal_callback_add(plus_item, "down", "plus_item", _plus_item_down_cb, page);
	elm_object_signal_callback_add(plus_item, "up", "plus_item", _plus_item_up_cb, page);


	evas_object_data_set(page, DATA_KEY_REAL_PAGE, scroller_info->plus_page);
	evas_object_data_set(page, PRIVATE_DATA_KEY_EDIT_DO_NOT_SUPPORT_ENLARGE_EFFECT, (void *) 1);
	evas_object_data_set(scroller_info->plus_page, DATA_KEY_PROXY_PAGE, page);

	edit_info->plus_page = page;
	edit_scroller_info->plus_page = page;

	return page;

ERROR:
	_destroy_plus_page(edit);
	return NULL;
}



static Evas_Object *_create_center_page(Evas_Object *edit, page_direction_e page_direction)
{
	Evas_Object *center_page = NULL;
	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;
	scroller_info_s *scroller_info = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	page_info_s *center_page_info = NULL;

	retv_if(!edit, NULL);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	retv_if(!edit_info, NULL);

	layout_info = evas_object_data_get(edit_info->layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, NULL);

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	edit_scroller_info = evas_object_data_get(edit_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!edit_scroller_info, NULL);

	/* Create the center page */
	center_page = edit_create_proxy_page(edit_info->scroller, scroller_info->center, PAGE_CHANGEABLE_BG_ON);
	if (!center_page) {
		_E("Cannot create a page");
		return NULL;
	}
	evas_object_data_set(center_page, PRIVATE_DATA_KEY_EDIT_UNFOCUSABLE, (void *) 1);

	center_page_info = evas_object_data_get(center_page, DATA_KEY_PAGE_INFO);
	if (!center_page_info) {
		_E("Cannot get the page_info");
		edit_destroy_proxy_page(center_page);
		return NULL;
	}
	elm_access_info_cb_set(center_page_info->focus, ELM_ACCESS_CONTEXT_INFO, _access_tab_to_move_cb, center_page);

	if (page_direction == PAGE_DIRECTION_LEFT) {
		evas_object_smart_callback_add(center_page_info->focus, "clicked", _clicked_center_cb, center_page);
	} else {
		evas_object_smart_callback_add(center_page_info->focus, "clicked", _clicked_noti_cb, center_page);
	}
	elm_object_signal_emit(center_page_info->page_inner, "disable", "cover");
	elm_object_signal_emit(center_page_info->page_inner, "select", "cover_clipper");
	elm_object_signal_emit(center_page_info->page_inner, "enable", "blocker");

	edit_info->center_page = center_page;

	return center_page;
}



static void _destroy_center_page(Evas_Object *edit)
{
	edit_info_s *edit_info = NULL;

	ret_if(!edit);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);
	ret_if(!edit_info->center_page);

	evas_object_data_del(edit_info->center_page, PRIVATE_DATA_KEY_EDIT_UNFOCUSABLE);

	edit_destroy_proxy_page(edit_info->center_page);
	edit_info->center_page = NULL;
}



static w_home_error_e _scroller_push_right_page(Evas_Object *edit, Evas_Object *scroller)
{
	Evas_Object *page = NULL;
	Evas_Object *center_page = NULL;
	Evas_Object *plus_page = NULL;
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;
	edit_info_s *edit_info = NULL;
	page_info_s *page_info = NULL;
	int count = 0;

	retv_if(!edit, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(!scroller, W_HOME_ERROR_INVALID_PARAMETER);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	retv_if(!edit_info, W_HOME_ERROR_FAIL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, W_HOME_ERROR_FAIL);

	center_page = _create_center_page(edit, PAGE_DIRECTION_RIGHT);
	if (center_page) {
		scroller_push_page(edit_info->scroller, center_page, SCROLLER_PUSH_TYPE_CENTER);
	}

	EINA_LIST_FREE(list, page) {
		Evas_Object *proxy_page = NULL;
		page_info_s *proxy_page_info = NULL;
		Evas_Object *page_inner;

		continue_if(!page);

		page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		if (!page_info) continue;
		if (PAGE_DIRECTION_RIGHT != page_info->direction) continue;
		if (page == scroller_info->plus_page) continue;

		page_inner = elm_object_part_content_unset(page, "inner");
		if (!page_inner) {
			elm_object_part_content_set(page, "inner", page_info->page_inner);
			page_inner = elm_object_part_content_unset(page, "inner");
			if (!page_inner) {
				_E("Fault - Impossible");
				page_inner = page_info->page_inner;
			}
		}
		evas_object_move(page_inner, 0, 0);

		evas_object_dynamicbox_resume(page_info->item);

		proxy_page = edit_create_proxy_page(edit_info->scroller, page, PAGE_CHANGEABLE_BG_ON);
		continue_if(!proxy_page);

		proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
		if (!proxy_page_info) {
			_E("Cannot get the page_info");
			edit_destroy_proxy_page(proxy_page);
			continue;
		}
		elm_object_signal_emit(proxy_page_info->page_inner, "select", "cover_clipper");

		elm_access_info_cb_set(proxy_page_info->focus, ELM_ACCESS_CONTEXT_INFO, _access_tab_to_move_cb, NULL);
		evas_object_smart_callback_add(proxy_page_info->focus, "clicked", _clicked_dbox_cb, proxy_page);
		evas_object_smart_callback_add(proxy_page_info->remove_focus, "clicked", _del_dbox_cb, proxy_page);

		scroller_push_page(edit_info->scroller, proxy_page, SCROLLER_PUSH_TYPE_LAST);
		count++;
	}

	/* Plus page */
	plus_page = _create_plus_page(edit);
	retv_if(!plus_page, W_HOME_ERROR_FAIL);
	scroller_push_page(edit_info->scroller, plus_page, SCROLLER_PUSH_TYPE_LAST);
	edit_arrange_plus_page(edit);

	return W_HOME_ERROR_NONE;
}



static void _scroller_pop_right_page(Evas_Object *edit)
{
	Evas_Object *proxy_page = NULL;
	Eina_List *edit_list = NULL;
	edit_info_s *edit_info = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *proxy_page_info = NULL;

	ret_if(!edit);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);

	scroller_info = evas_object_data_get(edit_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	scroller_pop_page(edit_info->scroller, edit_info->center_page);
	_destroy_center_page(edit);

	if (edit_info->plus_page) {
		evas_object_data_del(edit_info->plus_page, DATA_KEY_REAL_PAGE);
		evas_object_data_del(scroller_info->plus_page, DATA_KEY_PROXY_PAGE);
		_destroy_plus_page(edit);
	}

	edit_list = elm_box_children_get(scroller_info->box);
	if (!edit_list) {
		_D("There is no page");
		return;
	}

	EINA_LIST_FREE(edit_list, proxy_page) {
		continue_if(!proxy_page);

		elm_box_unpack(scroller_info->box, proxy_page);

		proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
		continue_if(!proxy_page_info);

		edit_destroy_proxy_page(proxy_page);
	}
}



#define PRIVATE_DATA_KEY_SCROLLER_PAD_BF "p_sc_pbf"
#define PRIVATE_DATA_KEY_SCROLLER_PAD_AF "p_sc_paf"
static void _add_padding(Evas_Object *edit_scroller)
{
	Evas_Object *pad_bf, *pad_af;
	scroller_info_s *edit_scroller_info = NULL;

	ret_if(!edit_scroller);

	edit_scroller_info = evas_object_data_get(edit_scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!edit_scroller_info);

	pad_bf = evas_object_rectangle_add(main_get_info()->e);
	ret_if(!pad_bf);
	evas_object_size_hint_min_set(pad_bf, PAGE_EDIT_SIDE_PAD_WIDTH, PAGE_EDIT_HEIGHT);
	evas_object_color_set(pad_bf, 0, 0, 0, 0);
	evas_object_show(pad_bf);
	elm_box_pack_start(edit_scroller_info->box_layout, pad_bf);
	evas_object_data_set(edit_scroller, PRIVATE_DATA_KEY_SCROLLER_PAD_BF, pad_bf);

	pad_af = evas_object_rectangle_add(main_get_info()->e);
	ret_if(!pad_af);
	evas_object_size_hint_min_set(pad_af, PAGE_EDIT_SIDE_PAD_WIDTH, PAGE_EDIT_HEIGHT);
	evas_object_color_set(pad_af, 0, 0, 0, 0);
	evas_object_show(pad_af);
	elm_box_pack_end(edit_scroller_info->box_layout, pad_af);
	evas_object_data_set(edit_scroller, PRIVATE_DATA_KEY_SCROLLER_PAD_AF, pad_af);
}



static void _remove_padding(Evas_Object *edit_scroller)
{
	Evas_Object *pad_bf, *pad_af;

	ret_if(!edit_scroller);

	pad_bf = evas_object_data_del(edit_scroller, PRIVATE_DATA_KEY_SCROLLER_PAD_BF);
	if (pad_bf) evas_object_del(pad_bf);

	pad_af = evas_object_data_del(edit_scroller, PRIVATE_DATA_KEY_SCROLLER_PAD_AF);
	if (pad_af) evas_object_del(pad_af);
}



static void _edit_scroll_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *edit_scroller = obj;
	Evas_Object *page_current = NULL;
	scroller_info_s *edit_scroller_info = NULL;

	ret_if(!edit_scroller);

	edit_scroller_info = evas_object_data_get(edit_scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!edit_scroller_info);

	if (evas_object_data_get(edit_scroller, PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED)) return;
	if (!edit_scroller_info->scroll_cover) return;

	page_current = scroller_get_focused_page(edit_scroller);
	ret_if(!page_current);
	edit_change_focus(edit_scroller, page_current);
}



static w_home_error_e _scroller_push_left_page(Evas_Object *edit, Evas_Object *scroller)
{
	Evas_Object *page = NULL;
	Evas_Object *center_page = NULL;
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;
	edit_info_s *edit_info = NULL;
	page_info_s *page_info = NULL;

	retv_if(!edit, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(!scroller, W_HOME_ERROR_INVALID_PARAMETER);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	retv_if(!edit_info, W_HOME_ERROR_FAIL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, W_HOME_ERROR_FAIL);

	center_page = _create_center_page(edit, PAGE_DIRECTION_LEFT);
	if (center_page) {
		scroller_push_page(edit_info->scroller, center_page, SCROLLER_PUSH_TYPE_CENTER);
	}

	EINA_LIST_FREE(list, page) {
		Evas_Object *proxy_page = NULL;
		page_info_s *proxy_page_info = NULL;

		continue_if(!page);

		page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		if (!page_info) continue;
		if (PAGE_DIRECTION_LEFT != page_info->direction) continue;

		proxy_page = edit_create_proxy_page(edit_info->scroller, page, PAGE_CHANGEABLE_BG_OFF);
		continue_if(!proxy_page);

		evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_EDIT_DISABLE_REORDERING, (void *) 1);

		proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
		if (!proxy_page_info) {
			_E("Cannot get the page_info");
			edit_destroy_proxy_page(proxy_page);
			continue;
		}

		if (scroller_info->center_neighbor_left == page) {
			evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_EDIT_UNFOCUSABLE, (void *) 1);
			elm_object_signal_emit(proxy_page_info->page_inner, "disable", "cover");
		}
		elm_object_signal_emit(proxy_page_info->page_inner, "select", "cover_clipper");

		elm_access_info_cb_set(proxy_page_info->focus, ELM_ACCESS_CONTEXT_INFO, _access_tab_to_move_cb, NULL);

		evas_object_smart_callback_add(proxy_page_info->focus, "clicked", _clicked_noti_cb, proxy_page);
		evas_object_smart_callback_add(proxy_page_info->remove_focus, "clicked", _del_noti_cb, proxy_page);

		scroller_push_page(edit_info->scroller, proxy_page, SCROLLER_PUSH_TYPE_CENTER_LEFT);
	}

	return W_HOME_ERROR_NONE;
}



static void _scroller_pop_left_page(Evas_Object *edit)
{
	Evas_Object *edit_page = NULL;
	Eina_List *edit_list = NULL;
	edit_info_s *edit_info = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	page_info_s *page_info = NULL;

	ret_if(!edit);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);

	scroller_pop_page(edit_info->scroller, edit_info->center_page);
	_destroy_center_page(edit);

	edit_scroller_info = evas_object_data_get(edit_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!edit_scroller_info);

	edit_list = elm_box_children_get(edit_scroller_info->box);
	ret_if(!edit_list);

	EINA_LIST_FREE(edit_list, edit_page) {
		continue_if(!edit_page);

		page_info = evas_object_data_get(edit_page, DATA_KEY_PAGE_INFO);
		continue_if(!page_info);

		evas_object_data_del(edit_page, PRIVATE_DATA_KEY_EDIT_DISABLE_REORDERING);
		evas_object_data_del(edit_page, PRIVATE_DATA_KEY_EDIT_UNFOCUSABLE);
		scroller_pop_page(edit_info->scroller, edit_page);
		edit_destroy_proxy_page(edit_page);
	}
}



static char *_access_page_num_cb(Evas_Object *scroller)
{
	Evas_Object *page = NULL;
	page_info_s *page_info = NULL;
	scroller_info_s *scroller_info = NULL;
	Eina_List *list = NULL;
	int count = 0;
	int cur = 0;
	char number[BUFSZE];
	char *tmp;

	retv_if(NULL == scroller, NULL);

	page = scroller_get_focused_page(scroller);
	retv_if(!page, NULL);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	list = elm_box_children_get(scroller_info->box);
	retv_if(!list, NULL);

	count = eina_list_count(list);
	cur = scroller_seek_page_position(scroller, page) + 1;

	snprintf(number, sizeof(number), _("IDS_AT_BODY_PAGE_P1SD_OF_P2SD_T_TTS"), cur, count);

	tmp = strdup(number);
	if (!tmp) return NULL;
	return tmp;
}



static void _edit_anim_stop_cb(void *data, Evas_Object *scroller, void *event_info)
{
	Evas_Object *press_page = NULL;
	char *tmp = NULL;

	_D("stop the scroller(%p) animation", scroller);

	ret_if(!scroller);

	if (!main_get_info()->is_tts) return;

	press_page = evas_object_data_get(scroller, PRIVATE_DATA_KEY_SCROLLER_PRESS_PAGE);
	ret_if(!press_page);

	tmp = _access_page_num_cb(scroller);
	if (tmp) {
		elm_access_say(tmp);
		free(tmp);
	}
}



#define EDIT_EDJ EDJEDIR"/edit.edj"
#define EDIT_GROUP "edit"
HAPI Evas_Object *_create_right_layout(Evas_Object *layout)
{
	Evas_Object *edit = NULL;
	Evas_Object *edit_scroller = NULL;

	layout_info_s *layout_info = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	edit_info_s *edit_info = NULL;
	Eina_Bool ret;

	retv_if(!layout, NULL);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, NULL);

	edit = elm_layout_add(layout);
	retv_if(NULL == edit, NULL);

	edit_info = calloc(1, sizeof(edit_info_s));
	if (!edit_info) {
		_E("Cannot calloc for edit_info");
		free(edit_info);
		return NULL;
	}
	evas_object_data_set(edit, DATA_KEY_EDIT_INFO, edit_info);

	ret = elm_layout_file_set(edit, EDIT_EDJ, EDIT_GROUP);
	if (EINA_FALSE == ret) {
		_E("cannot set the file into the layout");
		free(edit_info);
		evas_object_del(edit);
		return NULL;
	}
	evas_object_size_hint_weight_set(edit, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(edit, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_resize(edit, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_show(edit);

	layout_del_mouse_cb(layout);

	edit_scroller = scroller_create(layout, edit, PAGE_EDIT_WIDTH, PAGE_EDIT_HEIGHT, SCROLLER_INDEX_SINGULAR);
	if (!edit_scroller) {
		_E("Cannot create edit_scroller");
		free(edit_info);
		evas_object_del(edit);
		return NULL;
	}
	evas_object_smart_callback_add(edit_scroller, "scroll", _edit_scroll_cb, NULL);
	evas_object_smart_callback_add(edit_scroller, "scroll,anim,stop", _edit_anim_stop_cb, NULL);
	_add_padding(edit_scroller);

	elm_object_part_content_set(layout, "edit", edit);
	elm_object_part_content_set(edit, "scroller", edit_scroller);

	edit_info->layout = layout;
	edit_info->scroller = edit_scroller;

	layout_info->edit = edit;

	edit_scroller_info = evas_object_data_get(edit_scroller, DATA_KEY_SCROLLER_INFO);
	if (!edit_scroller_info) {
		_E("Cannot create scroller");
		scroller_destroy(layout);
		free(layout_info);
		evas_object_del(layout);
		return NULL;
	}
	edit_info->scroller = edit_scroller;

	edit_scroller_info->index[PAGE_DIRECTION_RIGHT] = index_create(edit, edit_scroller, PAGE_DIRECTION_RIGHT);
	if (!edit_scroller_info->index[PAGE_DIRECTION_RIGHT]) _E("Cannot create the right index");
	else elm_object_part_content_set(edit, "index", edit_scroller_info->index[PAGE_DIRECTION_RIGHT]);

	_scroller_push_right_page(edit, layout_info->scroller);

	return edit;
}



HAPI void _destroy_right_layout(Evas_Object *layout)
{
	layout_info_s *layout_info = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	scroller_info_s *scroller_info = NULL;
	edit_info_s *edit_info = NULL;
	Evas_Object *focus_page = NULL;

	ret_if(!layout);

	edit_destroy_add_viewer(layout);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);
	ret_if(!layout_info->edit);

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);

	if (!evas_object_data_get(layout, PRIVATE_DATA_KEY_EDIT_SYNC_IS_DONE)) {
		_sync_from_edit_to_normal(layout);
	}
	evas_object_data_del(layout, PRIVATE_DATA_KEY_EDIT_SYNC_IS_DONE);

	layout_add_mouse_cb(layout);

	edit_scroller_info = evas_object_data_get(edit_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!edit_scroller_info);

	focus_page = scroller_get_focused_page(layout_info->scroller);
	scroller_push_page(layout_info->scroller, scroller_info->plus_page, SCROLLER_PUSH_TYPE_LAST);
	if (focus_page) scroller_region_show_page(layout_info->scroller, focus_page, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT);
	else _E("Cannot get the focused page");

	if (edit_scroller_info->index[PAGE_DIRECTION_RIGHT]) {
		index_destroy(edit_scroller_info->index[PAGE_DIRECTION_RIGHT]);
	}

	_scroller_pop_right_page(layout_info->edit);
	_remove_padding(edit_info->scroller);

	evas_object_data_del(edit_info->scroller, PRIVATE_DATA_KEY_EDIT_FOCUS_OBJECT);
	evas_object_data_del(edit_info->scroller, PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED);
	scroller_destroy(layout_info->edit);

	evas_object_del(layout_info->edit);
	free(edit_info);
	layout_info->edit = NULL;

	/* Only for right layout */
	wms_change_favorite_order(W_HOME_WMS_BACKUP);
}



#define EDIT_EDJ EDJEDIR"/edit.edj"
#define EDIT_GROUP "edit"
HAPI Evas_Object *_create_left_layout(Evas_Object *layout)
{
	Evas_Object *edit = NULL;
	Evas_Object *edit_scroller = NULL;
	Evas_Object *box = NULL;

	layout_info_s *layout_info = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	edit_info_s *edit_info = NULL;
	Eina_Bool ret;

	retv_if(!layout, NULL);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, NULL);

	edit = elm_layout_add(layout);
	retv_if(NULL == edit, NULL);

	edit_info = calloc(1, sizeof(edit_info_s));
	if (!edit_info) {
		_E("Cannot calloc for edit_info");
		free(edit_info);
		return NULL;
	}
	evas_object_data_set(edit, DATA_KEY_EDIT_INFO, edit_info);

	ret = elm_layout_file_set(edit, EDIT_EDJ, EDIT_GROUP);
	if (EINA_FALSE == ret) {
		_E("cannot set the file into the layout");
		free(edit_info);
		evas_object_del(edit);
		return NULL;
	}
	evas_object_size_hint_weight_set(edit, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(edit, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_resize(edit, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_show(edit);

	layout_del_mouse_cb(layout);

	evas_object_smart_callback_call(layout, LAYOUT_SMART_SIGNAL_EDIT_ON, layout);

	edit_scroller = scroller_create(layout, edit, PAGE_EDIT_WIDTH, PAGE_EDIT_HEIGHT, SCROLLER_INDEX_SINGULAR);
	if (!edit_scroller) {
		_E("Cannot create edit_scroller");
		free(edit_info);
		evas_object_del(edit);
		return NULL;
	}
	evas_object_smart_callback_add(edit_scroller, "scroll", _edit_scroll_cb, NULL);
	_add_padding(edit_scroller);

	box = elm_object_content_get(edit_scroller);
	evas_object_size_hint_align_set(box, 1.0, 0.5);

	elm_object_part_content_set(layout, "edit", edit);
	elm_object_part_content_set(edit, "scroller", edit_scroller);

	edit_info->layout = layout;
	edit_info->scroller = edit_scroller;

	layout_info->edit = edit;

	edit_scroller_info = evas_object_data_get(edit_scroller, DATA_KEY_SCROLLER_INFO);
	if (!edit_scroller_info) {
		_E("Cannot create scroller");
		scroller_destroy(layout);
		free(layout_info);
		evas_object_del(layout);
		return NULL;
	}
	edit_info->scroller = edit_scroller;

	edit_scroller_info->index[PAGE_DIRECTION_LEFT] = index_create(edit, edit_scroller, PAGE_DIRECTION_LEFT);
	if (!edit_scroller_info->index[PAGE_DIRECTION_LEFT]) _E("Cannot create the left index");
	else elm_object_part_content_set(edit, "index", edit_scroller_info->index[PAGE_DIRECTION_LEFT]);

	_scroller_push_left_page(edit, layout_info->scroller);

	return edit;
}



HAPI void _destroy_left_layout(Evas_Object *layout)
{
	layout_info_s *layout_info = NULL;
	scroller_info_s *edit_scroller_info = NULL;
	edit_info_s *edit_info = NULL;

	ret_if(!layout);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);
	ret_if(!layout_info->edit);

	edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);

	edit_scroller_info = evas_object_data_get(edit_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!edit_scroller_info);

	_scroller_pop_left_page(layout_info->edit);
	_remove_padding(edit_info->scroller);

	if (edit_scroller_info->index[PAGE_DIRECTION_LEFT]) {
		index_destroy(edit_scroller_info->index[PAGE_DIRECTION_LEFT]);
	}

	scroller_destroy(layout_info->edit);

	layout_add_mouse_cb(layout);

	free(edit_info);
	evas_object_data_del(layout_info->edit, DATA_KEY_EDIT_INFO);
	evas_object_del(layout_info->edit);
	layout_info->edit = NULL;

	evas_object_smart_callback_call(layout, LAYOUT_SMART_SIGNAL_EDIT_OFF, layout);
}



static key_cb_ret_e _edit_back_key_cb(void *data)
{
	_W("");
	Evas_Object *layout = data;
	Evas_Object *effect_page = NULL;
	Evas_Object *proxy_page = NULL;
	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;

	retv_if(!layout, KEY_CB_RET_CONTINUE);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, KEY_CB_RET_CONTINUE);

	if (!layout_info->edit) {
		_D("Layout is not edited");
		return KEY_CB_RET_CONTINUE;
	}

	edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
	goto_if(!edit_info, OUT);

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	goto_if(!scroller_info, OUT);

	proxy_page = scroller_get_focused_page(edit_info->scroller);
	if (!proxy_page) goto OUT;
	if (evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_EDIT_DO_NOT_SUPPORT_ENLARGE_EFFECT)) goto OUT;

	effect_page = edit_create_enlarge_effect_page(proxy_page);
	if (effect_page) edit_enlarge_effect_page(effect_page);
	else goto OUT;

	return KEY_CB_RET_STOP;

OUT:
	if (!scroller_get_right_page(edit_info->scroller, proxy_page)) {
		Evas_Object *real_page = NULL;
		int i = 0;

		_D("The proxy page(%p) does not have the right neighbor.", proxy_page);

		elm_scroller_last_page_get(layout_info->scroller, &i, NULL);
		real_page = scroller_get_page_at(layout_info->scroller, i);
		if (real_page) {
			_D("The last real page is (%p)", real_page);
			scroller_region_show_page(layout_info->scroller, real_page, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT);
			if (scroller_info->plus_page == real_page) {
				_D("The last page is the plus page");
				page_info = evas_object_data_get(real_page, DATA_KEY_PAGE_INFO);
				if (page_info->item) elm_object_signal_emit(page_info->item, "show,widget,ani", "plus_item");
				elm_object_signal_emit(page_info->page_inner, "deselect", "line");
			}
		}
	}
	edit_destroy_layout(layout);
	return KEY_CB_RET_STOP;
}



static key_cb_ret_e _edit_home_key_cb(void *data)
{
	Evas_Object *layout = data;
	Evas_Object *effect_page = NULL;
	Evas_Object *proxy_page = NULL;
	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;

	retv_if(!layout, KEY_CB_RET_CONTINUE);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, KEY_CB_RET_CONTINUE);

	if (!layout_info->edit) {
		_D("Layout is not edited");
		return KEY_CB_RET_CONTINUE;
	}

	edit_info = evas_object_data_get(layout_info->edit, DATA_KEY_EDIT_INFO);
	goto_if(!edit_info, OUT);

	if (evas_object_data_get(edit_info->scroller, PRIVATE_DATA_KEY_EDIT_IS_LONGPRESSED)) return KEY_CB_RET_STOP;

	proxy_page = scroller_get_focused_page(edit_info->scroller);
	if (!proxy_page) goto OUT;
	if (evas_object_data_get(proxy_page, PRIVATE_DATA_KEY_EDIT_DO_NOT_SUPPORT_ENLARGE_EFFECT)) goto OUT;

	effect_page = edit_create_enlarge_effect_page(proxy_page);
	if (effect_page) edit_enlarge_effect_page(effect_page);
	else goto OUT;

	return KEY_CB_RET_STOP;

OUT:
	edit_destroy_layout(layout);
	return KEY_CB_RET_CONTINUE;
}



static void _destroy_proxy_bg(Evas_Object *clip_bg)
{
	Evas_Object *proxy_bg = NULL;

	proxy_bg = evas_object_data_del(clip_bg, PRIVATE_DATA_KEY_PROXY_BG);
	if(proxy_bg) evas_object_del(proxy_bg);
	evas_object_del(clip_bg);
}



static Evas_Object *_create_proxy_bg(Evas_Object *item)
{
	Evas_Object *bg = NULL;
	Evas_Object *clip_bg = NULL;
	Evas_Object *proxy_bg = NULL;
	Eina_Bool ret;
	int x, y, w, h;

	clip_bg = evas_object_rectangle_add(main_get_info()->e);
	retv_if(!clip_bg, NULL);

	evas_object_geometry_get(item, &x, &y, &w, &h);
	evas_object_resize(clip_bg, w, h);
	evas_object_move(clip_bg, x, y);
	evas_object_color_set(clip_bg, 255, 255, 255, 255);
	evas_object_show(clip_bg);

	bg = evas_object_data_get(main_get_info()->win, DATA_KEY_BG);
	goto_if(!bg, ERROR);

	proxy_bg = evas_object_image_filled_add(main_get_info()->e);
	goto_if(!proxy_bg, ERROR);

	evas_object_data_set(clip_bg, PRIVATE_DATA_KEY_PROXY_BG, proxy_bg);

	ret = evas_object_image_source_set(proxy_bg, bg);
	goto_if(EINA_FALSE == ret, ERROR);
	evas_object_image_source_visible_set(proxy_bg, EINA_TRUE);
	evas_object_image_source_clip_set(proxy_bg, EINA_FALSE);
	evas_object_resize(proxy_bg, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_move(proxy_bg, 0, 0);
	evas_object_show(proxy_bg);

	evas_object_clip_set(proxy_bg, clip_bg);

	return clip_bg;

ERROR:
	_destroy_proxy_bg(clip_bg);
	return NULL;
}



static w_home_error_e _lang_changed_cb(void *data)
{
	Evas_Object *scroller = data;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	char max_text[TEXT_LEN] = {0, };
	char *count_str = NULL;

	retv_if(!scroller, W_HOME_ERROR_FAIL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, W_HOME_ERROR_FAIL);

	page_info = evas_object_data_get(scroller_info->plus_page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, W_HOME_ERROR_FAIL);

	count_str = edit_get_count_str_from_icu(MAX_DBOX);
	if (!count_str) {
		_E("count_str is NULL");
		count_str = calloc(1, LOCALE_LEN);
		retv_if(!count_str, W_HOME_ERROR_FAIL);
		snprintf(count_str, LOCALE_LEN, "%d", MAX_DBOX);
	}

	snprintf(max_text
			, sizeof(max_text)
			, _("IDS_HS_BODY_THE_MAXIMUM_NUMBER_OF_WIDGETS_HPD_HAS_BEEN_REACHED_DELETE_SOME_WIDGETS_AND_TRY_AGAIN_ABB")
			, count_str);
	elm_object_part_text_set(page_info->item, "max_text", max_text);
	free(count_str);

	return W_HOME_ERROR_NONE;
}



HAPI Evas_Object *edit_create_layout(Evas_Object *layout, edit_mode_e edit_mode)
{
	Evas_Object *edit = NULL;
	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;

	retv_if(!layout, NULL);

	_W("Create the edit layout");

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, NULL);

	if (W_HOME_ERROR_NONE != key_register_cb(KEY_TYPE_BACK, _edit_back_key_cb, layout)) {
		_E("Cannot register the key callback");
	}

	if (W_HOME_ERROR_NONE != key_register_cb(KEY_TYPE_HOME, _edit_home_key_cb, layout)) {
		_E("Cannot register the key callback");
	}
#if 0
	if (W_HOME_ERROR_NONE != gesture_register_cb(BEZEL_DOWN, _edit_bezel_down_cb, layout)) {
		_E("Cannot register the gesture callback");
	}
#endif

	switch (edit_mode) {
	case EDIT_MODE_LEFT:
		edit = _create_left_layout(layout);
		break;
	case EDIT_MODE_CENTER:
		break;
	case EDIT_MODE_RIGHT:
		edit = _create_right_layout(layout);
		break;
	default:
		_E("Cannot reach here");
		break;
	}
	elm_object_signal_emit(layout, "show", "edit");
	elm_object_signal_emit(edit, "init", "edit");
	elm_object_signal_emit(layout, "hide", "scroller");

	evas_object_data_set(layout, DATA_KEY_EDIT_MODE, (void *) edit_mode);
	layout_info->edit = edit;

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	if (edit_info && edit_info->scroller) {
		scroller_focus(edit_info->scroller);
		main_register_cb(APP_STATE_LANGUAGE_CHANGED, _lang_changed_cb, edit_info->scroller);
	} else _E("Cannot get the edit_info & edit_info->scroller");

	elm_object_tree_focus_allow_set(layout_info->scroller, EINA_FALSE);
	/* If you want to set the EINA_TRUE in this API,
	 you have to control the tree focus set when using content_set or content_unset API
	 in the layout that elm_object_tree_focus_allow_set is EINA_TRUE*/
	elm_object_tree_focus_allow_set(layout_info->edit, EINA_FALSE);

	return edit;
}



HAPI void edit_destroy_layout(void *layout)
{
	Evas_Object *current_page = NULL;
	layout_info_s *layout_info = NULL;
	scroller_info_s *scroller_info = NULL;
	edit_mode_e edit_mode = (int) evas_object_data_del(layout, DATA_KEY_EDIT_MODE);

	ret_if(!layout);

	main_unregister_cb(APP_STATE_LANGUAGE_CHANGED, _lang_changed_cb);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);
	ret_if(!layout_info->edit);

	key_unregister_cb(KEY_TYPE_BACK, _edit_back_key_cb);
	key_unregister_cb(KEY_TYPE_HOME, _edit_home_key_cb);
#if 0
	gesture_unregister_cb(BEZEL_DOWN, _edit_bezel_down_cb);
#endif

	elm_object_tree_focus_allow_set(layout_info->scroller, EINA_TRUE);
	elm_object_tree_focus_allow_set(layout_info->edit, EINA_FALSE);

	_D("Destroy the edit layout : %d", edit_mode);

	switch (edit_mode) {
	case EDIT_MODE_LEFT:
		_destroy_left_layout(layout);
		break;
	case EDIT_MODE_CENTER:
		break;
	case EDIT_MODE_RIGHT:
		_destroy_right_layout(layout);
		break;
	default:
		_E("Cannot reach here");
		break;
	}

	layout_info->edit = NULL;

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	current_page = scroller_get_focused_page(layout_info->scroller);
	ret_if(!current_page);

	if (scroller_info->index[PAGE_DIRECTION_LEFT]) {
		index_update(scroller_info->index[PAGE_DIRECTION_LEFT], layout_info->scroller, INDEX_BRING_IN_NONE);
		index_bring_in_page(scroller_info->index[PAGE_DIRECTION_LEFT], current_page);
	}

	if (scroller_info->index[PAGE_DIRECTION_RIGHT]) {
		index_update(scroller_info->index[PAGE_DIRECTION_RIGHT], layout_info->scroller, INDEX_BRING_IN_NONE);
		index_bring_in_page(scroller_info->index[PAGE_DIRECTION_RIGHT], current_page);
	}

	elm_object_signal_emit(layout, "hide", "edit");
	elm_object_signal_emit(layout, "show", "scroller");
}



static Eina_Bool _minify_page_anim_cb(void *data)
{
	Evas_Object *effect_page = data;
	Evas_Object *end_page = NULL;
	Evas_Object *page_inner = NULL;

	page_info_s *end_page_info = NULL;

	int cur_x, cur_y, cur_w, cur_h;
	int end_x, end_y, end_w, end_h;
	int vec_x, vec_y, vec_w, vec_h;
	int divide_factor;

	retv_if(!effect_page, ECORE_CALLBACK_CANCEL);

	end_page = evas_object_data_get(effect_page, DATA_KEY_PROXY_PAGE);
	retv_if(!end_page, ECORE_CALLBACK_CANCEL);

	end_page_info = evas_object_data_get(end_page, DATA_KEY_PAGE_INFO);
	goto_if(!end_page_info, ERROR);

	page_inner = end_page_info->page_inner;
	goto_if(!page_inner, ERROR);

	evas_object_geometry_get(page_inner, &end_x, &end_y, &end_w, &end_h);
	evas_object_geometry_get(effect_page, &cur_x, &cur_y, &cur_w, &cur_h);

	divide_factor = (int) evas_object_data_get(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR);
	if (divide_factor > 2) {
		evas_object_data_set(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR, (void *) divide_factor - 1);
	}

	vec_x = (end_x - cur_x) / divide_factor;
	vec_y = (end_y - cur_y) / divide_factor;
	vec_w = (end_w - cur_w) / divide_factor;
	vec_h = (end_h - cur_h) / divide_factor;

	if (!vec_x) vec_x = 1;
	if (!vec_y) vec_y = 1;
	if (!vec_w) vec_w = -1;
	if (!vec_h) vec_h = -1;

	cur_x += vec_x;
	cur_y += vec_y;
	cur_w += vec_w;
	cur_h += vec_h;

	if (cur_x > end_x) cur_x = end_x;
	if (cur_y > end_y) cur_y = end_y;
	if (cur_w < end_w) cur_w = end_w;
	if (cur_h < end_h) cur_h = end_h;

	if (cur_x >= end_x) {
		evas_object_data_del(effect_page, PRIVATE_DATA_KEY_ANIM_FOR_MINIFY);
		evas_object_data_del(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR);
		edit_destroy_minify_effect_page(effect_page);
		return ECORE_CALLBACK_CANCEL;
	}

	evas_object_move(effect_page, cur_x, cur_y);
	evas_object_resize(effect_page, cur_w, cur_h);

	return ECORE_CALLBACK_RENEW;

ERROR:
	_E("Critical! Effect page will be removed.");
	evas_object_del(effect_page);
	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _enlarge_page_anim_cb(void *data)
{
	Evas_Object *effect_page = data;
	Evas_Object *bg_rect = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *real_page = NULL;
	page_info_s *page_info = NULL;
	layout_info_s *layout_info = NULL;

	int cur_x, cur_y, cur_w, cur_h;
	int end_x, end_y, end_w, end_h;
	int vec_x, vec_y, vec_w, vec_h;
	int divide_factor;
	double rel;

	retv_if(!effect_page, ECORE_CALLBACK_CANCEL);

	layout = main_get_info()->layout;
	retv_if(!layout, ECORE_CALLBACK_CANCEL);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, ECORE_CALLBACK_CANCEL);
	retv_if(!layout_info->scroller, ECORE_CALLBACK_CANCEL);

	bg_rect = evas_object_data_get(effect_page, DATA_KEY_BG);
	goto_if(!bg_rect, OUT);

	real_page = evas_object_data_get(effect_page, DATA_KEY_REAL_PAGE);
	goto_if(!real_page, OUT);

	page_info = evas_object_data_get(real_page, DATA_KEY_PAGE_INFO);
	goto_if(!page_info, OUT);

	rel = (double) main_get_info()->root_w / ITEM_EDIT_WIDTH;
	end_w = PAGE_EDIT_WIDTH * rel;
	end_h = PAGE_EDIT_HEIGHT * rel;

	end_x = (end_w - main_get_info()->root_w) / -2;
	end_y = (end_h - main_get_info()->root_h) / -2;

	evas_object_geometry_get(effect_page, &cur_x, &cur_y, &cur_w, &cur_h);

	divide_factor = (int) evas_object_data_get(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR);
	if (divide_factor > 1) {
		evas_object_data_set(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR, (void *) divide_factor - 1);
	}

	vec_x = (end_x - cur_x) / divide_factor;
	vec_y = (end_y - cur_y) / divide_factor;
	vec_w = (end_w - cur_w) / divide_factor;
	vec_h = (end_h - cur_h) / divide_factor;

	if (!vec_x) vec_x = -1;
	if (!vec_y) vec_y = -1;
	if (!vec_w) vec_w = 1;
	if (!vec_h) vec_h = 1;

	cur_x += vec_x;
	cur_y += vec_y;
	cur_w += vec_w;
	cur_h += vec_h;

	if (cur_x < end_x) {
		goto OUT;
	}

	evas_object_move(effect_page, cur_x, cur_y);
	evas_object_resize(effect_page, cur_w, cur_h);
	evas_object_move(bg_rect, cur_x, cur_y);
	evas_object_resize(bg_rect, cur_w, cur_h);

	return ECORE_CALLBACK_RENEW;

OUT:
	evas_object_data_del(effect_page, PRIVATE_DATA_KEY_ANIM_FOR_ENLARGE);
	evas_object_data_del(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR);

	edit_destroy_layout(layout);
	/* Layout has to be updated before after destroying the layout */
	edje_object_message_signal_process(elm_layout_edje_get(layout));
	edit_destroy_enlarge_effect_page(effect_page);

	return ECORE_CALLBACK_CANCEL;
}



HAPI void edit_minify_effect_page(void *effect_page)
{
	Ecore_Animator *anim = NULL;

	ret_if(!effect_page);

	anim = evas_object_data_get(effect_page, PRIVATE_DATA_KEY_ANIM_FOR_MINIFY);
	if (anim) {
		_D("You've already an animator");
		return;
	}

	anim = ecore_animator_add(_minify_page_anim_cb, effect_page);
	if (anim) {
		evas_object_data_set(effect_page, PRIVATE_DATA_KEY_ANIM_FOR_MINIFY, anim);
		evas_object_data_set(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR, (void *) DIVIDE_FACTOR);
	} else {
		edit_destroy_minify_effect_page(effect_page);
	}
}



HAPI Evas_Object *edit_create_minify_effect_page(Evas_Object *page)
{
	Evas_Object *effect_page = NULL;
	page_info_s *page_info = NULL;
	Eina_Bool ret;
	double rel;
	int width, height, x, y;

	retv_if(!page, NULL);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	if (page_info->removable) {
		elm_object_signal_emit(page_info->page_inner, "hide", "del");
	}

	elm_object_signal_emit(page_info->page_inner, "deselect", "cover");
	elm_object_signal_emit(page_info->page_inner, "deselect", "cover_clipper");
	elm_object_signal_emit(page_info->page_inner, "deselect", "line");
	edje_object_message_signal_process(elm_layout_edje_get(page_info->page_inner));

	if (evas_object_data_get(page, PRIVATE_DATA_KEY_EDIT_UNFOCUSABLE)) {
		elm_object_signal_emit(page_info->page_inner, "disable", "cover");
		elm_object_signal_emit(page_info->page_inner, "deselect", "line");
	} else {
		elm_object_signal_emit(page_info->page_inner, "select", "cover");
		elm_object_signal_emit(page_info->page_inner, "select", "line");
	}
	elm_object_signal_emit(page_info->page_inner, "select", "cover_clipper");
	scroller_disable_cover_on_scroll(page_info->scroller);

	effect_page = evas_object_image_filled_add(main_get_info()->e);
	retv_if(!effect_page, NULL);

	ret = evas_object_image_source_set(effect_page, page);
	if (EINA_FALSE == ret) {
		_E("Cannot set the source into the proxy image");
		evas_object_del(effect_page);
		return NULL;
	}
	evas_object_image_source_visible_set(effect_page, EINA_FALSE);
	evas_object_image_source_clip_set(effect_page, EINA_FALSE);

	rel = (double) main_get_info()->root_w / ITEM_EDIT_WIDTH;
	width = PAGE_EDIT_WIDTH * rel;
	height = PAGE_EDIT_HEIGHT * rel;

	x = (width - main_get_info()->root_w) / -2;
	y = (height - main_get_info()->root_h) / -2;

	evas_object_size_hint_min_set(effect_page, width, height);
	evas_object_resize(effect_page, width, height);
	evas_object_move(effect_page, x, y);
	evas_object_show(effect_page);

	evas_object_data_set(effect_page, DATA_KEY_PROXY_PAGE, page);

	return effect_page;

}



HAPI void edit_destroy_minify_effect_page(Evas_Object *effect_page)
{
	Evas_Object *page = NULL;
	Evas_Object *page_current = NULL;
	page_info_s *page_info = NULL;

	ret_if(!effect_page);

	page = evas_object_data_del(effect_page, DATA_KEY_PROXY_PAGE);
	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	scroller_enable_cover_on_scroll(page_info->scroller);

	if (page_info->removable) {
		elm_object_signal_emit(page_info->page_inner, "show", "del");
	}

	evas_object_image_source_visible_set(effect_page, EINA_TRUE);
	evas_object_del(effect_page);

	page_current = scroller_get_focused_page(page_info->scroller);
	ret_if(!page_current);
	edit_change_focus(page_info->scroller, page_current);
}



HAPI void edit_enlarge_effect_page(void *effect_page)
{
	Evas_Object *real_page = NULL;
	layout_info_s *layout_info = NULL;
	page_info_s *page_info = NULL;
	Ecore_Animator *anim = NULL;

	ret_if(!effect_page);

	real_page = evas_object_data_get(effect_page, DATA_KEY_REAL_PAGE);
	goto_if(!real_page, ERROR);

	page_info = evas_object_data_get(real_page, DATA_KEY_PAGE_INFO);
	goto_if(!page_info, ERROR);
	goto_if(!page_info->layout, ERROR);

	layout_info = evas_object_data_get(page_info->layout, DATA_KEY_LAYOUT_INFO);
	goto_if(!layout_info, ERROR);
	goto_if(!layout_info->scroller, ERROR);

	anim = evas_object_data_get(effect_page, PRIVATE_DATA_KEY_ANIM_FOR_ENLARGE);
	if (anim) {
		_D("You've already an animator");
		return;
	}

	anim = ecore_animator_add(_enlarge_page_anim_cb, effect_page);
	if (anim) {
		evas_object_data_set(effect_page, PRIVATE_DATA_KEY_ANIM_FOR_ENLARGE, anim);
		evas_object_data_set(effect_page, PRIVATE_DATA_KEY_DIVIDE_FACTOR, (void *) DIVIDE_FACTOR);
	} else {
		edit_destroy_enlarge_effect_page(effect_page);
	}

	if (real_page) {
		scroller_region_show_page(layout_info->scroller, real_page, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT);
	}
	edje_object_message_signal_process(elm_layout_edje_get(page_info->layout));

	return;

ERROR:
	_E("Destroy the effect page(%p)", effect_page);
	edit_destroy_enlarge_effect_page(effect_page);
}



HAPI Evas_Object *edit_create_enlarge_effect_page(Evas_Object *page)
{
	Evas_Object *clip_bg = NULL;
	Evas_Object *effect_page = NULL;
	Evas_Object *real_page = NULL;
	page_info_s *page_info = NULL;
	layout_info_s *layout_info = NULL;
	Eina_Bool ret;
	int x, y, w, h;

	retv_if(!page, NULL);

	if (main_get_info()->layout) {
		layout_info = evas_object_data_get(main_get_info()->layout, DATA_KEY_LAYOUT_INFO);
		retv_if(!layout_info, NULL);
		if (layout_info->edit) elm_object_signal_emit(layout_info->edit, "hide", "edit");
	}

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	real_page = evas_object_data_get(page, DATA_KEY_REAL_PAGE);
	retv_if(!real_page, NULL);

	if (page_info->removable) {
		elm_object_signal_emit(page_info->page_inner, "hide", "del");
	}

	elm_object_signal_emit(page_info->page_inner, "deselect", "cover");
	elm_object_signal_emit(page_info->page_inner, "deselect", "cover_clipper");
	elm_object_signal_emit(page_info->page_inner, "deselect,im", "line");
	edje_object_message_signal_process(elm_layout_edje_get(page_info->page_inner));

	clip_bg = _create_proxy_bg(page_info->item);
	retv_if(!clip_bg, NULL);

	effect_page = evas_object_image_filled_add(main_get_info()->e);
	if (!effect_page) {
		_E("Cannot add effect page");
		_destroy_proxy_bg(clip_bg);
		return NULL;
	}

	evas_object_data_set(effect_page, DATA_KEY_REAL_PAGE, real_page);
	evas_object_data_set(effect_page, DATA_KEY_BG, clip_bg);

	ret = evas_object_image_source_set(effect_page, page_info->page_inner);
	if (EINA_FALSE == ret) {
		_E("Cannot set the source into the effect image");
		edit_destroy_enlarge_effect_page(effect_page);
		return NULL;
	}
	evas_object_image_source_visible_set(effect_page, EINA_FALSE);
	evas_object_image_source_clip_set(effect_page, EINA_FALSE);

	evas_object_geometry_get(page, &x, &y, &w, &h);

	evas_object_resize(effect_page, w, h);
	evas_object_move(effect_page, x, y);
	evas_object_show(effect_page);

	return effect_page;
}



HAPI void edit_destroy_enlarge_effect_page(Evas_Object *effect_page)
{
	Evas_Object *page = NULL;
	Evas_Object *clip_bg = NULL;
	page_info_s *page_info = NULL;

	ret_if(!effect_page);

	clip_bg = evas_object_data_del(effect_page, DATA_KEY_BG);
	if (clip_bg) _destroy_proxy_bg(clip_bg);

	page = evas_object_data_del(effect_page, DATA_KEY_REAL_PAGE);
	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	evas_object_image_source_visible_set(effect_page, EINA_TRUE);
	evas_object_del(effect_page);
}



static Evas_Object *_create_refresh_left_page(Evas_Object *edit, Evas_Object *page)
{
	Evas_Object *proxy_page = NULL;
	page_info_s *proxy_page_info = NULL;
	edit_info_s *edit_info = NULL;
	layout_info_s *layout_info = NULL;
	scroller_info_s *scroller_info = NULL;

	retv_if(!page, NULL);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	retv_if(!edit_info, NULL);

	layout_info = evas_object_data_get(edit_info->layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, NULL);

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	proxy_page = edit_create_proxy_page(edit_info->scroller, page, PAGE_CHANGEABLE_BG_OFF);
	retv_if(!proxy_page, NULL);

	evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_EDIT_DISABLE_REORDERING, (void *) 1);
	proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	goto_if(!proxy_page_info, ERROR);

	if (scroller_info->center_neighbor_left == page) {
		evas_object_data_set(proxy_page, PRIVATE_DATA_KEY_EDIT_UNFOCUSABLE, (void *) 1);
		elm_object_signal_emit(proxy_page_info->page_inner, "disable", "cover");
		elm_object_signal_emit(proxy_page_info->page_inner, "select", "cover_clipper");
	}

	elm_access_info_cb_set(proxy_page_info->focus, ELM_ACCESS_CONTEXT_INFO, _access_tab_to_move_cb, NULL);

	evas_object_smart_callback_add(proxy_page_info->focus, "clicked", _clicked_noti_cb, proxy_page);
	evas_object_smart_callback_add(proxy_page_info->remove_focus, "clicked", _del_noti_cb, proxy_page);

	proxy_page_info->direction = PAGE_DIRECTION_LEFT;

	return proxy_page;

ERROR:
	if (proxy_page) {
		edit_destroy_proxy_page(proxy_page);
	}
	return NULL;
}



static Evas_Object *_create_refresh_right_page(Evas_Object *edit, Evas_Object *page)
{
	Evas_Object *proxy_page = NULL;
	page_info_s *proxy_page_info = NULL;
	edit_info_s *edit_info = NULL;
	scroller_info_s *scroller_info = NULL;

	retv_if(edit, NULL);
	retv_if(page, NULL);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	retv_if(!edit_info, NULL);

	scroller_info = evas_object_data_get(edit_info->scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	proxy_page = edit_create_proxy_page(edit_info->scroller, page, PAGE_CHANGEABLE_BG_ON);
	retv_if(!proxy_page, NULL);

	proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
	goto_if(!proxy_page_info, ERROR);

	evas_object_smart_callback_add(proxy_page_info->focus, "clicked", _clicked_dbox_cb, proxy_page);
	evas_object_smart_callback_add(proxy_page_info->remove_focus, "clicked",  _del_dbox_cb, proxy_page);

	proxy_page_info->direction = PAGE_DIRECTION_RIGHT;

	return proxy_page;

ERROR:
	if (proxy_page) {
		edit_destroy_proxy_page(proxy_page);
	}
	return NULL;
}



HAPI void edit_refresh_scroller(Evas_Object *edit, page_direction_e direction)
{
	Evas_Object *page = NULL;
	Evas_Object *proxy_page = NULL;
	Eina_List *list = NULL;
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	edit_info_s *edit_info = NULL;
	layout_info_s *layout_info = NULL;
	page_info_s *page_info = NULL;
	scroller_info_s *scroller_info = NULL;

	ret_if(!edit);

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);

	layout_info = evas_object_data_get(edit_info->layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	list = elm_box_children_get(scroller_info->box);
	ret_if(!list);

	/*Add proxy page which didn't exist in eidt scroller list*/
	switch (direction) {
	case PAGE_DIRECTION_LEFT:
		EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
			continue_if(!page);
			page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
			continue_if(!page_info);
			if (page_info->direction != PAGE_DIRECTION_LEFT) continue;
			proxy_page = evas_object_data_get(page, DATA_KEY_PROXY_PAGE);
			if (!proxy_page) {
				proxy_page = _create_refresh_left_page(edit, page);
				break_if(!proxy_page);
			} else scroller_pop_page(edit_info->scroller, proxy_page);
			scroller_push_page(edit_info->scroller, proxy_page, SCROLLER_PUSH_TYPE_CENTER_LEFT);
		}
		break;
	case PAGE_DIRECTION_RIGHT:
		EINA_LIST_FOREACH_SAFE(list, l, ln, page) {
			continue_if(!page);
			page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
			continue_if(!page_info);
			if (page_info->direction != PAGE_DIRECTION_RIGHT) continue;
			proxy_page = evas_object_data_get(page, DATA_KEY_PROXY_PAGE);
			if (!proxy_page) {
				proxy_page = _create_refresh_right_page(edit, page);
				break_if(!proxy_page);
				if (edit_info->plus_page) {
					scroller_push_page_before(edit_info->scroller, proxy_page, edit_info->plus_page);
				} else {
					scroller_push_page_before(edit_info->scroller, proxy_page, NULL);
				}
			}
		}
		break;
	default:
		break;
	}
	eina_list_free(list);
}



HAPI void edit_arrange_plus_page(Evas_Object *edit)
{
	layout_info_s *layout_info = NULL;
	edit_info_s *edit_info = NULL;
	page_info_s *page_info = NULL;
	int count = 0;

	ret_if(!edit);

	_D("Arrange the plus page");

	edit_info = evas_object_data_get(edit, DATA_KEY_EDIT_INFO);
	ret_if(!edit_info);
	ret_if(!edit_info->plus_page);
	ret_if(!edit_info->scroller);

	page_info = evas_object_data_get(edit_info->plus_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	count = scroller_count_direction(edit_info->scroller, PAGE_DIRECTION_RIGHT);
	if (count > MAX_DBOX) {
		_D("Diable the plus page, count:%d", count);

		elm_object_signal_callback_del(page_info->item, "click", "plus_item", _plus_item_clicked_cb);
		elm_object_signal_callback_del(page_info->item, "down", "plus_item", _plus_item_down_cb);
		elm_object_signal_callback_del(page_info->item, "up", "plus_item", _plus_item_up_cb);

		elm_object_signal_emit(page_info->item, "max", "plus_in_edit");
		elm_object_focus_allow_set(page_info->focus, EINA_FALSE);

		edit_info->is_max = 1;
	} else {
		_D("Enable the plus page, count:%d", count);

		ret_if(!edit_info->layout);
		layout_info = evas_object_data_get(edit_info->layout, DATA_KEY_LAYOUT_INFO);
		ret_if(!layout_info);

		/* Callbacks could be added infinitely */
		if (edit_info->is_max) {
			elm_object_signal_callback_add(page_info->item, "click", "plus_item", _plus_item_clicked_cb, layout_info);
			elm_object_signal_callback_add(page_info->item, "down", "plus_item", _plus_item_down_cb, edit_info->plus_page);
			elm_object_signal_callback_add(page_info->item, "up", "plus_item", _plus_item_up_cb, edit_info->plus_page);

			edit_info->is_max = 0;
		}

		elm_object_signal_emit(page_info->item, "no_max", "plus_in_edit");
		elm_object_focus_allow_set(page_info->focus, EINA_TRUE);
	}
}



// End of this file
