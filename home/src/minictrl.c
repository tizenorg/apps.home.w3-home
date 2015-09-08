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
#include <Evas.h>
#include <stdbool.h>
#include <glib.h>
#include <aul.h>
#include <vconf.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
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

static void _minicontrol_monitor_cb(minicontrol_action_e action,
		const char *name,
		unsigned int width,
		unsigned int height,
		minicontrol_priority_e priority,
		void *data)
{
	clock_h clock;
	char tmp_name[4096];
	const char *org_name = NULL;
	int pid;

	/**
	 * "name" always has PID as a string.
	 */

	/* Remove '[', ']' from name or pid string */
	if (sscanf(name, "[%d]", &pid) != 1) {
		if (sscanf(name, "[%[^]]]", tmp_name) == 1) {
			org_name = name;
			name = (const char *)tmp_name;
		} else {
			_E("Failed to extract name (%s)", name);
		}
		pid = -1;
	}
	_D("Extract pid(%d) or name(%s) from origin name(%s)", pid, name, org_name);

	clock = clock_manager_clock_get(CLOCK_ATTACHED);
	if (!clock) {
		_E("Try to get CANDIDATE");
		clock = clock_manager_clock_get(CLOCK_CANDIDATE);
	} else if (pid > 0) {
		if (clock->pid == pid) {
		} else {
			clock_h clock_candidate;

			clock_candidate = clock_manager_clock_get(CLOCK_CANDIDATE);
			if (clock_candidate) {
				if (clock_candidate->pid == pid) {
					clock = clock_candidate;
				} else {
					_E("pid(%d) is wrong", pid);
					clock = NULL;
				}
			}
		}
	} else {
		if (clock->pkgname && !strstr(clock->pkgname, name)) {
			clock_h clock_candidate;

			clock_candidate = clock_manager_clock_get(CLOCK_CANDIDATE);
			if (clock_candidate) {
				if (clock_candidate->pkgname && strstr(clock_candidate->pkgname, name)) {
					clock = clock_candidate;
				} else {
					_E("pid(%d) is wrong", pid);
					clock = NULL;
				}
			}
		} else if (!clock->pkgname) {
			clock_h clock_candidate;

			clock_candidate = clock_manager_clock_get(CLOCK_CANDIDATE);
			if (clock_candidate) {
				if (clock_candidate->pkgname && strstr(clock_candidate->pkgname, name)) {
					clock = clock_candidate;
				} else {
					_E("pid(%d) is wrong", pid);
					clock = NULL;
				}
			}
		}
	}

	if (clock) {
		clock_inf_minictrl_event_hooker(action, clock->pid, org_name ? org_name : name, 1, width, height);
	} else {
		_E("Monitor: NO CLOCK?");
	}
}

/*!
 * constructor/deconstructor
 */
HAPI void minicontrol_init(void)
{
	int ret = minicontrol_monitor_start(_minicontrol_monitor_cb, NULL);
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
