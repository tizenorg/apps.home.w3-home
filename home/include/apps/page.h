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

#ifndef __APPS_PAGE_H__
#define __APPS_PAGE_H__

#include <Evas.h>
#include "util.h"

HAPI void apps_page_destroy(Evas_Object *page);
HAPI Evas_Object *apps_page_create(Evas_Object *scroller, Evas_Object *prev_page, Evas_Object *next_page);

HAPI apps_error_e apps_page_pack_item(Evas_Object *page, Evas_Object *item);
HAPI apps_error_e apps_page_unpack_item(Evas_Object *page, Evas_Object *item);

HAPI int apps_page_count_item(Evas_Object *page);

HAPI Evas_Object *apps_page_get_item_at(Evas_Object *page, int position);

HAPI apps_error_e apps_page_pack_nth(Evas_Object *page, Evas_Object *item, int position);
HAPI Evas_Object *apps_page_unpack_nth(Evas_Object *page, int position);

HAPI void apps_page_change_language(Evas_Object *page);

HAPI int apps_page_has_item(Evas_Object *page, Evas_Object *item);

/*
 * origin order :
 * ---------------------------------------------------------------------------
 * |   from_item   | from_item + 1 |     ...     | to_item - 1 |   to_item   |
 * ---------------------------------------------------------------------------
 * changed order : from_item will be returned.
 * ---------------------------------------------------------------------------
 * | from_item + 1 | from_item + 2 |     ...     |   to_item   | append_item |
 * ---------------------------------------------------------------------------
 */
HAPI Evas_Object *apps_page_move_item_prev(Evas_Object *page, Evas_Object *from_item, Evas_Object *to_item, Evas_Object *append_item);

/*
 * origin order :
 * ---------------------------------------------------------------------------
 * |   from_item   | from_item + 1 |     ...     | to_item - 1 |   to_item   |
 * ---------------------------------------------------------------------------
 * changed order : to_item will be returned.
 * ---------------------------------------------------------------------------
 * |  insert_item  |  from_item    |     ...     |     ...     | to_item - 1 |
 * ---------------------------------------------------------------------------
 */
HAPI Evas_Object *apps_page_move_item_next(Evas_Object *page, Evas_Object *from_item, Evas_Object *to_item, Evas_Object *insert_item);

HAPI int apps_page_seek_item_position(Evas_Object *page, Evas_Object *item);
HAPI void apps_page_print_item(Evas_Object *page);

HAPI void apps_page_edit(Evas_Object *page);
HAPI void apps_page_unedit(Evas_Object *page);

#endif //__APPS_PAGE_H__

// End of a file
