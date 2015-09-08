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

#ifndef __W_HOME_UTIL_H__
#define __W_HOME_UTIL_H__

#include <bundle.h>
#include <Evas.h>

/* Key */
#define DATA_KEY_ADD_VIEWER "av"
#define DATA_KEY_BG "bg"
#define DATA_KEY_CHECK "ch"
#define DATA_KEY_CHECK_POPUP "ch_pp"
#define DATA_KEY_CONFORMANT "cf"
#define DATA_KEY_EDIT_INFO "ed_if"
#define DATA_KEY_EDIT_MODE "ed_md"
#define DATA_KEY_EFFECT_PAGE "ef_pg"
#define DATA_KEY_EVENT_UPPER_IS_ON "ev_io"
#define DATA_KEY_EVENT_UPPER_PAGE "ev_up"
#define DATA_KEY_EVENT_UPPER_TIMER "ev_ut"
#define DATA_KEY_INDEX "id"
#define DATA_KEY_INDEX_INFO "id_if"
#define DATA_KEY_IS_LONGPRESS "is_lp"
#define DATA_KEY_ITEM_INFO "it_if"
#define DATA_KEY_LAYOUT "ly"
#define DATA_KEY_LAYOUT_INFO "ly_if"
#define DATA_KEY_MAPBUF "mb"
#define DATA_KEY_MAPBUF_DISABLED_PAGE "mb_dis"
#define DATA_KEY_PAGE "pg"
#define DATA_KEY_PAGE_INFO "pg_i"
#define DATA_KEY_PAGE_ONHOLD_COUNT "pg_ohc"
#define DATA_KEY_PROXY_PAGE "pr_p"
#define DATA_KEY_REAL_PAGE "rp"
#define DATA_KEY_SCROLLER "sc"
#define DATA_KEY_SCROLLER_INFO "sc_i"
#define DATA_KEY_WIN "win"


//Apps
#define DATA_KEY_BG "bg"
#define DATA_KEY_BOX "bx"
#define DATA_KEY_BUTTON "bt"
#define DATA_KEY_CURRENT_ITEM "cu_item"
#define DATA_KEY_EDJE_OBJECT "ej_o"
#define DATA_KEY_EVAS_OBJECT "ev_o"
#define DATA_KEY_EVENT_UPPER_ITEM "eu_it"
#define DATA_KEY_FUNC_DELETE_PACKAGE "fn_del_pkg"
#define DATA_KEY_FUNC_DESTROY_POPUP "fn_dr_pp"
#define DATA_KEY_FUNC_DISABLE_ITEM "fn_dis_it"
#define DATA_KEY_FUNC_ENABLE_ITEM "fn_en_it"
#define DATA_KEY_FUNC_HIDE_POPUP "fn_hd_pp"
#define DATA_KEY_FUNC_ROTATE_POPUP "fn_rt_pp"
#define DATA_KEY_FUNC_SHOW_POPUP "fn_sh_pp"
#define DATA_KEY_GIC "gic"
#define DATA_KEY_GRID "gr"
#define DATA_KEY_HIDE_TRAY_TIMER "htt"
#define DATA_KEY_HISTORY_LIST "hi_l"
#define DATA_KEY_IDLE_TIMER "idle_timer"
#define DATA_KEY_INSTANCE_INFO "ii"
#define DATA_KEY_ITEM "it"
#define DATA_KEY_ITEM_ICON_HEIGHT "it_ic_h"
#define DATA_KEY_ITEM_ICON_WIDTH "it_ic_w"
#define DATA_KEY_ITEM_HEIGHT "it_h"
#define DATA_KEY_ITEM_WIDTH "it_w"
#define DATA_KEY_LAYOUT "ly"
#define DATA_KEY_LAYOUT_IS_PAUSED "ly_is_ps"
#define DATA_KEY_LAYOUT_FOCUS_ON "ly_fcs_on"
#define DATA_KEY_LAYOUT_HIDE_TIMER "ly_hd_tm"
#define DATA_KEY_LIST "ls"
#define DATA_KEY_LIST_INDEX "ls_ix"
#define DATA_KEY_POPUP "pp"
#define DATA_KEY_SCROLLER "sc"
#define DATA_KEY_TIMER "timer"
#define DATA_KEY_WIN "win"
#define DATA_KEY_IS_VIRTUAL_ITEM "vit"
#define DATA_KEY_IS_ORDER_CHANGE "ioc"
#define DATA_KEY_ITEM_UNINSTALL_RESERVED "i_u_r"

#define STR_SIGNAL_EMIT_SIGNAL_ROTATE "rotate"
#define STR_SIGNAL_EMIT_SIGNAL_UNROTATE "unrotate"



/* Strings */
#define APPID_W_HOME "org.tizen.w-home"
#define APPID_APPS "org.tizen.apps-dbox"
#define FILE_W_HOME_DB DATADIR"/.w_home.db"
#define STR_SIGNAL_EMIT_SIGNAL_ROTATE "rotate"
#define STR_SIGNAL_EMIT_SIGNAL_UNROTATE "unrotate"



/* Multi-language */
#if !defined(_)
#define _(str) gettext(str)
#endif
#define gettext_noop(str) (str)
#define N_(str) gettext_noop(str)
#define D_(str) dgettext("sys_string", str)



