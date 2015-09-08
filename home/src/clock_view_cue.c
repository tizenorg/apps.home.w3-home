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

#include <Elementary.h>
#include <Evas.h>
#include <stdbool.h>
#include <vconf.h>
#include <bundle.h>
#include <aul.h>
#include <efl_assist.h>
#include <dlog.h>

#include "conf.h"
#include "layout.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "page_info.h"
#include "scroller_info.h"
#include "page.h"
#include "scroller.h"
#include "key.h"
#include "effect.h"
#include "clock_service.h"

static Evas_Object *_cue_layout_add(Evas_Object *page)
{
	Eina_Bool ret = EINA_TRUE;
	Evas_Object *item = page_get_item(page);
	retv_if(item == NULL, NULL);

	Evas_Object *layout = elm_layout_add(item);
	retv_if(layout == NULL, NULL);
	ret = elm_layout_file_set(layout, PAGE_CLOCK_EDJE_FILE, "bottom_cue");
	retv_if(ret == EINA_FALSE, NULL);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(layout);

	return layout;
}

HAPI Evas_Object *clock_view_cue_add(Evas_Object *page)
{
	Evas_Object *cue_layout = NULL;
	retv_if(page == NULL, NULL);

	cue_layout = _cue_layout_add(page);
	retv_if(cue_layout == NULL, NULL);

	return cue_layout;
}

HAPI void clock_view_cue_display_set(Evas_Object *page, int is_display, int is_need_vi)
{
	Evas_Object *item = NULL;
	Evas_Object *cue = NULL;
	ret_if(page == NULL);

	_D("Cue for Apps, is_display:%d, is_need_vi:%d", is_display, is_need_vi);

	if (util_feature_enabled_get(FEATURE_CLOCK_VISUAL_CUE) == 0 && is_display == 1) {
		return;
	}

	item =  page_get_item(page);
	ret_if(item == NULL);

	cue = elm_object_part_content_get(item, "bottom_cue");
	ret_if(cue == NULL);

	if (is_display == 1) {
		elm_object_signal_emit(cue, "cue,enable", "prog");
	} else {
		elm_object_signal_emit(cue, "cue,disable", "prog");
	}
}
