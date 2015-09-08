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
#include "page_info.h"
#include "page.h"
#include "scroller_info.h"
#include "scroller.h"
#include "notification/detail.h"
#include "notification/time.h"

#define NOTIFICATION_EDJ_FILE EDJEDIR"/noti.edj"
#define NOTIFICATION_SUMMARY_GROUP "summary"
#define PRIVATE_DATA_KEY_PKGNAME "pdkp"



HAPI Evas_Object *summary_get_page(Evas_Object *scroller, const char *pkgname)
{
	Evas_Object *page = NULL;
	char *tmp = NULL;
	int idx = 0;

	retv_if(!scroller, NULL);
	retv_if(!pkgname, NULL);

	do {
		page = scroller_get_page_at(scroller, idx);
		if (!page) break;
		idx++;

		tmp = evas_object_data_get(page, PRIVATE_DATA_KEY_PKGNAME);
		if (!tmp) break;

		if (!strcmp(tmp, pkgname)) {
			return page;
		}
	} while (tmp);

	_D("There is no page for %s's summary", pkgname);

	return NULL;
}



static void _launch_summary_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *win = NULL;
	Evas_Object *page = data;
	char *pkgname = NULL;

	ret_if(!page);

	pkgname = evas_object_data_get(page, PRIVATE_DATA_KEY_PKGNAME);
	ret_if(!pkgname);

	_D("Create a detail window");

	win = detail_create_win(pkgname);
	ret_if(!win);
}



HAPI void summary_destroy_item(Evas_Object *item)
{
	Evas_Object *btn = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *rect = NULL;

	ret_if(!item);

	btn = elm_object_part_content_unset(item, "del,btn");
	if (btn) evas_object_del(btn);

	btn = elm_object_part_content_unset(item, "launch,btn");
	if (btn) evas_object_del(btn);

	icon = elm_object_part_content_unset(item, "icon");
	if (icon) evas_object_del(icon);

	rect = elm_object_part_content_unset(item, "bg");
	if (rect) evas_object_del(rect);

	evas_object_del(item);
}



HAPI void summary_destroy_page(Evas_Object *page)
{
	Evas_Object *item = NULL;
	char *pkgname = NULL;

	ret_if(!page);

	item = page_get_item(page);
	if (item) {
		summary_destroy_item(item);
	}
	pkgname = evas_object_data_del(page, PRIVATE_DATA_KEY_PKGNAME);
	free(pkgname);

	page_destroy(page);
}



static void _delete_summary_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *page = data;
	ret_if(!page);
	_D("Destroy a summary page");
	summary_destroy_page(page);
}



HAPI Evas_Object *summary_create_item(Evas_Object *page, const char *pkgname, const char *icon_path, const char *title, const char *content, int count, time_t *time)
{
	Evas_Object *item = NULL;
	Evas_Object *rect = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *launch_button = NULL;
	Evas_Object *delete_button = NULL;

	char *text_time = NULL;
	char buf[BUFSZE] = {0, };

	Eina_Bool ret = EINA_FALSE;

	retv_if(!page, NULL);
	retv_if(!pkgname, NULL);

	item = elm_layout_add(page);
	retv_if(!item, NULL);

	ret = elm_layout_file_set(item, NOTIFICATION_EDJ_FILE, NOTIFICATION_SUMMARY_GROUP);
	goto_if(EINA_FALSE == ret, ERROR);

	evas_object_size_hint_weight_set(item, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(item);

	/* BG */
	rect = evas_object_rectangle_add(evas_object_evas_get(page));
	goto_if(!rect, ERROR);
	evas_object_size_hint_min_set(rect, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_color_set(rect, 0, 0, 0, 0);
	evas_object_show(rect);
	elm_object_part_content_set(item, "bg", rect);

	/* Icon */
	if (icon_path) {
		icon = elm_icon_add(item);
		goto_if(!icon, ERROR);
		goto_if(elm_image_file_set(icon, icon_path, NULL) == EINA_FALSE, ERROR);

		elm_image_preload_disabled_set(icon, EINA_TRUE);
		elm_image_smooth_set(icon, EINA_TRUE);
		elm_image_no_scale_set(icon, EINA_FALSE);

		evas_object_size_hint_min_set(icon, NOTIFICATION_ICON_WIDTH, NOTIFICATION_ICON_HEIGHT);
		evas_object_show(icon);

		elm_object_part_content_set(item, "icon", icon);
		elm_object_signal_emit(item, "show,icon", "icon");
	}

	/* Title */
	if (title) {
		elm_object_part_text_set(item, "title", title);
	}

	/* Time */
	if (time) {
		notification_time_to_string(time, buf, sizeof(buf));
		text_time = elm_entry_utf8_to_markup(buf);
	}
	if (text_time) {
		elm_object_part_text_set(item, "time", text_time);
		free(text_time);
	}

	/* Content */
	if (content) {
		elm_object_part_text_set(item, "text", content);
	}

	/* Count */
	if (count) {
		snprintf(buf, sizeof(buf), "%d", count);
		elm_object_part_text_set(item, "count", buf);
	}

	/* Launch Button */
	launch_button = elm_button_add(item);
	goto_if(!launch_button, ERROR);
	elm_object_style_set(launch_button, "focus");
	elm_object_part_content_set(item, "launch,btn", launch_button);
	evas_object_smart_callback_add(launch_button, "clicked", _launch_summary_cb, page);

	/* Delete Button */
	delete_button = elm_button_add(item);
	goto_if(!delete_button, ERROR);
	elm_object_style_set(delete_button, "focus");
	elm_object_part_content_set(item, "del,btn", delete_button);
	evas_object_smart_callback_add(delete_button, "clicked", _delete_summary_cb, page);

	return item;

ERROR:
	if (launch_button) evas_object_del(launch_button);
	if (icon) evas_object_del(icon);
	if (rect) evas_object_del(rect);
	if (item) evas_object_del(item);
	return NULL;
}



HAPI Evas_Object *summary_create_page(Evas_Object *parent, const char *pkgname, const char *icon_path, const char *title, const char *content, int count, time_t *time)
{

	Evas_Object *page = NULL;
	Evas_Object *item = NULL;

	char *tmp = NULL;

	retv_if(!parent, NULL);
	retv_if(!pkgname, NULL);

	page = page_create(parent
			, NULL
			, NULL
			, NULL
			, main_get_info()->root_w, main_get_info()->root_h
			, PAGE_CHANGEABLE_BG_OFF, PAGE_REMOVABLE_OFF);
	goto_if(!page, ERROR);

	tmp = strdup(pkgname);
	goto_if(!tmp, ERROR);

	evas_object_data_set(page, PRIVATE_DATA_KEY_PKGNAME, tmp);

	item = summary_create_item(page, pkgname, icon_path, title, content, count, time);
	goto_if(!item, ERROR);

	page_set_item(page, item);

	return page;

ERROR:
	if (page) page_destroy(page);
	if (item) summary_destroy_item(item);

	return NULL;
}
