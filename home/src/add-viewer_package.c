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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#include <Evas.h>
#include <Eina.h>
#include <dlog.h>

#include <widget_service_internal.h>
#include <widget_service.h>
#include <widget_errno.h>
#include <ail.h>
#if defined(CHECK_PRELOAD)
#include <pkgmgr-info.h>
#endif

#if defined(LOG_TAG)
#undef LOG_TAG
#endif
#define LOG_TAG "ADD_VIEWER"

#include "add-viewer_package.h"
#include "add-viewer_debug.h"
#include "add-viewer_ucol.h"
#include "add-viewer_pkgmgr.h"

static struct info {
	Eina_List *package_list;
	Eina_List *event_list;
	int initialized;
} s_info = {
	.package_list = NULL,
	.event_list = NULL,
	.initialized = 0,
};

struct package_list_event {
	enum pkg_evt_type event;
	struct add_viewer_package *package;
	int (*cb)(struct add_viewer_package *package, void *data);
	void *data;
};

struct add_viewer_preview {
	int size_type;
	void *data;
};

struct add_viewer_package {
	enum {
		PACKAGE_VALID = 0xbeefbeef,
		PACKAGE_INVALID = 0xdeaddead,
	} valid;

	char *name; /*!< Display name */
	char *pkgname; /*!< Package name */
	char *icon;
	int disabled;
	int skipped;
	char *pkgid;

	struct {
		char *key;
		char *data;
	} extra;

	enum package_type type;
	Eina_List *preview_list;

	void *data;
};

static inline int sort_cb(const void *a, const void *b)
{
	int ret;

	ret = add_viewer_ucol_compare(add_viewer_package_list_name((struct add_viewer_package *)a), add_viewer_package_list_name((struct add_viewer_package *)b));
	return ret;
}

#if defined(CHECK_PRELOAD)
static int is_preloaded(const char *pkgid)
{
	int ret;
	bool preload;
	pkgmgrinfo_pkginfo_h handle;

	ret = pkgmgrinfo_pkginfo_get_pkginfo(pkgid, &handle);
	if (ret != PMINFO_R_OK) {
		return 0;
	}

	ret = pkgmgrinfo_pkginfo_is_preload(handle, &preload);
	if (ret != PMINFO_R_OK) {
		pkgmgrinfo_pkginfo_destroy_pkginfo(handle);
		return 0;
	}

	pkgmgrinfo_pkginfo_destroy_pkginfo(handle);
	return preload == true ? 1 : 0;
}
#endif

static int load_name_and_icon(struct add_viewer_package *item)
{
	item->name = widget_service_get_name(item->pkgname, NULL);
	item->icon = widget_service_get_icon(item->pkgname, NULL);
	if (item->icon && access(item->icon, R_OK) != 0) {
		char *new_icon;
		ErrPrint("%s - %s\n", item->icon, strerror(errno));
		new_icon = strdup(RESDIR"/image/unknown.png");
		if (new_icon) {
			free(item->icon);
			item->icon = new_icon;
		} else {
			ErrPrint("Heap: %s\n", strerror(errno));
		}
	}

	if (!item->name || !item->icon) {
		char *pkgname;
		ail_appinfo_h ai;
		int ret;

		pkgname = widget_service_get_main_app_id(item->pkgname);
		if (!pkgname) {
			ErrPrint("%s has no appid\n", item->pkgname);
			return -EINVAL;
		}

		ret = ail_get_appinfo(pkgname, &ai);
		free((char *)pkgname);

		if (ret != AIL_ERROR_OK) {
			ErrPrint("Failed to get appinfo: %s\n", add_viewer_package_list_pkgname(item));
			if (!item->name) {
				ret = add_viewer_package_list_set_name(item, add_viewer_package_list_pkgname(item));
				if (ret != 0) {
					return ret;
				}
			}

			if (!item->icon) {
				ret = add_viewer_package_list_set_icon(item, RESDIR"/image/unknown.png");
				if (ret != 0) {
					return ret;
				}
			}

			s_info.package_list = eina_list_sorted_insert(s_info.package_list, sort_cb, item);
			return 0;
		}

		if (!item->icon) {
			char *icon;
			ret = ail_appinfo_get_str(ai, AIL_PROP_ICON_STR, &icon);
			if (ret != AIL_ERROR_OK || !icon || access(icon, R_OK) != 0) {
				ErrPrint("Fail to get the icon path %s - %d\n", icon, errno);
				icon = RESDIR"/image/unknown.png";
			}

			ret = add_viewer_package_list_set_icon(item, icon); 
			if (ret != 0) {
				ail_destroy_appinfo(ai);
				return ret;
			}
		}

		if (!item->name) {
			char *name;
			ret = ail_appinfo_get_str(ai, AIL_PROP_NAME_STR, &name);
			if (ret != AIL_ERROR_OK || !name) {
				name = item->pkgname;
			}

			ret = add_viewer_package_list_set_name(item, name);
			if (ret != 0) {
				ail_destroy_appinfo(ai);
				return ret;
			}
		}

		ail_destroy_appinfo(ai);
	}

	return 0;
}

