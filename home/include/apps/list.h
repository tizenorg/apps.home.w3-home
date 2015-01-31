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

#ifndef __APPS_LIST_H__
#define __APPS_LIST_H__

#include <Evas.h>
#include <stdbool.h>

#include "apps/item_info.h"
#include "util.h"

typedef struct _app_list {
	Eina_List *list;
	unsigned int cur_idx;
} app_list;

HAPI bool list_is_included(const char *id);

HAPI app_list *list_create(void);
HAPI app_list *list_create_by_appid(void);
HAPI void list_destroy(app_list *list);

HAPI void list_change_language(app_list *list);

#endif //__APPS_LIST_H__

// End of a file
