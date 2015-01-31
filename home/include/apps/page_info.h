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

#ifndef __APPS_PAGE_INFO_H__
#define __APPS_PAGE_INFO_H__

#include <Evas.h>
#include "apps/apps_main.h"

typedef struct {
	Evas_Object *win;
	instance_info_s *instance_info;

	Evas_Object *scroller;
	Evas_Object *page_rect;

	Evas_Object *prev_page;
	Evas_Object *page;
	Evas_Object *next_page;
} page_info_s;

#endif //__APPS_PAGE_INFO_H__

// End of a file
