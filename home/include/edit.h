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

#ifndef __W_HOME_EDIT_H__
#define __W_HOME_EDIT_H__

#include "page.h"

typedef enum {
	EDIT_MODE_INVALID = 0,
	EDIT_MODE_LEFT,
	EDIT_MODE_CENTER,
	EDIT_MODE_RIGHT,
	EDIT_MODE_MAX,
} edit_mode_e;

extern Evas_Object *edit_create_layout(Evas_Object *layout, edit_mode_e edit_mode);
extern void edit_destroy_layout(void *layout);

extern Evas_Object *edit_create_proxy_page(Evas_Object *edit_scroller, Evas_Object *real_page, page_changeable_bg_e changable_color);
extern void edit_destroy_proxy_page(Evas_Object *proxy_page);

extern Evas_Object *edit_create_add_viewer(Evas_Object *layout);
extern void edit_destroy_add_viewer(Evas_Object *layout);

extern void edit_change_focus(Evas_Object *edit_scroller, Evas_Object *page_current);

extern w_home_error_e edit_push_page_before(Evas_Object *scroller
		, Evas_Object *page
		, Evas_Object *before);
extern w_home_error_e edit_push_page_after(Evas_Object *scroller
		, Evas_Object *page
		, Evas_Object *after);

extern void edit_minify_effect_page(void *effect_page);
extern Evas_Object *edit_create_minify_effect_page(Evas_Object *page);
extern void edit_destroy_minify_effect_page(Evas_Object *effect_page);

extern void edit_enlarge_effect_page(void *effect_page);
extern Evas_Object *edit_create_enlarge_effect_page(Evas_Object *page);
extern void edit_destroy_enlarge_effect_page(Evas_Object *effect_page);

extern void edit_arrange_plus_page(Evas_Object *edit);
extern char *edit_get_count_str_from_icu(int count);

#endif /* __W_HOME_EDIT_H__ */
