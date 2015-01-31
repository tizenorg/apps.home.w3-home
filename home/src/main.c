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

#include <app.h>
#include <appcore-efl.h>
#include <aul.h>
#include <bundle.h>
#include <Ecore.h>
#include <Ecore_X.h>
#include <efl_assist.h>
#include <Elementary.h>
#include <errno.h>
#include <dynamicbox.h>
#include <dynamicbox_service.h>
#include <dynamicbox_errno.h>
#include <utilX.h>
#include <unistd.h>
#include <vconf.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
#include <minicontrol-handler.h>
#include <dlog.h>

#include "bg.h"
#include "conf.h"
#include "layout.h"
#include "layout_info.h"
#include "dynamicbox_evas.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "pkgmgr.h"
#include "win.h"
#include "key.h"
#include "effect.h"
#include "page_info.h"
#include "page.h"
#include "scroller_info.h"
#include "scroller.h"
#include "minictrl.h"
#include "clock_service.h"
#include "dbus.h"
#include "gesture.h"
#include "add-viewer.h"
#include "power_mode.h"
#include "noti_broker.h"
#include "wms.h"
#include "tutorial.h"
#include "apps/apps_main.h"
#include "critical_log.h"
#include "db.h"
#include "xml.h"
#include "dbox.h"
#include "edit.h"
#include "popup.h"

#define HOME_SERVICE_KEY "home_op"
#define HOME_SERVICE_VALUE_POWERKEY "powerkey"
#define HOME_SERVICE_VALUE_EDIT "edit"
#define HOME_SERVICE_VALUE_SHOW_APPS "show_apps"
#define HOME_SERVICE_VALUE_SHOW_NOTI "show_noti"
#define HOME_SERVICE_VALUE_APPS_EDIT "apps_edit"
#define HOME_SERVICE_VALUE_FIRST_BOOT "first_boot"

#define VCONFKEY_HOME_VISIBILITY "memory/homescreen/clock_visibility"
#define VCONFKEY_SETAPPL_CHANGE_UI_THEME_INT VCONFKEY_SETAPPL_PREFIX"/change_ui/theme"

#define PRIVATE_DATA_KEY_FOCUS_IN_EVENT_HANDLER "k_fi_ev_hd"
#define PRIVATE_DATA_KEY_FOCUS_OUT_EVENT_HANDLER "k_fo_ev_hd"
#define PRIVATE_DATA_KEY_VISIBILITY_EVENT_HANDLER "k_vi_ev_hd"

#define APP_CONTROL_OPERATION_MAIN "http://tizen.org/appcontrol/operation/main"

#define VISIBILITY_TIMEOUT 8.0f
#define LAZY_NOTI_TIMER 3.0f
#define LAZY_DBOX_TIMER 5.0f


static main_s main_info = {
	.color_theme = NULL,
	.font_theme = NULL,
	.theme = NULL,
	.layout = NULL,

	.state = APP_STATE_CREATE,
	.is_mapbuf = 0,
	.apps_pid = 0,
	.is_tts = false,
	.is_lcd_on = -1,
	.is_alpm_clock_enabled = 0,

	.safety_init_timer = NULL,
};



typedef struct {
	w_home_error_e (*result_cb)(void *);
	void *result_data;
} main_cb_s;



static void _activate_window_job_cb(void *data);



HAPI main_s *main_get_info(void)
{
	return &main_info;
}



static void _home_visibility_vconf_set(int is_visible)
{
	int ret = 0;

	if ((ret = vconf_set_int(VCONFKEY_HOME_VISIBILITY, is_visible)) != 0) {
		_E("Failed to set key:%s(%d) value:%d", VCONFKEY_HOME_VISIBILITY, ret, is_visible);
	}
}



static int _is_window_on_top(void)
{
	int pid = -1;
	char *cmd = NULL;

	pid = util_find_top_visible_window(&cmd);
	if (pid == -1) {
		_E("Failed to get top visible window pid");
	}

	if (pid == getpid()) {
		free(cmd);
		return 1;
	}

	free(cmd);

	return 0;
}



static int _is_tutorial_enabled(void)
{
	int ret = 0;
	int tutorial_enabled = 0;
	if ((ret = vconf_get_int(VCONF_KEY_HOME_IS_TUTORIAL, &tutorial_enabled)) != 0) {
		_E("Cannot get the vconf for %s %d", VCONF_KEY_HOME_IS_TUTORIAL, ret);
		return 0;
	}

	return tutorial_enabled;
}



static int _is_lcd_turned_on(void)
{
	int val = VCONFKEY_PM_STATE_NORMAL;

	if (main_info.is_lcd_on >= 0) {
		return main_get_info()->is_lcd_on;
	} else {
		if (vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
			_E("Failed to get PM STATE");
		}

		if (val == VCONFKEY_PM_STATE_NORMAL || val == VCONFKEY_PM_STATE_LCDDIM) {
			return 1;
		}
	}

	return 0;
}



static void _del_list(void *data)
{
	Eina_List *page_info_list = data;
	ret_if(!page_info_list);
	page_info_list_destroy(page_info_list);
}



static inline char *_ltrim(char *str)
{
	retv_if(NULL == str, NULL);
	while (*str && (*str == ' ' || *str == '\t' || *str == '\n')) str ++;
	return str;
}



