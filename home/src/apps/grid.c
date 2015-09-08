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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "util.h"
#include "apps/apps_conf.h"
#include "apps/grid.h"
#include "apps/layout.h"
#include "apps/list.h"
#include "apps/apps_main.h"

#define PRIVATE_DATA_KEY_ITEM_INFO_LIST_APPENDED "p_iil_and"
#define PRIVATE_DATA_KEY_ITEM_INFO_LIST_INDEX "p_iil_idx"
#define PRIVATE_DATA_KEY_ITEM_INFO_LIST_TIMER "p_iil_tm"


HAPI void grid_destroy(Evas_Object *win, Evas_Object *grid)
{
	Ecore_Timer *timer = NULL;
	Elm_Gengrid_Item_Class *gic = NULL;

	evas_object_data_del(grid, DATA_KEY_WIN);
	evas_object_data_del(grid, DATA_KEY_LAYOUT);
	evas_object_data_del(grid, DATA_KEY_INSTANCE_INFO);
	evas_object_data_del(grid, DATA_KEY_ITEM_ICON_WIDTH);
	evas_object_data_del(grid, DATA_KEY_ITEM_ICON_HEIGHT);
	evas_object_data_del(grid, DATA_KEY_ITEM_WIDTH);
	evas_object_data_del(grid, DATA_KEY_ITEM_HEIGHT);
	evas_object_data_del(grid, DATA_KEY_CURRENT_ITEM);
	evas_object_data_del(grid, DATA_KEY_LIST_INDEX);
	evas_object_data_del(grid, DATA_KEY_LIST);
	gic = evas_object_data_del(grid, DATA_KEY_GIC);
	if (gic) elm_gengrid_item_class_free(gic);

	timer = evas_object_data_del(grid, DATA_KEY_TIMER);
	if (timer) {
		ecore_timer_del(timer);
		timer = NULL;
	}

	evas_object_data_del(win, DATA_KEY_GRID);

	ret_if(NULL == grid);
	evas_object_del(grid);
}



static char *_text_get(void *data, Evas_Object *obj, const char *part)
{
	item_info_s *item_info = data;
	retv_if(NULL == item_info, NULL);

	if (!strcmp(part, "txt")) {
		retv_if(!item_info->name, NULL);
		return strdup(item_info->name);
	}

	return NULL;
}



static Evas_Object *_add_icon(Evas_Object *parent, const char *file)
{
	const char *real_icon_file = NULL;
	Evas_Object *icon = NULL;
	int w, h;

	real_icon_file = file;
	if (access(real_icon_file, R_OK) != 0) {
		_APPS_E("Failed to access an icon(%s)", real_icon_file);
		real_icon_file = DEFAULT_ICON;
	}

	icon = elm_icon_add(parent);
	retv_if(NULL == icon, NULL);

	if (elm_image_file_set(icon, real_icon_file, NULL) == EINA_FALSE) {
		_APPS_E("Icon file is not accessible (%s)", real_icon_file);
		evas_object_del(icon);
		return NULL;
	}

	w = (int) evas_object_data_get(parent, DATA_KEY_ITEM_ICON_WIDTH);
	h = (int) evas_object_data_get(parent, DATA_KEY_ITEM_ICON_HEIGHT);
	evas_object_size_hint_min_set(icon, w, h);

	elm_image_preload_disabled_set(icon, EINA_TRUE);
	elm_image_smooth_set(icon, EINA_TRUE);
	elm_image_no_scale_set(icon, EINA_FALSE);

	return icon;
}


static Evas_Object *_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object *bg = NULL;

	item_info_s *item_info = data;
	retv_if(NULL == item_info, NULL);

	if (!strcmp(part, "bg")) {
		bg = evas_object_rectangle_add(evas_object_evas_get(obj));
		retv_if(NULL == bg, NULL);

		int w = (int) evas_object_data_get(obj, DATA_KEY_ITEM_WIDTH);
		int h = (int) evas_object_data_get(obj, DATA_KEY_ITEM_HEIGHT);

		evas_object_size_hint_min_set(bg, w, h);
		evas_object_color_set(bg, 0, 0, 0, 0);
		evas_object_show(bg);
		return bg;
	} else if (!strcmp(part, "icon_image")) {
		retv_if(NULL == item_info->icon, NULL);

		return _add_icon(obj, item_info->icon);
	}

	return NULL;
}



static void _item_selected(void *data, Evas_Object *obj, void *event_info)
{
	item_info_s *item_info = data;

	ret_if(NULL == data);

	_APPS_D("Selected an item[%s]", item_info->appid);

	util_launch_main_operation(NULL, item_info->appid, item_info->name);
	elm_gengrid_item_selected_set(item_info->item, EINA_FALSE);
}



