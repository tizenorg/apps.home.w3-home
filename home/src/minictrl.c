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
#include <Evas.h>
#include <stdbool.h>
#include <glib.h>
#include <aul.h>
#include <vconf.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
#include <minicontrol-handler.h>
#include <efl_assist.h>
#include <dlog.h>

#include "conf.h"
#include "layout.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "page_info.h"
#include "scroller_info.h"
#include "scroller.h"
#include "page.h"
#include "minictrl.h"
#include "power_mode.h"
#include "clock_service.h"

/*!
 * functions to handle MC IPC & object
 */
static void _minicontrol_monitor_cb_with_handler(minicontrol_action_e action, const char *name,
	unsigned int width, unsigned int height, minicontrol_priority_e priority,
	minicontrol_h handler, void *data)
{
	int pid = 0;
	int state = 0;
	char *service_name = NULL;
	char *category_name = NULL;

	if(name == NULL || handler == NULL) {
		_E("Invalid paremter %p %p", name, handler);
		return;
	}

	/* This API is not kept in Tizen 2.3 */
	//minicontrol_handler_get_state(handler, &state);
	minicontrol_handler_get_pid(handler, &pid);
	minicontrol_handler_get_service_name(handler, &service_name);
	minicontrol_handler_get_category(handler, &category_name);
	if (service_name == NULL) {
		_E("Failed to get service name from a minicontrol handler");
		return;
	}

	_E("socket:%s, service:%s pid:%d category:%s action:%d, flag1:%d, flag2:%d, handler:%p"
		,name, service_name, pid, category_name, action, width, height, handler);

	if (strcmp(category_name, MINICONTROL_HDL_CATEGORY_CLOCK) == 0 ||
		strcmp(category_name, MINICONTROL_HDL_CATEGORY_UNKNOWN) == 0) {
		clock_inf_minictrl_event_hooker(action, pid, name, state, width, height);
	} else {
		_E("Not supported categroy");
		return ;
	}
}

static void _minicontrol_monitor_cb_without_handler(minicontrol_action_e action, const char *name,
	unsigned int width, unsigned int height, minicontrol_priority_e priority,
	minicontrol_h handler, void *data)
{
	ret_if(name == NULL);

	Minictrl_Entry *entry = minictrl_manager_entry_get_by_name(name);
	ret_if(entry == NULL);

	_E("socket:%s, category:%d action:%d, flag1:%d, flag2:%d"
		,name, entry->category, action, width, height);

	if (action == MINICONTROL_ACTION_STOP) {
		if (entry->view != NULL) {
			minictrl_manager_entry_del_by_name(name);
		} else {
			_E("failed to get view, do nothing");
		}
	}
}

static void _minicontrol_monitor_cb(minicontrol_action_e action, const char *name,
	unsigned int width, unsigned int height, minicontrol_priority_e priority,
	minicontrol_h handler, void *data)
{
	if (handler != NULL) {
		_minicontrol_monitor_cb_with_handler(action, name,
			width, height, priority, handler, data);
	} else {
		_E("A minicontrol provider may be died forcely");
		_minicontrol_monitor_cb_without_handler(action, name,
			width, height, priority, handler, data);
	}
}

/*!
 * constructor/deconstructor
 */
HAPI void minicontrol_init(void)
{
	int ret = minicontrol_monitor_start_with_handler(_minicontrol_monitor_cb, NULL);
	if (ret != MINICONTROL_ERROR_NONE) {
		_E("Failed to attach minicontrol monitor:%d", ret);
	}
}

HAPI void minicontrol_fini(void)
{
	int ret = minicontrol_monitor_stop();
	if (ret != MINICONTROL_ERROR_NONE) {
		_E("Failed to deattach minicontrol monitor:%d", ret);
	}
}
