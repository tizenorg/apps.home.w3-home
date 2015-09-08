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

#include "log.h"
#include "util.h"
#include "apps/apps_conf.h"
#include "apps/item.h"
#include "apps/item_badge.h"
#include "apps/apps_main.h"
#include "apps/page.h"
#include "apps/page_info.h"
#include "apps/scroller_info.h"



HAPI void apps_page_destroy(Evas_Object *page)
{
	page_info_s *prev_page_info = NULL;
	page_info_s *page_info = NULL;
	page_info_s *next_page_info = NULL;

	ret_if(!page);

	page_info = evas_object_data_del(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	if (page_info->prev_page) {
		prev_page_info = evas_object_data_get(page_info->prev_page, DATA_KEY_PAGE_INFO);
		if (prev_page_info) prev_page_info->next_page = NULL;
	}

	if (page_info->next_page) {
		next_page_info = evas_object_data_get(page_info->next_page, DATA_KEY_PAGE_INFO);
		if (next_page_info) next_page_info->prev_page = NULL;
	}

	if (prev_page_info && next_page_info) {
		prev_page_info->next_page = next_page_info->page;
		next_page_info->prev_page = prev_page_info->page;
	}

	evas_object_del(page_info->page_rect);
	free(page_info);
	evas_object_del(page);
}



#define PAGE_OUTER_EDJE_FILE EDJEDIR"/apps_page.edj"
HAPI Evas_Object *apps_page_create(Evas_Object *scroller, Evas_Object *prev_page, Evas_Object *next_page)
{
	Evas_Object *page = NULL;
	Evas_Object *page_rect = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *prev_page_info = NULL;
	page_info_s *next_page_info = NULL;
	page_info_s *page_info = NULL;

	retv_if(!scroller, NULL);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	retv_if(!scroller_info, NULL);

	page = elm_layout_add(scroller);
	retv_if(!page, NULL);
	elm_layout_file_set(page, PAGE_OUTER_EDJE_FILE, "page");
	evas_object_size_hint_weight_set(page, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(page);

	page_rect = evas_object_rectangle_add(scroller_info->instance_info->e);
	goto_if(!page_rect, ERROR);
	evas_object_size_hint_min_set(page_rect, scroller_info->instance_info->root_w, ITEM_HEIGHT * apps_main_get_info()->scale);
	evas_object_color_set(page_rect, 0, 0, 0, 0);
	evas_object_show(page_rect);
	elm_object_part_content_set(page, "bg", page_rect);

	page_info = calloc(1, sizeof(page_info_s));
	if (!page_info) {
		_APPS_E("Cannot calloc for page_info");
		apps_page_destroy(page);
		return NULL;
	}
	page_info->win = scroller_info->win;
	page_info->instance_info = scroller_info->instance_info;
	page_info->scroller = scroller;
	page_info->page_rect = page_rect;
	page_info->prev_page = prev_page;
	page_info->page = page;
	page_info->next_page = next_page;
	evas_object_data_set(page, DATA_KEY_PAGE_INFO, page_info);

	if (prev_page) {
		prev_page_info = evas_object_data_get(prev_page, DATA_KEY_PAGE_INFO);
		if (prev_page_info) {
			prev_page_info->next_page = page;
		}
	}

	if (next_page) {
		next_page_info = evas_object_data_get(next_page, DATA_KEY_PAGE_INFO);
		if (next_page_info) {
			next_page_info->prev_page = page;
		}
	}

	return page;

ERROR:
	apps_page_destroy(page);
	return NULL;
}



HAPI apps_error_e apps_page_pack_item(Evas_Object *page, Evas_Object *item)
{
	item_info_s *item_info = NULL;
	register int i = 0;
	char part_name[PART_NAME_SIZE] = {0, };
	char center_name[PART_NAME_SIZE] = {0, };

	retv_if(!page, APPS_ERROR_INVALID_PARAMETER);
	retv_if(!item, APPS_ERROR_INVALID_PARAMETER);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, APPS_ERROR_FAIL);

	for (; i < APPS_PER_PAGE; i++) {
		Evas_Object *tmp = NULL;

		snprintf(part_name, sizeof(part_name), "item_%d", i);
		tmp = elm_object_part_content_get(page, part_name);
		if (tmp) continue;

		/* We have to pack item & item_center together and to set the info */
		elm_object_part_content_set(page, part_name, item);
		if (item_info->center) {
			snprintf(center_name, sizeof(part_name), "item_center_%d", i);
			elm_object_part_content_set(page, center_name, item_info->center);
		}

		item_info->page = page;

		return APPS_ERROR_NONE;
	}

	return APPS_ERROR_FAIL;
}



HAPI apps_error_e apps_page_unpack_item(Evas_Object *page, Evas_Object *item)
{
	item_info_s *item_info = NULL;
	register int i = 0;
	char part_name[PART_NAME_SIZE] = {0, };
	char center_name[PART_NAME_SIZE] = {0, };

	retv_if(!page, APPS_ERROR_INVALID_PARAMETER);
	retv_if(!item, APPS_ERROR_INVALID_PARAMETER);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, APPS_ERROR_FAIL);

	for (; i < APPS_PER_PAGE; i++) {
		Evas_Object *tmp = NULL;

		snprintf(part_name, sizeof(part_name), "item_%d", i);
		tmp = elm_object_part_content_get(page, part_name);
		if (tmp != item) continue;

		/* We have to unpack item & item_center together and to set the info */
		elm_object_part_content_unset(page, part_name);
		if (item_info->center) {
			snprintf(center_name, sizeof(part_name), "item_center_%d", i);
			elm_object_part_content_unset(page, center_name);
		}

		item_info->page = NULL;

		return APPS_ERROR_NONE;
	}

	return APPS_ERROR_FAIL;
}



