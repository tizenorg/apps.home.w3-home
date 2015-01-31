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
#include <aul.h>
#include <efl_assist.h>
#include <stdio.h>
#include <vconf.h>
#include <dlog.h>
#include <dynamicbox_service.h>

#include "conf.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "layout.h"
#include "clock_service.h"
#include "power_mode.h"

#define TIZEN_CLOCK_CATEGORY "org.tizen.wmanager.WATCH_CLOCK"

extern clock_inf_s clock_inf_minictrl;
extern clock_inf_s clock_inf_dbox;

static struct _s_info {
	clock_h attached_clock;
	clock_h candidate_clock;
} s_info = {
	.attached_clock = NULL,
	.candidate_clock = NULL,
};

static clock_inf_s *_get_clock_inf(int inf_type)
{
	if (inf_type == CLOCK_INF_MINICONTROL) {
		return &clock_inf_minictrl;
	} else if (inf_type == CLOCK_INF_DBOX) {
		return &clock_inf_dbox;
	}

	return NULL;
}

static int _get_inf_type_by_pkgname(const char *pkgname)
{
	int ret = CLOCK_INF_MINICONTROL;
	char *dbox_id = NULL;
	retv_if(pkgname == NULL, ret);

	dbox_id = dynamicbox_service_dbox_id(pkgname);
	if (dbox_id != NULL) {
		char *category = NULL;
		category = dynamicbox_service_category(dbox_id);

		if (category != NULL) {
			if (!strcmp(category, TIZEN_CLOCK_CATEGORY)) {
				ret = CLOCK_INF_DBOX;
			}
			free(category);
		}
		free(dbox_id);
	}

	return ret;
}

HAPI clock_h clock_new(const char *pkgname)
{
	clock_h clock = NULL;
	retv_if(pkgname == NULL, NULL);

	clock = (clock_h)calloc(1, sizeof(clock_s));
	if (clock != NULL) {
		clock->appid = util_get_appid_by_pkgname(pkgname);
		goto_if(clock->appid == NULL, ERR);
		clock->pkgname = strdup(pkgname);
		goto_if(clock->pkgname == NULL, ERR);
		clock->app_type = util_get_app_type(clock->appid);

		int inf_type = _get_inf_type_by_pkgname(pkgname);
		clock_inf_s *inf = _get_clock_inf(inf_type);
		goto_if(inf == NULL, ERR);

		clock->use_dead_monitor = inf->use_dead_monitor;
		clock->async = inf->async;
		clock->state = CLOCK_STATE_IDLE;
		clock->interface = inf_type;
		clock->view = NULL;
	} else {
		_E("failed to allocate memory");
		goto ERR;
	}
	return clock;

ERR:
	if (clock) {
		free(clock->appid);
		free(clock->pkgname);
		free(clock);
	}
	return NULL;
}

HAPI void clock_del(clock_h clock)
{
	ret_if(clock == NULL);

	free(clock->view_id);
	clock->view_id = NULL;
	free(clock->appid);
	clock->appid = NULL;
	free(clock->pkgname);
	clock->pkgname = NULL;
	free(clock);
}

HAPI int clock_manager_view_prepare(clock_h clock)
{
	int ret = CLOCK_RET_FAIL;
	retv_if(clock == NULL, CLOCK_RET_FAIL);
	clock_inf_s *inf = _get_clock_inf(clock->interface);

	if (inf != NULL && inf->prepare != NULL) {
		ret = inf->prepare(clock);
	}

	return ret;
}

HAPI int clock_manager_view_configure(clock_h clock, int type)
{
	int ret = CLOCK_RET_FAIL;
	retv_if(clock == NULL, CLOCK_RET_FAIL);
	clock_inf_s *inf = _get_clock_inf(clock->interface);

	if (inf != NULL && inf->config != NULL) {
		ret = inf->config(clock, type);
	}

	return ret;
}

