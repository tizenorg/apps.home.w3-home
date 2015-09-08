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

#include <Elementary.h>
#include <stdbool.h>
#include <stdio.h>
#include <aul.h>
#include <pkgmgr-info.h>
#include <vconf.h>
#include <dlog.h>
#include <efl_assist.h>
#include <app.h>

#include "conf.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "dbus.h"
#include "clock_service.h"

#define CLOCK_LAUNCH_WAITING_TERMINATE_TIME 3.0
#define CLOCK_LAUNCH_RETRY_COUNT_NATIVE 10
#define CLOCK_LAUNCH_RETRY_COUNT_WEBAPP 20
#define CLOCK_LAUNCH_RETRY_DELAY 100000

extern int aul_kill_pid(int pid);

static int _app_kill_by_pid(int pid)
{
	int ret = aul_kill_pid(pid);

	_D("app kill pid:%d(ret:%d)", pid, ret);

	return ret;
}

static int _app_terminate_by_pid(int pid)
{
	int ret = aul_terminate_pid(pid);

	_D("app terminate pid:%d(ret:%d)", pid, ret);

	return ret;
}



static int _clock_launch(char *appid, int *pid_a, int *pkgtype_a, int conf)
{
	_D("%s", __func__);
	int i = 0, pid = 0;
	int ret = W_HOME_ERROR_FAIL;
	retv_if(appid == NULL, ret);

	int pkgtype = util_get_app_type(appid);

	if (pkgtype == APP_TYPE_WEB) {
		_D("Webapp clock:%s", appid);
		for (i = 0; i < CLOCK_LAUNCH_RETRY_COUNT_WEBAPP; i++) {
			home_dbus_cpu_booster_signal_send();
			pid = aul_open_app(appid);
			_E("[retry] aul_open_app : %s(%d) - %d", appid, pid, i);
			if (pid >= AUL_R_OK) {
				ret = W_HOME_ERROR_NONE;
				break;
			}
			usleep(CLOCK_LAUNCH_RETRY_DELAY);
		}
	} else if (pkgtype == APP_TYPE_NATIVE) {
		_D("App clock:%s", appid);
		bundle *b = bundle_create();
		if (b != NULL) {
			bundle_add(b, "__APP_SVC_OP_TYPE__", APP_CONTROL_OPERATION_MAIN);
			clock_util_setting_conf_bundle_add(b, conf);

			home_dbus_cpu_booster_signal_send();
			pid = aul_launch_app((const char *)appid, b);
			_D("aul_launch_app[%d] : %s(%d)", pkgtype, appid, pid);

			if (pid == AUL_R_ECOMM || pid == AUL_R_ETERMINATING) {
				for (i = 0; i < CLOCK_LAUNCH_RETRY_COUNT_NATIVE; i++) {
					home_dbus_cpu_booster_signal_send();
					pid = aul_launch_app(appid, b);
					_E("[retry] aul_launch_app : %s(%d) - %d", appid, pid, i);
					if (pid >= AUL_R_OK) {
						ret = W_HOME_ERROR_NONE;
						break;
					}
					usleep(CLOCK_LAUNCH_RETRY_DELAY);
				}
			} else if (pid < AUL_R_OK) {
				_E("Failed to launch APP : %s(%d)", appid, pid);
			} else {
				ret = W_HOME_ERROR_NONE;
			}

			bundle_free(b);
		} else {
			_E("failed to create a bundle");
		}
	} else if (pkgtype == APP_TYPE_WIDGET) {
		_D("Widget type: %s\n", appid);
		ret = W_HOME_ERROR_NONE;
	} else {
		_E("invalid package type:%d", pkgtype);
	}

	if (pid_a != NULL) {
		*pid_a = pid;
	}

	if (pkgtype_a != NULL) {
		*pkgtype_a = pkgtype;
	}

	return ret;
}

