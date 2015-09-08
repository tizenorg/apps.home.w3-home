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

#ifndef __APPS_SCROLLER_H__
#define __APPS_SCROLLER_H__

#include <Evas.h>

#include "apps/apps_main.h"
#include "util.h"

HAPI void apps_scroller_destroy(Evas_Object *layout);
HAPI Evas_Object *apps_scroller_create(Evas_Object *layout);

HAPI int apps_scroller_count_page(Evas_Object *scroller);
HAPI int apps_scroller_count_item(Evas_Object *scroller);
HAPI Eina_Bool apps_scroller_is_scrolling(Evas_Object *scroller);

HAPI void apps_scroller_freeze(Evas_Object *scroller);
HAPI void apps_scroller_unfreeze(Evas_Object *scroller);

HAPI void apps_scroller_bring_in(Evas_Object *scroller, int page_no);
HAPI void apps_scroller_bring_in_page(Evas_Object *scroller, Evas_Object *page);

HAPI void apps_scroller_region_show(Evas_Object *scroller, int x, int y);

HAPI void apps_scroller_append_item(Evas_Object *scroller, Evas_Object *item);
HAPI void apps_scroller_remove_item(Evas_Object *scroller, Evas_Object *item);

HAPI void apps_scroller_append_list(Evas_Object *scroller, Eina_List *list);
HAPI void apps_scroller_remove_list(Evas_Object *scroller, Eina_List *list);

HAPI void apps_scroller_trim(Evas_Object *scroller);

HAPI void apps_scroller_change_language(Evas_Object *scroller);

/* return a page that has the item */
HAPI Evas_Object *apps_scroller_has_item(Evas_Object *scroller, Evas_Object *item);

HAPI Evas_Object *apps_scroller_move_item_prev(Evas_Object *scroller, Evas_Object *from_item, Evas_Object *to_item, Evas_Object *append_item);
HAPI Evas_Object *apps_scroller_move_item_next(Evas_Object *scroller, Evas_Object *from_item, Evas_Object *to_item, Evas_Object *insert_item);

HAPI int apps_scroller_seek_item_position(Evas_Object *scroller, Evas_Object *item);
HAPI void apps_scroller_bring_in_region_by_vector(Evas_Object *scroller, int vector);

HAPI apps_error_e apps_scroller_read_list(Evas_Object *scroller, Eina_List *item_info_list);
HAPI void apps_scroller_write_list(Evas_Object *scroller);

HAPI Evas_Object *apps_scroller_get_item_by_pkgid(Evas_Object *scroller, const char *pkgid);
HAPI Evas_Object *apps_scroller_get_item_by_appid(Evas_Object *scroller, const char *appid);

HAPI void apps_scroller_edit(Evas_Object *scroller);
HAPI void apps_scroller_unedit(Evas_Object *scroller);

#endif //__APPS_SCROLLER_H__

// End of a file
