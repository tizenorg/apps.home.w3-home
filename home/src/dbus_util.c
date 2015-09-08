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
#include <utilX.h>
#include <dd-display.h>
#include <aul.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <syspopup_caller.h>
#include <Evas.h>
#include <efl_assist.h>
#include <dlog.h>

#include "util.h"
#include "main.h"
#include "log.h"
#include "dbus.h"

#define DBUS_CPU_BOOSTER_SEC 200
#define DBUS_LCD_ON_SEC 10000
#define DBUS_REPLY_TIMEOUT (120 * 1000)

static int _append_variant(DBusMessageIter *iter, const char *sig, char *param[])
{
	char *ch;
	int i;
	int int_type;
	uint64_t int64_type;

	if (!sig || !param)
		return 0;

	for (ch = (char*)sig, i = 0; *ch != '\0'; ++i, ++ch) {
		switch (*ch) {
			case 'i':
				int_type = atoi(param[i]);
				dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &int_type);
				break;
			case 'u':
				int_type = atoi(param[i]);
				dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &int_type);
				break;
			case 't':
				int64_type = atoi(param[i]);
				dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &int64_type);
				break;
			case 's':
				dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param[i]);
				break;
			default:
				return -EINVAL;
		}
	}

	return 0;
}

static int _dbus_message_send(const char *path, const char *interface, const char *member)
{
	int ret = 0;
	DBusMessage *msg = NULL;
	DBusConnection *conn = NULL;

	conn = (DBusConnection *)home_dbus_connection_get();
	if (!conn) {
		_E("dbus_bus_get error");
		return -1;
	}

	msg = dbus_message_new_signal(path, interface, member);
	if (!msg) {
		_E("dbus_message_new_signal(%s:%s-%s)", path, interface, member);
		return -1;
	}

	ret = dbus_connection_send(conn, msg, NULL); //async call
	dbus_message_unref(msg);
	if (ret != TRUE) {
		_E("dbus_connection_send error(%s:%s-%s)", path, interface, member);
		return -ECOMM;
	}
	_D("dbus_connection_send, ret=%d", ret);
	return 0;
}

static int _dbus_method_async_call(const char *dest, const char *path,
				const char *interface, const char *method,
						const char *sig, char *param[])
{
	int ret = 0;
	DBusConnection *conn = NULL;
	DBusMessage *msg = NULL;
	DBusMessageIter iter;
	DBusError err;

	conn = home_dbus_connection_get();
	if (!conn) {
		_E("dbus_bus_get error");
		return -1;
	}

	msg = dbus_message_new_method_call(dest, path, interface, method);
	if (!msg) {
		_D("dbus_message_new_method_call(%s:%s-%s)", path, interface, method);
		return -1;
	}

	dbus_message_iter_init_append(msg, &iter);
	ret = _append_variant(&iter, sig, param);
	if (ret < 0) {
		_E("append_variant error(%d)", ret);
		dbus_message_unref(msg);
		return -1;
	}

	dbus_error_init(&err);

	ret = dbus_connection_send(conn, msg, NULL); //async call
	dbus_message_unref(msg);
	if (ret != TRUE) {
		_E("dbus_connection_send error(%s:%s:%s-%s)", dest, path, interface, method);
		_E("dbus_connection_send error(%s:%s)", err.name, err.message);
		return -ECOMM;
	}

	_D("dbus_connection_send, ret=%d", ret);
	dbus_error_free(&err);

	return 0;
}


static DBusMessage *_dbus_method_sync_call(const char *dest, const char *path,
				const char *interface, const char *method,
						const char *sig, char *param[])
{
	int ret = 0;
	DBusConnection *conn = NULL;
	DBusMessage *msg = NULL;
	DBusMessageIter iter;
	DBusMessage *reply = NULL;
	DBusError err;

	conn = home_dbus_connection_get();
	if (!conn) {
		_E("dbus_bus_get error");
		return NULL;
	}

	msg = dbus_message_new_method_call(dest, path, interface, method);
	if (!msg) {
		_D("dbus_message_new_method_call(%s:%s-%s)", path, interface, method);
		return NULL;
	}

	dbus_message_iter_init_append(msg, &iter);
	ret = _append_variant(&iter, sig, param);
	if (ret < 0) {
		_E("append_variant error(%d)", ret);
		return NULL;
	}

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_REPLY_TIMEOUT, &err);
	if (!reply) {
		_E("dbus_connection_send error(No reply): (%s:%s:%s:%s)", dest, path, interface, method);
	}

	if (dbus_error_is_set(&err)) {
		_E("dbus_connection_send error(%s:%s)", err.name, err.message);
		reply = NULL;
	}

	dbus_message_unref(msg);
	dbus_error_free(&err);

	return reply;
}

