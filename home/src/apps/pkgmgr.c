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
#include <package-manager.h>
#include <pkgmgr-info.h>

#include "log.h"
#include "util.h"
#include "apps/db.h"
#include "apps/item.h"
#include "apps/list.h"
#include "apps/apps_main.h"
#include "apps/pkgmgr.h"
#include "apps/scroller.h"
#include "apps/scroller_info.h"
#include "apps/item_badge.h"
#include "apps/layout.h"



struct pkgmgr_handler {
	const char *key;
	int (*func)(const char *package, const char *val, void *data);
};

typedef struct {
	char* package;
	char* key;
	char* val;
} pkgmgr_reserve_s;

typedef struct {
	char* package;
	char* status;
} pkgmgr_request_s;

typedef struct {
	char* pkgid;
	char* app_id;
	Evas_Object *item;
} pkgmgr_install_s;

static struct {
	pkgmgr_client *listen_pc;
	Eina_List *reserve_list;
	Eina_List *request_list;
	Eina_List *item_list;
} pkg_mgr_info = {
	.listen_pc = NULL,
	.reserve_list = NULL,
	.request_list = NULL,
	.item_list = NULL,
};



static apps_error_e _append_request_in_list(const char *package, const char *status)
{
	char *tmp_pkg = NULL;
	char *tmp_st = NULL;

	retv_if(NULL == package, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == status, APPS_ERROR_INVALID_PARAMETER);

	pkgmgr_request_s *rt = calloc(1, sizeof(pkgmgr_request_s));
	retv_if(NULL == rt, APPS_ERROR_FAIL);

	tmp_pkg = strdup(package);
	goto_if(NULL == tmp_pkg, ERROR);

	tmp_st = strdup(status);
	goto_if(NULL == tmp_st, ERROR);

	rt->package = tmp_pkg;
	rt->status = tmp_st;

	pkg_mgr_info.request_list = eina_list_append(pkg_mgr_info.request_list, rt);
	goto_if(NULL == pkg_mgr_info.request_list, ERROR);

	return APPS_ERROR_NONE;

ERROR:
	if (rt) {
		free(rt->status);
		free(rt->package);
		free(rt);
	}

	return APPS_ERROR_FAIL;
}



static apps_error_e _remove_request_in_list(const char *package)
{
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	pkgmgr_request_s *rt = NULL;

	retv_if(NULL == package, APPS_ERROR_INVALID_PARAMETER);

	if (NULL == pkg_mgr_info.request_list) return APPS_ERROR_NONE;

	EINA_LIST_FOREACH_SAFE(pkg_mgr_info.request_list, l, ln, rt) {
		if (!rt) continue;
		if (!rt->package) continue;
		if (strcmp(rt->package, package)) continue;

		pkg_mgr_info.request_list = eina_list_remove(pkg_mgr_info.request_list, rt);
		free(rt->package);
		free(rt->status);
		free(rt);
		return APPS_ERROR_NONE;
	}

	return APPS_ERROR_FAIL;
}



static int _exist_request_in_list(const char *package)
{
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	pkgmgr_request_s *rt = NULL;

	retv_if(NULL == package, 0);

	if (NULL == pkg_mgr_info.request_list) return 0;

	EINA_LIST_FOREACH_SAFE(pkg_mgr_info.request_list, l, ln, rt) {
		if (!rt) continue;
		if (!rt->package) continue;
		if (strcmp(rt->package, package)) continue;
		return 1;
	}

	return 0;
}



static pkgmgr_request_s *_get_request_in_list(const char *package)
{
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	pkgmgr_request_s *rt = NULL;

	retv_if(NULL == package, NULL);

	if (NULL == pkg_mgr_info.request_list) return NULL;

	EINA_LIST_FOREACH_SAFE(pkg_mgr_info.request_list, l, ln, rt) {
		if (!rt) continue;
		if (!rt->package) continue;
		if (strcmp(rt->package, package)) continue;
		return rt;
	}

	return NULL;
}



