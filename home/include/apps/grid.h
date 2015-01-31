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

#ifndef __APPS_GRID_H__
#define __APPS_GRID_H__

#include <Evas.h>

#include "util.h"
#include "apps/list.h"

HAPI void grid_destroy(Evas_Object *win, Evas_Object *grid);
HAPI Evas_Object *grid_create(Evas_Object *win, Evas_Object *parent);
HAPI void grid_refresh(Evas_Object *grid);

HAPI void grid_show_top(Evas_Object *grid);

HAPI void grid_append_list(Evas_Object *grid, Eina_List *list);
HAPI void grid_remove_list(Evas_Object *grid, Eina_List *list);

#endif //__APPS_GRID_H__

// End of a file
