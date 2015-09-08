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

#ifndef __APPS_SCROLLER_INFO_H__
#define __APPS_SCROLLER_INFO_H__

#include <Evas.h>
#include "apps/apps_main.h"

typedef struct {
	Evas_Object *win;
	instance_info_s *instance_info;

	Evas_Object *layout;
	Evas_Object *box_layout;
	Evas_Object *box;
	Evas_Object *layout_focus;
	Evas_Object *top_focus;
	Evas_Object *bottom_focus;

	Eina_List *list;
	int list_index;
} scroller_info_s;

#endif //__APPS_SCROLLER_INFO_H__

// End of a file
