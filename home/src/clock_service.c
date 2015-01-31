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
#include <stdbool.h>
#include <stdio.h>
#include <aul.h>
#include <vconf.h>
#include <efl_assist.h>
#include <dlog.h>

#include "conf.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "layout.h"
#include "page_info.h"
#include "scroller_info.h"
#include "scroller.h"
#include "page.h"
#include "dbus.h"
#include "key.h"
#include "clock_service.h"
#include "power_mode.h"
#include "edit.h"

#define ERROR_RETRY_COUNT 20
#define ERROR_ACTION_NEED_TO_RETRY 1
#define ERROR_ACTION_NEED_TO_CHANGE_PKGNAME_AND_RETRY 2
#define ERROR_ACTION_NEED_TO_RESET_CLOCK_VCONF 3
#define ERROR_ACTION_NEED_TO_KEEP_CURRENT_STATUS 4

#define CLOCK_SERVICE_STATE_NOT_INITIALIZED 0
#define CLOCK_SERVICE_STATE_READY 1
#define CLOCK_SERVICE_STATE_WAITING 2

#define CLOCK_VIEW_ACTIVATE_WAIT_TIME 0.30
#define CLOCK_VIEW_CONF_RESET_TIME 0.3
#define CLOCK_PUMP_WAIT_TIME 1.0
#define CLOCK_VIEW_WAITING_TIME 5.0

static struct _s_info {
	int mode;
	int state;
	int is_pump_ignored;
	int is_scroller_screezed;

	int error_count;
	int error_pid;
	char *error_pkgname;

	Ecore_Timer *waiting_timer;
	Ecore_Timer *pump_timer;
	Ecore_Timer *activate_timer;
	Ecore_Timer *conf_reset_timer;
} s_info = {
	.mode = CLOCK_SERVICE_MODE_NORMAL,
	.state = CLOCK_SERVICE_STATE_NOT_INITIALIZED,
	.is_pump_ignored = 0,
	.is_scroller_screezed = 0,

	.error_count = 0,
	.error_pkgname = NULL,

	.waiting_timer = NULL,
	.pump_timer = NULL,
	.activate_timer = NULL,
	.conf_reset_timer = NULL,
};

static void _clock_service_heartbeat_pump(void);

static const char *_get_state_str(int state)
{
	if (state == CLOCK_SERVICE_STATE_NOT_INITIALIZED) {
		return "state not initialized";
	}
	if (state == CLOCK_SERVICE_STATE_READY) {
		return "state ready";
	}
	if (state == CLOCK_SERVICE_STATE_WAITING) {
		return "state waiting";
	}

	return "invalid state";
}

static void _scroller_freeze(int is_freeze)
{
	Evas_Object *win = main_get_info()->win;
	Evas_Object *layout = NULL;
	Evas_Object *scroller = NULL;
	ret_if(win == NULL);

	layout = evas_object_data_get(win, DATA_KEY_LAYOUT);
	ret_if(layout == NULL);
	scroller = elm_object_part_content_get(layout, "scroller");
	ret_if(scroller == NULL);

	if (is_freeze == 1) {
		s_info.is_scroller_screezed = 1;
		scroller_freeze(scroller);
		scroller_region_show_by_push_type(scroller, SCROLLER_PUSH_TYPE_CENTER, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_ANIMATOR);
	} else if (is_freeze == 0) {
		s_info.is_scroller_screezed = 0;
		scroller_unfreeze(scroller);
	} else {
		_E("not a case");
	}
}

