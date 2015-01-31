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

#ifndef __APP_TRAY_LANG_H__
#define __APP_TRAY_LANG_H__

extern w_home_error_e lang_add_id(Evas_Object *obj, const char *group, const char *id, int domain);
extern void lang_remove_id(Evas_Object *obj, const char *group);

extern w_home_error_e lang_register_cb(w_home_error_e (*result_cb)(void *), void *result_data);
extern void lang_unregister_cb(w_home_error_e (*result_cb)(void *));

extern void lang_refresh_ids(void);

#endif /* __APP_TRAY_LANG_H__ */
