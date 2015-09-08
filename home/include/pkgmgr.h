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

#ifndef __W_HOME_PKGMGR_H__
#define __W_HOME_PKGMGR_H__

extern void pkgmgr_hold_event(void);
extern void pkgmgr_unhold_event(void);

extern w_home_error_e pkgmgr_init(void);
extern void pkgmgr_fini(void);

extern w_home_error_e pkgmgr_uninstall(const char *appid);

extern w_home_error_e pkgmgr_item_list_append_item(const char *pkg_id, const char *app_id, Evas_Object *item);
extern w_home_error_e pkgmgr_item_list_remove_item(const char *pkg_id, const char *app_id, Evas_Object *item);

extern void pkgmgr_item_list_affect_pkgid(const char *pkg_id, Eina_Bool (*_affected_cb)(const char *, Evas_Object *, void *), void *data);
extern void pkgmgr_item_list_affect_appid(const char *app_id, Eina_Bool (*_affected_cb)(const char *, Evas_Object *, void *), void *data);

extern w_home_error_e pkgmgr_reserve_list_push_request(const char *package, const char *key, const char *val);
extern w_home_error_e pkgmgr_reserve_list_pop_request(void);

extern int pkgmgr_exist(char *appid);

#endif //__W_HOME_PKGMGR_H__

// End of a file
