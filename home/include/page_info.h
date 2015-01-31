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

#ifndef __W_HOME_PAGE_INFO_H__
#define __W_HOME_PAGE_INFO_H__

typedef enum {
	PAGE_DIRECTION_LEFT = 0,
	PAGE_DIRECTION_CENTER,
	PAGE_DIRECTION_RIGHT,
	PAGE_DIRECTION_ANY, /* This direction is only for scroller_pop_pages, never use this in the push functions */
	PAGE_DIRECTION_MAX,
} page_direction_e;

typedef enum {
	PAGE_CATEGORY_IDLE_CLOCK = 0,
	PAGE_CATEGORY_APP,
	PAGE_CATEGORY_WIDGET,
	PAGE_CATEGORY_MORE_APPS,
	PAGE_CATEGORY_NXN_PAGE,
	PAGE_CATEGORY_MAX,
} page_category_e;

typedef enum {
	PAGE_REMOVABLE_OFF = 0,
	PAGE_REMOVABLE_ON,
} page_removable_e;

typedef struct {
	/* innate features */
	char *id;
	char *subid;
	char *title;
	double period;
	int width;
	int height;
	page_direction_e direction;
	page_category_e category;
	page_removable_e removable;

	/* acquired features */
	Evas_Object *layout;
	Evas_Object *scroller;
	Evas_Object *page;
	Evas_Object *page_rect;
	Evas_Object *page_inner;
	Evas_Object *page_inner_area;
	Evas_Object *page_inner_bg;
	Evas_Object *focus;
	Evas_Object *remove_focus;
	Evas_Object *item;
	Evas_Object *focus_prev;
	Evas_Object *focus_next;

	int ordering;
	int layout_longpress;
	int highlighted;
	int drag_threshold_page;
	Eina_Bool need_to_unhighlight;
	Eina_Bool highlight_changed;
	Eina_Bool is_scrolled_object;
	Eina_Bool appended;
	Eina_Bool need_to_read;
	void *faulted_hl_timer;
} page_info_s;

extern page_info_s *page_info_create(const char *id, const char *subid, double period);
extern void page_info_destroy(page_info_s *page_info);
extern page_info_s *page_info_dup(page_info_s *page_info);

extern void page_info_list_destroy(Eina_List *page_info_list);
extern int page_info_is_removable(const char *id);

#endif // __W_HOME_PAGE_INFO_H__
