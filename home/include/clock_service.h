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

#ifndef __CLOCK_SERVICE_H
#define __CLOCK_SERVICE_H

#define PAGE_CLOCK_EDJE_FILE EDJEDIR"/page_clock.edj"

#define CLOCK_RET_OK 0
#define CLOCK_RET_FAIL 1
#define CLOCK_RET_ASYNC 2
#define CLOCK_RET_NEED_DESTROY_PREVIOUS 4

#define CLOCK_SERVICE_MODE_NORMAL 1
#define CLOCK_SERVICE_MODE_NORMAL_RECOVERY 2
#define CLOCK_SERVICE_MODE_RECOVERY 3
#define CLOCK_SERVICE_MODE_EMERGENCY 4
#define CLOCK_SERVICE_MODE_COOLDOWN 5

#define CLOCK_APP_TYPE_NATIVE 1
#define CLOCK_APP_TYPE_WEBAPP 2
#define CLOCK_APP_FACTORY_DEFAULT "org.tizen.w-idle-clock-weather2"
#define CLOCK_APP_RECOVERY "org.tizen.idle-clock-digital"
#define CLOCK_APP_EMERGENCY "org.tizen.idle-clock-ups"
#define CLOCK_APP_COOLDOWN "org.tizen.idle-clock-emergency"

#define CLOCK_STATE_IDLE 0
#define CLOCK_STATE_WAITING 1
#define CLOCK_STATE_RUNNING 2

/*
 * Events
 */
#define CLOCK_EVENT_MAP(SOURCE, TYPE) ((SOURCE << 16) | TYPE)

/*
 * instant events
 */
#define CLOCK_EVENT_VIEW 0x0000
#define CLOCK_EVENT_VIEW_READY				CLOCK_EVENT_MAP(CLOCK_EVENT_VIEW, 0x0)
#define CLOCK_EVENT_VIEW_RESIZED			CLOCK_EVENT_MAP(CLOCK_EVENT_VIEW, 0x1)
#define CLOCK_EVENT_VIEW_HIDDEN_SHOW		CLOCK_EVENT_MAP(CLOCK_EVENT_VIEW, 0x2)
#define CLOCK_EVENT_VIEW_HIDDEN_HIDE		CLOCK_EVENT_MAP(CLOCK_EVENT_VIEW, 0x4)
#define CLOCK_EVENT_APP 0x0001
#define CLOCK_EVENT_APP_PROVIDER_ERROR		CLOCK_EVENT_MAP(CLOCK_EVENT_APP, 0x0)
#define CLOCK_EVENT_APP_PROVIDER_ERROR_FATAL	CLOCK_EVENT_MAP(CLOCK_EVENT_APP, 0x1)
#define CLOCK_EVENT_APP_PAUSE				CLOCK_EVENT_MAP(CLOCK_EVENT_APP, 0x2)
#define CLOCK_EVENT_APP_RESUME				CLOCK_EVENT_MAP(CLOCK_EVENT_APP, 0x4)
#define CLOCK_EVENT_APP_LANGUAGE_CHANGED	CLOCK_EVENT_MAP(CLOCK_EVENT_APP, 0x8)
#define CLOCK_EVENT_DEVICE 0x0002
#define CLOCK_EVENT_DEVICE_BACK_KEY			CLOCK_EVENT_MAP(CLOCK_EVENT_DEVICE, 0x0)
#define CLOCK_EVENT_DEVICE_LCD 0x0004
#define CLOCK_EVENT_DEVICE_LCD_ON			CLOCK_EVENT_MAP(CLOCK_EVENT_DEVICE_LCD, 0x0)
#define CLOCK_EVENT_DEVICE_LCD_OFF			CLOCK_EVENT_MAP(CLOCK_EVENT_DEVICE_LCD, 0x1)

/*
 * ongoing events
 */
