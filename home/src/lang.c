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
#include <bundle.h>
#include <dlog.h>

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

typedef struct {
	w_home_error_e (*result_cb)(void *);
	void *result_data;
} lang_cb_s;



HAPI w_home_error_e lang_add_id(Evas_Object *obj, const char *group, const char *id, int domain)
{
	lang_element *le;

	retv_if(NULL == obj, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(NULL == group, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(NULL == id, W_HOME_ERROR_INVALID_PARAMETER);

	le = malloc(sizeof(lang_element));
	retv_if(NULL == le, W_HOME_ERROR_FAIL);

	le->obj = obj;
	le->group = group;
	le->id = id;
	le->domain = domain;

	lang_list = eina_list_append(lang_list, le);

	return W_HOME_ERROR_NONE;
}



HAPI void lang_remove_id(Evas_Object *obj, const char *group)
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



HAPI w_home_error_e lang_register_cb(w_home_error_e (*result_cb)(void *), void *result_data)
{
	retv_if(NULL == result_cb, W_HOME_ERROR_INVALID_PARAMETER);

	lang_cb_s *cb = calloc(1, sizeof(lang_cb_s));
	retv_if(NULL == cb, W_HOME_ERROR_FAIL);

	cb->result_cb = result_cb;
	cb->result_data = result_data;

	cbs_list = eina_list_append(cbs_list, cb);
	retv_if(NULL == cbs_list, W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}



HAPI void lang_unregister_cb(w_home_error_e (*result_cb)(void *))
{
	const Eina_List *l;
	const Eina_List *n;
	lang_cb_s *cb;
	EINA_LIST_FOREACH_SAFE(cbs_list, l, n, cb) {
		continue_if(NULL == cb);
		if (result_cb != cb->result_cb) continue;
		cbs_list = eina_list_remove(cbs_list, cb);
		free(cb);
		return;
	}
}



HAPI void lang_refresh_ids(void)
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

	lang_cb_s *cb;
	EINA_LIST_FOREACH_SAFE(cbs_list, l, n, cb) {
		continue_if(NULL == cb);
		continue_if(NULL == cb->result_cb);

		if (W_HOME_ERROR_NONE != cb->result_cb(cb->result_data)) _E("There are some problems");
	}
}

// End of a file
