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

#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Elementary.h>
#include <utilX.h>
#include <dd-display.h>
#include <aul.h>
#include <syspopup_caller.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <efl_assist.h>
#include <dlog.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>

#include "util.h"
#include "main.h"
#include "log.h"
#include "dbus.h"
#include "gesture.h"
#include "minictrl.h"
#include "page_info.h"
#include "scroller_info.h"
#include "scroller.h"
#include "apps/apps_main.h"

static struct _info {
	DBusConnection *connection;
	Eina_List *cbs_list[DBUS_EVENT_MAX];
} s_info = {
	.connection = NULL,
	.cbs_list = {NULL, },
};

typedef struct {
	void (*result_cb)(void *, void *);
	void *result_data;
} dbus_cb_s;

HAPI int home_dbus_register_cb(
	int type,
	void (*result_cb)(void *, void *), void *result_data)
{
	retv_if(result_cb == NULL, W_HOME_ERROR_INVALID_PARAMETER);

	dbus_cb_s *cb = calloc(1, sizeof(dbus_cb_s));
	retv_if(cb == NULL, W_HOME_ERROR_FAIL);

	cb->result_cb = result_cb;
	cb->result_data = result_data;

	s_info.cbs_list[type] = eina_list_prepend(s_info.cbs_list[type], cb);
	retv_if(s_info.cbs_list[type] == NULL, W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}

HAPI void home_dbus_unregister_cb(
	int type,
	void (*result_cb)(void *, void *))
{
	const Eina_List *l;
	const Eina_List *n;
	dbus_cb_s *cb;
	EINA_LIST_FOREACH_SAFE(s_info.cbs_list[type], l, n, cb) {
		continue_if(cb == NULL);
		if (result_cb != cb->result_cb) continue;
		s_info.cbs_list[type] = eina_list_remove(s_info.cbs_list[type], cb);
		free(cb);
		return;
	}
}

static void _execute_cbs(int type, void *event_info)
{
	const Eina_List *l = NULL;
	const Eina_List *n = NULL;
	dbus_cb_s *cb = NULL;
	EINA_LIST_FOREACH_SAFE(s_info.cbs_list[type], l, n, cb) {
		continue_if(cb == NULL);
		continue_if(cb->result_cb == NULL);

		cb->result_cb(cb->result_data, event_info);
	}
}

static Evas_Object *_scroller_get(void) {
	Evas_Object *win = main_get_info()->win;
	Evas_Object *layout = NULL;
	Evas_Object *scroller = NULL;

	if (win != NULL) {
		layout = evas_object_data_get(win, DATA_KEY_LAYOUT);
		if (layout != NULL) {
			scroller = elm_object_part_content_get(layout, "scroller");
		}
	}

	return scroller;
}

static DBusConnection *_dbus_connection_get(void) {

	if (s_info.connection == NULL) {
		DBusError derror;
		DBusConnection *connection = NULL;

		dbus_error_init(&derror);
		connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &derror);
		if (connection == NULL) {
			_E("Failed to get dbus connection:%s", derror.message);
			dbus_error_free(&derror);
			return NULL;
		}
		dbus_connection_setup_with_g_main(connection, NULL);
		dbus_error_free(&derror);

		s_info.connection = connection;
	}

	return s_info.connection;
}