HAPI char *clock_util_wms_configuration_get(void)
{
	return vconf_get_str(VCONFKEY_WMS_CLOCKS_SET_IDLE);
}

HAPI int clock_util_wms_configuration_set(const char *pkgname)
{
	int ret = 0;
	retv_if(pkgname == NULL, W_HOME_ERROR_FAIL);

	if ((ret = vconf_set_str(VCONFKEY_WMS_CLOCKS_SET_IDLE, pkgname)) != 0) {
		_E("Failed to set %s:%d", VCONFKEY_WMS_CLOCKS_SET_IDLE, ret);
		return W_HOME_ERROR_FAIL;
	}

	return W_HOME_ERROR_NONE;
}

HAPI int clock_util_provider_launch(const char *clock_pkgname, int *pid_a, int conf)
{
	int pid = 0;
	int ret = W_HOME_ERROR_FAIL;
	char *appid = NULL;
	retv_if(clock_pkgname == NULL, W_HOME_ERROR_FAIL);

	appid = util_get_appid_by_pkgname(clock_pkgname);
	ret = _clock_launch(appid, &pid, NULL, conf);
	free(appid);

	if (ret == W_HOME_ERROR_FAIL) {
		_E("Launching app pid:%d(%d)", pid, ret);
		if (strncmp(clock_pkgname, CLOCK_APP_RECOVERY, strlen(clock_pkgname)) != 0) {
			_E("try to recovery: %s", clock_pkgname);
			clock_util_wms_configuration_set(CLOCK_APP_RECOVERY);
		} else {
			_E("It's already set to recovery clock, give up");
		}
	}

	if (pid_a != NULL) {
		*pid_a = pid;
	}

	return ret;
}

static Eina_Bool _clock_service_terminate_timeout_cb(void *data)
{
	int pid = (int)data;

	if (pid > 0) {
		_app_kill_by_pid(pid);
	}

	return ECORE_CALLBACK_CANCEL;
}

HAPI void clock_util_terminate_clock_by_pid(int pid)
{
	if (_app_terminate_by_pid(pid) != AUL_R_OK) {
		if (_app_kill_by_pid(pid) != AUL_R_OK) {
			_E("Failed to terminate clock, we will kill it(%d) after %d seconds",
				pid, CLOCK_LAUNCH_WAITING_TERMINATE_TIME);
			ecore_timer_add(CLOCK_LAUNCH_WAITING_TERMINATE_TIME,
				_clock_service_terminate_timeout_cb, (void *)pid);
		}
	}
}

HAPI int clock_util_screen_reader_enabled_get(void)
{
	int val = 0;

	if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &val) == 0) {
		return val;
	}

	return 0;
}

#define VCONFKEY_CLOCK_SETTING_CONF_KEY "db/setting/idle_clock_show"
HAPI int clock_util_setting_conf_get(void)
{
	int ret = 0;
	int val = 0;

	if ((ret = vconf_get_int(VCONFKEY_CLOCK_SETTING_CONF_KEY, &val)) < 0) {
		_E("Failed to get %s(%d)", VCONFKEY_CLOCK_SETTING_CONF_KEY, ret);
	}

	return val;
}

HAPI void clock_util_setting_conf_set(int value)
{
	int ret = 0;

	if ((ret = vconf_set_int(VCONFKEY_CLOCK_SETTING_CONF_KEY, value)) < 0) {
		_E("Failed to set %s(%d)", VCONFKEY_CLOCK_SETTING_CONF_KEY, ret);
	}
}

HAPI void clock_util_setting_conf_bundle_add(bundle *b, int type)
{
	ret_if(b == NULL);

	if (type == CLOCK_CONF_CLOCK_CONFIGURATION) {
		bundle_add(b, "view_type", "setting");
	}
}

HAPI const char *clock_util_setting_conf_content(int type)
{
	if (type == CLOCK_CONF_CLOCK_CONFIGURATION) {
		return "view_type=setting;";
	}

	return NULL;
}

