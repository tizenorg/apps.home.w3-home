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

#ifndef _W_HOME_NOTIFICATION_SUMMARY_H_
#define _W_HOME_NOTIFICATION_SUMMARY_H_

#include <Elementary.h>

extern Evas_Object *summary_get_page(Evas_Object *scroller, const char *pkgname);

extern void summary_destroy_item(Evas_Object *item);
extern Evas_Object *summary_create_item(Evas_Object *page, const char *pkgname, const char *icon_path, const char *title, const char *content, int count, time_t *time);

extern Evas_Object *summary_create_page(Evas_Object *parent, const char *pkgname, const char *icon_path, const char *title, const char *content, int count, time_t *time);
extern void summary_destroy_page(Evas_Object *page);


#endif // _W_HOME_NOTIFICATION_SUMMARY_H_
// End of file
