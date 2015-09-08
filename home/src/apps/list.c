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
#include <ail.h>
#include <package-manager.h>
#include <pkgmgr-info.h>

#include "log.h"
#include "apps/item_info.h"
#include "apps/list.h"



static int _is_included_cb(pkgmgrinfo_appinfo_h handle, void *user_data)
{
	bool enabled = false;

	retv_if(NULL == handle, 0);
	retv_if(NULL == user_data, 0);
	retv_if(PMINFO_R_OK != pkgmgrinfo_appinfo_is_enabled(handle, &enabled), 0);

	* (bool *) user_data = enabled;

	return 0;
}



HAPI bool list_is_included(const char *appid)
{
	pkgmgrinfo_appinfo_filter_h handle = NULL;
	bool is_included = false;

	retv_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_create(&handle), NULL);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_add_string(handle, PMINFO_APPINFO_PROP_APP_ID, appid), ERROR);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, _is_included_cb, &is_included), ERROR);

	pkgmgrinfo_appinfo_filter_destroy(handle);

	_APPS_SD("%s is included ? [%d]", appid, is_included);
	return is_included;

ERROR:
	if (handle) pkgmgrinfo_appinfo_filter_destroy(handle);

	_APPS_SD("%s is not included.", appid);
	return false;
}



static int _apps_cb(pkgmgrinfo_appinfo_h handle, void *user_data)
{
	app_list *list = user_data;
	char *appid = NULL;
	item_info_s *item_info = NULL;

	retv_if(NULL == handle, 0);
	retv_if(NULL == user_data, 0);

	pkgmgrinfo_appinfo_get_appid(handle, &appid);
	retv_if(NULL == appid, 0);

	item_info = apps_item_info_create(appid);
	if (NULL == item_info) {
		_APPS_D("%s does not have the item_info", appid);
		return 0;
	}

	list->list = eina_list_append(list->list, item_info);

	return 0;
}



static int _apps_sort_cb(const void *d1, const void *d2)
{
	item_info_s *tmp1 = (item_info_s *) d1;
	item_info_s *tmp2 = (item_info_s *) d2;

	if (NULL == tmp1 || NULL == tmp1->name) return 1;
	else if (NULL == tmp2 || NULL == tmp2->name) return -1;

	return strcmp(tmp1->name, tmp2->name);
}



HAPI app_list *list_create(void)
{
	pkgmgrinfo_appinfo_filter_h handle = NULL;
	app_list *list = calloc(1, sizeof(app_list));

	retv_if(NULL == list, NULL);

	if (PMINFO_R_OK != pkgmgrinfo_appinfo_filter_create(&handle)) {
		free(list);
		return NULL;
	}

	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_add_bool(handle, PMINFO_APPINFO_PROP_APP_NODISPLAY, 0), ERROR);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, _apps_cb, list), ERROR);

	list->list = eina_list_sort(list->list, eina_list_count(list->list), _apps_sort_cb);

ERROR:
	if (handle) pkgmgrinfo_appinfo_filter_destroy(handle);

	return list;
}



static int _sort_by_appid_cb(const void *d1, const void *d2)
{
	const item_info_s *info1 = d1;
	const item_info_s *info2 = d2;

	if (NULL == info1 || NULL == info1->appid) return 1;
	else if (NULL == info2 || NULL == info2->appid) return -1;

	return strcmp(info1->appid, info2->appid);
}



HAPI app_list *list_create_by_appid(void)
{
	pkgmgrinfo_appinfo_filter_h handle = NULL;
	app_list *list = calloc(1, sizeof(app_list));

	retv_if(NULL == list, NULL);

	if (PMINFO_R_OK != pkgmgrinfo_appinfo_filter_create(&handle)) {
		free(list);
		return NULL;
	}

	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_add_bool(handle, PMINFO_APPINFO_PROP_APP_NODISPLAY, 0), ERROR);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, _apps_cb, list), ERROR);

	list->list = eina_list_sort(list->list, eina_list_count(list->list), _sort_by_appid_cb);

ERROR:
	if (handle) pkgmgrinfo_appinfo_filter_destroy(handle);

	return list;
}



HAPI void list_destroy(app_list *list)
{
	item_info_s *item_info = NULL;

	ret_if(NULL == list);
	ret_if(NULL == list->list);

	EINA_LIST_FREE(list->list, item_info) {
		if (NULL == item_info) break;
		apps_item_info_destroy(item_info);
	}

	eina_list_free(list->list);
	free(list);
}



static char *_get_app_name(const char *appid)
{
	pkgmgrinfo_appinfo_h appinfo_h = NULL;
	char *tmp = NULL;
	char *name = NULL;

	retv_if(0 > pkgmgrinfo_appinfo_get_appinfo(appid, &appinfo_h), NULL);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_get_label(appinfo_h, &tmp), ERROR);

	if (tmp) {
		name = strdup(tmp);
		goto_if(NULL == name, ERROR);
	}

	pkgmgrinfo_appinfo_destroy_appinfo(appinfo_h);
	return name;

ERROR:
	pkgmgrinfo_appinfo_destroy_appinfo(appinfo_h);
	return NULL;
}



HAPI void list_change_language(app_list *list)
{
	item_info_s *item_info = NULL;
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;

	ret_if(NULL == list);
	ret_if(NULL == list->list);

	EINA_LIST_FOREACH_SAFE(list->list, l, ln, item_info) {
		continue_if(!item_info);
		free(item_info->name);
		item_info->name = _get_app_name(item_info->appid);
	}
	list->list = eina_list_sort(list->list, eina_list_count(list->list), _apps_sort_cb);
}



// End of a file