static int widget_list_callback(const char *appid, const char *widget_id, int is_prime, void *data)
{
	struct add_viewer_package *item;
	int cnt;
	int *size_types;
	int ret;
	int valid_size;
	struct add_viewer_preview *preview;
	Eina_List *preview_list = NULL;

	if (!widget_id || !appid) {
		ErrPrint("widget_id(%p) appid(%p) is not valid", widget_id, appid);
		return 0;
	}

	if (widget_service_get_nodisplay(widget_id) == 1) {
		DbgPrint("NoDisplay: %s\n", widget_id);
		return 0;
	}

	cnt = WIDGET_NR_OF_SIZE_LIST;
	ret = widget_service_get_supported_size_types(widget_id, &cnt, &size_types); 
	if (ret != WIDGET_ERROR_NONE) {
		ErrPrint("Size is not valid: %s\n", widget_id);
		return 0;
	}

#if defined(CHECK_PRELOAD)
	int preloaded = 0;
	if (cnt > 0) {
		/*!
		 * If there are small size box,
		 * we should check whether it is preloaded or not.
		 * If it is preloaded packages, we have not to display it on the list.
		 */
		switch (size_types[0]) {
		case WIDGET_SIZE_TYPE_2x2:
			preloaded = is_preloaded(appid);
			break;
		case WIDGET_SIZE_TYPE_1x1:
		case WIDGET_SIZE_TYPE_2x1:
		default:
			break;
		}
	}
#endif

	valid_size = 0;
	while (--cnt >= 0) {
		preview = NULL;

		switch (size_types[cnt]) {
		case WIDGET_SIZE_TYPE_2x2:
#if defined(CHECK_PRELOAD)
			if (preloaded) {
				continue;
			}
#endif
			preview = calloc(1, sizeof(*preview));
			if (!preview) {
				ErrPrint("Heap: %s\n", strerror(errno));
				EINA_LIST_FREE(preview_list, preview) {
					free(preview);
				}
				free(size_types);
				return -ENOMEM;
			}

			valid_size++;
			preview->size_type = size_types[cnt];
			preview->data = NULL;
			preview_list = eina_list_append(preview_list, preview);
			break;
		case WIDGET_SIZE_TYPE_1x1:
		case WIDGET_SIZE_TYPE_2x1:
		case WIDGET_SIZE_TYPE_4x1:
		case WIDGET_SIZE_TYPE_4x2:
		case WIDGET_SIZE_TYPE_4x3:
		case WIDGET_SIZE_TYPE_4x4:
		case WIDGET_SIZE_TYPE_4x5:
		case WIDGET_SIZE_TYPE_4x6:
		case WIDGET_SIZE_TYPE_EASY_1x1:
		case WIDGET_SIZE_TYPE_EASY_3x1:
		case WIDGET_SIZE_TYPE_EASY_3x3:
			break;
		default:
			break;
		}
	}

	if (!valid_size) {
		DbgPrint("Has no valid size: %s\n", widget_id);
		free(size_types);
		return 0;
	}

	item = calloc(1, sizeof(*item));
	if (!item) {
		ErrPrint("Heap: %s\n", strerror(errno));
		EINA_LIST_FREE(preview_list, preview) {
			free(preview);
		}
		free(size_types);
		return -ENOMEM;
	}

	item->pkgname = strdup(widget_id);
	if (!item->pkgname) {
		ErrPrint("Heap: %s\n", strerror(errno));
		free(item);
		EINA_LIST_FREE(preview_list, preview) {
			free(preview);
		}
		free(size_types);
		return -ENOMEM;
	}

	item->pkgid = strdup(appid);
	if (!item->pkgid) {
		ErrPrint("Heap: %s\n", strerror(errno));
		free(item->pkgname);
		free(item);
		EINA_LIST_FREE(preview_list, preview) {
			free(preview);
		}
		free(size_types);
		return -ENOMEM;
	}

	item->type = PACKAGE_TYPE_BOX;
	item->preview_list = preview_list;
	ret = load_name_and_icon(item);
	if (ret < 0) {
		free(item->pkgid);
		free(item->name);
		free(item->icon);
		free(item->pkgname);
		free(item);
		EINA_LIST_FREE(preview_list, preview) {
			free(preview);
		}
		free(size_types);
		return ret;
	}
	item->valid = PACKAGE_VALID;
	s_info.package_list = eina_list_sorted_insert(s_info.package_list, sort_cb, item);
	free(size_types);
	return 0;
}