/* Accessibility */
#define ACCESS_BUTTON "button"
#define ACCESS_EDIT "IDS_AT_POP_EDIT"



/* SIZE */
#define NAME_LEN 256
#define TEXT_LEN 256
#define LOCALE_LEN 32
#define BUFSZE 1024
#define SPARE_LEN 128
#define INDEX_COUNT_LEN 3
#define PART_NAME_SIZE 128


/* VCONF */
#define VCONF_KEY_WMS_APPS_ORDER "memory/wms/apps_order"
#define VCONF_KEY_WMS_FAVORITES_ORDER "memory/wms/favorites_order"
#define VCONF_KEY_HOME_LOGGING "db/private/org.tizen.w-home/logging"
#define VCONF_KEY_HOME_IS_TUTORIAL "memory/private/org.tizen.w-home/tutorial"
#define VCONF_KEY_HOME_IS_TUTORIAL_ENABLED_TO_RUN "db/private/org.tizen.w-home/enabled_tutorial"
#define VCONFKEY_APPS_IS_INITIAL_POPUP "db/private/org.tizen.w-home/apps_initial_popup"


/* Build */
#define HAPI __attribute__((visibility("hidden")))

/* Packaging */
#define DEFAULT_ICON IMAGEDIR"/unknown.png"


/* Enum */
typedef enum {
	W_HOME_ERROR_NONE = 0,
	W_HOME_ERROR_FAIL = -1,
	W_HOME_ERROR_DB_FAILED = -2,
	W_HOME_ERROR_OUT_OF_MEMORY = -3,
	W_HOME_ERROR_INVALID_PARAMETER = -4,
	W_HOME_ERROR_NO_DATA = -5,
} w_home_error_e;



typedef enum {
	APP_STATE_CREATE = 0,
	APP_STATE_PAUSE,
	APP_STATE_RESET,
	APP_STATE_RESUME,
	APP_STATE_LANGUAGE_CHANGED,
	APP_STATE_TERMINATE,
	APP_STATE_POWER_OFF,
	APP_STATE_MAX,
} app_state_e;



typedef enum {
	FEATURE_CLOCK_SELECTOR = 0x0001,
	FEATURE_CLOCK_CHANGE = 0x0002,
	FEATURE_CLOCK_VISUAL_CUE = 0x0004,
	FEATURE_CLOCK_HIDDEN_BUTTON = 0x0008,
	FEATURE_APPS_BY_BEZEL_UP = 0x0010,
	FEATURE_APPS = 0x0020,
	FEATURE_TUTORIAL = 0x0040,
	FEATURE_MAX,
} feature_e;


typedef enum {
	APPS_ERROR_NONE = 0,
	APPS_ERROR_FAIL = -1,
	APPS_ERROR_DB_FAILED = -2,
	APPS_ERROR_OUT_OF_MEMORY = -3,
	APPS_ERROR_INVALID_PARAMETER = -4,
	APPS_ERROR_NO_DATA = -5,
} apps_error_e;

enum {
	APPS_APP_STATE_CREATE = 0,
	APPS_APP_STATE_PAUSE,
	APPS_APP_STATE_RESET,
	APPS_APP_STATE_RESUME,
	APPS_APP_STATE_TERMINATE,
	APPS_APP_STATE_POWER_OFF,
	APPS_APP_STATE_MAX,
};



enum {
	APPS_ROTATE_PORTRAIT = 0,
	APPS_ROTATE_LANDSCAPE,
};



enum {
	APP_TYPE_NATIVE = 0,
	APP_TYPE_WEB,
	APP_TYPE_WIDGET,
};



#if !defined(_EDJ)
#define _EDJ(a) elm_layout_edje_get(a)
#endif

/* Functions */
extern void _evas_object_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
extern void _evas_object_event_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
extern void _evas_object_event_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
extern void _evas_object_event_show_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
extern void _evas_object_event_hide_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
extern void _evas_object_event_changed_size_hints_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

extern void util_post_message_for_launch_fail(const char *name);
extern void util_launch(const char *package, const char *name);
extern void util_launch_with_arg(const char *app_id, const char *arg, const char *name);
extern void util_launch_with_bundle(const char *app_id, bundle *b, const char *name);
extern void util_notify_to_home(int pid);

extern int util_launch_app(const char *appid, const char *key, const char *value);
extern int util_feature_enabled_get(int feature);

extern int util_find_top_visible_window(char **command);

//apps
extern void apps_util_launch(Evas_Object *win, const char *package, const char *name);
extern void apps_util_launch_main_operation(Evas_Object *win, const char *app_id, const char *name);
extern void apps_util_launch_with_arg(Evas_Object *win, const char *app_id, const char *arg, const char *name);
extern void apps_util_launch_with_bundle(Evas_Object *win, const char *app_id, bundle *b, const char *name);

extern void apps_util_post_message_for_launch_fail(const char *name);
extern void apps_util_notify_to_home(int pid);

extern int util_create_toast_popup(Evas_Object *parent, const char* text);
extern int util_create_check_popup(Evas_Object *parent, const char* text, void _clicked_cb(void *, Evas_Object *, void *));

extern int util_get_app_type(const char *appid);
extern char *util_get_appid_by_pkgname(const char *pkgname);

extern const char *util_basename(const char *name);
extern double util_timestamp(void);

extern void util_activate_home_window(void);
#endif /* __W_HOME_UTIL_H__ */