HAPI apps_error_e apps_pkgmgr_item_list_append_item(const char *pkgid, const char *app_id, Evas_Object *item)
{
	char *tmp_pkg_id = NULL;
	char *tmp_app_id = NULL;
	pkgmgr_install_s *pi = NULL;

	retv_if(NULL == pkgid, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == app_id, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == item, APPS_ERROR_INVALID_PARAMETER);

	pi = calloc(1, sizeof(pkgmgr_install_s));
	goto_if(NULL == pi, ERROR);

	tmp_pkg_id = strdup(pkgid);
	goto_if(NULL == tmp_pkg_id, ERROR);

	tmp_app_id = strdup(app_id);
	goto_if(NULL == tmp_app_id, ERROR);

	pi->pkgid = tmp_pkg_id;
	pi->app_id = tmp_app_id;
	pi->item = item;

	pkg_mgr_info.item_list = eina_list_append(pkg_mgr_info.item_list, pi);
	goto_if(NULL == pkg_mgr_info.item_list, ERROR);

	return APPS_ERROR_NONE;

ERROR:
	free(tmp_app_id);
	free(tmp_pkg_id);
	free(pi);

	return APPS_ERROR_FAIL;
}



HAPI apps_error_e apps_pkgmgr_item_list_remove_item(const char *pkgid, const char *app_id, Evas_Object *item)
{
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	pkgmgr_install_s *pi = NULL;

	retv_if(NULL == pkgid, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == app_id, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == item, APPS_ERROR_INVALID_PARAMETER);

	EINA_LIST_FOREACH_SAFE(pkg_mgr_info.item_list, l, ln, pi) {
		continue_if(NULL == pi);
		continue_if(NULL == pi->pkgid);
		continue_if(NULL == pi->app_id);
		continue_if(NULL == pi->item);

		if (strcmp(pi->pkgid, pkgid)) continue;
		if (strcmp(pi->app_id, app_id)) continue;
		if (pi->item != item) continue;

		pkg_mgr_info.item_list = eina_list_remove(pkg_mgr_info.item_list, pi);

		free(pi->app_id);
		free(pi->pkgid);
		free(pi);

		return APPS_ERROR_NONE;
	}

	return APPS_ERROR_FAIL;
}



HAPI void apps_pkgmgr_item_list_affect_pkgid(const char *pkgid, Eina_Bool (*_affected_cb)(const char *, Evas_Object *, void *), void *data)
{
	const Eina_List *l;
	const Eina_List *ln;
	pkgmgr_install_s *pi;

	ret_if(NULL == pkg_mgr_info.item_list);
	ret_if(NULL == pkgid);
	ret_if(NULL == _affected_cb);

	EINA_LIST_FOREACH_SAFE(pkg_mgr_info.item_list, l, ln, pi) {
		continue_if(NULL == pi);
		continue_if(NULL == pi->app_id);
		continue_if(NULL == pi->item);

		if (strcmp(pkgid, pi->pkgid)) continue;
		/* It's possible that many items with the same package name are in the install list */
		continue_if(EINA_TRUE != _affected_cb(pi->app_id, pi->item, data));
	}
}



HAPI void apps_pkgmgr_item_list_affect_appid(const char *app_id, Eina_Bool (*_affected_cb)(const char *, Evas_Object *, void *), void *data)
{
	const Eina_List *l;
	const Eina_List *ln;
	pkgmgr_install_s *pi;

	ret_if(NULL == pkg_mgr_info.item_list);
	ret_if(NULL == app_id);
	ret_if(NULL == _affected_cb);

	EINA_LIST_FOREACH_SAFE(pkg_mgr_info.item_list, l, ln, pi) {
		continue_if(NULL == pi);
		continue_if(NULL == pi->app_id);
		continue_if(NULL == pi->item);

		if (strcmp(app_id, pi->app_id)) continue;
		/* It's possible that many items with the same package name are in the install list */
		continue_if(EINA_TRUE != _affected_cb(pi->app_id, pi->item, data));
	}
}



static Eina_Bool _enable_item(const char *appid, Evas_Object *item, void *data)
{
	void (*_func_item_enable)(Evas_Object *);
	Evas_Object *grid = evas_object_data_get(item, DATA_KEY_GRID);
	retv_if(NULL == grid, EINA_FALSE);

	_func_item_enable = evas_object_data_get(grid, DATA_KEY_FUNC_ENABLE_ITEM);
	retv_if(NULL == _func_item_enable, EINA_FALSE);

	_func_item_enable(item);

	return EINA_TRUE;
}




