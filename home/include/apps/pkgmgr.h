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

#ifndef __APPS_PKGMGR_H__
#define __APPS_PKGMGR_H__

#include <Evas.h>

#include "apps/list.h"
#include "util.h"



HAPI apps_error_e apps_pkgmgr_init(void);
HAPI void apps_pkgmgr_fini(void);

HAPI apps_error_e apps_pkgmgr_item_list_append_item(const char *pkgid, const char *app_id, Evas_Object *item);
HAPI apps_error_e apps_pkgmgr_item_list_remove_item(const char *pkgid, const char *app_id, Evas_Object *item);
HAPI void apps_pkgmgr_item_list_affect_pkgid(const char *pkgid, Eina_Bool (*_affected_cb)(const char *, Evas_Object *, void *), void *data);
HAPI void apps_pkgmgr_item_list_affect_appid(const char *app_id, Eina_Bool (*_affected_cb)(const char *, Evas_Object *, void *), void *data);

HAPI apps_error_e apps_pkgmgr_item_list_enable_mounted_item(void);
HAPI apps_error_e apps_pkgmgr_item_list_disable_unmounted_item(void);

#endif //__APPS_PKGMGR_H__

// End of a file
