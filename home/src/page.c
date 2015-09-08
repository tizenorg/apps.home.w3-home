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
#include <bundle.h>
#include <dlog.h>
#include <efl_assist.h>
#include <vconf.h>
#include <widget_viewer_evas.h>

#include "util.h"
#include "conf.h"
#include "edit.h"
#include "edit_info.h"
#include "effect.h"
#include "layout.h"
#include "layout_info.h"
#include "log.h"
#include "main.h"
#include "page_info.h"
#include "scroller_info.h"
#include "page.h"
#include "scroller.h"
#include "index.h"
#include "bg.h"
#include "add-viewer_package.h"

#define PRIVATE_DATA_KEY_PAGE_ACCESS_OBJECT "pg_pd_ao"
#define PRIVATE_DATA_KEY_PAGE_FOCUS_OBJECT "p_fo"
#define PRIVATE_DATA_KEY_PAGE_LEFT_EFFECT "p_le"
#define PRIVATE_DATA_KEY_PAGE_RIGHT_EFFECT "p_re"



static void _move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *page = obj;
	page_info_s *page_info = NULL;
	Evas_Coord x, y, w, h;

	ret_if(!page);

	evas_object_geometry_get(page, &x, &y, &w, &h);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	page_info->appended = EINA_TRUE;
	_D("Page(%p) is appended into the scroller (%d:%d:%d:%d)", page, x, y, w, h);

	evas_object_event_callback_del(page, EVAS_CALLBACK_MOVE, _move_cb);
}



static void _down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ei = event_info;
	Evas_Object *page = data;

	layout_info_s *layout_info = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;

	int x = ei->output.x;
	int y = ei->output.y;

	_D("Down (%p)(%d, %d)", page, x, y);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	layout_info = evas_object_data_get(page_info->layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	layout_info->pressed_page = page;
	layout_info->pressed_item = page_info->item;
	if (layout_info->pressed_page == scroller_info->plus_page) {
		evas_object_data_set(layout_info->pressed_item, DATA_KEY_IS_LONGPRESS, (void *)0);
	}
}



static void _up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Up *ei = event_info;
	Evas_Object *page = data;

	layout_info_s *layout_info = NULL;
	page_info_s *page_info = NULL;

	int x = ei->output.x;
	int y = ei->output.y;

	_D("Up (%p)(%d, %d)", page, x, y);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	layout_info = evas_object_data_get(page_info->layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	layout_info->pressed_page = page;
	layout_info->pressed_item = NULL;

	if (evas_object_data_get(page, DATA_KEY_PAGE_ONHOLD_COUNT)) {
		evas_object_data_del(page, DATA_KEY_PAGE_ONHOLD_COUNT);
		ei->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
		_D("Event ON_HOLD flag set on a page(%p)", page);
	}
}