static int _enable_cb(const pkgmgrinfo_appinfo_h handle, void *user_data)
{
	char *appid = NULL;
	pkgmgrinfo_appinfo_get_appid(handle, &appid);
	apps_pkgmgr_item_list_affect_appid(appid, _enable_item, NULL);
	return 0;
}



HAPI apps_error_e apps_pkgmgr_item_list_enable_mounted_item(void)
{
	retv_if(PMINFO_R_OK != pkgmgrinfo_appinfo_get_mounted_list(_enable_cb, NULL), APPS_ERROR_FAIL);
	return APPS_ERROR_NONE;
}



static Eina_Bool _disable_item(const char *appid, Evas_Object *item, void *data)
{
	void (*_func_item_disable)(Evas_Object *);
	Evas_Object *grid = evas_object_data_get(item, DATA_KEY_GRID);
	retv_if(NULL == grid, EINA_FALSE);

	_func_item_disable = evas_object_data_get(grid, DATA_KEY_FUNC_DISABLE_ITEM);
	retv_if(NULL == _func_item_disable, EINA_FALSE);

	_func_item_disable(item);

	return EINA_TRUE;
}




static int _disable_cb(const pkgmgrinfo_appinfo_h handle, void *user_data)
{
	char *appid = NULL;
	pkgmgrinfo_appinfo_get_appid(handle, &appid);
	apps_pkgmgr_item_list_affect_appid(appid, _disable_item, NULL);
	return 0;
}



HAPI apps_error_e apps_pkgmgr_item_list_disable_unmounted_item(void)
{
	retv_if(PMINFO_R_OK != pkgmgrinfo_appinfo_get_unmounted_list(_disable_cb, NULL), APPS_ERROR_FAIL);
	return APPS_ERROR_NONE;
}



static apps_error_e _start_download(const char *package, void *data)
{
	_APPS_D("Start downloading for the package(%s)", package);
	return APPS_ERROR_NONE;
}



static apps_error_e _start_uninstall(const char *package, void *data)
{
	_APPS_D("Start uninstalling for the package(%s)", package);
	return APPS_ERROR_NONE;
}



static apps_error_e _start_update(const char *package, void *data)
{
	_APPS_D("Start updating for the package(%s)", package);
	return APPS_ERROR_NONE;
}



static apps_error_e _start_recover(const char *package, void *data)
{
	_APPS_D("Start recovering for the package(%s)", package);
	return APPS_ERROR_NONE;
}



static apps_error_e _start_install(const char *package, void *data)
{
	_APPS_D("Start installing for the package(%s)", package);
	return APPS_ERROR_NONE;
}



static apps_error_e _start(const char *package, const char *val, void *data)
{
	register unsigned int i;
	struct start_cb_set {
		const char *name;
		int (*handler)(const char *package, void *data);
	} start_cb[] = {
		{
			.name = "download",
			.handler = _start_download,
		},
		{
			.name = "uninstall",
			.handler = _start_uninstall,
		},
		{
			.name = "install",
			.handler = _start_install,
		},
		{
			.name = "update",
			.handler = _start_update,
		},
		{
			.name = "recover",
			.handler = _start_recover,
		},
		{
			.name = NULL,
			.handler = NULL,
		},
	};

	_APPS_D("package [%s], val [%s]", package, val);
	retv_if(_exist_request_in_list(package), APPS_ERROR_FAIL);
	retv_if(APPS_ERROR_NONE != _append_request_in_list(package, val), APPS_ERROR_FAIL);

	for (i = 0; start_cb[i].name; i ++) {
		if (strcasecmp(val, start_cb[i].name)) continue;
		break_if(NULL == start_cb[i].handler);
		return start_cb[i].handler(package, data);
	}

	_APPS_E("Unknown status for starting phase signal'd from package manager");
	return APPS_ERROR_NONE;
}



static apps_error_e _icon_path(const char *package, const char *val, void *data)
{
	_APPS_D("package(%s) with %s", package, val);
	return APPS_ERROR_NONE;
}



static apps_error_e _download_percent(const char *package, const char *val, void *data)
{
	_APPS_D("package(%s) with %s", package, val);
	return APPS_ERROR_NONE;
}



