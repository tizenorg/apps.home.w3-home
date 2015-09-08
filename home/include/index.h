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

#ifndef __W_HOME_INDEX_H__
#define __W_HOME_INDEX_H__

#include "page.h"

typedef enum {
	INDEX_BRING_IN_NONE = 0,
	INDEX_BRING_IN_AFTER,
} index_bring_in_e;

extern void index_bring_in_page(Evas_Object *index, Evas_Object *page);

extern Evas_Object *index_create(Evas_Object *layout, Evas_Object *scroller, page_direction_e direction);
extern void index_destroy(Evas_Object *index);
extern void index_update(Evas_Object *index, Evas_Object *scroller, index_bring_in_e after);

#endif /* __W_HOME_INDEX_H__ */
