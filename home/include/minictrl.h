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

#ifndef __W_MINICTRL_H__
#define __W_MINICTRL_H__

#define MINICONTROL_MAGIC 0xCAFEDEAD

#define _MINICTRL_VIEW(obj) (page_get_item(obj))
#define _MINICTRL_PLUG_OBJ(obj) (elm_object_part_content_get(_MINICTRL_VIEW(obj), "item"))

#define MC_CATEGORY_CLOCK 1
#define MC_CATEGORY_NOTIFICATION 2
#define MC_CATEGORY_DASHBOARD 3

#define MINICTRL_DATA_KEY_MAGIC "mc_magic"
#define MINICTRL_DATA_KEY_PID "mc_pid"
#define MINICTRL_DATA_KEY_VISIBILITY "mc_vis"
#define MINICTRL_DATA_KEY_CATEGORY "mc_cat"
#define MINICTRL_DATA_KEY_NAME "mc_name"

#define MINICTRL_EVENT_APP_RESUME "mc_resume"
#define MINICTRL_EVENT_APP_PAUSE "mc_pause"
#define MINICTRL_EVENT_VIEW_REMOVED "mc_removed"

typedef struct _Minictrl_Entry {
	int category;
	char *name;
	char *service_name;
	Evas_Object *view;
} Minictrl_Entry;

typedef void (*Minictrl_Entry_Foreach_Cb)(Minictrl_Entry *entry, void *user_data);

/*!
 * Main functions
 */
extern void minicontrol_init(void);
extern void minicontrol_fini(void);
extern void minicontrol_resume(void);
extern void minicontrol_pause(void);

/*!
 * Object manager functions
 */
extern void minictrl_manager_entry_add_with_data(const char *name, const char *service_name, int category, Evas_Object *view);
extern Evas_Object *minictrl_manager_view_get_by_category(int category);
extern Evas_Object *minictrl_manager_view_get_by_name(const char *name);
extern Minictrl_Entry *minictrl_manager_entry_get_by_category(int category);
extern Minictrl_Entry *minictrl_manager_entry_get_by_name(const char *name);
extern int minictrl_manager_entry_del_by_category(int category);
extern int minictrl_manager_entry_del_by_name(const char *name);
extern void minictrl_manager_entry_foreach(Minictrl_Entry_Foreach_Cb cb, void *user_data);

/*!
 * Utility functions
 */
extern int minicontrol_message_send(Evas_Object *obj, char *message);
extern void minicontrol_visibility_set(Evas_Object *obj, int visibility);
extern int minicontrol_visibility_get(Evas_Object *obj);
extern void minicontrol_magic_set(Evas_Object *obj);
extern int minicontrol_magic_get(Evas_Object *obj);
extern void minicontrol_name_set(Evas_Object *obj, const char *name);
extern char *minicontrol_name_get(Evas_Object *obj);
extern void minicontrol_category_set(Evas_Object *obj, int category);
extern int minicontrol_category_get(Evas_Object *obj);
extern void minicontrol_pid_set(Evas_Object *obj, int pid);
extern int minicontrol_pid_get(Evas_Object *obj);
extern char *minicontrol_appid_get_by_pid(int pid);

#endif
