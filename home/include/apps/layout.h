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

#ifndef __APPS_LAYOUT_H__
#define __APPS_LAYOUT_H__

#include <Evas.h>
#include "util.h"

HAPI Evas_Object *apps_layout_create(Evas_Object *win, const char *file, const char *group);
HAPI void apps_layout_destroy(Evas_Object *layout);

HAPI apps_error_e apps_layout_show(Evas_Object *win, Eina_Bool show);

HAPI Evas_Object* apps_layout_load_edj(Evas_Object *parent, const char *edjname, const char *grpname);
HAPI void apps_layout_unload_edj(Evas_Object *layout);

HAPI void apps_layout_rotate(Evas_Object *layout, int is_rotated);

HAPI void apps_layout_block(Evas_Object *layout);
HAPI void apps_layout_unblock(Evas_Object *layout);

HAPI int apps_layout_is_edited(Evas_Object *layout);
HAPI void apps_layout_edit(Evas_Object *layout);
HAPI void apps_layout_unedit(Evas_Object *layout);

#endif //__APPS_LAYOUT_H__

// End of a file