static inline int _rtrim(char *str)
{
	int len;

	retv_if(NULL == str, 0);

	len = strlen(str);
	while (--len >= 0 && (str[len] == ' ' || str[len] == '\n' || str[len] == '\t')) {
		str[len] = '\0';
	}

	return len;
}



Eina_List *_create_tts_list(const char *file)
{
	retv_if(!file, NULL);

	FILE *fp;
	char *line = NULL;
	size_t size = 0;
	ssize_t read;
	char *data = NULL;
	Eina_List *list = NULL;

	fp = fopen(file, "r");
	retv_if(!fp, NULL);

	while ((read = getline(&line, &size, fp)) != -1) {
		char *begin;

		if (size <= 0) {
			free(line);
			line = NULL;
			break;
		}

		begin = _ltrim(line);
		_rtrim(line);

		if (*begin == '#' || *begin == '\0') {
			free(line);
			line = NULL;
			continue;
		}

		data = strdup(begin);
		list = eina_list_append(list, data);

		if (line) {
			free(line);
			line = NULL;
		}
	}

	fclose(fp);

	return list;
}


void _destroy_tts_list(Eina_List *list)
{
	char *data = NULL;

	ret_if(!list);

	EINA_LIST_FREE(list, data) {
		free(data);
	}
}



static int _page_info_in_list(page_info_s *page_info, Eina_List *list)
{
	char *name = NULL;
	const Eina_List *l, *ln;

	retv_if(!list, 0);
	retv_if(!page_info, 0);
	retv_if(!page_info->id, 0);

	EINA_LIST_FOREACH_SAFE(list, l, ln, name) {
		continue_if(!name);
		if(!strcmp(name, page_info->id)) return 1;
	}

	return 0;
}


#define BLACK_LIST RESDIR"/tts_black.list"
#define WHITE_LIST RESDIR"/tts_white.list"
static void _tts_cb(void *data, Evas_Object *obj, void *event_info)
{
	int val = -1;

	_W("Change TTS");

	if (0 == vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &val)
			&& main_info.is_tts != val)
	{
		Evas_Object *scroller = NULL;
		Eina_List *page_info_list = NULL;
		page_info_s *page_info = NULL;
		layout_info_s *layout_info = NULL;
		scroller_info_s *scroller_info = NULL;
		Eina_List *white_list = NULL;
		Eina_List *black_list = NULL;
		const Eina_List *l, *ln;

		main_info.is_tts = val;
		apps_main_list_tts(val);

		layout_info = evas_object_data_get(main_info.layout, DATA_KEY_LAYOUT_INFO);
		ret_if(!layout_info);

		if (evas_object_data_get(main_info.layout, DATA_KEY_ADD_VIEWER) != NULL) {
			_D("destroy a addviewer");
			edit_destroy_add_viewer(main_info.layout);
		}

		if (layout_info->edit != NULL) {
			_D("destroy a editing layout");
			edit_destroy_layout(main_info.layout);
		}

		scroller = evas_object_data_get(main_info.layout, DATA_KEY_SCROLLER);
		ret_if(!scroller);

		scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
		ret_if(!scroller_info);

		page_info_list = db_write_list();

		if (main_info.is_tts) {
			_W("TTS is on");

			white_list = _create_tts_list(WHITE_LIST);
			ret_if(!white_list);

			black_list = _create_tts_list(BLACK_LIST);
			ret_if(!black_list);

			EINA_LIST_FOREACH_SAFE(page_info_list, l, ln, page_info) {
				if(_page_info_in_list(page_info, white_list)) continue;
				if(_page_info_in_list(page_info, black_list)) {
					page_info_list = eina_list_remove(page_info_list, page_info);
					continue;
				}
			}

			_destroy_tts_list(white_list);
			_destroy_tts_list(black_list);

			if (W_HOME_ERROR_NONE != db_read_list(page_info_list)) {
				_E("Cannot write items into the DB");
			}

			/* If TTS is on, use focus & do not use the enhanced scroller. */
			elm_object_tree_focus_allow_set(main_info.layout, EINA_TRUE);
			_D("tree_focus_allow_set layout(%p) as TRUE", main_info.layout);

			page_focus(scroller_info->center);
			scroller_backup_inner_focus(scroller);
			scroller_restore_inner_focus(scroller);
		} else {
			_W("TTS is off");

			/* If TTS is off, do not use focus */
			elm_object_tree_focus_allow_set(main_info.layout, EINA_FALSE);
			_D("tree_focus_allow_set layout(%p) as FALSE", main_info.layout);
		}

		scroller_pop_pages(scroller, PAGE_DIRECTION_RIGHT);
		scroller_push_pages(scroller, page_info_list, _del_list, page_info_list);
	}

	if (apps_main_is_visible() == EINA_TRUE) {
		apps_main_launch(APPS_LAUNCH_HIDE);
	}

	/* UX requirement: activate home window when TTS option is changed */
	ecore_job_add(_activate_window_job_cb, NULL);
}



static void _tutorial_cb(keynode_t *node, void *data)
{
	int tutorial_enabled = _is_tutorial_enabled();

	if (tutorial_enabled == 0) {
		if (main_info.state == APP_STATE_RESUME) {
			_home_visibility_vconf_set(1);
		} else {
			_home_visibility_vconf_set(0);
		}

		popup_show(POPUP_STAGE_AFTER_TUTORIAL);
	}
}