static void invoke_update_event_callback(struct add_viewer_package *package)
{
	struct package_list_event *item;
	Eina_List *l;
	Eina_List *n;

	EINA_LIST_FOREACH_SAFE(s_info.event_list, l, n, item) {
		if (item->package == package && item->event == PACKAGE_LIST_EVENT_UPDATE) {
			if (item->cb(package, item->data) < 0) {
				s_info.event_list = eina_list_remove(s_info.event_list, item);
				free(item);
			}
		}
	}
}

static void invoke_del_event_callback(struct add_viewer_package *package)
{
	struct package_list_event *item;
	Eina_List *l;
	Eina_List *n;

	EINA_LIST_FOREACH_SAFE(s_info.event_list, l, n, item) {
		if (item->package == package && item->event == PACKAGE_LIST_EVENT_DEL) {
			if (item->cb(package, item->data) < 0) {
				s_info.event_list = eina_list_remove(s_info.event_list, item);
				free(item);
			}
		}
	}
}

static void invoke_reload_event_callback(void)
{
	struct package_list_event *item;
	Eina_List *l;
	Eina_List *n;

	EINA_LIST_FOREACH_SAFE(s_info.event_list, l, n, item) {
		if (item->event == PACKAGE_LIST_EVENT_RELOAD) {
			if (item->cb(NULL, item->data) < 0) {
				s_info.event_list = eina_list_remove(s_info.event_list, item);
				free(item);
			}
		}
	}
}

static void package_delete(struct add_viewer_package *package)
{
	struct add_viewer_preview *preview;

	s_info.package_list = eina_list_remove(s_info.package_list, package);

	DbgPrint("%s is deleted\n", package->pkgname);

	free(package->name);
	free(package->pkgid);
	free(package->pkgname);
	free(package->icon);
	free(package->extra.key);
	free(package->extra.data);

	EINA_LIST_FREE(package->preview_list, preview) {
		free(preview);
	}

	free(package);
}

static int widget_add_by_pkgid_cb(const char *widget_id, int is_prime, void *data)
{
	Eina_List *l;
	struct add_viewer_package *item;
	int exists = 0;

	DbgPrint("Add [%s]\n", widget_id);

	EINA_LIST_FOREACH(s_info.package_list, l, item) {
		if (!strcmp(item->pkgname, widget_id)) {
			exists = 1;
			break;
		}
	}

	if (!exists) {
		char *appid;

		appid = widget_service_get_package_id(widget_id);
		if (appid) {
			widget_list_callback(appid, widget_id, is_prime, NULL);
			free(appid);
		}

		EINA_LIST_FOREACH(s_info.package_list, l, item) {
			if (!strcmp(item->pkgname, widget_id)) {
				invoke_update_event_callback(item);
				break;
			}
		}
	}

	return 0;
}

