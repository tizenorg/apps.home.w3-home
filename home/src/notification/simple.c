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

#include "util.h"
#include "log.h"
#include "conf.h"
#include "main.h"

#define NOTIFICATION_EDJ_FILE EDJEDIR"/noti.edj"
#define NOTIFICATION_SIMPLE_GROUP "simple"
#define PRIVATE_DATA_KEY_TIMER "pdkt"
#define PRIVATE_DATA_KEY_ITEM "pdki"
#define NOTIFICATION_SIMPLE_DESTROY_TIME (3.0f)



HAPI void simple_destroy_item(Evas_Object *item)
{
	Evas_Object *btn = NULL;
	Evas_Object *icon = NULL;

	btn = elm_object_part_content_unset(item, "del,btn");
	if (btn) evas_object_del(btn);

	icon = elm_object_part_content_unset(item, "icon");
	if (icon) evas_object_del(icon);

	evas_object_del(item);
}



static void _delete_simple_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *win = data;
	Evas_Object *item = NULL;

	ret_if(!win);

	item = evas_object_data_get(win, PRIVATE_DATA_KEY_ITEM);
	if (item) {
		simple_destroy_item(item);
	}
}



HAPI Evas_Object *simple_create_item(Evas_Object *win, const char *icon_path, const char *title)
{
	Evas_Object *item = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *button = NULL;

	Eina_Bool ret = EINA_FALSE;

	retv_if(!win, NULL);

	item = elm_layout_add(win);
	retv_if(!item, NULL);

	ret = elm_layout_file_set(item, NOTIFICATION_EDJ_FILE, NOTIFICATION_SIMPLE_GROUP);
	goto_if(EINA_FALSE == ret, ERROR);

	evas_object_size_hint_weight_set(item, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(item);

	/* Icon */
	if (icon_path) {
		icon = elm_icon_add(item);
		goto_if(!icon, ERROR);
		goto_if(elm_image_file_set(icon, icon_path, NULL) == EINA_FALSE, ERROR);

		evas_object_size_hint_min_set(icon, NOTIFICATION_ICON_WIDTH, NOTIFICATION_ICON_HEIGHT);
		elm_image_preload_disabled_set(icon, EINA_TRUE);
		elm_image_smooth_set(icon, EINA_TRUE);
		elm_image_no_scale_set(icon, EINA_FALSE);
		elm_object_part_content_set(item, "icon", icon);
		evas_object_show(icon);
		elm_object_signal_emit(item, "show,icon", "icon");
	}

	/* Title */
	if (title) {
		elm_object_part_text_set(item, "title", title);
	}

	button = elm_button_add(item);
	goto_if(!button, ERROR);
	elm_object_style_set(button, "focus");
	elm_object_part_content_set(item, "del,btn", button);
	evas_object_smart_callback_add(button, "clicked", _delete_simple_cb, win);

	return item;

ERROR:
	if (icon) evas_object_del(icon);
	if (item) evas_object_del(item);
	return NULL;
}



HAPI void simple_destroy_win(Evas_Object *win)
{
	Evas_Object *item = NULL;
	Ecore_Timer *timer = NULL;

	ret_if(!win);

	timer = evas_object_data_del(win, PRIVATE_DATA_KEY_TIMER);
	if (timer) {
		ecore_timer_del(timer);
	}

	item = evas_object_data_del(win, PRIVATE_DATA_KEY_ITEM);
	if (item) {
		simple_destroy_item(item);
	}

	evas_object_del(win);
}



static Eina_Bool _delete_win_timer_cb(void *data)
{
	Evas_Object *win = data;
	retv_if(!win, ECORE_CALLBACK_CANCEL);
	simple_destroy_win(win);
	return ECORE_CALLBACK_CANCEL;
}



HAPI Evas_Object *simple_create_win(const char *icon_path, const char *title)
{
	Evas_Object *win = NULL;
	Evas_Object *item = NULL;
	Ecore_Timer *timer = NULL;

	win = elm_win_add(NULL, "Notification", ELM_WIN_NOTIFICATION);
	retv_if(!win, NULL);

	/* FIXME : We have to fix the height of this window  */
	evas_object_resize(win, main_get_info()->root_w, main_get_info()->root_h / 2);
	evas_object_move(win, 0, 0);
	evas_object_show(win);

	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_alpha_set(win, EINA_FALSE);
	elm_win_indicator_mode_set(win, ELM_WIN_INDICATOR_HIDE);
	elm_win_activate(win);

	item = simple_create_item(win, icon_path, title);
	goto_if(!item, ERROR);

	evas_object_data_set(win, PRIVATE_DATA_KEY_ITEM, item);
	elm_win_resize_object_add(win, item);

	timer = ecore_timer_add(NOTIFICATION_SIMPLE_DESTROY_TIME, _delete_win_timer_cb, win);
	if (timer) {
		evas_object_data_set(win, PRIVATE_DATA_KEY_TIMER, timer);
	} else {
		_E("cannot add a timer");
	}

	return win;

ERROR:
	evas_object_del(win);
	return NULL;
}



