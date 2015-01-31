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

#ifndef __W_HOME_ITEM_INFO_H__
#define __W_HOME_ITEM_INFO_H__

typedef enum {
	ITEM_TYPE_IDLE_CLOCK = 0,
	ITEM_TYPE_APP,
	ITEM_TYPE_WIDGET,
	ITEM_TYPE_MORE_APPS,
	ITEM_TYPE_NXN_PAGE,
	ITEM_TYPE_MAX,
} item_type_e;

typedef enum {
	ITEM_ORDERING_ALL = 0,
	ITEM_ORDERING_FIRST_TO_CENTER_LEFT,
	ITEM_ORDERING_FIRST_TO_CENTER,
	ITEM_ORDERING_CENTER_TO_LAST,
	ITEM_ORDERING_CENTER_RIGHT_TO_LAST,
	ITEM_ORDERING_MAX,
} item_ordering_e;

typedef struct {
	/* innate features */
	char *id;
	char *subid;
	item_type_e category;
	int ordering;

	/* acquired features */
	Evas_Object *page;
	Evas_Object *item;
	int removable;
} item_info_s;

extern item_info_s *item_info_create(const char *id, const char *subid);
extern void item_info_destroy(item_info_s *item_info);
extern void item_info_list_destroy(Eina_List *page_info_list);

extern int item_info_is_removable(const char *id);

#endif // __W_HOME_ITEM_INFO_H__