static Eina_Bool _conf_reset_timer_cb(void *data)
{
	s_info.conf_reset_timer = NULL;

	clock_util_setting_conf_set(CLOCK_CONF_NONE);

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _setting_conf_apply_timer_cb(void *data)
{
	int configure = (int)data;
	s_info.activate_timer = NULL;

	util_activate_home_window();
	if (configure == 2) {
		clock_service_event_handler(NULL, CLOCK_EVENT_SCROLLER_FREEZE_ON);
	}

	if (s_info.conf_reset_timer != NULL) {
		ecore_timer_del(s_info.conf_reset_timer);
		s_info.conf_reset_timer = NULL;
	}
	s_info.conf_reset_timer = ecore_timer_add(CLOCK_VIEW_CONF_RESET_TIME,
		_conf_reset_timer_cb, NULL);

	return ECORE_CALLBACK_CANCEL;
}

static void _setting_conf_apply(int configure)
{
	if (s_info.activate_timer != NULL) {
		ecore_timer_del(s_info.activate_timer);
		s_info.activate_timer = NULL;
	}

	if (configure > 0) {
		s_info.activate_timer = ecore_timer_add(CLOCK_VIEW_ACTIVATE_WAIT_TIME,
			_setting_conf_apply_timer_cb, (void*)configure);
	}
}

static int _state_get(void)
{
	return s_info.state;
}

static void _state_set(int state)
{
	_D("state %s --> %s ", _get_state_str(s_info.state), _get_state_str(state));
	s_info.state = state;
}

static void _clock_destroy(clock_h clock)
{
	ret_if(clock == NULL);

	_D("destroying clock:%s", clock->pkgname);
	//clock_manager_view_deattach(clock);
	clock_manager_view_destroy(clock);
	clock_del(clock);
}

static int _clock_error_action_get(char *pkgname)
{
	retv_if(pkgname == NULL, ERROR_ACTION_NEED_TO_KEEP_CURRENT_STATUS);

	if (s_info.error_pkgname != NULL) {
		if (strcmp(s_info.error_pkgname, pkgname) == 0) {
			s_info.error_count++;

			_E("error count:%d", s_info.error_count);
			if (s_info.error_count >= ERROR_RETRY_COUNT) {
				return ERROR_ACTION_NEED_TO_RESET_CLOCK_VCONF;
			} else {
				return ERROR_ACTION_NEED_TO_RETRY;
			}
		} else {
			return ERROR_ACTION_NEED_TO_CHANGE_PKGNAME_AND_RETRY;
		}
	}

	return ERROR_ACTION_NEED_TO_CHANGE_PKGNAME_AND_RETRY;
}

static void _clock_error_pkgname_set(char *pkgname)
{
	char *pkgname_clone = NULL;
	ret_if(pkgname == NULL);

	if (s_info.error_pkgname != NULL) {
		free(s_info.error_pkgname);
		s_info.error_pkgname = NULL;
	}

	if (pkgname != NULL) {
		pkgname_clone = strdup(pkgname);
		if (pkgname_clone != NULL) {
			s_info.error_pkgname = pkgname_clone;
		} else {
			_E("failed to strdup pkgname");
		}
	}
}

static void _clock_error_report(char *pkgname, int is_fatal_error)
{
	int treat = 0;
	ret_if(pkgname == NULL);

	if (is_fatal_error == 0) {
		treat = _clock_error_action_get(pkgname);
	} else {
		treat = ERROR_ACTION_NEED_TO_RESET_CLOCK_VCONF;
	}
	switch (treat) {
	case ERROR_ACTION_NEED_TO_RETRY:
		clock_service_request(CLOCK_SERVICE_MODE_NORMAL_RECOVERY);
		break;
	case ERROR_ACTION_NEED_TO_CHANGE_PKGNAME_AND_RETRY:
		_clock_error_pkgname_set(pkgname);
		s_info.error_count = 1;
		clock_service_request(CLOCK_SERVICE_MODE_NORMAL_RECOVERY);
		break;
	case ERROR_ACTION_NEED_TO_RESET_CLOCK_VCONF:
		clock_util_wms_configuration_set(CLOCK_APP_RECOVERY);
		clock_service_request(CLOCK_SERVICE_MODE_RECOVERY);
		s_info.error_count = 0;
		break;
	case ERROR_ACTION_NEED_TO_KEEP_CURRENT_STATUS:
		_E("");
		break;
	}

	_E("error:%s treat:%d", pkgname, treat);
}

static Eina_Bool _pump_timer_cb(void *data)
{
	_W("restart clock service, pump it again");
	s_info.pump_timer = NULL;

	_state_set(CLOCK_SERVICE_STATE_READY);
	_clock_service_heartbeat_pump();

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _waiting_timer_cb(void *data)
{
	_D("clock waiting timer is expired");
	s_info.waiting_timer = NULL;

	clock_h clock = clock_manager_clock_get(CLOCK_CANDIDATE);
	retv_if(clock == NULL, ECORE_CALLBACK_CANCEL);

	if (clock->state == CLOCK_STATE_WAITING) {
		_E("provider:%s didn't provide anything, we will ignore it", clock->pkgname);

		_setting_conf_apply(clock->configure);

		clock_manager_clock_set(CLOCK_CANDIDATE, NULL);
		_clock_destroy(clock);

		if (s_info.pump_timer != NULL) {
			ecore_timer_del(s_info.pump_timer);
			s_info.pump_timer = NULL;
		}
		s_info.pump_timer = ecore_timer_add(CLOCK_PUMP_WAIT_TIME, _pump_timer_cb, NULL);
	}

	return ECORE_CALLBACK_CANCEL;
}

static void _del_waiting_timer(void)
{
	_E("clock waiting timer is deleted");
	if (s_info.waiting_timer != NULL) {
		ecore_timer_del(s_info.waiting_timer);
		s_info.waiting_timer = NULL;
	}
}

static void _set_waiting_timer(void)
{
	_E("clock waiting timer is started");
	if (s_info.waiting_timer != NULL) {
		_del_waiting_timer();
	}

	s_info.waiting_timer = ecore_timer_add(CLOCK_VIEW_WAITING_TIME,
		_waiting_timer_cb, NULL);
}

static char *_clock_pkg_get(int mode)
{
	char *clock_pkgname = NULL;

	switch (mode) {
	case CLOCK_SERVICE_MODE_NORMAL:
	case CLOCK_SERVICE_MODE_NORMAL_RECOVERY:
		clock_pkgname = clock_util_wms_configuration_get();
		break;
	case CLOCK_SERVICE_MODE_EMERGENCY:
		clock_pkgname = strdup(CLOCK_APP_EMERGENCY);
		break;
	case CLOCK_SERVICE_MODE_COOLDOWN:
		clock_pkgname = strdup(CLOCK_APP_COOLDOWN);
		break;
	case CLOCK_SERVICE_MODE_RECOVERY:
		clock_pkgname = strdup(CLOCK_APP_RECOVERY);
		break;
	}

	return clock_pkgname;
}

static int _clock_provider_change(char *clock_pkgname, clock_h *clock_ret, int configure)
{
	clock_h clock = NULL;
	clock_h prev_clock = NULL;
	retv_if(clock_pkgname == NULL, CLOCK_RET_FAIL);
	retv_if(clock_ret == NULL, CLOCK_RET_FAIL);
	int ret;

	_E("clock will be changed to %s", clock_pkgname);

	clock = clock_new(clock_pkgname);
	retv_if(clock == NULL, CLOCK_RET_FAIL);

	clock->configure = configure;

	ret = clock_manager_view_prepare(clock);
	switch (ret & ~CLOCK_RET_NEED_DESTROY_PREVIOUS) {
	case CLOCK_RET_ASYNC:
		clock->async = 1;
		if ((ret & CLOCK_RET_NEED_DESTROY_PREVIOUS) == CLOCK_RET_NEED_DESTROY_PREVIOUS) {
			prev_clock = clock_manager_clock_get(CLOCK_ATTACHED);
			if (prev_clock != NULL) {
				Evas_Object *empty_view = clock_view_empty_add();
				if (empty_view != NULL) {
					clock_manager_view_exchange(prev_clock, empty_view);
				}
			}
		}
		break;
	case CLOCK_RET_OK:
		break;
	case CLOCK_RET_FAIL:
	default:
		goto ERR;
	}

	if (clock->async == 1) {
		*clock_ret = clock;
		return CLOCK_RET_OK;
	}

	if (clock_manager_view_create(clock) != CLOCK_RET_OK) {
		_E("Failed to create view");
		goto ERR;
	}

	prev_clock = clock_manager_clock_get(CLOCK_ATTACHED);
	if (clock_manager_view_attach(clock) != CLOCK_RET_OK) {
		_E("Failed to attach view");
		goto ERR;
	}

	clock->state = CLOCK_STATE_RUNNING;
	clock_manager_clock_set(CLOCK_ATTACHED, clock);
	_setting_conf_apply(clock->configure);

	*clock_ret = clock;

	/*
	 * sequence is meaningful
	 */
	if (prev_clock != NULL) {
		_D("destroying previous attached clock:%s", prev_clock->pkgname);
		_clock_destroy(prev_clock);
	}

	return CLOCK_RET_OK;

ERR:
	_E("Failed to change clock provider");

	clock_del(clock);

	return CLOCK_RET_FAIL;
}

static void _clock_service_heartbeat_pump(void)
{
	int ret = CLOCK_RET_OK;
	clock_h clock = NULL;
	clock_h prev_clock = NULL;
	clock_h candidate_clock = NULL;
	char *pkgname = _clock_pkg_get(s_info.mode);
	ret_if(pkgname == NULL);

	prev_clock = clock_manager_clock_get(CLOCK_ATTACHED);
	candidate_clock = clock_manager_clock_get(CLOCK_CANDIDATE);

	int configure = 0;
	int check_duplication = 1;
	int ignore_state = 0;
	int destroy_candidate = 0;

	switch (s_info.mode) {
	case CLOCK_SERVICE_MODE_NORMAL:
		check_duplication = 1;
		ignore_state = 0;
		destroy_candidate = 0;
		configure = clock_util_setting_conf_get();
		clock_service_event_handler(NULL, CLOCK_EVENT_SCROLLER_FREEZE_OFF);
		break;
	case CLOCK_SERVICE_MODE_NORMAL_RECOVERY:
		check_duplication = 0;
		ignore_state = 1;
		destroy_candidate = 1;
		configure = clock_util_setting_conf_get();
		clock_service_event_handler(NULL, CLOCK_EVENT_SCROLLER_FREEZE_OFF);
		break;
	case CLOCK_SERVICE_MODE_EMERGENCY:
	case CLOCK_SERVICE_MODE_COOLDOWN:
		check_duplication = 1;
		ignore_state = 1;
		destroy_candidate = 1;
		configure = 0;
		_setting_conf_apply(configure);
		if (s_info.mode == CLOCK_SERVICE_MODE_EMERGENCY) {
			clock_service_event_handler(NULL, CLOCK_EVENT_SCROLLER_FREEZE_OFF);
		}
		break;
	case CLOCK_SERVICE_MODE_RECOVERY:
		check_duplication = 0;
		ignore_state = 1;
		destroy_candidate = 1;
		configure = 0;
		_setting_conf_apply(configure);
		clock_service_event_handler(NULL, CLOCK_EVENT_SCROLLER_FREEZE_OFF);
		break;
	}

	/*
	 * this modes are oneshot mode
	 */
	if (s_info.mode == CLOCK_SERVICE_MODE_NORMAL_RECOVERY || s_info.mode == CLOCK_SERVICE_MODE_RECOVERY) {
		s_info.mode = CLOCK_SERVICE_MODE_NORMAL;
	}

	_W("clock service pump: %s %d %d %d %d", pkgname, check_duplication, ignore_state, destroy_candidate, configure);

	if (configure == CLOCK_CONF_CLOCK_CONFIGURATION) {
		if (prev_clock != NULL && prev_clock->pkgname != NULL) {
			if (strcmp(prev_clock->pkgname, pkgname) == 0) {
				_E("%s is already attached to home, just configure it", pkgname);
				if (clock_manager_view_configure(prev_clock, configure) != CLOCK_RET_OK) {
					_E("Failed to configure %s", pkgname);
					goto ERR;
				}
				_setting_conf_apply(configure);
				return;
			}
		}
	}
	if (ignore_state == 0) {
		if (_state_get() != CLOCK_SERVICE_STATE_READY) {
			_E("not ready to create a new clock:%s", pkgname);
			s_info.is_pump_ignored = 1;
			return;
		}
	} else {
			if (_state_get() != CLOCK_SERVICE_STATE_READY &&
				candidate_clock != NULL) {
				if (candidate_clock->pkgname != NULL) {
					if (strcmp(candidate_clock->pkgname, pkgname) == 0 &&
						s_info.waiting_timer != NULL) {
						_W("we already requested a view to clock(%s)", pkgname);
						return;
					}
				}
		}
	}
	if (check_duplication == 1) {
		if (prev_clock != NULL && prev_clock->pkgname != NULL) {
			if (strcmp(prev_clock->pkgname, pkgname) == 0) {
				_E("%s is already attached to home", pkgname);
				_setting_conf_apply(configure);
				return;
			}
		}
	}
	if (destroy_candidate == 1) {
		if (candidate_clock != NULL) {
			if (candidate_clock->pkgname != NULL) {
				if (strcmp(candidate_clock->pkgname, pkgname) != 0) {
					_W("destroying candicate clock:%s", candidate_clock->pkgname);
					clock_manager_clock_set(CLOCK_CANDIDATE, NULL);
					_clock_destroy(candidate_clock);
				}
			}
		}
	}

	_state_set(CLOCK_SERVICE_STATE_WAITING);
	_del_waiting_timer();
	ret = _clock_provider_change(pkgname, &clock, configure);
	if (ret == CLOCK_RET_FAIL) {
		_E("Failed to create clock: %s", pkgname);
		_state_set(CLOCK_SERVICE_STATE_READY);
		goto ERR;
	}

	if (clock->async == 1) {
		clock->state = CLOCK_STATE_WAITING;
		clock_manager_clock_set(CLOCK_CANDIDATE, clock);

		_del_waiting_timer();
		_set_waiting_timer();
		_D("%s will be created with async manner", pkgname);
	} else {
		_state_set(CLOCK_SERVICE_STATE_READY);
		_D("%s will be created with sync manner", pkgname);
	}

	return ;

ERR:
	if (clock == NULL) {
		_E("Failed create clock of %s", pkgname);
		_clock_error_report(pkgname, 0);
	}
	_setting_conf_apply(configure);
}

static void _wms_clock_vconf_cb(keynode_t *node, void *data)
{
	if (util_feature_enabled_get(FEATURE_CLOCK_CHANGE) == 1) {
		home_dbus_lcd_on_signal_send(EINA_FALSE);
		_clock_service_heartbeat_pump();
	} else {
		_E("Editing is in progress, delaying changing a clock");
	}
}

HAPI void clock_service_init(void)
{
	int ret = 0;

	if((ret = vconf_notify_key_changed(VCONFKEY_WMS_CLOCKS_SET_IDLE, _wms_clock_vconf_cb, NULL)) < 0) {
		_E("Failed to register the vconf callback(WMS_CLOCKS_SET_IDLE) : %d", ret);
	}

	_state_set(CLOCK_SERVICE_STATE_READY);

	/*
	 * workaround, prevent to apply invalid mode
	 */
	if (s_info.mode == CLOCK_SERVICE_MODE_EMERGENCY) {
		if (emergency_mode_enabled_get() == 0) {
			s_info.mode = CLOCK_SERVICE_MODE_NORMAL;
		}
	}
	if (s_info.mode == CLOCK_SERVICE_MODE_COOLDOWN) {
		if (cooldown_mode_enabled_get() == 0) {
			s_info.mode = CLOCK_SERVICE_MODE_NORMAL;
		}
	}

	_clock_service_heartbeat_pump();
	clock_shortcut_init();
}

HAPI void clock_service_fini(void)
{
	int ret = 0;

	if((ret = vconf_ignore_key_changed(VCONFKEY_WMS_CLOCKS_SET_IDLE,
		_wms_clock_vconf_cb)) < 0) {
		_E("Failed to ignore the vconf callback(WMS_CLOCKS_SET_IDLE) : %d", ret);
	}

	_state_set(CLOCK_SERVICE_STATE_NOT_INITIALIZED);
	clock_shortcut_fini();
}

HAPI char *clock_service_clock_pkgname_get(void)
{
	return _clock_pkg_get(s_info.mode);
}

HAPI void clock_service_mode_set(int mode)
{
	if (mode == CLOCK_SERVICE_MODE_NORMAL_RECOVERY) {
		if (s_info.mode == CLOCK_SERVICE_MODE_EMERGENCY ||
			s_info.mode == CLOCK_SERVICE_MODE_COOLDOWN) {
			return ;
		}
	}

	s_info.mode = mode;
}

HAPI int clock_service_mode_get(void)
{
	return s_info.mode;
}

HAPI void clock_service_request(int mode)
{
	if (mode >= 0) {
		clock_service_mode_set(mode);
	}

	if (_state_get() != CLOCK_SERVICE_STATE_NOT_INITIALIZED) {
		_clock_service_heartbeat_pump();
	}
}

HAPI void clock_service_pause(void)
{
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	ret_if(clock == NULL);

	clock_view_event_handler(clock, CLOCK_EVENT_APP_PAUSE, clock->need_event_relay);
}

HAPI void clock_service_resume(void)
{
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	ret_if(clock == NULL);

	clock_view_event_handler(clock, CLOCK_EVENT_APP_RESUME, clock->need_event_relay);
}

HAPI void clock_service_event_handler(clock_h clock, int event)
{
	int need_pump = 0;
	clock_h prev_clock = NULL;
	clock_h candicate_clock = NULL;

	switch (event) {
		case CLOCK_EVENT_VIEW_READY:
			ret_if(clock == NULL);
			candicate_clock = clock_manager_clock_get(CLOCK_CANDIDATE);
			if (clock != candicate_clock) {
				_E("clock %s isn't candidate clock", clock->pkgname);
				return ;
			}

			if (clock->state == CLOCK_STATE_WAITING) {
				if (s_info.is_pump_ignored == 1) {
					need_pump = 1;
					s_info.is_pump_ignored = 0;
				}

				_del_waiting_timer();
				clock_manager_clock_set(CLOCK_CANDIDATE, NULL);

				if (clock_manager_view_create(clock) != CLOCK_RET_OK) {
					_E("failed to create view");
					clock_del(clock);
					_state_set(CLOCK_SERVICE_STATE_READY);
					if (need_pump == 1) {
						_clock_service_heartbeat_pump();
					}
					return ;
				}

				prev_clock = clock_manager_clock_get(CLOCK_ATTACHED);

				if (clock_manager_view_attach(clock) != CLOCK_RET_OK) {
					_E("Failed to attach view");
					clock_manager_view_destroy(clock);
					clock_del(clock);
					_state_set(CLOCK_SERVICE_STATE_READY);
					if (need_pump == 1) {
						_clock_service_heartbeat_pump();
					}
					return ;
				}

				clock->state = CLOCK_STATE_RUNNING;

				clock_manager_clock_set(CLOCK_ATTACHED, clock);
				clock_manager_clock_set(CLOCK_CANDIDATE, NULL);
				_setting_conf_apply(clock->configure);

				/*
				 * sequence is meaningful
				 */
				if (prev_clock != NULL) {
					_clock_destroy(prev_clock);
				}

				_state_set(CLOCK_SERVICE_STATE_READY);
				if (need_pump == 1) {
					_clock_service_heartbeat_pump();
				}
			} else {
				_E("Not a async clock: %s ", clock->appid);
			}
			break;
		case CLOCK_EVENT_VIEW_RESIZED:
			ret_if(clock == NULL);

			if (clock->view != NULL) {
				clock_view_event_handler(clock, CLOCK_EVENT_VIEW_RESIZED, 0);
			}
			break;
		case CLOCK_EVENT_SCROLLER_FREEZE_ON:
			_W("scroller freeze on");
			_scroller_freeze(1);
			break;
		case CLOCK_EVENT_SCROLLER_FREEZE_OFF:
			_W("scroller freeze off");
			_scroller_freeze(0);
			break;
		case CLOCK_EVENT_APP_PROVIDER_ERROR:
			ret_if(clock == NULL);
			_scroller_freeze(0);

			_clock_error_report(clock->pkgname, 0);
			break;
		case CLOCK_EVENT_APP_PROVIDER_ERROR_FATAL:
			ret_if(clock == NULL);
			_scroller_freeze(0);

			_clock_error_report(clock->pkgname, 1);
			break;
	}
}

#define CLOCK_SELECTOR_PKGNAME "org.tizen.clocksetting.clock"
HAPI int clock_service_clock_selector_launch(void)
{
	int pid = 0;
	bundle *b = bundle_create();
	if (b != NULL) {
		bundle_add(b, "__APP_SVC_OP_TYPE__", APP_CONTROL_OPERATION_MAIN);
		bundle_add(b, "launch_type", "home");

		home_dbus_cpu_booster_signal_send();
		pid = aul_launch_app(CLOCK_SELECTOR_PKGNAME, b);
		_D("aul_launch_app: %s(%d)", CLOCK_SELECTOR_PKGNAME, pid);

		if (pid < 0) {
			_E("Failed to launch %s(%d)", CLOCK_SELECTOR_PKGNAME, pid);
		}
	}

	return pid;
}

HAPI void clock_service_scroller_freezed_set(int is_freeze)
{
	_W("clock freeze set:%d", is_freeze);

	_scroller_freeze(is_freeze);
}

HAPI int clock_service_scroller_freezed_get(void)
{
	return s_info.is_scroller_screezed;
}

HAPI const int const clock_service_get_retry_count(void)
{
	return ERROR_RETRY_COUNT;
}

