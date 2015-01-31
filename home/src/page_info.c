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

#include <stdlib.h>
#include <Eina.h>
#include <Evas.h>
#include <bundle.h>
#include <dlog.h>

#include "log.h"
#include "page_info.h"
#include "util.h"

HAPI page_info_s *page_info_create(const char *id, const char *subid, double period)
{
	page_info_s *page_info = NULL;

	page_info = calloc(1, sizeof(page_info_s));
	retv_if(!page_info, NULL);

	if (id) {
		page_info->id = strdup(id);
		if (!page_info->id) {
			free(page_info);
			return NULL;
		}
	}

	if (subid) {
		page_info->subid = strdup(subid);
		if (!page_info->subid) {
			free(page_info->id);
			free(page_info);
			return NULL;
		}
	}

	if (id) {
		page_info->removable = 1;
	}

	page_info->period = period;

	return page_info;
}



HAPI void page_info_destroy(page_info_s *page_info)
{
	ret_if(!page_info);

	free(page_info->id);
	free(page_info->subid);
	free(page_info);
}



HAPI page_info_s *page_info_dup(page_info_s *page_info)
{
	page_info_s *dup = NULL;

	retv_if(!page_info, NULL);

	dup = page_info_create(page_info->id, page_info->subid, page_info->period);
	retv_if(!dup, NULL);

	if (page_info->title) {
		dup->title = strdup(page_info->title);
	}
	dup->width = page_info->width;
	dup->height = page_info->height;
	dup->direction = page_info->direction;
	dup->category = page_info->category;
	dup->removable = page_info->removable;

	dup->layout = page_info->layout;
	dup->scroller = page_info->scroller;
	dup->page = page_info->page;
	dup->page_rect = page_info->page_rect;
	dup->page_inner = page_info->page_inner;
	dup->page_inner_area = page_info->page_inner_area;
	dup->page_inner_bg = page_info->page_inner_bg;
	dup->focus = page_info->focus;
	dup->remove_focus = page_info->remove_focus;
	dup->item = page_info->item;
	dup->focus_prev = page_info->focus_prev;
	dup->focus_next = page_info->focus_next;

	dup->ordering = page_info->ordering;
	dup->layout_longpress = page_info->layout_longpress;
	dup->highlighted = page_info->highlighted;
	dup->need_to_unhighlight = page_info->need_to_unhighlight;
	dup->highlight_changed = page_info->highlight_changed;
	dup->is_scrolled_object = page_info->is_scrolled_object;

	return dup;
}



HAPI void page_info_list_destroy(Eina_List *page_info_list)
{
	page_info_s *page_info = NULL;

	ret_if(!page_info_list);

	EINA_LIST_FREE(page_info_list, page_info) {
		continue_if(!page_info);
		page_info_destroy(page_info);
	}
}



HAPI int page_info_is_removable(const char *id)
{
	if(!id) return 0;
	return 1;
}



// End of a file
