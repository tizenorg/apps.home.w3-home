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

#ifndef __W_HOME_SCROLLER_INFO_H__
#define __W_HOME_SCROLLER_INFO_H__

typedef enum {
	SCROLLER_INDEX_INVALID = 0,
	SCROLLER_INDEX_SINGULAR,
	SCROLLER_INDEX_PLURAL,
	SCROLLER_INDEX_MAX,
} scroller_index_e;

typedef struct {
	/* innate features; */
	int root_width;
	int root_height;
	int page_width;
	int page_height;
	int max_page;
	scroller_index_e index_number;

	/* acquired features */
	int scroll_focus;
	int scroll_index;
	int scroll_effect;
	int scroll_cover;
	int scrolling;
	int unpack_page_inners_on_scroll;
	int enable_highlight;
	int mouse_down;

	Evas_Object *layout;
	Evas_Object *parent;
	Evas_Object *box_layout;
	Evas_Object *box;
	Evas_Object *center_neighbor_left;
	Evas_Object *center;
	Evas_Object *center_neighbor_right;
	Evas_Object *plus_page;
	Evas_Object *index[PAGE_DIRECTION_MAX];
	Ecore_Timer *index_update_timer;
	Eina_List *effect_page_list;
} scroller_info_s;

#endif // __W_HOME_SCROLLER_INFO_H__
