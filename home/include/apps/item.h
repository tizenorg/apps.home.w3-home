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

#ifndef __APPS_ITEM_H__
#define __APPS_ITEM_H__

#include <Evas.h>
#include "apps/item_info.h"
#include "util.h"

HAPI void item_destroy(Evas_Object *item);
HAPI Evas_Object *item_create(Evas_Object *scroller, item_info_s *info);

HAPI Evas_Object *item_virtual_create(Evas_Object *scroller);
HAPI void item_virtual_destroy(Evas_Object *item);

HAPI void item_change_language(Evas_Object *item);

HAPI void item_edit(Evas_Object *item);
HAPI void item_unedit(Evas_Object *item);

HAPI Evas_Object* item_get_press_item(Evas_Object *scroller);
HAPI int item_is_longpressed(Evas_Object *item);

HAPI void item_is_pressed(Evas_Object *item);
HAPI void item_is_released(Evas_Object *item);

#endif // __APPS_ITEM_H__
