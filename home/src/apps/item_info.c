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

#include <sys/stat.h>
#include <pkgmgr-info.h>

#include "log.h"
#include "util.h"
#include "apps/db.h"
#include "apps/item_info.h"
#include "apps/list.h"
#include "apps/apps_main.h"
#include "apps/xml.h"

#define APP_TYPE_WGT "wgt"



HAPI void apps_item_info_destroy(item_info_s *item_info)
{
	ret_if(!item_info);

	if (item_info->pkgid) free(item_info->pkgid);
	if (item_info->appid) free(item_info->appid);
	if (item_info->name) free(item_info->name);
	if (item_info->icon) free(item_info->icon);
	if (item_info->type) free(item_info->type);
	if (item_info) free(item_info);
}



HAPI item_info_s *apps_item_info_create(const char *appid)
{
	item_info_s *item_info = NULL;
	pkgmgrinfo_appinfo_h appinfo_h = NULL;
	pkgmgrinfo_pkginfo_h pkghandle = NULL;
	char *pkgid = NULL;
	char *name = NULL;
	char *icon = NULL;
	char *type = NULL;
	bool nodisplay = false;
	bool enabled = false;

	retv_if(!appid, NULL);

	item_info = calloc(1, sizeof(item_info_s));
	if (NULL == item_info) {
		return NULL;
	}

	goto_if(0 > pkgmgrinfo_appinfo_get_appinfo(appid, &appinfo_h), ERROR);

	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_get_label(appinfo_h, &name), ERROR);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_get_icon(appinfo_h, &icon), ERROR);

	do {
		break_if(PMINFO_R_OK != pkgmgrinfo_appinfo_get_pkgid(appinfo_h, &pkgid));
		break_if(NULL == pkgid);

		break_if(0 > pkgmgrinfo_pkginfo_get_pkginfo(pkgid, &pkghandle));
		break_if(NULL == pkghandle);
	} while (0);

	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_is_nodisplay(appinfo_h, &nodisplay), ERROR);
	if (nodisplay) goto ERROR;

	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_is_enabled(appinfo_h, &enabled), ERROR);
	if (!enabled) goto ERROR;

	goto_if(PMINFO_R_OK != pkgmgrinfo_pkginfo_get_type(pkghandle, &type), ERROR);
#ifdef RUN_ON_DEVICE
	int support_mode = 0;
	goto_if(PMINFO_R_OK != pkgmgrinfo_pkginfo_get_support_mode(pkghandle, &support_mode), ERROR);
#endif

	if (pkgid) {
		item_info->pkgid = strdup(pkgid);
		goto_if(NULL == item_info->pkgid, ERROR);
	}

	item_info->appid = strdup(appid);
	goto_if(NULL == item_info->appid, ERROR);

	if (name) {
		item_info->name = strdup(name);
		goto_if(NULL == item_info->name, ERROR);
	}

	if (type) {
		item_info->type = strdup(type);
		goto_if(NULL == item_info->type, ERROR);
		if (!strncmp(item_info->type, APP_TYPE_WGT, strlen(APP_TYPE_WGT))) {
			item_info->open_app = 1;
		} else {
			item_info->open_app = 0;
		}
	}

	if (icon) {
		if (strlen(icon) > 0) {
			item_info->icon = strdup(icon);
			goto_if(NULL == item_info->icon, ERROR);
		} else {
			item_info->icon = strdup(DEFAULT_ICON);
			goto_if(NULL == item_info->icon, ERROR);
		}
	} else {
		item_info->icon = strdup(DEFAULT_ICON);
		goto_if(NULL == item_info->icon, ERROR);
	}

#ifdef RUN_ON_DEVICE
	if (support_mode & PMINFO_MODE_PROP_SCREEN_READER)
		item_info->tts = 1;
	else
		item_info->tts = 0;
#else
	item_info->tts = 1;
#endif

	pkgmgrinfo_appinfo_destroy_appinfo(appinfo_h);
	if (pkghandle) pkgmgrinfo_pkginfo_destroy_pkginfo(pkghandle);

	return item_info;

