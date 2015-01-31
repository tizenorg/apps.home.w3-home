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

#ifndef __W_HOME_PAGE_H__
#define __W_HOME_PAGE_H__

#include <Evas.h>
#include "page_info.h"
#include "util.h"

typedef enum {
	PAGE_CHANGEABLE_BG_OFF = 0,
	PAGE_CHANGEABLE_BG_ON,
} page_changeable_bg_e;

typedef enum {
	PAGE_EFFECT_TYPE_LEFT = 0,
	PAGE_EFFECT_TYPE_RIGHT,
} page_effect_type_e;

extern void page_destroy(Evas_Object *page);
extern Evas_Object *page_create(Evas_Object *scroller
	, Evas_Object *item
	, const char *id
	, const char *subid
	, int width, int height
	, page_changeable_bg_e changeable_bg
	, page_removable_e removable);

/* page_set_item returns the old item. You have to delete the old item */
extern Evas_Object *page_set_item(Evas_Object *page, Evas_Object *item);
extern Evas_Object *page_get_item(Evas_Object *page);

extern Evas_Object *page_create_plus_page(Evas_Object *scroller);
extern void page_destroy_plus_page(Evas_Object *scroller);
extern void page_arrange_plus_page(Evas_Object *scroller, int toast_popup);

extern void page_focus(Evas_Object *page);
extern void page_unfocus(Evas_Object *page);
extern char *page_read_title(Evas_Object *page);

extern void page_set_effect(Evas_Object *page, void (*left_effect)(Evas_Object *), void (*right_effect)(Evas_Object *));
extern void page_unset_effect(Evas_Object *page);
extern void *page_get_effect(Evas_Object *page, page_effect_type_e effect_type);
extern void page_clean_effect(Evas_Object *scroller);

extern void page_effect_none(Evas_Object *page);
extern void page_effect_smaller_by_remainder(Evas_Object *page);

extern void page_backup_inner_focus(Evas_Object *page, Evas_Object *prev_page, Evas_Object *next_page);
extern void page_restore_inner_focus(Evas_Object *page);

extern void page_set_title(Evas_Object *page, const char *title);
extern const char *page_get_title(Evas_Object *page);

extern void page_highlight(Evas_Object *page);
extern void page_unhighlight(Evas_Object *page);

extern int page_is_appended(Evas_Object *page);


#endif /* __W_HOME_PAGE_H__ */
