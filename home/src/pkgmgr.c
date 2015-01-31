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
#include <efl_assist.h>
#include <bundle.h>
#include <dlog.h>

#include <dynamicbox_service.h>

#include "conf.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "pkgmgr.h"



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
	char* pkg_id;
	char* app_id;
	Evas_Object *item;
} pkgmgr_install_s;

static struct {
	pkgmgr_client *listen_pc;
	Eina_List *reserve_list;
	Eina_List *request_list;
	Eina_List *item_list;
	Eina_List *dynamicbox_list;
	int hold;
} pkg_mgr_info = {
	.listen_pc = NULL,
	.reserve_list = NULL,
	.request_list = NULL,
	.item_list = NULL,
	.dynamicbox_list = NULL,
};



HAPI void pkgmgr_hold_event(void)
{
	pkg_mgr_info.hold = 1;
}



HAPI void pkgmgr_unhold_event(void)
{
	pkg_mgr_info.hold = 0;
}



static w_home_error_e _append_request_in_list(const char *package, const char *status)
{
	pkgmgr_request_s *rt = NULL;
	char *tmp_pkg = NULL;
	char *tmp_st = NULL;

	retv_if(NULL == package, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(NULL == status, W_HOME_ERROR_INVALID_PARAMETER);

	rt = calloc(1, sizeof(pkgmgr_request_s));
	retv_if(NULL == rt, W_HOME_ERROR_FAIL);

	tmp_pkg = strdup(package);
	goto_if(NULL == tmp_pkg, ERROR);

	rt->package = tmp_pkg;

	tmp_st = strdup(status);
	goto_if(NULL == tmp_st, ERROR);

	rt->status = tmp_st;

	pkg_mgr_info.request_list = eina_list_append(pkg_mgr_info.request_list, rt);
	goto_if(NULL == pkg_mgr_info.request_list, ERROR);

	return W_HOME_ERROR_NONE;

ERROR:
	free(rt->package);
	free(rt->status);
	free(rt);

	return W_HOME_ERROR_FAIL;
}



static w_home_error_e _remove_request_in_list(const char *package)
{
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	pkgmgr_request_s *rt = NULL;

	retv_if(NULL == package, W_HOME_ERROR_INVALID_PARAMETER);

	if (NULL == pkg_mgr_info.request_list) return W_HOME_ERROR_NONE;

	EINA_LIST_FOREACH_SAFE(pkg_mgr_info.request_list, l, ln, rt) {
		if (!rt) continue;
		if (!rt->package) continue;
		if (strcmp(rt->package, package)) continue;

		pkg_mgr_info.request_list = eina_list_remove(pkg_mgr_info.request_list, rt);
		free(rt->package);
		free(rt->status);
		free(rt);
		return W_HOME_ERROR_NONE;
	}

	return W_HOME_ERROR_FAIL;
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



HAPI w_home_error_e pkgmgr_item_list_append_item(const char *pkg_id, const char *app_id, Evas_Object *item)
{
	pkgmgr_install_s *pi = NULL;
	char *tmp_pkg_id = NULL;
	char *tmp_app_id = NULL;

	retv_if(NULL == pkg_id, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(NULL == app_id, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(NULL == item, W_HOME_ERROR_INVALID_PARAMETER);

	pi = calloc(1, sizeof(pkgmgr_install_s));
	goto_if(NULL == pi, ERROR);

	tmp_pkg_id = strdup(pkg_id);
	goto_if(NULL == tmp_pkg_id, ERROR);

	tmp_app_id = strdup(app_id);
	goto_if(NULL == tmp_app_id, ERROR);

	pi->pkg_id = tmp_pkg_id;
	pi->app_id = tmp_app_id;
	pi->item = item;

	pkg_mgr_info.item_list = eina_list_append(pkg_mgr_info.item_list, pi);
	goto_if(NULL == pkg_mgr_info.item_list, ERROR);

	return W_HOME_ERROR_NONE;

ERROR:
	free(tmp_app_id);
	free(tmp_pkg_id);
	free(pi);

	return W_HOME_ERROR_FAIL;
}



HAPI w_home_error_e pkgmgr_item_list_remove_item(const char *pkg_id, const char *app_id, Evas_Object *item)
{
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	pkgmgr_install_s *pi = NULL;

	retv_if(NULL == pkg_id, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(NULL == app_id, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(NULL == item, W_HOME_ERROR_INVALID_PARAMETER);

	EINA_LIST_FOREACH_SAFE(pkg_mgr_info.item_list, l, ln, pi) {
		continue_if(NULL == pi);
		continue_if(NULL == pi->pkg_id);
		continue_if(NULL == pi->app_id);
		continue_if(NULL == pi->item);

		if (strcmp(pi->pkg_id, pkg_id)) continue;
		if (strcmp(pi->app_id, app_id)) continue;
		if (pi->item != item) continue;

		pkg_mgr_info.item_list = eina_list_remove(pkg_mgr_info.item_list, pi);

		free(pi->app_id);
		free(pi->pkg_id);
		free(pi);

		return W_HOME_ERROR_NONE;
	}

	return W_HOME_ERROR_FAIL;
}



HAPI void pkgmgr_item_list_affect_pkgid(const char *pkg_id, Eina_Bool (*_affected_cb)(const char *, Evas_Object *, void *), void *data)
{
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	pkgmgr_install_s *pi = NULL;

	ret_if(NULL == pkg_mgr_info.item_list);
	ret_if(NULL == pkg_id);
	ret_if(NULL == _affected_cb);

	EINA_LIST_FOREACH_SAFE(pkg_mgr_info.item_list, l, ln, pi) {
		continue_if(NULL == pi);
		continue_if(NULL == pi->app_id);
		continue_if(NULL == pi->item);

		if (strcmp(pkg_id, pi->pkg_id)) continue;
		/* It's possible that many items with the same package name are in the install list */
		continue_if(EINA_TRUE != _affected_cb(pi->app_id, pi->item, data));
	}
}



HAPI void pkgmgr_item_list_affect_appid(const char *app_id, Eina_Bool (*_affected_cb)(const char *, Evas_Object *, void *), void *data)
{
	const Eina_List *l = NULL;
	const Eina_List *ln = NULL;
	pkgmgr_install_s *pi = NULL;

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



HAPI w_home_error_e pkgmgr_uninstall(const char *appid)
{
	pkgmgr_client *req_pc = NULL;
	int ret = W_HOME_ERROR_NONE;
	char *pkgid = NULL;

	retv_if(NULL == appid, W_HOME_ERROR_FAIL);

	req_pc = pkgmgr_client_new(PC_REQUEST);
	retv_if(NULL == req_pc, W_HOME_ERROR_FAIL);

	pkgmgrinfo_appinfo_h handle = NULL;
	if (PMINFO_R_OK != pkgmgrinfo_appinfo_get_appinfo(appid, &handle)) {
		if (PKGMGR_R_OK != pkgmgr_client_free(req_pc)) {
			_E("cannot free pkgmgr_client for request.");
		}
		return W_HOME_ERROR_FAIL;
	}

	if (PMINFO_R_OK != pkgmgrinfo_appinfo_get_pkgid(handle, &pkgid)) {
		if (PMINFO_R_OK != pkgmgrinfo_appinfo_destroy_appinfo(handle)) {
			_E("cannot destroy the appinfo");
		}

		if (PKGMGR_R_OK != pkgmgr_client_free(req_pc)) {
			_E("cannot free pkgmgr_client for request.");
		}

		return W_HOME_ERROR_FAIL;
	}

	if (!pkgid) pkgid = (char *) appid;

	_D("Uninstall a package[%s] from an app[%s]", pkgid, appid);
	if (pkgmgr_client_uninstall(req_pc, NULL, pkgid, PM_QUIET, NULL, NULL) < 0) {
		_E("cannot uninstall %s.", appid);
		ret = W_HOME_ERROR_FAIL;
	}

	if (PMINFO_R_OK != pkgmgrinfo_appinfo_destroy_appinfo(handle)) {
		_E("cannot destroy the appinfo");
		ret = W_HOME_ERROR_FAIL;
	}

	if (PMINFO_R_OK != pkgmgr_client_free(req_pc)) {
		_E("cannot free pkgmgr_client");
		ret = W_HOME_ERROR_FAIL;
	}

	return ret;
}



static w_home_error_e _start_download(const char *package, void *scroller)
{
	_D("Start downloading for the package(%s)", package);
	return W_HOME_ERROR_NONE;
}



static w_home_error_e _start_uninstall(const char *package, void *scroller)
{
	_D("Start uninstalling for the package(%s)", package);
	return W_HOME_ERROR_NONE;
}



static w_home_error_e _start_update(const char *package, void *scroller)
{
	_D("Start updating for the package(%s)", package);
	return W_HOME_ERROR_NONE;
}



static w_home_error_e _start_recover(const char *package, void *scroller)
{
	_D("Start recovering for the package(%s)", package);
	return W_HOME_ERROR_NONE;
}



static w_home_error_e _start_install(const char *package, void *scroller)
{
	_D("Start installing for the package(%s)", package);
	return W_HOME_ERROR_NONE;
}


static int _get_dynamicbox_list_cb(const char *dbox_id, int is_prime, void *data)
{
	char *id = NULL;

	retv_if(NULL == dbox_id, -1);

	id = strdup(dbox_id);
	retv_if(!id, -1);

	_D("dbox_id(%s %d) with %s", id, is_prime, data);

	pkg_mgr_info.dynamicbox_list = eina_list_append(pkg_mgr_info.dynamicbox_list, id);

	return 0;
}


static w_home_error_e _start(const char *package, const char *val, void *scroller)
{
	struct start_cb_set {
		const char *name;
		int (*handler)(const char *package, void *scroller);
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

	_D("package [%s], val [%s]", package, val);
	retv_if(_exist_request_in_list(package), W_HOME_ERROR_FAIL);
	retv_if(W_HOME_ERROR_NONE != _append_request_in_list(package, val), W_HOME_ERROR_FAIL);

	dynamicbox_service_get_pkglist_by_pkgid(package, _get_dynamicbox_list_cb, (void*) val);

	register unsigned int i;
	for (i = 0; start_cb[i].name; i ++) {
		if (strcasecmp(val, start_cb[i].name)) continue;
		break_if(NULL == start_cb[i].handler);
		return start_cb[i].handler(package, scroller);
	}

	_E("Unknown status for starting phase signal'd from package manager");
	return W_HOME_ERROR_NONE;
}



static w_home_error_e _icon_path(const char *package, const char *val, void *scroller)
{
	_D("package(%s) with %s", package, val);
	return W_HOME_ERROR_NONE;
}



static w_home_error_e _download_percent(const char *package, const char *val, void *scroller)
{
	_D("package(%s) with %s", package, val);
	return W_HOME_ERROR_NONE;
}



static w_home_error_e _install_percent(const char *package, const char *val, void *scroller)
{
	_D("package(%s) with %s", package, val);
	if (_exist_request_in_list(package)) return W_HOME_ERROR_NONE;
	retv_if(W_HOME_ERROR_NONE != _append_request_in_list(package, "install"), W_HOME_ERROR_FAIL);
	return W_HOME_ERROR_NONE;
}



static w_home_error_e _error(const char *package, const char *val, void *scroller)
{
	_D("package(%s) with %s", package, val);
	return W_HOME_ERROR_NONE;
}



static int _end_cb(pkgmgrinfo_appinfo_h handle, void *user_data)
{
	char *appid = NULL;
	pkgmgr_request_s *rt = NULL;

	retv_if(NULL == handle, -1);
	retv_if(NULL == user_data, -1);

	pkgmgrinfo_appinfo_get_appid(handle, &appid);

	rt = user_data;
	if (!strcmp(rt->status, "install")) {
		/* Install an app */
	} else if (!strcmp(rt->status, "update")) {
		/* Update an app */
	} else {
		_E("No routines for this status (%s:%s)", rt->package, rt->status);
	}

	return 0;
}



static Eina_Bool _uninstall_cb(const char *app_id, Evas_Object *item, void *data)
{
	/* Uninstall a dbox */

	return EINA_TRUE;
}



static w_home_error_e _end(const char *package, const char *val, void *data)
{
	pkgmgr_request_s *rt = NULL;
	pkgmgrinfo_pkginfo_h handle = NULL;

	retv_if(!_exist_request_in_list(package), W_HOME_ERROR_FAIL);

	rt = _get_request_in_list(package);
	retv_if(NULL == rt, W_HOME_ERROR_FAIL);
	retv_if(strcasecmp(val, "ok"), W_HOME_ERROR_FAIL);

	_D("Package(%s) : key(%s) - val(%s)", package, rt->status, val);

	/* Criteria : pkgid */
	if (!strcasecmp("uninstall", rt->status)) {
		char *dbox_id = NULL;

		pkgmgr_item_list_affect_pkgid(package, _uninstall_cb, NULL);

		EINA_LIST_FREE(pkg_mgr_info.dynamicbox_list, dbox_id) {
			continue_if(NULL == dbox_id);
			pkgmgr_item_list_affect_pkgid(dbox_id, _uninstall_cb, NULL);
			free(dbox_id);
		}
		pkg_mgr_info.dynamicbox_list = NULL;

		goto OUT;
	}

	retv_if(PMINFO_R_OK != pkgmgrinfo_pkginfo_get_pkginfo(package, &handle), W_HOME_ERROR_FAIL);

	/* Criteria : appid */
	if (PMINFO_R_OK != pkgmgrinfo_appinfo_get_list(handle, PMINFO_UI_APP, _end_cb, rt)) {
		if (W_HOME_ERROR_NONE != _remove_request_in_list(package))
			_E("cannot remove a request(%s:%s)", rt->package, rt->status);
		pkgmgrinfo_pkginfo_destroy_pkginfo(handle);
		return W_HOME_ERROR_FAIL;
	}

OUT:
	if (W_HOME_ERROR_NONE != _remove_request_in_list(package))
		_E("cannot remove a request(%s:%s)", rt->package, rt->status);
	if (handle) pkgmgrinfo_pkginfo_destroy_pkginfo(handle);

	return W_HOME_ERROR_NONE;
}



static w_home_error_e _change_pkg_name(const char *package, const char *val, void *scroller)
{
	_D("package(%s) with %s", package, val);
	return W_HOME_ERROR_NONE;
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



static w_home_error_e _pkgmgr_cb(int req_id, const char *pkg_type, const char *package, const char *key, const char *val, const void *pmsg, void *data)
{
	register unsigned int i;

	_D("pkgmgr request [%s:%s] for %s", key, val, package);

	if (BOOTING_STATE_DONE > main_get_info()->booting_state) {
		pkgmgr_reserve_list_push_request(package, key, val);
		return W_HOME_ERROR_NONE;
	}

	if (pkg_mgr_info.hold) {
		pkgmgr_reserve_list_push_request(package, key, val);
		return W_HOME_ERROR_NONE;
	}

	for (i = 0; i < sizeof(pkgmgr_cbs) / sizeof(struct pkgmgr_handler); i++) {
		if (strcasecmp(pkgmgr_cbs[i].key, key)) continue;
		break_if(!pkgmgr_cbs[i].func);

		if (W_HOME_ERROR_NONE != pkgmgr_cbs[i].func(package, val, NULL)) {
			_E("pkgmgr_cbs[%u].func has errors.", i);
		}

		return W_HOME_ERROR_NONE;
	}

	return W_HOME_ERROR_FAIL;
}



HAPI w_home_error_e pkgmgr_reserve_list_push_request(const char *package, const char *key, const char *val)
{
	char *tmp_package = NULL;
	char *tmp_key = NULL;
	char *tmp_val = NULL;

	retv_if(NULL == package, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(NULL == key, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(NULL == val, W_HOME_ERROR_INVALID_PARAMETER);

	pkgmgr_reserve_s *pr = calloc(1, sizeof(pkgmgr_reserve_s));
	retv_if(NULL == pr, W_HOME_ERROR_FAIL);

	tmp_package = strdup(package);
	goto_if(NULL == tmp_package, ERROR);
	pr->package = tmp_package;

	tmp_key = strdup(key);
	goto_if(NULL == tmp_key, ERROR);
	pr->key = tmp_key;

	tmp_val = strdup(val);
	goto_if(NULL == tmp_val, ERROR);
	pr->val = tmp_val;

	pkg_mgr_info.reserve_list = eina_list_append(pkg_mgr_info.reserve_list, pr);
	goto_if(NULL == pkg_mgr_info.reserve_list, ERROR);

	return W_HOME_ERROR_NONE;

ERROR:
	free(tmp_val);
	free(tmp_key);
	free(tmp_package);
	free(pr);

	return W_HOME_ERROR_FAIL;
}



HAPI w_home_error_e pkgmgr_reserve_list_pop_request(void)
{
	pkgmgr_reserve_s *pr = NULL;

	if (!pkg_mgr_info.reserve_list) return W_HOME_ERROR_NO_DATA;

	pr = eina_list_nth(pkg_mgr_info.reserve_list, 0);
	if (!pr) return W_HOME_ERROR_NO_DATA;
	pkg_mgr_info.reserve_list = eina_list_remove(pkg_mgr_info.reserve_list, pr);

	goto_if(W_HOME_ERROR_NONE != _pkgmgr_cb(0, NULL, pr->package, pr->key, pr->val, NULL, NULL), ERROR);

	free(pr->package);
	free(pr->key);
	free(pr->val);
	free(pr);

	return W_HOME_ERROR_NONE;

ERROR:
	free(pr->package);
	free(pr->key);
	free(pr->val);
	free(pr);

	return W_HOME_ERROR_FAIL;
}



HAPI w_home_error_e pkgmgr_init(void)
{
	if (NULL != pkg_mgr_info.listen_pc) {
		return W_HOME_ERROR_NONE;
	}

	pkg_mgr_info.listen_pc = pkgmgr_client_new(PC_LISTENING);
	retv_if(NULL == pkg_mgr_info.listen_pc, W_HOME_ERROR_FAIL);
	retv_if(pkgmgr_client_listen_status(pkg_mgr_info.listen_pc,
			_pkgmgr_cb, NULL) != PKGMGR_R_OK, W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}



HAPI void pkgmgr_fini(void)
{
	ret_if(NULL == pkg_mgr_info.listen_pc);
	if (pkgmgr_client_free(pkg_mgr_info.listen_pc) != PKGMGR_R_OK) {
		_E("cannot free pkgmgr_client for listen.");
	}
	pkg_mgr_info.listen_pc = NULL;
}



HAPI int pkgmgr_exist(char *appid)
{
	int ret = 0;
	pkgmgrinfo_appinfo_h handle = NULL;

	ret = pkgmgrinfo_appinfo_get_appinfo(appid, &handle);
	if (PMINFO_R_OK != ret || NULL == handle) {
		_SD("%s doesn't exist in this binary", appid);
		return 0;
	}

	pkgmgrinfo_appinfo_destroy_appinfo(handle);

	return 1;
}



// End of a file
