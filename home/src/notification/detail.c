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
#include <time.h>

#include "log.h"
#include "util.h"
#include "conf.h"
#include "main.h"
#include "key.h"
#include "notification/detail.h"
#include "notification/time.h"

#define NOTIFICATION_EDJ_FILE EDJEDIR"/noti.edj"
#define NOTIFICATION_DETAIL_GROUP "detail"
#define PRIVATE_DATA_KEY_WIN "pdkw"
#define PRIVATE_DATA_KEY_SCROLLER "pdks"
#define PRIVATE_DATA_KEY_RECT "pdkr"
#define PRIVATE_DATA_KEY_ITEM_INFO "pdkii"



const char *const DETAIL_WINDOW_NAME = "Detail notification window";



struct {
	Eina_List *list;
	Eina_List *event_list;
} detail_s = {
	.list = NULL,
	.event_list = NULL,
};



typedef struct {
	int type;
	void (*event_cb)(void *, void *);
	void *data;
} detail_event_s;



HAPI int detail_register_event_cb(int type, void (*event_cb)(void *, void *), void *data)
{
	detail_event_s *detail_event_info = NULL;

	retv_if(!event_cb, W_HOME_ERROR_INVALID_PARAMETER);

	detail_event_info = calloc(1, sizeof(detail_event_s));
	retv_if(!detail_event_info, W_HOME_ERROR_OUT_OF_MEMORY);

	detail_event_info->type = type;
	detail_event_info->event_cb = event_cb;
	detail_event_info->data = data;

	detail_s.event_list = eina_list_append(detail_s.event_list, detail_event_info);

	return W_HOME_ERROR_NONE;
}



HAPI void detail_unregister_event_cb(int type, void (*event_cb)(void *, void *))
{
	detail_event_s *detail_event_info = NULL;
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;

	ret_if(!event_cb);

	EINA_LIST_FOREACH_SAFE(detail_s.event_list, l, ln, detail_event_info) {
		if (detail_event_info->type == type
			&& detail_event_info->event_cb == event_cb)
		{
			detail_s.event_list = eina_list_remove(detail_s.event_list, detail_event_info);
			break;
		}
	}
}



HAPI detail_item_s *detail_list_append_info(int priv_id, const char *pkgname, const char *icon, const char *title, const char *content, time_t time)
{
	detail_item_s *detail_item_info = NULL;

	retv_if(!pkgname, NULL);

	detail_item_info = calloc(1, sizeof(detail_item_s));
	retv_if(!detail_item_info, NULL);

	detail_item_info->priv_id = priv_id;
	detail_item_info->pkgname = strdup(pkgname);
	goto_if(!detail_item_info->pkgname, ERROR);

	if (icon) {
		detail_item_info->icon = strdup(icon);
		goto_if(!detail_item_info->icon, ERROR);
	}

	if (title) {
		detail_item_info->title = strdup(title);
		goto_if(!detail_item_info->title, ERROR);
	}

	if (content) {
		detail_item_info->content = strdup(content);
		goto_if(!detail_item_info->content, ERROR);
	}

	detail_item_info->time = time;

	detail_s.list = eina_list_prepend(detail_s.list, detail_item_info);

	return detail_item_info;

ERROR:
	if (detail_item_info) {
		free(detail_item_info->content);
		free(detail_item_info->title);
		free(detail_item_info->icon);
		free(detail_item_info->pkgname);
		free(detail_item_info);
	}

	return NULL;
}



HAPI detail_item_s *detail_list_remove_list(int priv_id)
{
	detail_item_s *detail_item_info = NULL;
	const Eina_List *l = NULL;

	EINA_LIST_FOREACH(detail_s.list, l, detail_item_info) {
		if (detail_item_info->priv_id == priv_id) {
			detail_s.list = eina_list_remove(detail_s.list, detail_item_info);
			return detail_item_info;
		}
	}

	return NULL;
}



HAPI void detail_list_remove_info(int priv_id)
{
	detail_item_s *detail_item_info = NULL;
	const Eina_List *l = NULL;

	EINA_LIST_FOREACH(detail_s.list, l, detail_item_info) {
		if (detail_item_info->priv_id == priv_id) {
			detail_s.list = eina_list_remove(detail_s.list, detail_item_info);
			free(detail_item_info->content);
			free(detail_item_info->title);
			free(detail_item_info->icon);
			free(detail_item_info->pkgname);
			free(detail_item_info);
			break;
		}
	}
}