#define CLOCK_EVENT_SCREEN_READER 0x0008
#define CLOCK_EVENT_SCREEN_READER_ON		CLOCK_EVENT_MAP(CLOCK_EVENT_SCREEN_READER, 0x0)
#define CLOCK_EVENT_SCREEN_READER_OFF		CLOCK_EVENT_MAP(CLOCK_EVENT_SCREEN_READER, 0x1)
#define CLOCK_EVENT_SAP 0x0020
#define CLOCK_EVENT_SAP_ON		CLOCK_EVENT_MAP(CLOCK_EVENT_SAP, 0x0)
#define CLOCK_EVENT_SAP_OFF		CLOCK_EVENT_MAP(CLOCK_EVENT_SAP, 0x1)
#define CLOCK_EVENT_MODEM 0x0040
#define CLOCK_EVENT_MODEM_ON		CLOCK_EVENT_MAP(CLOCK_EVENT_MODEM, 0x0)
#define CLOCK_EVENT_MODEM_OFF		CLOCK_EVENT_MAP(CLOCK_EVENT_MODEM, 0x1)
#define CLOCK_EVENT_DND 0x0080
#define CLOCK_EVENT_DND_ON		CLOCK_EVENT_MAP(CLOCK_EVENT_DND, 0x0)
#define CLOCK_EVENT_DND_OFF		CLOCK_EVENT_MAP(CLOCK_EVENT_DND, 0x1)
#define CLOCK_EVENT_SCROLLER 0x0100
#define CLOCK_EVENT_SCROLLER_FREEZE_ON	CLOCK_EVENT_MAP(CLOCK_EVENT_SCROLLER, 0x0)
#define CLOCK_EVENT_SCROLLER_FREEZE_OFF	CLOCK_EVENT_MAP(CLOCK_EVENT_SCROLLER, 0x1)
#define CLOCK_EVENT_POWER 0x0200
#define CLOCK_EVENT_POWER_ENHANCED_MODE_ON	CLOCK_EVENT_MAP(CLOCK_EVENT_POWER, 0x0)
#define CLOCK_EVENT_POWER_ENHANCED_MODE_OFF	CLOCK_EVENT_MAP(CLOCK_EVENT_POWER, 0x1)
#define CLOCK_EVENT_SIM 0x0400
#define CLOCK_EVENT_SIM_INSERTED	CLOCK_EVENT_MAP(CLOCK_EVENT_SIM, 0x0)
#define CLOCK_EVENT_SIM_NOT_INSERTED	CLOCK_EVENT_MAP(CLOCK_EVENT_SIM, 0x1)
#define CLOCK_EVENT_CFWD 0x0800

#define CLOCK_EVENT_CATEGORY(X) CLOCK_EVENT_MAP(X, 0x0)

#define CLOCK_SMART_SIGNAL_PAUSE "clock,paused"
#define CLOCK_SMART_SIGNAL_RESUME "clock,resume"
#define CLOCK_SMART_SIGNAL_VIEW_REQUEST "clock,view,request"

#define CLOCK_VIEW_REQUEST_DRAWER_HIDE 1
#define CLOCK_VIEW_REQUEST_DRAWER_SHOW 2

#define CLOCK_ATTACHED 1
#define CLOCK_CANDIDATE 2

#define CLOCK_INF_MINICONTROL 1
#define CLOCK_INF_DBOX 2

#define CLOCK_CONF_NONE 0
#define CLOCK_CONF_WIN_ACTIVATION 1
#define CLOCK_CONF_CLOCK_CONFIGURATION 2

#define CLOCK_VIEW_TYPE_DRAWER 1

#define VCONFKEY_MUSIC_STATUS "memory/homescreen/music_status"

enum {
	INDICATOR_HIDE_WITH_EFFECT,
	INDICATOR_HIDE,
	INDICATOR_SHOW,
	INDICATOR_SHOW_WITH_EFFECT,
};

typedef struct {
	int configure;
	int interface;
	int app_type;
	int async;
	int state;
	int use_dead_monitor;
	int pid;
	int need_event_relay;
	char *view_id;
	char *appid;
	char *pkgname;
	void *view;
	int w;
	int h;
} clock_s;
typedef clock_s * clock_h;

typedef struct _clock_inf_s {
	int async;
	int use_dead_monitor;
	int (*prepare) (clock_h);
	int (*config) (clock_h, int);
	int (*create) (clock_h);
	int (*attached_cb) (clock_h);
	int (*destroy) (clock_h);
} clock_inf_s;