static void _del(void *data, Evas_Object *obj)
{
	_APPS_D("Delete an item[%p]", obj);
}



Eina_Bool _init_timer_cb(void *data)
{
	Evas_Object *grid = data;
	Elm_Gengrid_Item_Class *gic = NULL;
	Eina_List *list = NULL;
	item_info_s *item_info = NULL;
	int index, count;

	retv_if(!grid, ECORE_CALLBACK_CANCEL);

	list = evas_object_data_get(grid, DATA_KEY_LIST);
	retv_if(!list, ECORE_CALLBACK_CANCEL);

	index = (int) evas_object_data_get(grid, DATA_KEY_LIST_INDEX);
	count = eina_list_count(list);
	if (index == count) goto OUT;

	gic = evas_object_data_get(grid, DATA_KEY_GIC);
	goto_if(!gic, OUT);

	item_info = eina_list_nth(list, index);
	goto_if(!item_info, OUT);

	item_info->item = elm_gengrid_item_append(grid, gic, item_info, _item_selected, item_info);
	item_info->ordering = index;

	index ++;
	if (index == count) goto OUT;
	evas_object_data_set(grid, DATA_KEY_LIST_INDEX, (void *) index);

	return ECORE_CALLBACK_RENEW;

OUT:
	_APPS_D("Loading apps is done");

	Elm_Object_Item *first_it = elm_gengrid_first_item_get(grid);
	if (first_it) {
		elm_gengrid_item_bring_in(first_it, ELM_GENGRID_ITEM_SCROLLTO_TOP);
		evas_object_data_set(grid, DATA_KEY_CURRENT_ITEM, first_it);
	}
	evas_object_data_del(grid, DATA_KEY_LIST_INDEX);
	evas_object_data_del(grid, DATA_KEY_TIMER);

	index = 0;

	return ECORE_CALLBACK_CANCEL;
}



