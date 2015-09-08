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

#ifndef _W_HOME_NOTIFICATION_DETAIL_H_
#define _W_HOME_NOTIFICATION_DETAIL_H_

#include <Elementary.h>

typedef struct {
	/* Innate features */
	char *pkgname;
	char *icon;
	char *title;
	char *content;
	time_t time;
	int priv_id;

	/* Acquire features */
	Evas_Object *item;
} detail_item_s;



enum {
	DETAIL_EVENT_REMOVE_ITEM = 0,
};



extern int detail_register_event_cb(int type, void (*event_cb)(void *, void *), void *data);
extern void detail_unregister_event_cb(int type, void (*event_cb)(void *, void *));

extern detail_item_s *detail_list_append_info(int priv_id, const char *pkgname, const char *icon, const char *title, const char *content, time_t time);
extern detail_item_s *detail_list_remove_list(int priv_id);
extern void detail_list_remove_info(int priv_id);
extern void detail_list_remove_pkgname(const char *pkgname);
extern detail_item_s *detail_list_get_info(int priv_id);

extern int detail_list_count_pkgname(const char *pkgname);
extern detail_item_s *detail_list_get_latest_info(const char *pkgname);

extern void detail_destroy_item(detail_item_s *detail_item_info);
extern Evas_Object *detail_create_item(Evas_Object *parent, detail_item_s *detail_item_info);

extern void detail_destroy_scroller(Evas_Object *scroller);
extern Evas_Object *detail_create_scroller(Evas_Object *win, const char *pkgname);

extern void detail_destroy_win(Evas_Object *win);
extern Evas_Object *detail_create_win(const char *pkgname);


#endif // _W_HOME_NOTIFICATION_DETAIL_H_
// End of file
