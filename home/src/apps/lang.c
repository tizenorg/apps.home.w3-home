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

#include <Elementary.h>

#include "log.h"
#include "util.h"



static Eina_List *lang_list;
static Eina_List *cbs_list;

typedef struct {
	Evas_Object *obj;
	const char *group;
	const char *id;
	int domain;
} lang_element;



HAPI apps_error_e apps_lang_add_id(Evas_Object *obj, const char *group, const char *id, int domain)
{
	lang_element *le;

	retv_if(NULL == obj, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == group, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == id, APPS_ERROR_INVALID_PARAMETER);

	le = malloc(sizeof(lang_element));
	retv_if(NULL == le, APPS_ERROR_FAIL);

	le->obj = obj;
	le->group = group;
	le->id = id;
	le->domain = domain;

	lang_list = eina_list_append(lang_list, le);

	return APPS_ERROR_NONE;
}



HAPI void apps_lang_remove_id(Evas_Object *obj, const char *group)
{
	const Eina_List *l;
	const Eina_List *n;
	lang_element *le;

	ret_if(NULL == obj);
	ret_if(NULL == group);

	EINA_LIST_FOREACH_SAFE(lang_list, l, n, le) {
		if (le->obj == obj && !strcmp(group, le->group)) {
			lang_list = eina_list_remove(lang_list, le);
			free(le);
			return;
		}
	}
}



typedef struct {
	apps_error_e (*result_cb)(void *);
	void *result_data;
} cb_s;



HAPI apps_error_e apps_lang_register_cb(apps_error_e (*result_cb)(void *), void *result_data)
{
	cb_s *cb = NULL;

	retv_if(NULL == result_cb, APPS_ERROR_INVALID_PARAMETER);

	cb = calloc(1, sizeof(cb_s));
	retv_if(NULL == cb, APPS_ERROR_FAIL);

	cb->result_cb = result_cb;
	cb->result_data = result_data;

	cbs_list = eina_list_append(cbs_list, cb);
	retv_if(NULL == cbs_list, APPS_ERROR_FAIL);

	return APPS_ERROR_NONE;
}



HAPI void apps_lang_unregister_cb(apps_error_e (*result_cb)(void *))
{
	const Eina_List *l;
	const Eina_List *n;
	cb_s *cb;

	EINA_LIST_FOREACH_SAFE(cbs_list, l, n, cb) {
		continue_if(NULL == cb);
		if (result_cb != cb->result_cb) continue;
		cbs_list = eina_list_remove(cbs_list, cb);
		free(cb);
		return;
	}
}



HAPI void apps_lang_refresh_ids(void)
{
	const Eina_List *l;
	const Eina_List *n;
	lang_element *le;
	char *temp;

	EINA_LIST_FOREACH_SAFE(lang_list, l, n, le) {
		if (le->domain) {
			temp = D_(le->id);
		} else {
			temp = _(le->id);
		}

		elm_object_part_text_set(le->obj, le->group, temp);
	}

	cb_s *cb;
	EINA_LIST_FOREACH_SAFE(cbs_list, l, n, cb) {
		continue_if(NULL == cb);
		continue_if(NULL == cb->result_cb);

		if (APPS_ERROR_NONE != cb->result_cb(cb->result_data)) _APPS_E("There are some problems");
	}
}

// End of a file
