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

#include <Evas.h>
#include <stdlib.h>
#include <bundle.h>
#include <dlog.h>

#include "log.h"
#include "util.h"
#include "item_info.h"

HAPI item_info_s *item_info_create(const char *id, const char *subid)
{
	item_info_s *item_info = NULL;

	item_info = calloc(1, sizeof(item_info_s));
	retv_if(!item_info, NULL);

	if (id) {
		item_info->id = strdup(id);
		if (!item_info->id) {
			free(item_info);
			return NULL;
		}
	}

	if (subid) {
		item_info->subid = strdup(subid);
		if (!item_info->subid) {
			free(item_info->id);
			free(item_info);
			return NULL;
		}
	}

	if (id && item_info_is_removable(id)) {
		item_info->removable = 1;
	}

	return item_info;
}



HAPI void item_info_destroy(item_info_s *item_info)
{
	ret_if(!item_info);

	free(item_info->id);
	free(item_info->subid);
	free(item_info);
}



HAPI void item_info_list_destroy(Eina_List *item_info_list)
{
	item_info_s *item_info = NULL;

	ret_if(!item_info_list);

	EINA_LIST_FREE(item_info_list, item_info) {
		continue_if(!item_info);
		item_info_destroy(item_info);
	}
}



#define APPID_APPS "org.tizen.apps-dbox"
HAPI int item_info_is_removable(const char *id)
{
	retv_if(!id, 1);
	return strcmp(id, APPID_APPS);
}



// End of a file