HAPI int clock_manager_view_create(clock_h clock)
{
	int ret = CLOCK_RET_FAIL;
	retv_if(clock == NULL, CLOCK_RET_FAIL);
	clock_inf_s *inf = _get_clock_inf(clock->interface);

	if (inf != NULL && inf->create != NULL) {
		ret = inf->create(clock);
	}

	return ret;
}

HAPI int clock_manager_view_attach(clock_h clock)
{
	int mode = 0;
	int need_show_clock = 0;
	int ret = CLOCK_RET_FAIL;
	retv_if(clock == NULL, CLOCK_RET_FAIL);
	clock_inf_s *inf = _get_clock_inf(clock->interface);

	mode = clock_service_mode_get();
	if (clock_manager_clock_get(CLOCK_ATTACHED) == NULL
		|| mode == CLOCK_SERVICE_MODE_EMERGENCY
		|| mode == CLOCK_SERVICE_MODE_COOLDOWN) {
		/*
		 * Maybe this is first time which the function is called
		 */
		need_show_clock = 1;
	}

	if (clock->view != NULL) {
		ret = clock_view_attach(clock->view);
		if (ret == CLOCK_RET_OK) {
			if (need_show_clock == 1) {
				clock_view_show(clock->view);
			}
			if (inf != NULL && inf->attached_cb != NULL) {
				inf->attached_cb(clock);
			}
		}
	} else {
		_E("clock view of %s isn't created yet", clock->pkgname);
	}

	return ret;
}

HAPI int clock_manager_view_deattach(clock_h clock)
{
	int ret = CLOCK_RET_FAIL;
	retv_if(clock == NULL, CLOCK_RET_FAIL);

	if (clock->view != NULL) {
		ret = clock_view_deattach(clock->view);
	} else {
		_E("clock view of %s isn't created yet", clock->pkgname);
	}

	return ret;
}

HAPI int clock_manager_view_destroy(clock_h clock)
{
	int ret = CLOCK_RET_FAIL;
	clock_inf_s *inf = _get_clock_inf(clock->interface);

	if (inf != NULL && inf->create != NULL) {
		ret = inf->destroy(clock);
	}

	return ret;
}

HAPI int clock_manager_view_exchange(clock_h clock, void *view)
{
	int ret = CLOCK_RET_FAIL;
	clock_inf_s *inf = _get_clock_inf(clock->interface);
	retv_if(view == NULL, CLOCK_RET_FAIL);

	clock_view_attach(view);

	if (inf != NULL && inf->create != NULL) {
		ret = inf->destroy(clock);
	}

	clock->view = view;

	return ret;
}

HAPI void clock_manager_clock_set(int type, clock_h clock)
{
	if (type == CLOCK_ATTACHED) {
		if (s_info.attached_clock != NULL && clock != NULL) {
			_W("attached clock isn't cleaned-up, yet");
		}

		s_info.attached_clock = clock;
		if (clock != NULL) {
			_D("attached clock:%s(%d)", clock->pkgname, clock->pid);
		}
	} else if (type == CLOCK_CANDIDATE) {
		if (s_info.candidate_clock != NULL && clock != NULL) {
			_W("candidate clock isn't cleaned-up, yet");
		}

		s_info.candidate_clock = clock;
		if (clock != NULL) {
			_D("candidate clock:%s(%d)", clock->pkgname, clock->pid);
		}
	}
}

HAPI clock_h clock_manager_clock_get(int type)
{
	if (type == CLOCK_ATTACHED) {
		return s_info.attached_clock;
	} else if (type == CLOCK_CANDIDATE) {
		return s_info.candidate_clock;
	}

	return NULL;
}

HAPI int clock_manager_clock_inf_type_get(const char *pkgname)
{
	return _get_inf_type_by_pkgname(pkgname);
}

HAPI int clock_manager_view_state_get(int view_type)
{
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	if (clock != NULL) {
		return clock_view_display_state_get(clock->view, view_type);
	}

	return 0;
}