static int widget_del_by_pkgid_cb(const char *widget_id, int is_prime, void *data)
{
	Eina_List *l;
	Eina_List *n;
	struct add_viewer_package *item;

	DbgPrint("Del [%s]\n", widget_id);

	EINA_LIST_FOREACH_SAFE(s_info.package_list, l, n, item) {
		if (!strcmp(item->pkgname, widget_id)) {
			add_viewer_package_list_del(item);
			break;
		}
	}

	return 0;
}

static void widget_del_by_appid(const char *appid)
{
	Eina_List *l;
	Eina_List *n;
	struct add_viewer_package *item;

	DbgPrint("Del [%s]\n", appid);
	EINA_LIST_FOREACH_SAFE(s_info.package_list, l, n, item) {
		if (!strcmp(item->pkgid, appid)) {
			DbgPrint("> Del [%s]\n", item->pkgname);
			add_viewer_package_list_del(item);
			/**
			 * A pakcage can manage the several widgets
			 * So we have to clean them all up
			 * Don't break this loop until clear all of them.
			 */
		}
	}
}

static int pkgmgr_install_cb(const char *pkgname, enum pkgmgr_status status, double value, void *data)
{
	if (status != PKGMGR_STATUS_END) {
		return 0;
	}

	widget_service_get_widget_list_by_pkgid(pkgname, widget_add_by_pkgid_cb, NULL);

	invoke_reload_event_callback();
	return 0;
}

static int pkgmgr_uninstall_cb(const char *pkgname, enum pkgmgr_status status, double value, void *data)
{
	if (status != PKGMGR_STATUS_START) {
		return 0;
	}

	if (widget_service_get_widget_list_by_pkgid(pkgname, widget_del_by_pkgid_cb, NULL) <= 0) {
		widget_del_by_appid(pkgname);
	}

	invoke_reload_event_callback();
	return 0;
}

static int pkgmgr_update_cb(const char *pkgname, enum pkgmgr_status status, double value, void *data)
{
	if (status == PKGMGR_STATUS_START) {
		widget_service_get_widget_list_by_pkgid(pkgname, widget_del_by_pkgid_cb, NULL);
	} else if (status == PKGMGR_STATUS_END) {
		widget_service_get_widget_list_by_pkgid(pkgname, widget_add_by_pkgid_cb, NULL);
	}

	invoke_reload_event_callback();
	return 0;
}

HAPI int add_viewer_package_init(void)
{
	int ret;
	int cnt = 0;

	if (s_info.initialized) {
		return 0;
	}

	s_info.initialized = 1;

	ret = add_viewer_pkgmgr_init();
	if (ret != WIDGET_ERROR_NONE && ret != WIDGET_ERROR_ALREADY_STARTED) {
		ErrPrint("Failed to initialize the pkgmgr\n");
	}

	ret = widget_service_get_widget_list(widget_list_callback, NULL);
	if (ret > 0) {
		cnt += ret;
	}

	add_viewer_pkgmgr_add_event_callback(PKGMGR_EVENT_INSTALL, pkgmgr_install_cb, NULL);
	add_viewer_pkgmgr_add_event_callback(PKGMGR_EVENT_UPDATE, pkgmgr_update_cb, NULL);
	add_viewer_pkgmgr_add_event_callback(PKGMGR_EVENT_UNINSTALL, pkgmgr_uninstall_cb, NULL);

	return 0;
}