static void _init_theme(void)
{
	main_info.theme = elm_theme_new();
	elm_theme_ref_set(main_info.theme, NULL);
	elm_theme_extension_add(main_info.theme, EDJEDIR"/index.edj");

	/* Default Changeable UI State : off */
	ea_theme_changeable_ui_enabled_set(EINA_FALSE);

	if (!main_info.color_theme) {
		main_info.color_theme = ea_theme_color_table_new(PREFIX"/shared/res/tables/org.tizen.w-home_ChangeableColorInfo.xml");
		ret_if (!main_info.color_theme);
	}
	/* Default Theme Style : DEFAULT(B3 is Dark, Kiran is Light) */
	ea_theme_style_set(EA_THEME_STYLE_DEFAULT);
	ea_theme_colors_set(main_info.color_theme, EA_THEME_STYLE_DEFAULT);

	if (!main_info.font_theme) {
		main_info.font_theme = ea_theme_font_table_new(PREFIX"/shared/res/tables/org.tizen.w-home_ChangeableFontInfo.xml");
		ret_if(!main_info.font_theme);
	}
	ea_theme_fonts_set(main_info.font_theme);

	ea_theme_system_colors_apply();
	ea_theme_system_fonts_apply();
}



static void _destroy_theme(void)
{
	elm_theme_extension_del(main_info.theme, EDJEDIR"/index.edj");
	elm_theme_free(main_info.theme);
	main_info.theme = NULL;

	if (main_info.color_theme) {
		ea_theme_color_table_free(main_info.color_theme);
		main_info.color_theme = NULL;
	}
	if (main_info.font_theme) {
		ea_theme_font_table_free(main_info.font_theme);
		main_info.font_theme = NULL;
	}
}



static Eina_Bool _appcore_flush_cb(void *data)
{
	if (APP_STATE_PAUSE != main_info.state) return ECORE_CALLBACK_CANCEL;
	if (0 != appcore_flush_memory()) _E("Cannot flush memory");
	return ECORE_CALLBACK_CANCEL;
}



HAPI void main_inc_booting_state(void)
{
	main_info.booting_state++;
	if (BOOTING_STATE_DONE > main_info.booting_state) return;

	w_home_error_e ret = W_HOME_ERROR_NONE;
	do {
		ret = pkgmgr_reserve_list_pop_request();
	} while (W_HOME_ERROR_NONE == ret);

	/*  Cache memory is cleared when the application paused (every time, after 5 seconds (in appcore)),
	 *  but after running in a minimized mode (HIDE_LAUNCH) application have status AS_RUNNING.
	 *  Application have status AS_PAUSED only after change of visibility to hidden condition by user (on hiding window)
	 *  Cleaning must be performed only once after application loading in hidden condition
	 *  (and stay in a hidden condition at time of cleaning).
	 */
	if (APP_STATE_PAUSE == main_info.state) {
		ecore_timer_add(5.0f, _appcore_flush_cb, NULL);
	}
}



HAPI void main_dec_booting_state(void)
{
    main_info.booting_state--;
}



HAPI int main_get_booting_state(void)
{
	return main_info.booting_state;
}