#if CIRCLE_TYPE
#define LINE_IMAGE_DIR IMAGEDIR"/widget_circle_bg_stroke.png"
#else
#define LINE_IMAGE_DIR IMAGEDIR"/b_home_screen_widget_line.png"
#endif
static Evas_Object *_create_page_line(Evas_Object *page_inner)
{
	Evas_Object *image = NULL;
	Evas_Object *box = NULL;
	int ret;

	image = evas_object_image_filled_add(main_get_info()->e);
	retv_if(!image, NULL);

	evas_object_image_smooth_scale_set(image, EINA_TRUE);
	evas_object_image_load_size_set(image, ITEM_EDIT_LINE_WIDTH, ITEM_EDIT_LINE_HEIGHT);
	evas_object_image_file_set(image, LINE_IMAGE_DIR, NULL);

	ret = evas_object_image_load_error_get(image);
	goto_if (EVAS_LOAD_ERROR_NONE != ret, ERROR);

	evas_object_image_fill_set(image, 0, 0, ITEM_EDIT_LINE_WIDTH, ITEM_EDIT_LINE_HEIGHT);
	evas_object_image_border_set(image, 9, 9, 9, 9);

	evas_object_size_hint_weight_set(image, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(image, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(image);

	box = elm_box_add(page_inner);
	goto_if(!box, ERROR);

	evas_object_size_hint_min_set(image, ITEM_EDIT_LINE_WIDTH+2, ITEM_EDIT_LINE_HEIGHT+2);
	evas_object_size_hint_max_set(image, ITEM_EDIT_LINE_WIDTH+2, ITEM_EDIT_LINE_HEIGHT+2);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_box_pack_end(box, image);

	elm_object_part_content_set(page_inner, "line", box);

	return box;

ERROR:
	evas_object_del(image);
	return NULL;
}



static void _destroy_page_line(Evas_Object *page)
{
	Evas_Object *image = NULL;
	Evas_Object *box = NULL;
	Eina_List *list = NULL;

	ret_if(!page);
	box = elm_object_part_content_get(page, "line");
	ret_if(!box);

	list = elm_box_children_get(box);
	if (!list) {
		evas_object_del(box);
		return;
	}

	EINA_LIST_FREE(list, image) {
		evas_object_del(image);
	}
	evas_object_del(box);
}



HAPI void page_destroy(Evas_Object *page)
{
	page_info_s *page_info = NULL;

	ret_if(!page);

	page_info = evas_object_data_del(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	_destroy_page_line(page_info->page_inner);

	evas_object_del(page_info->focus);
	evas_object_del(page_info->page_inner_area);
	evas_object_del(page_info->page_inner_bg);

	evas_object_data_del(page_info->page_inner, PRIVATE_DATA_KEY_PAGE_ACCESS_OBJECT);
	evas_object_data_del(page_info->page_inner, DATA_KEY_PAGE_INFO);
	evas_object_del(page_info->page_inner);
	evas_object_del(page_info->page_rect);

	evas_object_data_del(page, DATA_KEY_EVENT_UPPER_IS_ON);

	free(page_info->id);
	free(page_info->subid);
	free(page_info->title);
	free(page_info);
	evas_object_del(page);
}



static char *_access_page_num_cb(void *data, Evas_Object *obj)
{
	Evas_Object *scroller = data;
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

	if (PAGE_DIRECTION_CENTER == page_info->direction) {
		_D("This page is the center page");
		return NULL;
	}

	if (widget_viewer_evas_is_widget(page_get_item(page)) == 1) {
		if (page_info->need_to_read == EINA_FALSE) {
			return NULL;
		}

		if (page_info->need_to_unhighlight == EINA_TRUE) {
			return NULL;
		}

		if (!widget_viewer_evas_is_faulted(page_info->item)) {
			page_info->need_to_read = EINA_FALSE;
		}
	}

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



static char *_access_plus_button_name_cb(void *data, Evas_Object *obj)
{
	char *tmp;

	tmp = strdup(_("IDS_HS_BODY_ADD_WIDGET"));
	retv_if(!tmp, NULL);
	return tmp;
}



#define PAGE_EDJE_FILE EDJEDIR"/page.edj"
HAPI Evas_Object *page_create(Evas_Object *scroller
	, Evas_Object *item
	, const char *id
	, const char *subid
	, int width, int height
	, page_changeable_bg_e changeable_bg
	, page_removable_e removable)
{
	Evas_Object *page = NULL;
	Evas_Object *focus = NULL;
	Evas_Object *page_rect = NULL;
	Evas_Object *page_inner = NULL;
	Evas_Object *page_inner_area = NULL;
	Evas_Object *page_inner_bg = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;

	retv_if(!scroller, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	page = elm_layout_add(scroller);
	retv_if(!page, NULL);
	elm_layout_file_set(page, PAGE_EDJE_FILE, "page");
	/* Do not have weight for unpacking from elm_box */
	evas_object_size_hint_weight_set(page, 0.0, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(page, width, height);
	evas_object_move(page, -10000, -10000);
	evas_object_show(page);
	evas_object_event_callback_add(page, EVAS_CALLBACK_MOVE, _move_cb, NULL);

	page_info = calloc(1, sizeof(page_info_s));
	if (!page_info) {
		_E("Cannot calloc for page_info");
		evas_object_del(page);
		return NULL;
	}
	evas_object_data_set(page, DATA_KEY_PAGE_INFO, page_info);
	evas_object_data_set(page, DATA_KEY_EVENT_UPPER_IS_ON, (void *) 1);

	page_rect = evas_object_rectangle_add(main_get_info()->e);
	goto_if(!page_rect, ERROR);
	evas_object_size_hint_min_set(page_rect, width, height);
	evas_object_show(page_rect);
	elm_object_part_content_set(page, "bg", page_rect);
	evas_object_color_set(page_rect, 0, 0, 0, 0);

	page_inner = elm_layout_add(scroller);
	goto_if(NULL == page_inner, ERROR);
	elm_layout_file_set(page_inner, PAGE_EDJE_FILE, "page_inner");
	evas_object_size_hint_weight_set(page_inner, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_repeat_events_set(page_inner, EINA_TRUE);
	evas_object_show(page_inner);
	elm_object_part_content_set(page, "inner", page_inner);
	evas_object_event_callback_add(page_inner, EVAS_CALLBACK_MOUSE_DOWN, _down_cb, page);
	evas_object_event_callback_add(page_inner, EVAS_CALLBACK_MOUSE_UP, _up_cb, page);
	evas_object_data_set(page_inner, DATA_KEY_PAGE_INFO, page_info);

	page_inner_area = evas_object_rectangle_add(main_get_info()->e);
	goto_if(!page_inner_area, ERROR);
	evas_object_size_hint_min_set(page_inner_area, width, height);
	evas_object_resize(page_inner_area, width, height);
	evas_object_color_set(page_inner_area, 0, 0, 0, 0);
	evas_object_repeat_events_set(page_inner_area, EINA_TRUE);
	evas_object_show(page_inner_area);
	elm_object_part_content_set(page_inner, "area", page_inner_area);

	page_inner_bg = evas_object_rectangle_add(main_get_info()->e);
	goto_if(!page_inner_bg, ERROR);
	evas_object_size_hint_weight_set(page_inner_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_color_set(page_inner_bg, 0, 0, 0, 0);
	evas_object_repeat_events_set(page_inner_bg, EINA_TRUE);
	evas_object_show(page_inner_bg);
	elm_object_part_content_set(page_inner, "bg", page_inner_bg);

	if (item) {
		elm_object_part_content_set(page_inner, "item", item);
	}

	if (!_create_page_line(page_inner)) {
		_E("cannot create line");
	}

	if (id) {
		page_info->id = strdup(id);
		if (!page_info->id) {
			_E("Critical, cannot strdup for id");
		}
	}

	if (subid) {
		page_info->subid = strdup(subid);
		if (!page_info->subid) {
			_E("Critical, cannot strdup for subid");
		}
	}

	if (changeable_bg) {
		bg_register_object(page_inner);
	}

	evas_object_data_set(page_inner, PRIVATE_DATA_KEY_PAGE_ACCESS_OBJECT, NULL);

	focus = elm_button_add(page_inner);
	retv_if(NULL == focus, NULL);

	elm_object_style_set(focus, "focus");
	elm_object_part_content_set(page_inner, "focus", focus);

	elm_access_info_cb_set(focus, ELM_ACCESS_TYPE, NULL, NULL);
	elm_access_info_cb_set(focus, ELM_ACCESS_STATE, _access_page_num_cb, scroller);
	evas_object_size_hint_weight_set(focus, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_focus_allow_set(focus, EINA_TRUE);

	page_info->width = width;
	page_info->height = height;

	page_info->layout = scroller_info->layout;
	page_info->scroller = scroller;
	page_info->page = page;
	page_info->page_rect = page_rect;
	page_info->page_inner = page_inner;
	page_info->page_inner_area = page_inner_area;
	page_info->page_inner_bg = page_inner_bg;
	page_info->item = item;
	page_info->removable = removable;
	page_info->focus = focus;
	page_info->layout_longpress = 1;
	page_info->appended = EINA_FALSE;
	page_info->need_to_read = EINA_FALSE;
	page_info->highlighted = EINA_FALSE;

	return page;

ERROR:
	page_destroy(page);
	return NULL;
}



static void _plus_item_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *layout = data;
	layout_info_s *layout_info = NULL;

	ret_if(!layout);

	_D("clicked plus page");

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	if (evas_object_data_get(obj, DATA_KEY_IS_LONGPRESS)) return;

	if (layout_info->pressed_item) {
		_D("There is already a pressed item");
		widget_viewer_evas_feed_mouse_up_event(layout_info->pressed_item);
	}

	if (!edit_create_add_viewer(layout)) _E("Cannot add the add-viewer");

	effect_play_sound();
}



static void _plus_item_down_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *page = data;
	page_info_s *page_info = NULL;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->item);

	elm_object_signal_emit(page_info->item, "press", "plus");
}



static void _plus_item_up_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *page = data;
	page_info_s *page_info = NULL;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->item);

	elm_object_signal_emit(page_info->item, "release", "plus");
}



#define PLUS_ITEM_EDJ EDJEDIR"/page.edj"
#define PLUS_ITEM_GROUP "plus_item"
HAPI Evas_Object *page_create_plus_page(Evas_Object *scroller)
{
	Evas_Object *page = NULL;
	Evas_Object *plus_item = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	Eina_Bool ret;

	retv_if(!scroller, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);
	retv_if(scroller_info->plus_page, NULL);

	plus_item = elm_layout_add(scroller);
	retv_if(!plus_item, NULL);

	ret = elm_layout_file_set(plus_item, PLUS_ITEM_EDJ, PLUS_ITEM_GROUP);
	if (EINA_FALSE == ret) {
		_E("cannot set the file into the layout");
		evas_object_del(plus_item);
		return NULL;
	}
	evas_object_size_hint_weight_set(plus_item, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(plus_item, 0.5, 0.5);
	evas_object_repeat_events_set(plus_item, EINA_TRUE);
	evas_object_show(plus_item);
	evas_object_data_set(plus_item, DATA_KEY_IS_LONGPRESS, (void *)0);
#if !CIRCLE_TYPE
	elm_object_signal_emit(plus_item, "emul,normal", "plus");
#endif

	page = page_create(scroller, plus_item, NULL, NULL, scroller_info->page_width, scroller_info->page_height, PAGE_CHANGEABLE_BG_OFF, PAGE_REMOVABLE_OFF);
	if (!page) {
		_E("Cannot create the page");
		evas_object_del(plus_item);
		return NULL;
	}

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	goto_if(!page_info, ERROR);
	page_info->layout_longpress = 0;

	elm_object_signal_emit(page_info->page_inner, "select", "cover");
	elm_object_signal_emit(page_info->page_inner, "show", "line,widget");

	elm_access_info_cb_set(page_info->focus, ELM_ACCESS_INFO, _access_plus_button_name_cb, NULL);
	//evas_object_smart_callback_add(page_info->focus, "clicked", _plus_item_clicked_cb, scroller_info->layout);
	scroller_info->plus_page = page;

	elm_object_signal_emit(plus_item, "show,widget", "plus_item");
	elm_object_signal_callback_add(plus_item, "down", "plus_item", _plus_item_down_cb, page);
	elm_object_signal_callback_add(plus_item, "up", "plus_item", _plus_item_up_cb, page);
	elm_object_signal_callback_add(plus_item, "click", "plus_item", _plus_item_clicked_cb, page_info->layout);

	return page;

ERROR:
	evas_object_del(page);

	return NULL;
}



HAPI void page_destroy_plus_page(Evas_Object *scroller)
{
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	page_info = evas_object_data_get(scroller_info->plus_page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	evas_object_data_del(page_info->item, DATA_KEY_IS_LONGPRESS);

	evas_object_del(page_info->item);
	page_destroy(scroller_info->plus_page);

	scroller_info->plus_page = NULL;
}



HAPI void page_arrange_plus_page(Evas_Object *scroller, int toast_popup)
{
	scroller_info_s *scroller_info = NULL;
	int count = 0;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	count = scroller_count_direction(scroller, PAGE_DIRECTION_RIGHT);
	if (count > MAX_WIDGET) {
		if (scroller_info->plus_page) {
			char text[TEXT_LEN] = {0, };
			char *count_str = NULL;

			scroller_pop_page(scroller, scroller_info->plus_page);
			page_destroy_plus_page(scroller);

			if (toast_popup) {
				count_str = edit_get_count_str_from_icu(MAX_WIDGET);
				if (!count_str) {
					_E("count_str is NULL");
					count_str = calloc(1, LOCALE_LEN);
					ret_if(!count_str);
					snprintf(count_str, LOCALE_LEN, "%d", MAX_WIDGET);
				}

				snprintf(text, sizeof(text), _("IDS_HS_TPOP_MAXIMUM_NUMBER_OF_WIDGETS_HPD_REACHED"), count_str);
				free(count_str);
				util_create_toast_popup(scroller, text);
			}
		} else {
			/* Leave me alone */
		}
	} else if (count == MAX_WIDGET) {
		/* Leave me alone */
	} else {
		if (scroller_info->plus_page) {
			/* Leave me alone */
			scroller_pop_page(scroller, scroller_info->plus_page);
			scroller_push_page(scroller, scroller_info->plus_page, SCROLLER_PUSH_TYPE_LAST);
		} else {
			page_create_plus_page(scroller);
			page_set_effect(scroller_info->plus_page, page_effect_none, page_effect_none);
			scroller_push_page(scroller, scroller_info->plus_page, SCROLLER_PUSH_TYPE_LAST);
		}
	}
}



Evas_Object *page_set_item(Evas_Object *page, Evas_Object *item)
{
	Evas_Object *old_item = NULL;
	page_info_s *page_info = NULL;

	retv_if(!page, NULL);
	retv_if(!item, NULL);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	old_item = elm_object_part_content_unset(page_info->page_inner, "item");
	if (old_item) {
		_D("You need to free the old item");
	}

	elm_object_part_content_set(page_info->page_inner, "item", item);
	page_info->item = item;

	return old_item;
}



HAPI Evas_Object *page_get_item(Evas_Object *page)
{
	page_info_s *page_info = NULL;

	retv_if(!page, NULL);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	return page_info->item;
}



HAPI void page_focus(Evas_Object *page)
{
	Eina_Bool is_focused = EINA_FALSE;
	page_info_s *page_info = NULL;

	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->focus);

	is_focused = elm_object_focus_get(page_info->focus);
	if (is_focused) return;

	/*
	 * workaround, to avoid scroller focus become invalid
	 */
	_D("focus set to %p", page_info->focus);
	elm_object_focus_set(page_info->focus, EINA_TRUE);
}



HAPI void page_unfocus(Evas_Object *page)
{
	page_info_s *page_info = NULL;

	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->focus);

	elm_object_focus_set(page_info->focus, EINA_FALSE);
}



HAPI void page_highlight(Evas_Object *page)
{
	page_info_s *page_info = NULL;

	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->focus);

	elm_access_object_register(page_info->focus, page);
}



HAPI void page_unhighlight(Evas_Object *page)
{
	page_info_s *page_info = NULL;

	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->focus);

	elm_access_object_unregister(page_info->focus);
}



/* You have to free returned value */
HAPI char *page_read_title(Evas_Object *page)
{
	char *title = NULL;
	const char *tmp = NULL;
	page_info_s *page_info = NULL;

	retv_if(!page, NULL);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);
	retv_if(!page_info->item, NULL);

	switch (page_info->direction) {
	case PAGE_DIRECTION_LEFT:
		break;
	case PAGE_DIRECTION_CENTER:
		break;
	case PAGE_DIRECTION_RIGHT:
		tmp = widget_viewer_evas_get_title_string(page_info->item);
		if (!tmp || strlen(tmp) == 0) {
			const char *widget_id;
			struct add_viewer_package *pkginfo;

			widget_id = widget_viewer_evas_get_widget_id(page_info->item);
			break_if(!widget_id);

			pkginfo = add_viewer_package_find(widget_id);
			if (pkginfo) {
				tmp = add_viewer_package_list_name(pkginfo);
			}

			if (!tmp) {
				//Fallback to the widget id
				tmp = widget_id;
			}
		}
		break;
	default:
		_E("Cannot reach here");
		break;
	}
	if (!tmp) return NULL;

	title = strdup(tmp);
	retv_if(!title, NULL);

	return title;
}



HAPI void page_set_effect(Evas_Object *page, void (*left_effect)(Evas_Object *), void (*right_effect)(Evas_Object *))
{
	ret_if(!page);

	if (left_effect) {
		evas_object_data_set(page, PRIVATE_DATA_KEY_PAGE_LEFT_EFFECT, left_effect);
	}

	if (right_effect) {
		evas_object_data_set(page, PRIVATE_DATA_KEY_PAGE_RIGHT_EFFECT, right_effect);
	}
}



HAPI void page_unset_effect(Evas_Object *page)
{
	evas_object_data_del(page, PRIVATE_DATA_KEY_PAGE_LEFT_EFFECT);
	evas_object_data_del(page, PRIVATE_DATA_KEY_PAGE_RIGHT_EFFECT);
}



HAPI void *page_get_effect(Evas_Object *page, page_effect_type_e effect_type)
{
	void (*effect)(Evas_Object *) = NULL;

	retv_if(!page, NULL);

	switch (effect_type) {
	case PAGE_EFFECT_TYPE_LEFT:
		effect = evas_object_data_get(page, PRIVATE_DATA_KEY_PAGE_LEFT_EFFECT);
		break;
	case PAGE_EFFECT_TYPE_RIGHT:
		effect = evas_object_data_get(page, PRIVATE_DATA_KEY_PAGE_RIGHT_EFFECT);
		break;
	default:
		_E("Cannot reach here");
		break;
	}

	return effect;
}



HAPI void page_effect_none(Evas_Object *page)
{
	Evas_Map *map = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	int cur_x, cur_y, cur_w, cur_h;

	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->page_inner);

	scroller_info = evas_object_data_get(page_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	/**
	 * Send mouse up event when we start to scroll pages.
	 */
	if (page_info->item) {
		widget_viewer_evas_feed_mouse_up_event(page_info->item);
	}

	evas_object_geometry_get(page, &cur_x, &cur_y, &cur_w, &cur_h);

	map = evas_map_new(4);
	ret_if(!map);

	/* LEFT */
	evas_map_point_coord_set(map, 0, cur_x, cur_y, 0);
	evas_map_point_image_uv_set(map, 0, 0, 0);
	evas_map_point_color_set(map, 0, 255, 255, 255, 255);

	/* RIGHT */
	evas_map_point_coord_set(map, 1, cur_x + cur_w, cur_y, 0);
	evas_map_point_image_uv_set(map, 1, cur_w, 0);
	evas_map_point_color_set(map, 1, 255, 255, 255, 255);

	/* BOTTOM-RIGHT */
	evas_map_point_coord_set(map, 2, cur_x + cur_w, cur_y + cur_h, 0);
	evas_map_point_image_uv_set(map, 2, cur_w, cur_h);
	evas_map_point_color_set(map, 2, 255, 255, 255, 255);

	/* BOTTOM-LEFT */
	evas_map_point_coord_set(map, 3, cur_x, cur_y + cur_h, 0);
	evas_map_point_image_uv_set(map, 3, 0, cur_h);
	evas_map_point_color_set(map, 3, 255, 255, 255, 255);

	evas_object_map_set(page_info->page_inner, map);
	evas_object_map_enable_set(page_info->page_inner, EINA_TRUE);

	evas_map_free(map);

	if (!eina_list_data_find_list(scroller_info->effect_page_list, page_info->page_inner)) {
		scroller_info->effect_page_list
			= eina_list_append(scroller_info->effect_page_list, page_info->page_inner);
	}
}






HAPI void page_clean_effect(Evas_Object *scroller)
{
	Evas_Object *page_inner = NULL;
	Eina_List *list = NULL;
	scroller_info_s *scroller_info = NULL;

	ret_if(!scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	list = scroller_info->effect_page_list;
	if (!list) return;

	EINA_LIST_FREE(list, page_inner) {
		evas_object_map_enable_set(page_inner, EINA_FALSE);
	}

	scroller_info->effect_page_list = NULL;
}



HAPI void page_backup_inner_focus(Evas_Object *page, Evas_Object *prev_page, Evas_Object *next_page)
{
	page_info_s *page_info = NULL;
	page_info_s *prev_page_info = NULL;
	page_info_s *next_page_info = NULL;

	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	if (prev_page) {
		prev_page_info = evas_object_data_get(prev_page, DATA_KEY_PAGE_INFO);
		ret_if(!prev_page_info);
	}
	if (next_page) {
		next_page_info = evas_object_data_get(next_page, DATA_KEY_PAGE_INFO);
		ret_if(!next_page_info);
	}

	if (prev_page) {
		elm_object_focus_next_object_set(page_info->focus, prev_page_info->focus, ELM_FOCUS_PREVIOUS);
		elm_access_highlight_next_set(page_info->focus, ELM_HIGHLIGHT_DIR_PREVIOUS, prev_page_info->focus);
	}

	if (next_page) {
		elm_object_focus_next_object_set(page_info->focus, next_page_info->focus, ELM_FOCUS_NEXT);
		elm_access_highlight_next_set(page_info->focus, ELM_HIGHLIGHT_DIR_NEXT, next_page_info->focus);
	}
}



HAPI void page_restore_inner_focus(Evas_Object *page)
{
	page_info_s *page_info = NULL;

	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	if (page_info->focus_prev) {
		elm_object_focus_next_object_set(page_info->focus, page_info->focus_prev, ELM_FOCUS_PREVIOUS);
		elm_access_highlight_next_set(page_info->focus, ELM_HIGHLIGHT_DIR_PREVIOUS, page_info->focus_prev);
	} else {
		elm_object_focus_next_object_set(page_info->focus, NULL, ELM_FOCUS_PREVIOUS);
		elm_access_highlight_next_set(page_info->focus, ELM_HIGHLIGHT_DIR_PREVIOUS, NULL);
	}

	if (page_info->focus_next) {
		elm_object_focus_next_object_set(page_info->focus, page_info->focus_next, ELM_FOCUS_NEXT);
		elm_access_highlight_next_set(page_info->focus, ELM_HIGHLIGHT_DIR_NEXT, page_info->focus_next);
	} else {
		elm_object_focus_next_object_set(page_info->focus, NULL, ELM_FOCUS_NEXT);
		elm_access_highlight_next_set(page_info->focus, ELM_HIGHLIGHT_DIR_NEXT, NULL);
	}
}



HAPI void page_set_title(Evas_Object *page, const char *title)
{
	page_info_s *page_info = NULL;

	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	free(page_info->title);

	/* Title can be NULL */
	if (title) {
		page_info->title = strdup(title);
	} else {
		page_info->title = NULL;
	}
}



HAPI const char *page_get_title(Evas_Object *page)
{
	page_info_s *page_info = NULL;

	retv_if(!page, NULL);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	return page_info->title;
}



HAPI int page_is_appended(Evas_Object *page)
{
	page_info_s *page_info = NULL;

	retv_if(!page, 0);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, 0);

	return page_info->appended == EINA_TRUE ? 1 : 0;
}



// End of this file
