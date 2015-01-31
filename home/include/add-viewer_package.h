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

struct add_viewer_package;
struct add_viewer_preview;

enum package_type {
	PACKAGE_TYPE_APP,
	PACKAGE_TYPE_SHORTCUT,
	PACKAGE_TYPE_BOX,
	PACKAGE_TYPE_UNKNOWN,
};

enum pkg_evt_type {
	PACKAGE_LIST_EVENT_DEL,
	PACKAGE_LIST_EVENT_UPDATE,
	PACKAGE_LIST_EVENT_RELOAD,
};

extern int add_viewer_package_init(void);
extern int add_viewer_package_fini(void);
extern void *add_viewer_package_list_handle(void);
extern struct add_viewer_package *add_viewer_package_list_item(void *handle);
extern void *add_viewer_package_list_next(void *handle);
extern void *add_viewer_package_list_prev(void *handle);
extern void add_viewer_package_list_del(struct add_viewer_package *package);

extern const char *add_viewer_package_list_name(struct add_viewer_package *package);
extern const char *add_viewer_package_list_pkgname(struct add_viewer_package *package);
extern const char *add_viewer_package_list_icon(struct add_viewer_package *package);
extern int add_viewer_package_list_type(struct add_viewer_package *package);
extern const char *add_viewer_package_list_extra_key(struct add_viewer_package *package);
extern const char *add_viewer_package_list_extra_data(struct add_viewer_package *package);

extern int add_viewer_package_list_set_name(struct add_viewer_package *package, const char *name);
extern int add_viewer_package_list_set_pkgname(struct add_viewer_package *package, const char *pkgname);
extern int add_viewer_package_list_set_icon(struct add_viewer_package *package, const char *icon);
extern void *add_viewer_package_list_preview_list(struct add_viewer_package *package);
extern struct add_viewer_preview *package_list_preview(struct add_viewer_package *package, void *handle);
extern void *add_viewer_package_list_preview_next(struct add_viewer_package *package, void *handle);
extern void *add_viewer_package_list_preview_prev(struct add_viewer_package *package, void *handle);
extern int add_viewer_package_list_preview_size(struct add_viewer_preview *preview);
extern void *add_viewer_package_list_preview_data(struct add_viewer_preview *preview);
extern void add_viewer_package_list_preview_set_data(struct add_viewer_preview *preview, void *data);
extern int add_viewer_package_is_disabled(struct add_viewer_package *package);
extern void add_viewer_package_set_disabled(struct add_viewer_package *package, int flag);
extern void add_viewer_package_set_skip(struct add_viewer_package *package, int skip);
extern int add_viewer_package_is_skipped(struct add_viewer_package *package);

extern void add_viewer_package_set_data(struct add_viewer_package *package, void *data);
extern void *add_viewer_package_data(struct add_viewer_package *package);

extern int add_viewer_package_list_add_event_callback(struct add_viewer_package *package, int event, int (*cb)(struct add_viewer_package *package, void *data), void *data);
extern int add_viewer_package_list_del_event_callback(struct add_viewer_package *package, int event, int (*cb)(struct add_viewer_package *package, void *data), void *data);

extern int add_viewer_package_is_valid(struct add_viewer_package *package);
extern struct add_viewer_package *add_viewer_package_find(const char *dbox_id);
extern int add_viewer_package_reload_name(void);

/* End of a file */