HAPI int apps_page_count_item(Evas_Object *page)
{
	int count = 0;
	register int i = 0;
	char part_name[PART_NAME_SIZE];

	retv_if(!page, -1);

	for (; i < APPS_PER_PAGE; i++) {
		Evas_Object *item = NULL;
		snprintf(part_name, sizeof(part_name), "item_%d", i);
		item = elm_object_part_content_get(page, part_name);
		if (!item) continue;
		count++;
	}

	return count;
}



HAPI Evas_Object *apps_page_get_item_at(Evas_Object *page, int position)
{
	Evas_Object *tmp = NULL;
	char part_name[PART_NAME_SIZE];

	retv_if(!page, NULL);
	retv_if(position < -1, NULL);

	snprintf(part_name, sizeof(part_name), "item_%d", position);
	tmp = elm_object_part_content_get(page, part_name);

	return tmp;
}



HAPI apps_error_e apps_page_pack_nth(Evas_Object *page, Evas_Object *item, int position)
{
	Evas_Object *tmp = NULL;
	item_info_s *item_info = NULL;
	char part_name[PART_NAME_SIZE] = {0, };
	char center_name[PART_NAME_SIZE] = {0, };

	retv_if(!page, APPS_ERROR_INVALID_PARAMETER);
	retv_if(!item, APPS_ERROR_INVALID_PARAMETER);
	retv_if(position < -1, APPS_ERROR_INVALID_PARAMETER);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, APPS_ERROR_FAIL);

	snprintf(part_name, sizeof(part_name), "item_%d", position);
	tmp = elm_object_part_content_get(page, part_name);
	if (tmp) {
		_APPS_E("Error : Already has an item");\
		return APPS_ERROR_FAIL;
	}

	elm_object_part_content_set(page, part_name, item);
	if (item_info->center) {
		snprintf(center_name, sizeof(part_name), "item_center_%d", position);
		elm_object_part_content_set(page, center_name, item_info->center);
	}

	item_info->page = page;

	return APPS_ERROR_NONE;
}



HAPI Evas_Object *apps_page_unpack_nth(Evas_Object *page, int position)
{
	Evas_Object *tmp = NULL;
	item_info_s *item_info = NULL;
	char part_name[PART_NAME_SIZE] = {0, };
	char center_name[PART_NAME_SIZE] = {0, };

	retv_if(!page, NULL);
	retv_if(position < -1, NULL);

	snprintf(part_name, sizeof(part_name), "item_%d", position);
	tmp = elm_object_part_content_unset(page, part_name);
	if (!tmp) return NULL;

	item_info = evas_object_data_get(tmp, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, NULL);

	if (item_info->center) {
		snprintf(center_name, sizeof(part_name), "item_center_%d", position);
		elm_object_part_content_unset(page, center_name);
	}

	item_info->page = NULL;

	return tmp;
}



HAPI void apps_page_change_language(Evas_Object *page)
{
	register int i = 0;

	ret_if(!page);

	for (; i < APPS_PER_PAGE; i++) {
		Evas_Object *item = NULL;
		char part_name[PART_NAME_SIZE];

		snprintf(part_name, sizeof(part_name), "item_%d", i);
		item = elm_object_part_content_get(page, part_name);
		if (!item) continue;

		item_change_language(item);
		item_badge_change_language(item);
	}
}