HAPI int add_viewer_package_fini(void)
{
	struct add_viewer_package *item;
	struct add_viewer_preview *preview;

	if (!s_info.initialized) {
		return 0;
	}

	add_viewer_pkgmgr_del_event_callback(PKGMGR_EVENT_INSTALL, pkgmgr_install_cb, NULL);
	add_viewer_pkgmgr_del_event_callback(PKGMGR_EVENT_UPDATE, pkgmgr_update_cb, NULL);
	add_viewer_pkgmgr_del_event_callback(PKGMGR_EVENT_UNINSTALL, pkgmgr_uninstall_cb, NULL);

	s_info.initialized = 0;

	EINA_LIST_FREE(s_info.package_list, item) {
		free(item->name);
		free(item->pkgid);
		free(item->pkgname);
		free(item->icon);
		free(item->extra.key);
		free(item->extra.data);

		EINA_LIST_FREE(item->preview_list, preview) {
			free(preview);
		}

		item->valid = PACKAGE_INVALID;
		free(item);
	}

	(void)add_viewer_pkgmgr_fini();

	return 0;
}

HAPI void *add_viewer_package_list_handle(void)
{
	return s_info.package_list;
}

HAPI struct add_viewer_package *add_viewer_package_list_item(void *handle)
{
	return eina_list_data_get(handle);
}

HAPI void *add_viewer_package_list_next(void *handle)
{
	handle = eina_list_next(handle);

	if (!handle || s_info.package_list == handle) {
		return NULL;
	}

	return handle;
}

HAPI void *add_viewer_package_list_prev(void *handle)
{
	if (!handle || handle == s_info.package_list) {
		return NULL;
	}

	return eina_list_prev(handle);
}

HAPI int add_viewer_package_list_add_event_callback(struct add_viewer_package *package, int event, int (*cb)(struct add_viewer_package *package, void *data), void *data)
{
	struct package_list_event *item;

	item = calloc(1, sizeof(*item));
	if (!item) {
		ErrPrint("Heap: Fail to calloc - %d\n", errno);
		return -ENOMEM;
	}

	item->event = event;
	item->package = package;
	item->cb = cb;
	item->data = data;

	s_info.event_list = eina_list_append(s_info.event_list, item);
	return 0;
}

HAPI int add_viewer_package_list_del_event_callback(struct add_viewer_package *package, int event, int (*cb)(struct add_viewer_package *package, void *data), void *data)
{
	Eina_List *l;
	Eina_List *n;
	struct package_list_event *item;
	int cnt = 0;

	EINA_LIST_FOREACH_SAFE(s_info.event_list, l, n, item) {
		if (item->package == package && item->cb == cb && item->data == data && item->event == event) {
			cnt++;

			s_info.event_list = eina_list_remove(s_info.event_list, item);
			free(item);
		}
	}

	return cnt > 0 ? 0 : -ENOENT;
}

HAPI void add_viewer_package_list_del(struct add_viewer_package *package)
{
	invoke_del_event_callback(package);
	package_delete(package);
}

HAPI const char *add_viewer_package_list_name(struct add_viewer_package *package)
{
	return package->name;
}

HAPI const char *add_viewer_package_list_pkgname(struct add_viewer_package *package)
{
	return package->pkgname;
}

HAPI const char *add_viewer_package_list_icon(struct add_viewer_package *package)
{
	return package->icon;
}

HAPI int add_viewer_package_list_type(struct add_viewer_package *package)
{
	return package->type;
}

HAPI const char *add_viewer_package_list_extra_key(struct add_viewer_package *package)
{
	return package->extra.key;
}

HAPI const char *add_viewer_package_list_extra_data(struct add_viewer_package *package)
{
	return package->extra.data;
}

HAPI int add_viewer_package_is_valid(struct add_viewer_package *package)
{
	return package->valid == PACKAGE_VALID;
}

HAPI int add_viewer_package_list_set_name(struct add_viewer_package *package, const char *name)
{
	char *new_name;

	if (name) {
		new_name = strdup(name);
		if (!new_name) {
			ErrPrint("Heap: Fail to strdup - %d \n", errno);
			return -ENOMEM;
		}
	} else {
		new_name = NULL;
	}

	if (package->name) {
		free(package->name);
	}

	package->name = new_name;
	return 0;
}