HAPI w_home_error_e main_register_cb(
		int state,
		w_home_error_e (*result_cb)(void *), void *result_data)
{
	main_cb_s *cb = NULL;

	retv_if(NULL == result_cb, W_HOME_ERROR_INVALID_PARAMETER);

	cb = calloc(1, sizeof(main_cb_s));
	retv_if(NULL == cb, W_HOME_ERROR_FAIL);

	cb->result_cb = result_cb;
	cb->result_data = result_data;

	main_info.cbs_list[state] = eina_list_append(main_info.cbs_list[state], cb);
	retv_if(NULL == main_info.cbs_list[state], W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}



HAPI void main_unregister_cb(
		int state,
		w_home_error_e (*result_cb)(void *))
{
	const Eina_List *l;
	const Eina_List *n;
	main_cb_s *cb;
	EINA_LIST_FOREACH_SAFE(main_info.cbs_list[state], l, n, cb) {
		continue_if(NULL == cb);
		if (result_cb != cb->result_cb) continue;
		main_info.cbs_list[state] = eina_list_remove(main_info.cbs_list[state], cb);
		free(cb);
		return;
	}
}



static void _execute_cbs(int state)
{
	const Eina_List *l;
	const Eina_List *n;
	main_cb_s *cb;
	EINA_LIST_FOREACH_SAFE(main_info.cbs_list[state], l, n, cb) {
		continue_if(NULL == cb);
		continue_if(NULL == cb->result_cb);

		if (W_HOME_ERROR_NONE != cb->result_cb(cb->result_data)) _E("There are some problems");
	}
}



static int _dead_cb(int pid, void *data)
{
	_D("PID(%d) is dead", pid);
	/* Who manages the idle clock? home_item_idle_clock_app_dead_cb */
	clock_service_event_app_dead_cb(pid);

	return 1;
}



#if 0 /* EFL private features */
static Eina_Bool _window_effect_setting_cb(void *data)
{
	gesture_home_window_effect_set(1);

	return ECORE_CALLBACK_CANCEL;
}
#endif



static void _resume_cb(void *data)
{
	_D("Resumed");

	if (main_info.state == APP_STATE_RESUME) {
		_E("resumed already");
		return;
	}

#if 0 /* EFL private features */
	ecore_timer_add(0.2f, _window_effect_setting_cb, NULL);
#endif

	if (apps_main_is_visible() && main_info.is_win_visible == 0) {
		_E("Apps is showing");
		return;
	}

	if (_is_tutorial_enabled() == 0) {
		_home_visibility_vconf_set(1);
	}

	main_info.state = APP_STATE_RESUME;
	_execute_cbs(APP_STATE_RESUME);

	if (main_info.is_alpm_clock_enabled == 0) {
		if (_is_lcd_turned_on() == 1) {
			_W("clock/widget resumed");
			evas_object_dynamicbox_resumed();
			clock_service_resume();
		}
	}
}



static void _pause_cb(void *data)
{
	_D("Paused");

	if (main_info.state == APP_STATE_PAUSE) {
		_E("paused already");
		return;
	}

	_home_visibility_vconf_set(0);

	main_info.state = APP_STATE_PAUSE;
	_execute_cbs(APP_STATE_PAUSE);

	/* DYNAMICBOX pause */
	_W("clock/widget paused");
	evas_object_dynamicbox_paused();
	clock_service_pause();
}



static void _setup_wizard_cb(keynode_t *node, void *data)
{
	Eina_List *page_info_list = NULL;
	layout_info_s *layout_info = NULL;
	const char *file = NULL;
	int setup_wizard = 1;

	if (vconf_get_int(VCONFKEY_SETUP_WIZARD_STATE, &setup_wizard) < 0) {
		_E("Failed to get VCONFKEY_SETUP_WIZARD_STATE");
	}

	if (setup_wizard) {
		_W("Setup wizard is still on");
		return;
	}

	/* If setup_wizard is off, please ignore this callback */
	vconf_ignore_key_changed(VCONFKEY_SETUP_WIZARD_STATE, _setup_wizard_cb);

	/* Below routines is same with _dbox_load_init_timer_cb. */
	if (main_info.is_tts) {
		_W("TTS is on");
		file = TTS_XML_FILE;
	} else {
		_W("TTS is off");
		file = DEFAULT_XML_FILE;
	}

	if (file && 0 == access(file, R_OK)) {
		_W("We'll read the file(%s)", file);
	} else {
		_E("Cannot access the file(%s)", file);
		file = DEFAULT_XML_FILE;
	}

	page_info_list = xml_write_list(file);
	if (!page_info_list) {
		_E("Critical! Cannot read xml file(%s)", file);
		return;
	}

	if (W_HOME_ERROR_NONE != db_read_list(page_info_list)) {
		_E("Cannot write items into the DB");
	}

	layout_info = evas_object_data_get(main_info.layout, DATA_KEY_LAYOUT_INFO);
	if (layout_info) {
		scroller_push_pages(layout_info->scroller, page_info_list, _del_list, page_info_list);
	}
}



static Eina_Bool _dbox_load_init_timer_cb(void *data)
{
	const char *file = NULL;
	Eina_List *page_info_list = NULL;
	layout_info_s *layout_info = NULL;

	_W("Loads DBoxes");

	if (emergency_mode_enabled_get()) {
		_W("Emergency mode, we don't need to add dboxes.");
		return ECORE_CALLBACK_CANCEL;
	}

	/* Order is very important. Do not change if-statments */
	if (main_info.first_boot) {
		int setup_wizard = 0;

		main_info.first_boot = 0;

		if (vconf_get_int(VCONFKEY_SETUP_WIZARD_STATE, &setup_wizard) < 0) {
			_E("Failed to get VCONFKEY_SETUP_WIZARD_STATE");
		}

		if (setup_wizard) {
			_W("Setup wizard is still on");
			if (vconf_notify_key_changed(VCONFKEY_SETUP_WIZARD_STATE, _setup_wizard_cb, NULL) < 0) {
				_E("Failed to register the setup_wizard callback");
			}
			return ECORE_CALLBACK_CANCEL;
		}

		/* Below routines is same with _setup_wizard_cb */
		if (main_info.is_tts) {
			_W("TTS is on");
			file = TTS_XML_FILE;
		} else {
			_W("TTS is off");
			file = DEFAULT_XML_FILE;
		}

		if (file && 0 == access(file, R_OK)) {
			_W("We'll read the file(%s)", file);
		} else {
			_E("Cannot access the file(%s)", file);
			file = DEFAULT_XML_FILE;
		}

		page_info_list = xml_write_list(file);
		if (!page_info_list) {
			_E("Critical! Cannot read xml file(%s)", file);
			return ECORE_CALLBACK_CANCEL;
		}

		if (W_HOME_ERROR_NONE != db_read_list(page_info_list)) {
			_E("Cannot write items into the DB");
		}
	} else {
		_W("Read DB for initial list");
		page_info_list = db_write_list();
	}

	layout_info = evas_object_data_get(main_info.layout, DATA_KEY_LAYOUT_INFO);
	if (layout_info) {
		scroller_push_pages(layout_info->scroller, page_info_list, _del_list, page_info_list);
	}

	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _noti_broker_init_timer_cb(void *data)
{
	_W("loading noti broker");
	noti_broker_init();

	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _clock_service_init_timer_cb(void *data)
{
	_W("clock service init");
	clock_service_init();
	minicontrol_init();
	power_mode_ui_init();

	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _visibility_timeout_cb(void *data)
{
	_E("Visibility event is not reached in %lf seconds", VISIBILITY_TIMEOUT);

	effect_init();
	_clock_service_init_timer_cb(data);
	_dbox_load_init_timer_cb(NULL);

	main_info.safety_init_timer = NULL;
	main_info.is_service_initialized = 1;
	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _window_visibility_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Visibility_Change *ev = event;
	retv_if(ev == NULL, ECORE_CALLBACK_PASS_ON);

	if (!main_info.win) {
		return ECORE_CALLBACK_PASS_ON;
	}

	Ecore_X_Window xWin = elm_win_xwindow_get(main_info.win);

	if (xWin != ev->win) {
		return ECORE_CALLBACK_PASS_ON;
	}

	_W("Window [0x%X] is now visible(%d)", xWin, ev->fully_obscured);

	if (ev->fully_obscured == 1) {
		main_info.is_win_visible = 0;
	} else {
		main_info.is_win_visible = 1;
	}

	if (main_info.is_service_initialized == 0) {
		if (main_info.safety_init_timer) {
			/**
			 * Delete the safety timer,
			 * We are successfully get the visibility change event.
			 * So we don't need to keep the safety timer anymore.
			 * Delete it.
			 */
			ecore_timer_del(main_info.safety_init_timer);
			main_info.safety_init_timer = NULL;
		}

		_clock_service_init_timer_cb(NULL);

		if (!ecore_timer_add(LAZY_NOTI_TIMER, _noti_broker_init_timer_cb, NULL)) {
			_E("Failed to create a new timer for noti-broker");
			_noti_broker_init_timer_cb(NULL);
		}

		if (!ecore_timer_add(LAZY_DBOX_TIMER, _dbox_load_init_timer_cb, NULL)) {
			_E("Failed to create a new timer for dbox initializer");
			_dbox_load_init_timer_cb(NULL);
		}

		effect_init();

		main_info.is_service_initialized = 1;
	}

	return ECORE_CALLBACK_PASS_ON;
}



static Eina_Bool _window_focus_in_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Focus_In *ev = event;

	if (!main_info.win) {
		return ECORE_CALLBACK_PASS_ON;
	}

	Ecore_X_Window xWin = elm_win_xwindow_get(main_info.win);
	if (xWin == ev->win) {
		_D("focus in");
		_resume_cb(NULL);
	} else {
		_E("win[%d], ev->win[%d]", xWin, ev->win);
	}

	return ECORE_CALLBACK_PASS_ON;
}



static Eina_Bool _window_focus_out_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Focus_Out *ev = event;

	if (!main_info.win) {
		return ECORE_CALLBACK_PASS_ON;
	}

	Ecore_X_Window xWin = elm_win_xwindow_get(main_info.win);
	if (xWin == ev->win) {
		_D("focus out");
		_pause_cb(NULL);

		if (tutorial_is_exist()) {
			Ecore_X_Window xwin = ecore_x_window_focus_get();
			if (tutorial_is_apps(xwin)) {
				_D("tutorial shows apps");
				tutorial_set_transient_for(xwin);
			} else if (tutorial_is_indicator(xwin)) {
				_D("tutorial shows indicator");
				tutorial_set_transient_for(xwin);
			} else {
				_E("Cannot reach here");
			}
		}
	} else {
		_E("win[%d], ev->win[%d]", xWin, ev->win);
	}

	return ECORE_CALLBACK_PASS_ON;
}



static void _change_apps_order_cb(keynode_t *node, void *data)
{
	int value = -1;

	/* check Emergency Mode */
	if (emergency_mode_enabled_get()) {
		_E("emergency mode enabled");
		return;
	}

	// 0 : init, 1 : backup request, 2 : restore request, 3: write done
	if (vconf_get_int(VCONF_KEY_WMS_APPS_ORDER,  &value) < 0) {
		_E("Failed to get VCONFKEY_WMS_APPS_ORDER");
		return;
	}

	_D("Change apps order vconf:[%d]", value);
	wms_change_apps_order(value);
}



static void _alpm_manager_status_changed_cb(void *user_data, void *event_info)
{
	char *status = event_info;
	ret_if(status == NULL);

	_W("alpm status:%s", status);

	main_s *main_info = main_get_info();
	ret_if(main_info == NULL);

	if (strcmp(status, ALPM_MANAGER_STATUS_SHOW) == 0) {
		main_info->is_alpm_clock_enabled = 1;

	} else if (strcmp(status, ALPM_MANAGER_STATUS_ALPM_HIDE) == 0) {
		_D("is win visible?:%d", main_info->is_win_visible);
		main_info->is_alpm_clock_enabled = 0;
		if (main_info->state == APP_STATE_RESUME || main_info->is_win_visible == 1) {
			if (_is_lcd_turned_on() == 1) {
				_W("clock/widget resumed");
				evas_object_dynamicbox_resumed();
				clock_service_resume();
			}
		}
	} else if (strcmp(status, ALPM_MANAGER_STATUS_SIMPLE_HIDE) == 0)  {
		main_info->is_alpm_clock_enabled = 0;
		evas_object_dynamicbox_resumed();
		clock_service_resume();
	}
}


static void _lcd_on_cb(void *user_data, void *event_info)
{
	main_s *main_info = main_get_info();
	ret_if(main_info == NULL);

	_W("LCD: off->on");
	main_info->is_lcd_on = 1;
	if (main_info->state == APP_STATE_RESUME) {
		if (main_info->is_alpm_clock_enabled == 0) {
			_W("clock/widget resumed");
			evas_object_dynamicbox_resumed();
			clock_service_resume();
		}
	}
}

static void _lcd_off_cb(void *user_data, void *event_info)
{
	main_s *main_info = main_get_info();
	ret_if(main_info == NULL);

	_D("LCD: on->off");
	main_info->is_lcd_on = 0;
	if (main_info->state == APP_STATE_RESUME) {
		_W("clock/widget paused");
		evas_object_dynamicbox_paused();
		clock_service_pause();
	}
}



#define LANGUAGE_MALI "ml_IN"
#define LANGUAGE_GEORGIA "ka_GE"
static void _check_lang(void)
{
	char *lang = NULL;

	lang = vconf_get_str(VCONFKEY_LANGSET);
	if (!lang) {
		return;
	}
	elm_language_set(lang);

	if (!strncmp(lang, LANGUAGE_MALI, strlen(LANGUAGE_MALI))) {
		main_info.is_wide_character = TUTORIAL_LANGUAGE_MALI;
	} else if (!strncmp(lang, LANGUAGE_GEORGIA, strlen(LANGUAGE_GEORGIA))) {
		main_info.is_wide_character = TUTORIAL_LANGUAGE_GEORGIA;
	} else {
		main_info.is_wide_character = TUTORIAL_LANGUAGE_NORMAL;
	}

	free(lang);
}



#define TUTORIAL_TIMER 1.5f
static bool _create_cb(void *data)
{
	Evas_Object *bg = NULL;
	int tutorial_enabled = 0;
	Ecore_Event_Handler *handler;
	char *pkgname;

	_D("Created");
	home_dbus_init(NULL);
	power_mode_init();
	db_init(DB_FILE_NORMAL);

	main_info.state = APP_STATE_CREATE;
	evas_object_add_viewer_init();

	if (vconf_notify_key_changed(VCONF_KEY_WMS_APPS_ORDER, _change_apps_order_cb, NULL) < 0) {
		_E("Failed to register the changed_apps_order callback");
	}
	/* wms vconf has to be dealt after pushing pages */

	if (vconf_notify_key_changed(VCONF_KEY_HOME_IS_TUTORIAL, _tutorial_cb, NULL) < 0) {
		_E("Failed to register the tutorial callback");
	}

	retv_if(vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &main_info.is_tts) < 0, W_HOME_ERROR_FAIL);
	_check_lang();

	wms_register_setup_wizard_vconf();

#if 1 //TBD
	home_gesture_init();
	if (home_dbus_register_cb(DBUS_EVENT_LCD_ON, _lcd_on_cb, NULL) != W_HOME_ERROR_NONE) {
		_E("Failed to register lcd status changed cb");
	}
	if (home_dbus_register_cb(DBUS_EVENT_LCD_OFF, _lcd_off_cb, NULL) != W_HOME_ERROR_NONE) {
		_E("Failed to register lcd status changed cb");
	}
#else
	if (vconf_notify_key_changed(VCONFKEY_PM_STATE, _pm_state_cb, NULL) < 0) {
		_E("Failed to register the pm status callback");
	}
#endif
	if (home_dbus_register_cb(DBUS_EVENT_ALPM_MANAGER_STATE_CHANGED, _alpm_manager_status_changed_cb, NULL) != W_HOME_ERROR_NONE) {
		_E("Failed to register alpm manager status changed cb");
	}

	/**
	 * Register the X Event handlers before creating our Window.
	 * If you don't want to be bothered by timing issue of these **cking events ;)
	 */
	handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_IN, _window_focus_in_cb, NULL);
	if (!handler) {
		_E("Failed to add an ecore event handler (FOCUS_IN)");
	}
	evas_object_data_set(main_info.win, PRIVATE_DATA_KEY_FOCUS_IN_EVENT_HANDLER, handler);

	handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, _window_focus_out_cb, NULL);
	if (!handler) {
		_E("Failed to add an ecore event handler (FOCUS_OUT)");
	}
	evas_object_data_set(main_info.win, PRIVATE_DATA_KEY_FOCUS_OUT_EVENT_HANDLER, handler);

	handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE, _window_visibility_cb, NULL);
	if (!handler) {
		_E("Failed to add an ecore event handler (VISIBILITY_CHANGE)");
	}
	evas_object_data_set(main_info.win, PRIVATE_DATA_KEY_VISIBILITY_EVENT_HANDLER, handler);

	main_info.win = win_create("W-Home");
	retv_if(!main_info.win, false);
	evas_object_smart_callback_add(main_info.win, "access,changed", _tts_cb, NULL);

	/* DYNAMICBOX init */
	dbox_init(main_info.win);

	/* Key register */
	key_register();

	bg = bg_create(main_info.win);
	if (!bg) {
		_E("Cannot create bg");
		win_destroy(main_info.win);
		return false;
	}

	main_info.layout = layout_create(main_info.win);
	if (!main_info.layout) {
		_E("Cannot create layout");
		bg_destroy(main_info.win);
		win_destroy(main_info.win);
		return false;
	}

	if (main_info.is_tts) {
		/* If TTS is on, use focus & do not use the enhanced scroller. */
		_D("Set tree focus as true");
		elm_object_tree_focus_allow_set(main_info.layout, EINA_TRUE);
	} else {
		/* If TTS is off, do not use focus */
		_D("Set tree focus as false");
		elm_object_tree_focus_allow_set(main_info.layout, EINA_FALSE);
	}

	_execute_cbs(APP_STATE_CREATE);

	/**
	 * After preparation is done,
	 * Create a dbox clock if it is prepared.
	 */
	pkgname = clock_service_clock_pkgname_get();
	if (clock_inf_prepare_first_clock(pkgname) != CLOCK_RET_OK) {
		/* What happens */
	}
	free(pkgname);

	/* After creating a window */
	_init_theme();

	clock_service_event_register();
	/**
	 * This function initialize the noti-broker.
	 * But we already manage it from this file.
	 * and the _load function uses timer to initialize the noti.
	 * It uses 10 secs.....
	 */
	noti_broker_load();

	/*
	 * workaround: when window visibiltiy cb isn't called
	 */
	main_info.safety_init_timer = ecore_timer_add(VISIBILITY_TIMEOUT, _visibility_timeout_cb, NULL);
	if (!main_info.safety_init_timer) {
		_E("Failed to create a fallback init timer for safety");
	}

	if (vconf_get_int(VCONF_KEY_HOME_IS_TUTORIAL_ENABLED_TO_RUN, &tutorial_enabled) != 0) {
		_E("Cannot get the vconf for %s", VCONF_KEY_HOME_IS_TUTORIAL_ENABLED_TO_RUN);
	}

	if (tutorial_enabled && util_feature_enabled_get(FEATURE_TUTORIAL) == 1) {
		if (!tutorial_create(main_info.layout)) {
			_E("Cannot create a tutorial");
		}
	}

	/**
	 * Dead callback should be called after initialize all services.
	 * So we should register this at the last of this function.
	 */
	aul_listen_app_dead_signal(_dead_cb, NULL);

	return true;
}



