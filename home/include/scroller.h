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

#ifndef __W_HOME_SCROLLER_H__
#define __W_HOME_SCROLLER_H__

/*
 * FIRST - ... - CENTER_LEFT - CENTER_NEIGHBOR_LEFT - CENTER - CENTER_NEIGHBOR_RIGHT - CENTER_RIGHT - ... - LAST
 * If there is no center page,
 * then put the CENTER_LEFT & CENTER & CREATE_RIGHT page to LAST.
 * The scroller has only one CENTER, CENTER_NEIGHBOR_LEFT and CENTER_NEIGHBOR_RIGHT page each.
 * If there are already the CENTER & CENTER_NEIGHBOR pages, the scroller replaces them.
 */
typedef enum {
	SCROLLER_PUSH_TYPE_INVALID = 0,
	SCROLLER_PUSH_TYPE_FIRST,
	SCROLLER_PUSH_TYPE_CENTER_LEFT,
	SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_LEFT,
	SCROLLER_PUSH_TYPE_CENTER,
	SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_RIGHT,
	SCROLLER_PUSH_TYPE_CENTER_RIGHT,
	SCROLLER_PUSH_TYPE_LAST,
	SCROLLER_PUSH_TYPE_MAX,
} scroller_push_type_e;

typedef enum {
	SCROLLER_FREEZE_OFF = 0,
	SCROLLER_FREEZE_ON,
	SCROLLER_FREEZE_MAX,
} scroller_freeze_e;

typedef enum {
	SCROLLER_BRING_TYPE_INVALID = 0,
	SCROLLER_BRING_TYPE_INSTANT,
	SCROLLER_BRING_TYPE_INSTANT_SHOW,
	SCROLLER_BRING_TYPE_ANIMATOR,
	SCROLLER_BRING_TYPE_JOB,
	SCROLLER_BRING_TYPE_MAX,
} scroller_bring_type_e;

extern Evas_Object *scroller_get_focused_page(Evas_Object *scroller);
extern void scroller_destroy(Evas_Object *layout);
extern Evas_Object *scroller_create(Evas_Object *layout, Evas_Object *parent, int page_width, int page_height, scroller_index_e index);

extern w_home_error_e scroller_push_page(Evas_Object *scroller, Evas_Object *page, scroller_push_type_e scroller_type);
extern Evas_Object *scroller_pop_page(Evas_Object *scroller, Evas_Object *page);

extern w_home_error_e scroller_push_pages(Evas_Object *scroller, Eina_List *page_info_list, void (*after_func)(void *), void *data);
extern void scroller_pop_pages(Evas_Object *scroller, page_direction_e direction);

extern w_home_error_e scroller_push_page_before(Evas_Object *scroller
		, Evas_Object *page
		, Evas_Object *before);
extern w_home_error_e scroller_push_page_after(Evas_Object *scroller
		, Evas_Object *page
		, Evas_Object *after);

extern int scroller_count(Evas_Object *scroller);
extern int scroller_count_direction(Evas_Object *scroller, page_direction_e direction);

extern Eina_Bool scroller_is_scrolling(Evas_Object *scroller);

extern void scroller_freeze(Evas_Object *scroller);
extern void scroller_unfreeze(Evas_Object *scroller);

extern void scroller_bring_in(Evas_Object *scroller, int i, scroller_freeze_e freeze, scroller_bring_type_e bring_type);
extern void scroller_bring_in_page(Evas_Object *scroller, Evas_Object *page, scroller_freeze_e freeze, scroller_bring_type_e bring_type);
extern void scroller_bring_in_by_push_type(Evas_Object *scroller, scroller_push_type_e push_type, scroller_freeze_e freeze, scroller_bring_type_e bring_type);
extern void scroller_bring_in_center_of(Evas_Object *scroller
		, Evas_Object *page
		, scroller_freeze_e freeze
		, void (*before_func)(void *), void *before_data
		, void (*after_func)(void *), void *after_data);

extern void scroller_region_show_page(Evas_Object *scroller, Evas_Object *page, scroller_freeze_e freeze, scroller_bring_type_e bring_type);
extern void scroller_region_show_by_push_type(Evas_Object *scroller, scroller_push_type_e push_type, scroller_freeze_e freeze, scroller_bring_type_e bring_type);
extern void scroller_region_show_center_of(Evas_Object *scroller
		, Evas_Object *page
		, scroller_freeze_e freeze
		, void (*before_func)(void *), void *before_data
		, void (*after_func)(void *), void *after_data);
extern void scroller_region_show_page_without_timer(Evas_Object *scroller, Evas_Object *page);

extern void scroller_read_favorites_list(Evas_Object *scroller, Eina_List *page_info_list);
extern void scroller_read_list(Evas_Object *scroller, Eina_List *page_info_list);
extern Eina_List *scroller_write_list(Evas_Object *scroller);

extern Evas_Object *scroller_move_page_prev(Evas_Object *scroller, Evas_Object *from_page, Evas_Object *to_page, Evas_Object *append_page);
extern Evas_Object *scroller_move_page_next(Evas_Object *scroller, Evas_Object *from_page, Evas_Object *to_page, Evas_Object *insert_page);
extern int scroller_seek_page_position(Evas_Object *scroller, Evas_Object *page);

extern Evas_Object *scroller_get_page_at(Evas_Object *scroller, int idx);

extern int scroller_get_current_page_direction(Evas_Object *scroller);
extern Evas_Object *scroller_get_left_page(Evas_Object *scroller, Evas_Object *page);
extern Evas_Object *scroller_get_right_page(Evas_Object *scroller, Evas_Object *page);

extern void scroller_focus(Evas_Object *scroller);

extern void scroller_highlight(Evas_Object *scroller);
extern void scroller_unhighlight(Evas_Object *scroller);

extern void scroller_enable_focus_on_scroll(Evas_Object *scroller);
extern void scroller_disable_focus_on_scroll(Evas_Object *scroller);

extern void scroller_enable_index_on_scroll(Evas_Object *scroller);
extern void scroller_disable_index_on_scroll(Evas_Object *scroller);

extern void scroller_enable_effect_on_scroll(Evas_Object *scroller);
extern void scroller_disable_effect_on_scroll(Evas_Object *scroller);

extern void scroller_enable_cover_on_scroll(Evas_Object *scroller);
extern void scroller_disable_cover_on_scroll(Evas_Object *scroller);

extern void scroller_backup_inner_focus(Evas_Object *scroller);
extern void scroller_restore_inner_focus(Evas_Object *scroller);

extern void scroller_reorder_with_list(Evas_Object *scroller, Eina_List *list, page_direction_e page_direction);

#endif /* __W_HOME_SCROLLER_H__ */