HAPI int add_viewer_package_list_set_pkgname(struct add_viewer_package *package, const char *pkgname)
{
	char *new_name;

	if (pkgname) {
		new_name = strdup(pkgname);
		if (!new_name) {
			ErrPrint("Heap: Fail to strdup - %d \n", errno);
			return -ENOMEM;
		}
	} else {
		new_name = NULL;
	}

	if (package->pkgname) {
		free(package->pkgname);
	}

	package->pkgname = new_name;
	return 0;
}

HAPI int add_viewer_package_list_set_icon(struct add_viewer_package *package, const char *icon)
{
	char *new_name;

	if (icon) {
		new_name = strdup(icon);
		if (!new_name) {
			ErrPrint("Heap: Fail to strdup - %d\n", errno);
			return -ENOMEM;
		}
	} else {
		new_name = NULL;
	}

	if (package->icon) {
		free(package->icon);
	}

	package->icon = new_name;
	return 0;
}

HAPI void *add_viewer_package_list_preview_list(struct add_viewer_package *package)
{
	return package->preview_list;
}

HAPI struct add_viewer_preview *add_viewer_package_list_preview(struct add_viewer_package *package, void *handle)
{
	return eina_list_data_get(handle);
}

HAPI void *add_viewer_package_list_preview_next(struct add_viewer_package *package, void *handle)
{
	handle = eina_list_next(handle);
	if (!handle || handle == package->preview_list) {
		return NULL;
	}

	return handle;
}

HAPI void *add_viewer_package_list_preview_prev(struct add_viewer_package *package, void *handle)
{
	if (!handle || handle == package->preview_list) {
		return NULL;
	}

	handle = eina_list_prev(handle);
	return handle;
}

HAPI int add_viewer_package_list_preview_size(struct add_viewer_preview *preview)
{
	return preview->size_type;
}

HAPI void *add_viewer_package_list_preview_data(struct add_viewer_preview *preview)
{
	return preview->data;
}

HAPI void add_viewer_package_list_preview_set_data(struct add_viewer_preview *preview, void *data)
{
	preview->data = data;
}

HAPI int add_viewer_package_is_disabled(struct add_viewer_package *package)
{
	return package->disabled;
}

HAPI void add_viewer_package_set_disabled(struct add_viewer_package *package, int flag)
{
	package->disabled = flag;
}

HAPI void add_viewer_package_set_skip(struct add_viewer_package *package, int skip)
{
	if (skip == 0) {
		if (package->skipped > 0) {
			package->skipped--;
		}
	} else {
		package->skipped++;
	}
}

HAPI int add_viewer_package_is_skipped(struct add_viewer_package *package)
{
	return package->skipped;
}

HAPI void *add_viewer_package_data(struct add_viewer_package *package)
{
	return package->data;
}

HAPI void add_viewer_package_set_data(struct add_viewer_package *package, void *data)
{
	package->data = data;
}

HAPI struct add_viewer_package *add_viewer_package_find(const char *widget_id)
{
	Eina_List *l;
	struct add_viewer_package *package;

	EINA_LIST_FOREACH(s_info.package_list, l, package) {
		if (!strcmp(package->pkgname, widget_id)) {
			return package;
		}
	}

	return NULL;
}

HAPI int add_viewer_package_reload_name(void)
{
	char *name;
	char *icon;
	struct add_viewer_package *package;
	Eina_List *new_list = NULL;

	EINA_LIST_FREE(s_info.package_list, package) {
		name = package->name;
		icon = package->icon;
		if (load_name_and_icon(package) < 0) {
			ErrPrint("Unable to reload name: %s\n", package->pkgid);
			package->name = name;
			package->icon = icon;
		} else {
			free(name);
			free(icon);
		}

		new_list = eina_list_sorted_insert(new_list, sort_cb, package);
	}

	s_info.package_list = new_list;
	return 0;
}

/* End of a file */