static apps_error_e _install_percent(const char *package, const char *val, void *data)
{
	_APPS_D("package(%s) with %s", package, val);
	if (_exist_request_in_list(package)) return APPS_ERROR_NONE;
	retv_if(APPS_ERROR_NONE != _append_request_in_list(package, "install"), APPS_ERROR_FAIL);
	return APPS_ERROR_NONE;
}



static apps_error_e _error(const char *package, const char *val, void *data)
{
	_APPS_D("package(%s) with %s", package, val);
	return APPS_ERROR_NONE;
}



static void _install_app(Evas_Object *scroller, const char *appid)
{
	Evas_Object *item = NULL;
	item_info_s *item_info = NULL;

	ret_if(!scroller);
	ret_if(!appid);

	item_info = apps_item_info_create(appid);
	ret_if(!item_info);

	_APPS_SD("appid[%s], name[%s]", item_info->appid, item_info->name);
	item = item_create(scroller, item_info);
	if (!item) {
		_APPS_E("Cannot create an item");
		apps_item_info_destroy(item_info);
		return;
	}

	apps_scroller_append_item(scroller, item);
}



static void _uninstall_app(Evas_Object *scroller, const char *appid)
{
	Evas_Object *item = NULL;
	item_info_s *item_info = NULL;

	ret_if(!scroller);
	ret_if(!appid);

	item = apps_scroller_get_item_by_appid(scroller, appid);
	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	/* We have to try to destroy an item even if there is no item_info */

	int longpressed = item_is_longpressed(item);
	int is_edit = apps_layout_is_edited(item_info->layout);
	if(is_edit && longpressed) {
		evas_object_data_set(scroller, DATA_KEY_ITEM_UNINSTALL_RESERVED, (void*)appid);
	}
	else {
		apps_scroller_remove_item(scroller, item);
		apps_db_remove_item(appid);
		item_destroy(item);

		if (item_info) {
			_APPS_SD("appid[%s], name[%s]", item_info->appid, item_info->name);
			item_badge_remove(item_info->appid);
			item_badge_remove(item_info->pkgid);
			apps_item_info_destroy(item_info);
		}

		apps_scroller_trim(scroller);
	}
}



static void _update_app(Evas_Object *scroller, const char *appid)
{
	_uninstall_app(scroller, appid);
	_install_app(scroller, appid);
}



static void _manage_package(void (*_func)(Evas_Object *, const char *), const char *appid)
{
	Evas_Object *scroller = NULL;
	Eina_List *list = NULL;
	const Eina_List *l, *ln;
	instance_info_s *info = NULL;
	scroller_info_s *scroller_info = NULL;

	ret_if(!_func);
	ret_if(!appid);

	list = apps_main_get_info()->instance_list;
	ret_if(!list);

	EINA_LIST_FOREACH_SAFE(list, l, ln, info) {
		continue_if(!info);
		continue_if(!info->layout);

		scroller = evas_object_data_get(info->layout, DATA_KEY_SCROLLER);
		continue_if(!scroller);

		_func(scroller, appid);
	}

	ret_if(!scroller);
	apps_scroller_write_list(scroller);

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	ret_if(!scroller_info->layout);
	ret_if(!scroller_info->list);

	Eina_List *layout_list = evas_object_data_get(scroller_info->layout, DATA_KEY_LIST);
	eina_list_free(layout_list);
	layout_list= eina_list_clone(scroller_info->list);
	evas_object_data_set(scroller_info->layout, DATA_KEY_LIST, layout_list);

	apps_db_read_list(scroller_info->list);
}



static int _end_cb(pkgmgrinfo_appinfo_h handle, void *user_data)
{
	char *appid = NULL;
	pkgmgr_request_s *rt = user_data;

	retv_if(NULL == handle, -1);
	retv_if(NULL == user_data, -1);

	pkgmgrinfo_appinfo_get_appid(handle, &appid);

	_APPS_SD("status[%s], appid[%s]", rt->status, appid);

	if (!strcmp(rt->status, "install")) {
		_manage_package(_install_app, appid);
	} else if (!strcmp(rt->status, "update")) {
		_manage_package(_update_app, appid);
	} else {
		_APPS_E("No routines for this status (%s:%s)", rt->package, rt->status);
	}

	return 0;
}