HAPI void detail_list_remove_pkgname(const char *pkgname)
{
	Eina_List *l = NULL;
	Eina_List *ln = NULL;
	detail_item_s *detail_item_info = NULL;

	EINA_LIST_FOREACH_SAFE(detail_s.list, l, ln, detail_item_info) {
		continue_if(!detail_item_info->pkgname);
		if (!strcmp(detail_item_info->pkgname, pkgname)) {
			detail_s.list = eina_list_remove(detail_s.list, detail_item_info);
			free(detail_item_info->content);
			free(detail_item_info->title);
			free(detail_item_info->icon);
			free(detail_item_info->pkgname);
			free(detail_item_info);
		}
	}
}



HAPI detail_item_s *detail_list_get_info(int priv_id)
{
	const Eina_List *l = NULL;
	detail_item_s *detail_item_info = NULL;

	EINA_LIST_FOREACH(detail_s.list, l, detail_item_info) {
		if (detail_item_info->priv_id == priv_id) {
			return detail_item_info;
		}
	}

	return NULL;
}



HAPI int detail_list_count_pkgname(const char *pkgname)
{
	const Eina_List *l = NULL;
	detail_item_s *detail_item_info = NULL;
	int count = 0;

	EINA_LIST_FOREACH(detail_s.list, l, detail_item_info) {
		continue_if(!detail_item_info->pkgname);
		if (!strcmp(detail_item_info->pkgname, pkgname)) {
			count++;
		}
	}

	return count;
}



HAPI detail_item_s *detail_list_get_latest_info(const char *pkgname)
{
	detail_item_s *detail_item_info = NULL;
	const Eina_List *l = NULL;

	EINA_LIST_FOREACH(detail_s.list, l, detail_item_info) {
		continue_if(!detail_item_info->pkgname);
		if (!strcmp(detail_item_info->pkgname, pkgname)) {
			return detail_item_info;
		}
	}

	return NULL;
}



static void _delete_detail_item_cb(void *data, Evas_Object *obj, void *event_info)
{
	detail_item_s *detail_item_info = data;
	detail_event_s *detail_event_info = NULL;
	Evas_Object *win = NULL;
	const Eina_List *l = NULL;
	int count = 0;

	ret_if(!detail_item_info);
	ret_if(!detail_item_info->item);

	win = evas_object_data_get(detail_item_info->item, PRIVATE_DATA_KEY_WIN);
	ret_if(!win);

	/* Cannot search the removed detail_item_info, anymore */
	detail_s.list = eina_list_remove(detail_s.list, detail_item_info);

	EINA_LIST_FOREACH(detail_s.event_list, l, detail_event_info) {
		continue_if(!detail_event_info->event_cb);
		if (DETAIL_EVENT_REMOVE_ITEM != detail_event_info->type) {
			continue;
		}
		detail_event_info->event_cb(detail_item_info, detail_event_info->data);
	}

	count = detail_list_count_pkgname(detail_item_info->pkgname);
	if (count <= 0) {
		detail_destroy_win(win);
	}

	detail_destroy_item(detail_item_info);
	detail_list_remove_info(detail_item_info->priv_id);
}



HAPI void detail_destroy_item(detail_item_s *detail_item_info)
{
	Evas_Object *btn = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *rect = NULL;

	ret_if(!detail_item_info);
	ret_if(!detail_item_info->item);

	_D("Destroy a detail item[%s:%s:%s]", detail_item_info->icon, detail_item_info->title, detail_item_info->content);

	btn = elm_object_part_content_unset(detail_item_info->item, "del,btn");
	if (btn) evas_object_del(btn);

	icon = elm_object_part_content_unset(detail_item_info->item, "icon");
	if (icon) evas_object_del(icon);

	rect = elm_object_part_content_unset(detail_item_info->item, "bg");
	if (rect) evas_object_del(rect);

	evas_object_data_del(detail_item_info->item, PRIVATE_DATA_KEY_WIN);
	evas_object_data_del(detail_item_info->item, PRIVATE_DATA_KEY_ITEM_INFO);
	evas_object_del(detail_item_info->item);
	detail_item_info->item = NULL;
}



