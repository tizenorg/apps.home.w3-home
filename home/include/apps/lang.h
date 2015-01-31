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

#ifndef __APPS_LANG_H__
#define __APPS_LANG_H__

#include <Elementary.h>

#include "util.h"

HAPI apps_error_e apps_lang_add_id(Evas_Object *obj, const char *group, const char *id, int domain);
HAPI void apps_lang_remove_id(Evas_Object *obj, const char *group);

HAPI apps_error_e apps_lang_register_cb(apps_error_e (*result_cb)(void *), void *result_data);
HAPI void apps_lang_unregister_cb(apps_error_e (*result_cb)(void *));

HAPI void apps_lang_refresh_ids(void);

#endif /* __APPS_LANG_H__ */