static Eina_Bool _uninstall_cb(const char *appid, Evas_Object *item, void *data)
{
	retv_if(!appid, EINA_FALSE);

	_manage_package(_uninstall_app, appid);

	return EINA_TRUE;
}



static apps_error_e _end(const char *package, const char *val, void *data)
{
	pkgmgr_request_s *rt = NULL;
	pkgmgrinfo_pkginfo_h handle = NULL;

	retv_if(!_exist_request_in_list(package), APPS_ERROR_FAIL);

	rt = _get_request_in_list(package);
	retv_if(NULL == rt, APPS_ERROR_FAIL);
	retv_if(strcasecmp(val, "ok"), APPS_ERROR_FAIL);

	_APPS_D("Package(%s) : key(%s) - val(%s)", package, rt->status, val);

	/* Criteria : pkgid */
	if (!strcasecmp("uninstall", rt->status)) {
		apps_pkgmgr_item_list_affect_pkgid(package, _uninstall_cb, data);
		goto OUT;
	}

	retv_if(PMINFO_R_OK != pkgmgrinfo_pkginfo_get_pkginfo(package, &handle), APPS_ERROR_FAIL);

	/* Criteria : appid */
	if (PMINFO_R_OK != pkgmgrinfo_appinfo_get_list(handle, PMINFO_UI_APP, _end_cb, rt)) {
		if (APPS_ERROR_NONE != _remove_request_in_list(package))
			_APPS_E("cannot remove a request(%s:%s)", rt->package, rt->status);
		pkgmgrinfo_pkginfo_destroy_pkginfo(handle);
		return APPS_ERROR_FAIL;
	}

OUT:
	if (APPS_ERROR_NONE != _remove_request_in_list(package))
		_APPS_E("cannot remove a request(%s:%s)", rt->package, rt->status);
	if (handle) pkgmgrinfo_pkginfo_destroy_pkginfo(handle);

	return APPS_ERROR_NONE;
}



static apps_error_e _change_pkg_name(const char *package, const char *val, void *data)
{
	_APPS_D("package(%s) with %s", package, val);
	return APPS_ERROR_NONE;
}



static struct pkgmgr_handler pkgmgr_cbs[] = {
	{ "start", _start },
	{ "icon_path", _icon_path },
	{ "download_percent", _download_percent },
	{ "command", NULL },
	{ "install_percent", _install_percent },
	{ "error", _error },
	{ "end", _end },
	{ "change_pkg_name", _change_pkg_name },
};



static apps_error_e _pkgmgr_cb(int req_id, const char *pkg_type, const char *package, const char *key, const char *val, const void *pmsg, void *data)
{
	register unsigned int i;

	_APPS_D("pkgmgr request [%s:%s] for %s", key, val, package);

	for (i = 0; i < sizeof(pkgmgr_cbs) / sizeof(struct pkgmgr_handler); i++) {
		if (strcasecmp(pkgmgr_cbs[i].key, key)) continue;
		break_if(!pkgmgr_cbs[i].func);

		if (APPS_ERROR_NONE != pkgmgr_cbs[i].func(package, val, data)) {
			_APPS_E("pkgmgr_cbs[%u].func has errors.", i);
		}
	}

	return APPS_ERROR_NONE;
}



HAPI apps_error_e apps_pkgmgr_init(void)
{
	if (NULL != pkg_mgr_info.listen_pc) {
		return APPS_ERROR_NONE;
	}

	pkg_mgr_info.listen_pc = pkgmgr_client_new(PC_LISTENING);
	retv_if(NULL == pkg_mgr_info.listen_pc, APPS_ERROR_FAIL);
	retv_if(pkgmgr_client_listen_status(pkg_mgr_info.listen_pc,
			_pkgmgr_cb, NULL) != PKGMGR_R_OK, APPS_ERROR_FAIL);

	return APPS_ERROR_NONE;
}



HAPI void apps_pkgmgr_fini(void)
{
	ret_if(NULL == pkg_mgr_info.listen_pc);
	if (pkgmgr_client_free(pkg_mgr_info.listen_pc) != PKGMGR_R_OK) {
		_APPS_E("cannot free pkgmgr_client for listen.");
	}
	pkg_mgr_info.listen_pc = NULL;
}



// End of a file