HAPI Evas_Object *detail_create_item(Evas_Object *parent, detail_item_s *detail_item_info)
{
	Evas_Object *item = NULL;
	Evas_Object *rect = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *button = NULL;

	char *text_time = NULL;
	char buf[BUFSZE] = {0, };

	Eina_Bool ret = EINA_FALSE;

	retv_if(!parent, NULL);
	retv_if(!detail_item_info, NULL);

	item = elm_layout_add(parent);
	retv_if(!item, NULL);
	detail_item_info->item = item;

	ret = elm_layout_file_set(item, NOTIFICATION_EDJ_FILE, NOTIFICATION_DETAIL_GROUP);
	goto_if(EINA_FALSE == ret, ERROR);

	evas_object_size_hint_weight_set(item, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(item, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_data_set(item, PRIVATE_DATA_KEY_WIN, evas_object_data_get(parent, PRIVATE_DATA_KEY_WIN));
	evas_object_data_set(item, PRIVATE_DATA_KEY_ITEM_INFO, detail_item_info);
	evas_object_show(item);

	/* BG */
	rect = evas_object_rectangle_add(evas_object_evas_get(parent));
	goto_if(!rect, ERROR);
	evas_object_size_hint_min_set(rect, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_color_set(rect, 0, 0, 0, 0);
	evas_object_show(rect);
	elm_object_part_content_set(item, "bg", rect);

	_D("Create a detail item[%s:%s:%s]", detail_item_info->icon, detail_item_info->title, detail_item_info->content);

	/* Icon */
	if (detail_item_info->icon) {
		icon = elm_icon_add(item);
		goto_if(!icon, ERROR);
		goto_if(elm_image_file_set(icon, detail_item_info->icon, NULL) == EINA_FALSE, ERROR);

		elm_image_preload_disabled_set(icon, EINA_TRUE);
		elm_image_smooth_set(icon, EINA_TRUE);
		elm_image_no_scale_set(icon, EINA_FALSE);

		evas_object_size_hint_min_set(icon, NOTIFICATION_ICON_WIDTH, NOTIFICATION_ICON_HEIGHT);
		evas_object_show(icon);

		elm_object_part_content_set(item, "icon", icon);
		elm_object_signal_emit(item, "show,icon", "icon");
	}

	/* Title */
	if (detail_item_info->title) {
		elm_object_part_text_set(item, "title", detail_item_info->title);
	}

	/* Time */
	if (detail_item_info->time) {
		notification_time_to_string(&detail_item_info->time, buf, sizeof(buf));
		text_time = elm_entry_utf8_to_markup(buf);
	}
	if (text_time) {
		elm_object_part_text_set(item, "time", text_time);
		free(text_time);
	}

	/* Content */
	if (detail_item_info->content) {
		elm_object_part_text_set(item, "text", detail_item_info->content);
	}

	/* Delete Button */
	button = elm_button_add(item);
	goto_if(!button, ERROR);

	elm_object_style_set(button, "focus");
	elm_object_part_content_set(item, "del,btn", button);
	evas_object_smart_callback_add(button, "clicked", _delete_detail_item_cb, detail_item_info);

	return item;

ERROR:
	if (icon) evas_object_del(icon);
	if (rect) evas_object_del(rect);
	if (item) evas_object_del(item);
	return NULL;
}



HAPI void detail_destroy_scroller(Evas_Object *scroller)
{
	detail_item_s *detail_item_info = NULL;
	Evas_Object *box = NULL;
	Evas_Object *item = NULL;
	Eina_List *list = NULL;

	ret_if(!scroller);

	box = elm_object_content_unset(scroller);
	goto_if(!box, OUT);
	evas_object_data_del(box, PRIVATE_DATA_KEY_WIN);

	list = elm_box_children_get(box);
	goto_if(!list, OUT);

	EINA_LIST_FREE(list, item) {
		detail_item_info = evas_object_data_get(item, PRIVATE_DATA_KEY_ITEM_INFO);
		continue_if(!detail_item_info);
		continue_if(!detail_item_info->item);
		elm_box_unpack(box, detail_item_info->item);
		detail_destroy_item(detail_item_info);
	}

OUT:
	evas_object_del(box);
	evas_object_del(scroller);
}



HAPI Evas_Object *detail_create_scroller(Evas_Object *win, const char *pkgname)
{
	Evas_Object *scroller = NULL;
	Evas_Object *box = NULL;

	const Eina_List *l = NULL;
	detail_item_s *detail_item_info = NULL;

	retv_if(!win, NULL);

	_D("Create a detail scroller");

	scroller = elm_scroller_add(win);
	retv_if(!scroller, NULL);

	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_FALSE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_scroller_content_min_limit(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_single_direction_set(scroller, ELM_SCROLLER_SINGLE_DIRECTION_HARD);

	evas_object_move(scroller, 0, 0);
	evas_object_resize(scroller, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_show(scroller);

	box = elm_box_add(scroller);
	if (!box) {
		_E("Cannot create box_layout");
		evas_object_del(scroller);
		return NULL;
	}

	elm_box_horizontal_set(box, EINA_FALSE);
	/* The weight of box has to be set as EVAS_HINT_EXPAND on y-axis. */
	evas_object_size_hint_weight_set(box, 0.0, EVAS_HINT_EXPAND);
	evas_object_data_set(box, PRIVATE_DATA_KEY_WIN, win);
	evas_object_show(box);

	elm_object_content_set(scroller, box);

	EINA_LIST_FOREACH(detail_s.list, l, detail_item_info) {
		continue_if(!detail_item_info->pkgname);
		if (!strcmp(detail_item_info->pkgname, pkgname)) {
			Evas_Object *item = detail_create_item(box, detail_item_info);
			continue_if(!item);
			elm_box_pack_start(box, item);
		}
	}

	return scroller;
}



static key_cb_ret_e _detail_back_cb(void *data)
{
	Evas_Object *win = data;

	_D("Back key is released");

	retv_if(!win, KEY_CB_RET_STOP);
	detail_destroy_win(win);

	return KEY_CB_RET_STOP;
}



HAPI void detail_destroy_win(Evas_Object *win)
{
	Evas_Object *rect = NULL;
	Evas_Object *scroller = NULL;

	ret_if(!win);

	key_unregister_cb(KEY_TYPE_BACK, _detail_back_cb);

	rect = evas_object_data_del(win, PRIVATE_DATA_KEY_RECT);
	if (rect) {
		evas_object_del(rect);
	}

	scroller = evas_object_data_del(win, PRIVATE_DATA_KEY_SCROLLER);
	if (scroller) {
		detail_destroy_scroller(scroller);
	}

	evas_object_del(win);
}



HAPI Evas_Object *detail_create_win(const char *pkgname)
{
	Evas_Object *win = NULL;
	Evas_Object *rect = NULL;
	Evas_Object *scroller = NULL;

	retv_if(!pkgname, NULL);

	_D("Create a detail window");

	win = elm_win_add(NULL, "Detail Notification", ELM_WIN_BASIC);
	retv_if(!win, NULL);

	elm_win_title_set(win, DETAIL_WINDOW_NAME);
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_alpha_set(win, EINA_FALSE);
	elm_win_indicator_mode_set(win, ELM_WIN_INDICATOR_HIDE);

	evas_object_color_set(win, 0, 0, 0, 255);
	evas_object_resize(win, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_move(win, 0, 0);
	evas_object_show(win);
	elm_win_activate(win);

	rect = evas_object_rectangle_add(evas_object_evas_get(win));
	evas_object_color_set(rect, 100, 0, 0, 100);
	evas_object_move(rect, 0, 0);
	evas_object_resize(rect, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_data_set(win, PRIVATE_DATA_KEY_RECT, rect);
	elm_win_resize_object_add(win, rect);
	evas_object_show(rect);

	scroller = detail_create_scroller(win, pkgname);
	goto_if(!scroller, ERROR);
	evas_object_data_set(win, PRIVATE_DATA_KEY_SCROLLER, scroller);

	key_register_cb(KEY_TYPE_BACK, _detail_back_cb, win);

	return win;

ERROR:
	evas_object_del(win);
	return NULL;
}



