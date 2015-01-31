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

#ifndef __APPS_ITEM_BADGE_H__
#define __APPS_ITEM_BADGE_H__

#include <Elementary.h>

#include "util.h"

HAPI int item_badge_count(Evas_Object *item);

HAPI int item_badge_is_registered(const char *appid);
HAPI apps_error_e item_badge_init(Evas_Object *item);
HAPI void item_badge_destroy(Evas_Object *item);

HAPI void item_badge_register_changed_cb(Evas_Object *layout);
HAPI void item_badge_unregister_changed_cb(void);

HAPI void item_badge_show(Evas_Object *item, int count);
HAPI void item_badge_hide(Evas_Object *item);
HAPI void item_badge_change_language(Evas_Object *item);
HAPI void item_badge_remove(const char *pkgid);

#endif //__APPS_ITEM_BADGE_H__

// End of a file