ERROR:
	apps_item_info_destroy(item_info);
	if (appinfo_h) pkgmgrinfo_appinfo_destroy_appinfo(appinfo_h);
	if (pkghandle) pkgmgrinfo_pkginfo_destroy_pkginfo(pkghandle);

	return NULL;
}



HAPI item_info_s *apps_item_info_get(Evas_Object *win, const char *appid)
{
	Evas_Object *layout = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	item_info_s *info = NULL;

	retv_if(!win, NULL);
	retv_if(!appid, NULL);

	layout = evas_object_data_get(win, DATA_KEY_LAYOUT);
	retv_if(!layout, NULL);

	list = evas_object_data_get(layout, DATA_KEY_LIST);
	retv_if(!list, NULL);

	EINA_LIST_FOREACH_SAFE(list, l, ln, info) {
		continue_if(!info);
		continue_if(!info->appid);
		if (!strncmp(info->appid, appid, strlen(appid))) {
			return info;
		}
	}

	return NULL;
}



static int _apps_cb(pkgmgrinfo_appinfo_h handle, void *user_data)
{
	Eina_List **list = user_data;
	char *appid = NULL;
	item_info_s *item_info = NULL;

	retv_if(NULL == handle, 0);
	retv_if(NULL == user_data, 0);

	pkgmgrinfo_appinfo_get_appid(handle, &appid);
	retv_if(NULL == appid, 0);

	if (apps_db_count_item(appid)) {
		_APPS_D("%s is Top 4", appid);
		return 0;
	}

	item_info = apps_item_info_create(appid);
	if (NULL == item_info) {
		_APPS_D("%s does not have the item_info", appid);
		return 0;
	}

	*list = eina_list_append(*list, item_info);

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



static Eina_List *_read_all_except_top4(Eina_List **list)
{
	pkgmgrinfo_appinfo_filter_h handle = NULL;

	retv_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_create(&handle), NULL);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_add_bool(handle, PMINFO_APPINFO_PROP_APP_NODISPLAY, 0), ERROR);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, _apps_cb, list), ERROR);

	*list = eina_list_sort(*list, eina_list_count(*list), _apps_sort_cb);

ERROR:
	if (handle) pkgmgrinfo_appinfo_filter_destroy(handle);

	return *list;
}



static int _apps_all_cb(pkgmgrinfo_appinfo_h handle, void *user_data)
{
	Eina_List **list = user_data;
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

	*list = eina_list_append(*list, item_info);

	return 0;
}



static Eina_List *_read_all_apps(Eina_List **list)
{
	pkgmgrinfo_appinfo_filter_h handle = NULL;

	retv_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_create(&handle), NULL);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_add_bool(handle, PMINFO_APPINFO_PROP_APP_NODISPLAY, 0), ERROR);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, _apps_all_cb, list), ERROR);

	*list = eina_list_sort(*list, eina_list_count(*list), _apps_sort_cb);

ERROR:
	if (handle) pkgmgrinfo_appinfo_filter_destroy(handle);

	return *list;
}



static int _find_appid_cb(const void *d1, const void *d2)
{
	item_info_s *tmp1 = (item_info_s *) d1;
	item_info_s *tmp2 = (item_info_s *) d2;

	if (NULL == tmp1 || NULL == tmp1->appid) return 1;
	else if (NULL == tmp2 || NULL == tmp2->appid) return -1;

	return strcmp(tmp1->appid, tmp2->appid);
}



#define APPS_DEFAULT_ITEM_XML_FILE RESDIR"/apps_default_items.xml"
HAPI Eina_List *apps_item_info_list_create(item_info_list_type_e list_type)
{
	Eina_List *list = NULL;

	switch (list_type) {
	case ITEM_INFO_LIST_TYPE_ALL:
		/* It doesn't care whether the list is NULL or not */
		list = apps_db_write_list();
		list = _read_all_except_top4(&list);
		break;
	case ITEM_INFO_LIST_TYPE_XML:
		{
			Eina_List *xml_list = NULL;
			Eina_List *pkgmgr_list = NULL;

			xml_list = apps_xml_write_list(APPS_DEFAULT_ITEM_XML_FILE);
			_read_all_apps(&pkgmgr_list);

			const Eina_List *l, *ln;
			item_info_s *item_info = NULL;
			EINA_LIST_FOREACH_SAFE(xml_list, l, ln, item_info) {
				item_info_s *data = eina_list_search_unsorted(pkgmgr_list, _find_appid_cb, item_info);
				if(data) {
					_APPS_D("remove apps[%s] from pkg list", data->appid);
					pkgmgr_list = eina_list_remove(pkgmgr_list, data);
					apps_item_info_destroy(data);
				}
			}

			int nPkgListCount = eina_list_count(pkgmgr_list);
			_APPS_D("pkg list count[%d]", nPkgListCount);
			if(nPkgListCount) {
				list = eina_list_merge(xml_list, pkgmgr_list);
			}
			else {
				eina_list_free(pkgmgr_list);
				list = xml_list;
			}
		}
		break;
	default:
		_APPS_E("Cannot reach here");
		break;
	}

	return list;
}