/*!
 * Clock Service
 */
extern void clock_service_init(void);
extern void clock_service_fini(void);

extern void clock_service_mode_set(int mode);
extern int clock_service_mode_get(void);
extern char *clock_service_clock_pkgname_get(void);
extern void clock_service_request(int mode);
extern void clock_service_pause(void);
extern void clock_service_resume(void);
extern void clock_service_app_dead_cb(int pid);
extern void clock_service_event_handler(clock_h clock, int event);
extern int clock_service_clock_selector_launch(void);
extern void clock_service_scroller_freezed_set(int is_freeze);
extern int clock_service_scroller_freezed_get(void);
extern const int const clock_service_get_retry_count(void);

/*!
 * Clock Event
 */
extern void clock_service_event_register(void);
extern void clock_service_event_deregister(void);
extern int clock_service_event_state_get(int event_source);
extern void clock_service_event_app_dead_cb(int pid);

/*!
 * Clock manager
 */
extern clock_h clock_new(const char *pkgname);
extern void clock_del(clock_h clock);

extern int clock_manager_view_prepare(clock_h clock);
extern int clock_manager_view_configure(clock_h clock, int type);
extern int clock_manager_view_create(clock_h clock);
extern int clock_manager_view_destroy(clock_h clock);
extern int clock_manager_view_attach(clock_h clock);
extern int clock_manager_view_deattach(clock_h clock);
extern int clock_manager_view_exchange(clock_h clock, void *view);

extern int clock_manager_clock_inf_type_get(const char *pkgname);
extern clock_h clock_manager_clock_get(int type);
extern void clock_manager_clock_set(int type, clock_h clock);
extern int clock_manager_view_state_get(int view_type);

/*!
 * Clock View
 */
extern void clock_view_indicator_show(int is_show);
extern Evas_Object *clock_view_add(Evas_Object *parent, Evas_Object *item);
extern int clock_view_attach(Evas_Object *page);
extern int clock_view_deattach(Evas_Object *page);
extern void clock_view_show(Evas_Object *page);
extern void clock_view_event_handler(clock_h clock, int event, int need_relay);
extern Evas_Object *clock_view_empty_add(void);
extern int clock_view_display_state_get(Evas_Object *page, int view_type);
extern Evas_Object *clock_view_get_item(Evas_Object *view);

/*!
 * Clock Hidden view
 */
extern Evas_Object *clock_view_hidden_add(Evas_Object *page);
extern void clock_view_hidden_event_handler(Evas_Object *page, int event);

/*!
 * Clock Visual Cue
 */
extern Evas_Object *clock_view_cue_add(Evas_Object *page);
extern void clock_view_cue_display_set(Evas_Object *page, int is_display, int is_need_vi);

/*!
 * Clock Shortcut
 */
extern void clock_shortcut_init(void);
extern void clock_shortcut_fini(void);
extern void clock_shortcut_view_add(Evas_Object *page);
extern void clock_shortcut_app_dead_cb(int pid);

/*!
 * Clock indicator
 */
extern void clock_view_indicator_add(Evas_Object *page);
extern void clock_view_indicator_event_handler(Evas_Object *page, int event);

/*!
 * Clock Uitility
 */
extern char *clock_util_wms_configuration_get(void);
extern int clock_util_wms_configuration_set(const char *pkgname);
extern int clock_util_provider_launch(const char *clock_pkgname, int *pid_a, int configure);
extern void clock_util_terminate_clock_by_pid(int pid);
extern int clock_util_screen_reader_enabled_get(void);
extern int clock_util_setting_conf_get(void);
extern void clock_util_setting_conf_set(int value);
extern void clock_util_setting_conf_bundle_add(bundle *b, int type);
extern const char *clock_util_setting_conf_content(int type);

/*!
 * Clock interface::minicontrol
 */
extern void clock_inf_minictrl_event_hooker(int action, int pid, const char *minictrl_id, int is_realized, int width, int height);

/*!
 * Clock interface::dbox
 */
extern int clock_inf_prepare_first_clock(const char *pkgname);

#endif