static void _terminate_cb(void *data)
{
	layout_info_s *layout_info = NULL;

	main_dec_booting_state();
	evas_object_add_viewer_fini();

	_D("Terminated");

	home_dbus_fini(NULL);

	apps_main_fini();


	home_dbus_unregister_cb(DBUS_EVENT_LCD_ON, _lcd_on_cb);
	home_dbus_unregister_cb(DBUS_EVENT_LCD_OFF, _lcd_off_cb);
	home_dbus_unregister_cb(DBUS_EVENT_ALPM_MANAGER_STATE_CHANGED, _alpm_manager_status_changed_cb);

	Ecore_Event_Handler *handler = evas_object_data_del(main_info.win, PRIVATE_DATA_KEY_FOCUS_IN_EVENT_HANDLER);
	if (handler) ecore_event_handler_del(handler);

	handler = evas_object_data_del(main_info.win, PRIVATE_DATA_KEY_FOCUS_OUT_EVENT_HANDLER);
	if (handler) ecore_event_handler_del(handler);

	handler = evas_object_data_del(main_info.win, PRIVATE_DATA_KEY_VISIBILITY_EVENT_HANDLER);
	if (handler) ecore_event_handler_del(handler);

	main_info.state = APP_STATE_TERMINATE;
	_execute_cbs(APP_STATE_TERMINATE);

	wms_unregister_setup_wizard_vconf();

	layout_info = evas_object_data_get(main_info.layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	if (layout_info->tutorial) {
		tutorial_destroy(layout_info->tutorial);
		layout_info->tutorial = NULL;
	}

	_destroy_theme();

	vconf_ignore_key_changed(VCONF_KEY_WMS_APPS_ORDER, _change_apps_order_cb);
	vconf_ignore_key_changed(VCONF_KEY_HOME_IS_TUTORIAL, _tutorial_cb);

	effect_fini();

	clock_service_event_deregister();

	/* Clock Service fini */
	clock_service_fini();

	/* Minicontrol fini */
	minicontrol_fini();

	noti_broker_fini();

	/* Emergency Mode fini */
	power_mode_fini();

	/*!
	 * DBUS fini
	 * Gesture fini
	 * calling sequence is meaningful
	*/
	home_gesture_fini();

	bg_destroy(main_info.win);

	/* Key unregister */
	key_unregister();

	win_destroy(main_info.win);

	/* DYNAMICBOX fini */
	dbox_fini();

	if (main_info.apps_pid) {
		aul_terminate_pid(main_info.apps_pid);
		main_info.apps_pid = 0;
	}
}



static Eina_Bool _show_noti_timer_cb(void *data)
{
	Evas_Object *scroller = data;
	retv_if(scroller == NULL, ECORE_CALLBACK_CANCEL);

	scroller_bring_in_by_push_type(scroller, SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_LEFT, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_ANIMATOR);

	return ECORE_CALLBACK_CANCEL;
}



static void _activate_window_job_cb(void *data)
{
	ret_if(!main_info.win);

	_D("Activate the window");
	elm_win_activate(main_info.win);
	home_dbus_home_raise_signal_send();

	_resume_cb(NULL);
}



static Evas_Object *_layout_get(void)
{
	Evas_Object *layout = NULL;

	if (!main_info.win) {
		return NULL;
	}

	layout = evas_object_data_get(main_info.win, DATA_KEY_LAYOUT);

	return layout;
}



static Evas_Object *_scroller_get(void)
{
	Evas_Object *layout = _layout_get();
	retv_if(layout == NULL, NULL);

	return elm_object_part_content_get(layout, "scroller");
}



static void _app_control(app_control_h service, void *data)
{
	/* powerkey bundle */
	char *service_val = NULL;

	apps_main_init();

	app_control_get_extra_data(service, HOME_SERVICE_KEY, &service_val);
	_D("Service value : %s", service_val);

	if (service_val) {
		int is_clock_displayed = 0;
		int is_window_on_top = _is_window_on_top();
		is_window_on_top = (apps_main_is_visible() == EINA_TRUE) ? 0 : is_window_on_top;
		Evas_Object *layout = _layout_get();
		Evas_Object *scroller = _scroller_get();
		if (scroller_get_current_page_direction(scroller) == PAGE_DIRECTION_CENTER) {
			is_clock_displayed = 1;
		}

		if (!strncmp(service_val, HOME_SERVICE_VALUE_POWERKEY, strlen(HOME_SERVICE_VALUE_POWERKEY))) {
			int tutorial_exist = tutorial_is_exist();

			_D("Powerkey operation");

			if(tutorial_exist) {
				is_window_on_top = 1;
			} else {
				apps_main_launch(APPS_LAUNCH_HIDE);
			}

			if (is_clock_displayed == 1) {
				_D("clock is displayed");
				if (is_window_on_top == 1) {
					_D("home window is top");
					if (_is_lcd_turned_on() == 1) {
						if (layout_is_idle(layout) == 1) {
							home_dbus_lcd_off_signal_send();
						} else {
							_W("home isn't in idle");
						}
					}
				}
			} else if (!tutorial_exist) {
				scroller_bring_in_by_push_type(scroller, SCROLLER_PUSH_TYPE_CENTER, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_ANIMATOR);
			}

			key_cb_execute(KEY_TYPE_HOME);
		} else if (!strncmp(service_val, HOME_SERVICE_VALUE_EDIT, strlen(HOME_SERVICE_VALUE_EDIT))) {
			_D("Edit operation");
		} else if (!strncmp(service_val, HOME_SERVICE_VALUE_SHOW_APPS, strlen(HOME_SERVICE_VALUE_SHOW_APPS))) {
			if (util_feature_enabled_get(FEATURE_APPS) == 1) {
				apps_main_launch(APPS_LAUNCH_SHOW);
				_D("Show apps operation");
			}
		} else if (!strncmp(service_val, HOME_SERVICE_VALUE_APPS_EDIT, strlen(HOME_SERVICE_VALUE_APPS_EDIT))) {
			_D("Apps edit operation");
			apps_main_launch(APPS_LAUNCH_EDIT);
			is_window_on_top = 1;

		} else if (!strncmp(service_val, HOME_SERVICE_VALUE_SHOW_NOTI, strlen(HOME_SERVICE_VALUE_SHOW_NOTI))) {
			_D("Show noti operation");
			ecore_timer_add(0.250f, _show_noti_timer_cb, scroller);
		} else if (!strncmp(service_val, HOME_SERVICE_VALUE_FIRST_BOOT, strlen(HOME_SERVICE_VALUE_FIRST_BOOT))) {
			_D("First boot operation");
			main_info.first_boot = 1;
		}

		if (is_window_on_top == 0) {
			ecore_job_add(_activate_window_job_cb, NULL);
		}

		free(service_val);
	} else {
		ecore_job_add(_activate_window_job_cb, NULL);
		_execute_cbs(APP_STATE_RESET);
	}
}



static void _language_changed(void *data)
{
	_D("");
	apps_main_language_chnage();
	evas_object_add_viewer_reload();

	_check_lang();
	_execute_cbs(APP_STATE_LANGUAGE_CHANGED);
}




int main(int argc, char *argv[])
{
	int ret;
	app_event_callback_s event_callback;
	char *clock_pkgname;

	if (CRITICAL_LOG_INIT(util_basename(argv[0])) < 0) {
		_E("Failed to initiate the critical log system");
	}

	event_callback.create = _create_cb;
	event_callback.terminate = _terminate_cb;
	event_callback.pause = _pause_cb;
	event_callback.resume = _resume_cb;
	event_callback.app_control = _app_control;
	event_callback.low_memory = NULL;
	event_callback.low_battery = NULL;
	event_callback.device_orientation = NULL;
	event_callback.language_changed = _language_changed;
	event_callback.region_format_changed = NULL;

	if (setenv("ELM_ENGINE", "gl", 1) < 0) {
		_E("setenv(ELM_ENGINE) is failed: %s", strerror(errno));
	}

	if (setenv("COREGL_FASTPATH", "1", 1) < 0) {
		_E("setenv(COREGL_FASTPATH) is failed: %s", strerror(errno));
	}

	if (setenv("BUFMGR_LOCK_TYPE", "always", 0) < 0) {
		_E("setenv(BUFMGR_LOCK_TYPE) is failed: %s", strerror(errno));
	}

	if (setenv("BUFMGR_MAP_CACHE", "true", 0) < 0) {
		_E("setenv(BUFMGR_MAP_CACHE) is failed: %s", strerror(errno));
	}

	/* initial mode set */
	if (emergency_mode_enabled_get() == 1) {
		clock_service_mode_set(CLOCK_SERVICE_MODE_EMERGENCY);
	}
	if (cooldown_mode_enabled_get() == 1) {
		clock_service_mode_set(CLOCK_SERVICE_MODE_COOLDOWN);
	}

	/* Launch the clock */
	if ((clock_pkgname = clock_service_clock_pkgname_get()) != NULL) {
		clock_util_provider_launch(clock_pkgname, NULL, CLOCK_CONF_NONE);
		free(clock_pkgname);
	}

	ret = app_efl_main(&argc, &argv, &event_callback, &main_info);
	CRITICAL_LOG_FINI();
	return ret;
}



// End of a file