static Eina_Bool _procsweep_idler_cb(void *data)
{
	home_dbus_procsweep_signal_send();

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _home_raise_idler_cb(void *data)
{
	home_dbus_home_raise_signal_send();

	return ECORE_CALLBACK_CANCEL;
}



/* DBus message handler callback */
static DBusHandlerResult _dbus_message_recv_cb(DBusConnection *connection, DBusMessage *message, void *data)
{
	Evas_Object *win = main_get_info()->win;
	Evas_Object *scroller = _scroller_get();

	if (win == NULL || scroller == NULL) {
		_E("win or scroller is NULL, give up");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (dbus_message_is_signal(message, DBUS_LOW_BATTERY_INTERFACE, DBUS_LOW_BATTERY_MEMBER_EXTREME_LEVEL)) {
		_D("Low Battery signal received : %p", message);

		if (main_get_info()->win) {
			elm_win_activate(main_get_info()->win);
			scroller_bring_in_by_push_type(scroller, SCROLLER_PUSH_TYPE_CENTER, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_ANIMATOR);
			ecore_idler_add(_home_raise_idler_cb, NULL);
			ecore_idler_add(_procsweep_idler_cb, NULL);
		}
	} else if (dbus_message_is_signal(message, DBUS_DEVICED_DISPLAY_INTERFACE, DBUS_DEVICED_DISPLAY_MEMBER_LCD_ON)) {
		_W("LCD on");
		int ret = 0;
		DBusError derror;
		const char *state = NULL;
		dbus_error_init(&derror);
		ret = dbus_message_get_args(message, &derror, DBUS_TYPE_STRING, &state, DBUS_TYPE_INVALID);
		if (!ret) {
			_E("Failed to get reply (%s:%s)", derror.name, derror.message);
		}
		_execute_cbs(DBUS_EVENT_LCD_ON, (void*)state);
		dbus_error_free(&derror);
	} else if (dbus_message_is_signal(message, DBUS_DEVICED_DISPLAY_INTERFACE, DBUS_DEVICED_DISPLAY_MEMBER_LCD_OFF)) {
		_W("LCD on");
		int ret = 0;
		DBusError derror;
		const char *state = NULL;
		dbus_error_init(&derror);
		ret = dbus_message_get_args(message, &derror, DBUS_TYPE_STRING, &state, DBUS_TYPE_INVALID);
		if (!ret) {
			_E("Failed to get reply (%s:%s)", derror.name, derror.message);
		}
		_execute_cbs(DBUS_EVENT_LCD_OFF, (void*)state);
		dbus_error_free(&derror);
	} else if (dbus_message_is_signal(message, DBUS_DEVICED_SYSNOTI_INTERFACE, DBUS_DEVICED_SYSNOTI_MEMBER_COOLDOWN_CHANGED)) {
		_E("Cooldown status message received");
		int ret = 0;
		DBusError derror;
		const char *state = NULL;
		dbus_error_init(&derror);
		ret = dbus_message_get_args(message, &derror, DBUS_TYPE_STRING, &state, DBUS_TYPE_INVALID);
		if (!ret) {
			_E("Failed to get reply (%s:%s)", derror.name, derror.message);
		}
		_execute_cbs(DBUS_EVENT_COOLDOWN_STATE_CHANGED, (void *)state);
		dbus_error_free(&derror);
	} else if (dbus_message_is_signal(message, DBUS_ALPM_MANAGER_INTERFACE, DBUS_ALPM_MANAGER_MEMBER_STATUS)) {
		_E("ALPM manager status message received");
		int ret = 0;
		DBusError derror;
		const char *state = NULL;
		dbus_error_init(&derror);
		ret = dbus_message_get_args(message, &derror, DBUS_TYPE_STRING, &state, DBUS_TYPE_INVALID);
		if (!ret) {
			_E("Failed to get reply (%s:%s)", derror.name, derror.message);
		}
		_execute_cbs(DBUS_EVENT_ALPM_MANAGER_STATE_CHANGED, (void *)state);
		dbus_error_free(&derror);
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static w_home_error_e _dbus_sig_attach(char *path, char *interface, char *member)
{
	DBusError derror;
	DBusConnection *connection = NULL;
	retv_if(path == NULL, W_HOME_ERROR_FAIL);
	retv_if(interface == NULL, W_HOME_ERROR_FAIL);

	/* DBUS */
	connection = _dbus_connection_get();
	if (connection == NULL) {
		_E("failed to get DBUS connection");
		return W_HOME_ERROR_FAIL;
	}

	dbus_error_init(&derror);

	/* Set the DBus rule for the wakeup gesture signal */
	char rules[512] = {0,};
	sprintf(rules, "path='%s',type='signal',interface='%s',member='%s'", path, interface, member);
	dbus_bus_add_match(connection, rules, &derror);
	if (dbus_error_is_set(&derror)) {
		_E("D-BUS rule adding error: %s", derror.message);
		dbus_error_free(&derror);
		return W_HOME_ERROR_FAIL;
	}

	/* Set the callback function */
	if(dbus_connection_add_filter(connection, _dbus_message_recv_cb, NULL, NULL) == FALSE) {
		_E("Failed to add dbus filter : %s", derror.message);
		dbus_error_free(&derror);
		return W_HOME_ERROR_FAIL;
	}

	dbus_error_free(&derror);

	return W_HOME_ERROR_NONE;
}

static void _dbus_sig_dettach(const char *path, const char *interface, const char *member)
{
	DBusError err;
	DBusConnection *connection = NULL;
	ret_if(path == NULL);
	ret_if(interface == NULL);

	connection = _dbus_connection_get();
	if (connection == NULL) {
		_E("failed to get DBUS connection");
		return ;
	}

	dbus_error_init(&err);
	dbus_connection_remove_filter(connection, _dbus_message_recv_cb, NULL);

	char rules[512] = {0,};

	snprintf(rules, sizeof(rules), "path='%s',type='signal',interface='%s',member='%s'", path, interface, member);
	dbus_bus_remove_match(connection, rules, &err);
	if(dbus_error_is_set(&err)) {
		_E("Failed to dbus_bus_remove_match : %s", err.message);
	}

	dbus_error_free(&err);
}

HAPI void *home_dbus_connection_get(void)
{
	return (void *)_dbus_connection_get();
}

HAPI void home_dbus_init(void *data)
{
#if 0
	if(_dbus_sig_attach(
		DBUS_WAKEUP_GESTURE_PATH,
		DBUS_WAKEUP_GESTURE_INTERFACE,
		DBUS_WAKEUP_GESTURE_MEMBER_WAKEUP) != W_HOME_ERROR_NONE) {
		_E("Failed to attach gesture signal filter");
	}
#endif

	if(_dbus_sig_attach(
		DBUS_LOW_BATTERY_PATH,
		DBUS_LOW_BATTERY_INTERFACE,
		DBUS_LOW_BATTERY_MEMBER_EXTREME_LEVEL) != W_HOME_ERROR_NONE) {
		_E("Failed to attach low battery signal filter");
	}

	if(_dbus_sig_attach(
		DBUS_DEVICED_DISPLAY_PATH,
		DBUS_DEVICED_DISPLAY_INTERFACE,
		DBUS_DEVICED_DISPLAY_MEMBER_LCD_ON) != W_HOME_ERROR_NONE) {
		_E("Failed to attach LCD on signal filter");
	}

	if(_dbus_sig_attach(
		DBUS_DEVICED_DISPLAY_PATH,
		DBUS_DEVICED_DISPLAY_INTERFACE,
		DBUS_DEVICED_DISPLAY_MEMBER_LCD_OFF) != W_HOME_ERROR_NONE) {
		_E("Failed to attach LCD off signal filter");
	}

	if(_dbus_sig_attach(
		DBUS_DEVICED_SYSNOTI_PATH,
		DBUS_DEVICED_SYSNOTI_INTERFACE,
		DBUS_DEVICED_SYSNOTI_MEMBER_COOLDOWN_CHANGED) != W_HOME_ERROR_NONE) {
		_E("Failed to attach cooldown mode signal filter");
	}

	if(_dbus_sig_attach(
		DBUS_ALPM_MANAGER_PATH,
		DBUS_ALPM_MANAGER_INTERFACE,
		DBUS_ALPM_MANAGER_MEMBER_STATUS) != W_HOME_ERROR_NONE) {
		_E("Failed to attach alpm manager signal filter");
	}
}

HAPI void home_dbus_fini(void *data)
{
#if 0
	_dbus_sig_dettach(
		DBUS_WAKEUP_GESTURE_PATH,
		DBUS_WAKEUP_GESTURE_INTERFACE,
		DBUS_WAKEUP_GESTURE_MEMBER_WAKEUP);
#endif
	_dbus_sig_dettach(
		DBUS_LOW_BATTERY_PATH,
		DBUS_LOW_BATTERY_INTERFACE,
		DBUS_LOW_BATTERY_MEMBER_EXTREME_LEVEL);

	_dbus_sig_dettach(
		DBUS_DEVICED_DISPLAY_PATH,
		DBUS_DEVICED_DISPLAY_INTERFACE,
		DBUS_DEVICED_DISPLAY_MEMBER_LCD_ON);

	_dbus_sig_dettach(
		DBUS_DEVICED_DISPLAY_PATH,
		DBUS_DEVICED_DISPLAY_INTERFACE,
		DBUS_DEVICED_DISPLAY_MEMBER_LCD_OFF);

	_dbus_sig_dettach(
		DBUS_DEVICED_SYSNOTI_PATH,
		DBUS_DEVICED_SYSNOTI_INTERFACE,
		DBUS_DEVICED_SYSNOTI_MEMBER_COOLDOWN_CHANGED);

	_dbus_sig_dettach(
		DBUS_ALPM_MANAGER_PATH,
		DBUS_ALPM_MANAGER_INTERFACE,
		DBUS_ALPM_MANAGER_MEMBER_STATUS);

	if(s_info.connection != NULL) {
		dbus_connection_close(s_info.connection);
		dbus_connection_unref(s_info.connection);
		s_info.connection = NULL;

		_D("DBUS connection is closed");
	}
}