HAPI void home_dbus_lcd_on_signal_send(Eina_Bool lcd_on)
{
	int ret = 0;
	char *param[1];

	if (lcd_on == EINA_TRUE) {
		char val[32];
		snprintf(val, sizeof(val), "%d", DBUS_LCD_ON_SEC);
		param[0] = val;

		ret = _dbus_method_async_call(
				DBUS_DEVICED_BUS_NAME,
				DBUS_DEVICED_DISPLAY_PATH,
				DBUS_DEVICED_DISPLAY_INTERFACE,
				DBUS_DEVICED_DISPLAY_METHOD_CUSTOM_LCD_ON,
				"i",
				param);
	} else {
		param[0] = DBUS_DEVICED_DISPLAY_COMMAND_LCD_ON;

		ret = _dbus_method_async_call(
				DBUS_DEVICED_BUS_NAME,
				DBUS_DEVICED_DISPLAY_PATH,
				DBUS_DEVICED_DISPLAY_INTERFACE,
				DBUS_DEVICED_DISPLAY_METHOD_CHANGE_STATE,
				"s",
				param);
	}
	_E("Sending LCD ON request signal lcd_on:%d result:%d", lcd_on, ret);
}

HAPI void home_dbus_lcd_off_signal_send(void)
{
	int ret = 0;

	ret = _dbus_method_async_call(
			DBUS_DEVICED_BUS_NAME,
			DBUS_DEVICED_DISPLAY_PATH,
			DBUS_DEVICED_DISPLAY_INTERFACE,
			DBUS_DEVICED_DISPLAY_METHOD_LCD_OFF,
			NULL, NULL);
	_E("Sending LCD OFF request signal, result:%d", ret);
}

HAPI void home_dbus_procsweep_signal_send(void)
{
	int ret = 0;

	ret = _dbus_message_send(
			DBUS_PROCSWEEP_PATH,
			DBUS_PROCSWEEP_INTERFACE,
			DBUS_PROCSWEEP_METHOD);
	_E("Sending PROCSWEEP signal, result:%d", ret);
}

HAPI void home_dbus_home_raise_signal_send(void)
{
	int ret = 0;

	ret = _dbus_message_send(
			DBUS_HOME_RAISE_PATH,
			DBUS_HOME_RAISE_INTERFACE,
			DBUS_HOME_RAISE_MEMBER);
	_E("Sending HOME RAISE signal, result:%d", ret);
}

HAPI void home_dbus_cpu_booster_signal_send(void)
{
	int ret = 0;
	char *param[1] = {NULL, };
	char val[32] = {0, };

	snprintf(val, sizeof(val), "%d", DBUS_CPU_BOOSTER_SEC);
	param[0] = val;

	ret = _dbus_method_async_call(
			DBUS_DEVICED_BUS_NAME,
			DBUS_DEVICED_CPU_BOOSTER_PATH,
			DBUS_DEVICED_CPU_BOOSTER_INTERFACE,
			DBUS_DEVICED_CPU_BOOSTER_METHOD_HOME_LAUNCH,
			"i",
			param);
	_D("Sending cpu booster call:%d result:%d", DBUS_CPU_BOOSTER_SEC, ret);
}

HAPI void home_dbus_scroll_booster_signal_send(int sec)
{
	int ret = 0;
	char *param[1] = {NULL, };
	char val[32] = {0, };

	snprintf(val, sizeof(val), "%d", sec);
	param[0] = val;

	_dbus_method_async_call(
			DBUS_DEVICED_BUS_NAME,
			DBUS_DEVICED_CPU_BOOSTER_PATH,
			DBUS_DEVICED_CPU_BOOSTER_INTERFACE,
			DBUS_DEVICED_CPU_BOOSTER_METHOD_HOME,
			"i", param);
	_D("Sending scroll booster for %d sec result:%d", sec, ret);
}

HAPI char *home_dbus_cooldown_status_get(void)
{
	int ret = 0;
	DBusError err;
	DBusMessage *msg = NULL;
	const char *data = NULL;
	char *ret_val = NULL;

	msg = _dbus_method_sync_call(
			DBUS_DEVICED_BUS_NAME,
			DBUS_DEVICED_SYSNOTI_PATH,
			DBUS_DEVICED_SYSNOTI_INTERFACE,
			DBUS_DEVICED_SYSNOTI_METHOD_COOLDOWN_STATUS,
			NULL, NULL);
	retv_if(msg == NULL, NULL);

	dbus_error_init(&err);

	ret = dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &data, DBUS_TYPE_INVALID);
	if (!ret) {
		_E("Failed to get reply (%s:%s)", err.name, err.message);
	} else {
		if (data != NULL) {
			ret_val = strdup(data);
		}
	}

	dbus_message_unref(msg);
	dbus_error_free(&err);

	return ret_val;
}