HAPI int apps_page_has_item(Evas_Object *page, Evas_Object *item)
{
	register int i = 0;
	int has_item = 0;

	retv_if(!page, 0);

	for (; i < APPS_PER_PAGE; i++) {
		Evas_Object *tmp = NULL;
		char part_name[PART_NAME_SIZE];

		snprintf(part_name, sizeof(part_name), "item_%d", i);
		tmp = elm_object_part_content_get(page, part_name);
		if (tmp != item) continue;

		has_item = 1;
		break;
	}

	return has_item;
}



HAPI Evas_Object *apps_page_move_item_prev(Evas_Object *page, Evas_Object *from_item, Evas_Object *to_item, Evas_Object *append_item)
{
	Evas_Object *tmp = NULL;
	Evas_Object *tmp_next = append_item;
	item_info_s *item_info = NULL;
	register int i;
	int from_idx = -1;
	int to_idx = -1;
	char part_name[PART_NAME_SIZE];
	char center_name[PART_NAME_SIZE];

	retv_if(!page, NULL);

	if (from_item || to_item) {
		for (i = 0;  i < APPS_PER_PAGE; ++i) {
			snprintf(part_name, sizeof(part_name), "item_%d", i);
			tmp = elm_object_part_content_get(page, part_name);

			if (tmp && tmp == from_item) from_idx = i;
			if (tmp && tmp == to_item) to_idx = i;
		}
	}
	_APPS_D("Move item to the prev, from_idx:%d, to_idx:%d", from_idx, to_idx);

	if (-1 == from_idx) from_idx = 0;
	if (-1 == to_idx) to_idx = APPS_PER_PAGE - 1;

	for (i = to_idx; i >= from_idx; --i) {
		snprintf(part_name, sizeof(part_name), "item_%d", i);
		snprintf(center_name, sizeof(part_name), "item_center_%d", i);

		/* We have to unset item & item_center together. */
		tmp = elm_object_part_content_get(page, part_name);
		if (tmp) {
			elm_object_part_content_unset(page, part_name);
			item_info = evas_object_data_get(tmp, DATA_KEY_ITEM_INFO);
			continue_if(!item_info);
			item_info->page = NULL;

			if (item_info->center) {
				elm_object_part_content_unset(page, center_name);
			}
		}

		if (tmp_next) {
			/* We have to set item & item_center together. */
			elm_object_part_content_set(page, part_name, tmp_next);

			item_info = evas_object_data_get(tmp_next, DATA_KEY_ITEM_INFO);
			continue_if(!item_info);
			item_info->page = page;

			if (item_info->center) {
				elm_object_part_content_set(page, center_name, item_info->center);
			}

			elm_object_signal_emit(page, "next", part_name);
			edje_object_message_signal_process(elm_layout_edje_get(page));
			elm_object_signal_emit(page, "return", part_name);
		}
		tmp_next = tmp;
	}

	return tmp;
}



HAPI Evas_Object *apps_page_move_item_next(Evas_Object *page, Evas_Object *from_item, Evas_Object *to_item, Evas_Object *insert_item)
{
	Evas_Object *tmp = NULL;
	Evas_Object *tmp_prev = insert_item;
	item_info_s *item_info = NULL;
	register int i;
	int from_idx = -1;
	int to_idx = -1;
	char part_name[PART_NAME_SIZE];
	char center_name[PART_NAME_SIZE];

	retv_if(!page, NULL);

	if (from_item || to_item) {
		for (i = 0;  i < APPS_PER_PAGE; ++i) {
			snprintf(part_name, sizeof(part_name), "item_%d", i);
			tmp = elm_object_part_content_get(page, part_name);

			if (tmp && tmp == from_item) from_idx = i;
			if (tmp && tmp == to_item) to_idx = i;
		}
	}
	_APPS_D("Move item to the next, from_idx:%d, to_idx:%d", from_idx, to_idx);

	if (-1 == from_idx) from_idx = 0;
	if (-1 == to_idx) to_idx = APPS_PER_PAGE - 1;

	for (i = from_idx; i <= to_idx; ++i) {
		snprintf(part_name, sizeof(part_name), "item_%d", i);
		snprintf(center_name, sizeof(part_name), "item_center_%d", i);

		/* We have to unset item & item_center together. */
		tmp = elm_object_part_content_get(page, part_name);
		if (tmp) {
			elm_object_part_content_unset(page, part_name);
			item_info = evas_object_data_get(tmp, DATA_KEY_ITEM_INFO);
			continue_if(!item_info);
			item_info->page = NULL;

			if (item_info->center) {
				elm_object_part_content_unset(page, center_name);
			}
		}

		if (tmp_prev) {
			/* We have to set item & item_center together. */
			elm_object_part_content_set(page, part_name, tmp_prev);

			item_info = evas_object_data_get(tmp_prev, DATA_KEY_ITEM_INFO);
			continue_if(!item_info);
			item_info->page = page;

			if (item_info->center) {
				elm_object_part_content_set(page, center_name, item_info->center);
			}

			elm_object_signal_emit(page, "prev", part_name);
			edje_object_message_signal_process(elm_layout_edje_get(page));
			elm_object_signal_emit(page, "return", part_name);
		}
		tmp_prev = tmp;
	}

	return tmp;
}