#define ITEM_STYLE "recent-apps"
HAPI Evas_Object *grid_create(Evas_Object *win, Evas_Object *parent)
{
	Evas_Object *grid = NULL;
	Elm_Gengrid_Item_Class *gic = NULL;
	Ecore_Timer *timer = NULL;
	instance_info_s *info = NULL;
	Eina_List *list = evas_object_data_get(parent, DATA_KEY_LIST);
	int count = eina_list_count(list);
	double scale = elm_config_scale_get();
	int w = (int)(ITEM_WIDTH * scale);
	int h = (int)(ITEM_HEIGHT * scale);
	int icon_w = (int)(ITEM_ICON_WIDTH * scale);
	int icon_h = (int)(ITEM_ICON_HEIGHT * scale);

	if (0 == count) return NULL;

	info = evas_object_data_get(win, DATA_KEY_INSTANCE_INFO);
	retv_if(!info, NULL);

	grid = elm_gengrid_add(parent);
	goto_if(NULL == grid, ERROR);
	elm_object_theme_set(grid, apps_main_get_info()->theme);

	evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(grid, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_min_set(grid, info->root_w, GRID_EDIT_HEIGHT);
	evas_object_resize(grid, info->root_w, info->root_h);

	elm_gengrid_item_size_set(grid, w, h);
	elm_gengrid_align_set(grid, 0.5, 0.0);
	elm_gengrid_horizontal_set(grid, EINA_FALSE);
	elm_gengrid_multi_select_set(grid, EINA_FALSE);

	evas_object_data_set(grid, DATA_KEY_WIN, win);
	evas_object_data_set(grid, DATA_KEY_LAYOUT, parent);
	evas_object_data_set(grid, DATA_KEY_INSTANCE_INFO, info);
	evas_object_data_set(grid, DATA_KEY_ITEM_ICON_WIDTH, (void *) icon_w);
	evas_object_data_set(grid, DATA_KEY_ITEM_ICON_HEIGHT, (void *) icon_h);
	evas_object_data_set(grid, DATA_KEY_ITEM_WIDTH, (void *) w);
	evas_object_data_set(grid, DATA_KEY_ITEM_HEIGHT, (void *) h);
	evas_object_data_set(grid, DATA_KEY_LIST, list);
	evas_object_data_set(grid, DATA_KEY_LIST_INDEX, NULL);

	gic = elm_gengrid_item_class_new();
	goto_if(NULL == gic, ERROR);
	gic->item_style = ITEM_STYLE;
	gic->func.text_get = _text_get;
	gic->func.content_get = _content_get;
	gic->func.state_get = NULL;
	gic->func.del = _del;
	evas_object_data_set(grid, DATA_KEY_GIC, gic);

	timer = ecore_timer_add(0.02f, _init_timer_cb, grid);
	goto_if(NULL == timer, ERROR);

	evas_object_data_set(grid, DATA_KEY_TIMER, timer);
	evas_object_data_set(win, DATA_KEY_GRID, grid);

	return grid;

ERROR:
	if (grid) grid_destroy(win, grid);
	return NULL;
}



HAPI void grid_refresh(Evas_Object *grid)
{
	Ecore_Timer *timer;

	ret_if(!grid);

	/* 1. Clear all items in the gengrid */
	elm_gengrid_clear(grid);

	/* 2. Append items to the gengrid */
	timer = evas_object_data_get(grid, DATA_KEY_TIMER);
	if (timer) {
		ecore_timer_del(timer);
		evas_object_data_set(grid, DATA_KEY_TIMER, NULL);
	}

	timer = ecore_timer_add(0.02f, _init_timer_cb, grid);
	ret_if(!timer);
	evas_object_data_set(grid, DATA_KEY_TIMER, timer);
}



HAPI void grid_show_top(Evas_Object *grid)
{
	Elm_Object_Item *item;

	ret_if(!grid);

	item = elm_gengrid_first_item_get(grid);
	ret_if(!item);

	elm_gengrid_item_show(item, ELM_GENGRID_ITEM_SCROLLTO_TOP);
}



Eina_Bool _append_items_timer_cb(void *data)
{
	Evas_Object *grid = data;
	Elm_Gengrid_Item_Class *gic = NULL;
	Eina_List *list = NULL;
	item_info_s *item_info = NULL;
	int index, count;

	retv_if(!grid, ECORE_CALLBACK_CANCEL);

	list = evas_object_data_get(grid, PRIVATE_DATA_KEY_ITEM_INFO_LIST_APPENDED);
	retv_if(!list, ECORE_CALLBACK_CANCEL);

	index = (int) evas_object_data_get(grid, PRIVATE_DATA_KEY_ITEM_INFO_LIST_INDEX);
	count = eina_list_count(list);
	if (index == count) goto OUT;

	gic = evas_object_data_get(grid, DATA_KEY_GIC);
	goto_if(!gic, OUT);

	item_info = eina_list_nth(list, index);
	goto_if(!item_info, OUT);

	item_info->item = elm_gengrid_item_append(grid, gic, item_info, _item_selected, item_info);
	item_info->ordering = index;

	index ++;
	if (index == count) goto OUT;
	evas_object_data_set(grid, PRIVATE_DATA_KEY_ITEM_INFO_LIST_INDEX, (void *) index);

	return ECORE_CALLBACK_RENEW;

OUT:
	_APPS_D("Loading apps is done");

	Elm_Object_Item *first_it = elm_gengrid_first_item_get(grid);
	if (first_it) {
		elm_gengrid_item_bring_in(first_it, ELM_GENGRID_ITEM_SCROLLTO_TOP);
		evas_object_data_set(grid, DATA_KEY_CURRENT_ITEM, first_it);
	}
	evas_object_data_del(grid, PRIVATE_DATA_KEY_ITEM_INFO_LIST_TIMER);
	evas_object_data_del(grid, PRIVATE_DATA_KEY_ITEM_INFO_LIST_INDEX);
	evas_object_data_del(grid, PRIVATE_DATA_KEY_ITEM_INFO_LIST_APPENDED);

	index = 0;

	return ECORE_CALLBACK_CANCEL;
}



HAPI void grid_append_list(Evas_Object *grid, Eina_List *list)
{
	Ecore_Timer *timer = NULL;

	ret_if(!grid);
	ret_if(!list);

	timer = evas_object_data_del(grid, PRIVATE_DATA_KEY_ITEM_INFO_LIST_TIMER);
	if (timer) ecore_timer_del(timer);

	timer = ecore_timer_add(0.01f, _append_items_timer_cb, grid);
	ret_if(!timer);
	evas_object_data_set(grid, PRIVATE_DATA_KEY_ITEM_INFO_LIST_TIMER, timer);
	evas_object_data_set(grid, PRIVATE_DATA_KEY_ITEM_INFO_LIST_INDEX, NULL);
	evas_object_data_set(grid, PRIVATE_DATA_KEY_ITEM_INFO_LIST_APPENDED, list);
}



HAPI void grid_remove_list(Evas_Object *grid, Eina_List *list)
{
	const Eina_List *l, *ln;
	item_info_s *item_info = NULL;

	ret_if(!grid);
	ret_if(!list);

	EINA_LIST_FOREACH_SAFE(list, l, ln, item_info) {
		continue_if(!item_info);
		if (item_info->item) elm_object_item_del(item_info->item);
		item_info->item = NULL;
	}
}



// End of a file