HAPI void apps_item_info_list_destroy(Eina_List *item_info_list)
{
	item_info_s *item_info = NULL;

	ret_if(!item_info_list);

	EINA_LIST_FREE(item_info_list, item_info) {
		continue_if(!item_info);
		apps_item_info_destroy(item_info);
	}
}



HAPI Evas_Object *apps_item_info_list_get_item(Eina_List *item_info_list, const char *appid)
{
	const Eina_List *l, *ln;
	item_info_s *item_info = NULL;

	retv_if(!item_info_list, NULL);
	retv_if(!appid, NULL);

	EINA_LIST_FOREACH_SAFE(item_info_list, l, ln, item_info) {
		continue_if(!item_info);
		continue_if(!item_info->appid);
		_APPS_SD("item_info->appid[%s]", item_info->appid);
		if (strncmp(item_info->appid, appid, strlen(appid))) continue;

		return item_info->item;
	}

	return NULL;
}



HAPI int apps_item_info_list_get_ordering(Eina_List *item_info_list, const char *appid)
{
	const Eina_List *l, *ln;
	item_info_s *item_info = NULL;

	retv_if(!item_info_list, -1);
	retv_if(!appid, -1);

	EINA_LIST_FOREACH_SAFE(item_info_list, l, ln, item_info) {
		continue_if(!item_info);
		continue_if(!item_info->appid);
		if (strncmp(item_info->appid, appid, strlen(appid))) continue;

		return item_info->ordering;
	}

	return -1;
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



HAPI void apps_item_info_list_change_language(Eina_List *item_info_list)
{
	item_info_s *item_info = NULL;
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;

	ret_if(!item_info_list);

	EINA_LIST_FOREACH_SAFE(item_info_list, l, ln, item_info) {
		continue_if(!item_info);
		free(item_info->name);
		item_info->name = _get_app_name(item_info->appid);
	}
}


HAPI int apps_item_info_is_support_tts(const char *appid)
{
	pkgmgrinfo_appinfo_h appinfo_h = NULL;
	pkgmgrinfo_pkginfo_h pkghandle = NULL;
	char *pkgid = NULL;
	int support_mode = 0;
	int istts = 0;

	retv_if(!appid, 0);

	goto_if(0 > pkgmgrinfo_appinfo_get_appinfo(appid, &appinfo_h), ERROR);

	do {
		break_if(PMINFO_R_OK != pkgmgrinfo_appinfo_get_pkgid(appinfo_h, &pkgid));
		break_if(NULL == pkgid);

		break_if(0 > pkgmgrinfo_pkginfo_get_pkginfo(pkgid, &pkghandle));
		break_if(NULL == pkghandle);
	} while (0);

	goto_if(PMINFO_R_OK != pkgmgrinfo_pkginfo_get_support_mode(pkghandle, &support_mode), ERROR);

	if (support_mode & PMINFO_MODE_PROP_SCREEN_READER)
		istts = 1;
	else
		istts = 0;

	pkgmgrinfo_appinfo_destroy_appinfo(appinfo_h);
	if (pkghandle) pkgmgrinfo_pkginfo_destroy_pkginfo(pkghandle);

	return istts;

ERROR:
	if (appinfo_h) pkgmgrinfo_appinfo_destroy_appinfo(appinfo_h);
	if (pkghandle) pkgmgrinfo_pkginfo_destroy_pkginfo(pkghandle);

	return istts;
}
// End of a file