HAPI int apps_page_seek_item_position(Evas_Object *page, Evas_Object *item)
{
	register int i = 0;

	retv_if(!page, -1);
	retv_if(!item, -1);

	for (; i < APPS_PER_PAGE; i++) {
		Evas_Object *tmp = NULL;
		char part_name[PART_NAME_SIZE];

		snprintf(part_name, sizeof(part_name), "item_%d", i);
		tmp = elm_object_part_content_get(page, part_name);
		if (tmp == item) return i;
	}

	return -1;
}



HAPI void apps_page_print_item(Evas_Object *page)
{
	item_info_s *item_info = NULL;
	register int i = 0;

	ret_if(!page);

	for (; i < APPS_PER_PAGE; i++) {
		Evas_Object *tmp = NULL;
		char part_name[PART_NAME_SIZE];

		snprintf(part_name, sizeof(part_name), "item_%d", i);
		tmp = elm_object_part_content_get(page, part_name);
		continue_if(!tmp);

		item_info = evas_object_data_get(tmp, DATA_KEY_ITEM_INFO);
		continue_if(!item_info);

		_APPS_D("%s, %s", part_name, item_info->appid);
	}
}



HAPI void apps_page_edit(Evas_Object *page)
{
	register int i = 0;
	item_info_s *item_info = NULL;

	ret_if(!page);

//	elm_object_signal_emit(page, "edit", "item_center_-1");
//	elm_object_signal_emit(page, "edit", "item_center_0");
//	elm_object_signal_emit(page, "edit", "item_center_1");
//	elm_object_signal_emit(page, "edit", "item_center_2");

	for (; i < APPS_PER_PAGE; i++) {
		Evas_Object *tmp = NULL;
		char part_name[PART_NAME_SIZE];

		snprintf(part_name, sizeof(part_name), "item_%d", i);
		tmp = elm_object_part_content_get(page, part_name);
		if (!tmp) {
			continue_if(!item_info);

			page_info_s *page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
			Evas_Object *scroller = page_info->scroller;
			continue_if(!scroller);

			Evas_Object *virtual_item = item_virtual_create(scroller);
			evas_object_data_set(virtual_item, DATA_KEY_IS_VIRTUAL_ITEM, (void*)1);
			apps_page_pack_item(page, virtual_item);
			continue;
		}
		item_info = evas_object_data_get(tmp, DATA_KEY_ITEM_INFO);
		item_edit(tmp);
	}
}



HAPI void apps_page_unedit(Evas_Object *page)
{
	register int i = 0;

	ret_if(!page);

//	elm_object_signal_emit(page, "unedit", "item_center_-1");
//	elm_object_signal_emit(page, "unedit", "item_center_0");
//	elm_object_signal_emit(page, "unedit", "item_center_1");
//	elm_object_signal_emit(page, "unedit", "item_center_2");

	for (; i < APPS_PER_PAGE; i++) {
		Evas_Object *tmp = NULL;
		char part_name[PART_NAME_SIZE];

		snprintf(part_name, sizeof(part_name), "item_%d", i);
		tmp = elm_object_part_content_get(page, part_name);
		if (!tmp) continue;

		int bIsVirtual = (int)evas_object_data_get(tmp, DATA_KEY_IS_VIRTUAL_ITEM);
		if(bIsVirtual) {
			apps_page_unpack_item(page, tmp);
			item_virtual_destroy(tmp);
		}
		else {
			item_unedit(tmp);
		}
	}
}



// End of this file
