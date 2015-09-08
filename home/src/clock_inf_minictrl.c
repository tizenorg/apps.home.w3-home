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
#include <vconf.h>
#include <bundle.h>
#include <efl_assist.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
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
#include "key.h"
#include "minictrl.h"
#include "dbus.h"
#include "clock_service.h"

static struct info {
	Ecore_Timer *waiting_timer;
} s_info = {
	.waiting_timer = NULL,
};

#define MINICTRL_PRIVATE_KEY_STATE "mc_pr_k_s"
#define MINICTRL_STATE_PAUSE 0xDEAD0040
#define MINICTRL_STATE_RESUME 0xCAFE0080

static Evas_Object *_scroller_get(void)
{
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

static void _size_set(Evas_Object *minictrl_obj, int width, int height)
{
	evas_object_resize(minictrl_obj, width, height);
	evas_object_size_hint_min_set(minictrl_obj, width, height);
	evas_object_size_hint_max_set(minictrl_obj, width, height);
}

static void _pause_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *minictrl = data;
	ret_if(minictrl == NULL);

	int state = (int)evas_object_data_get(minictrl, MINICTRL_PRIVATE_KEY_STATE);
	if (state == MINICTRL_STATE_PAUSE) {
		return ;
	}

	if (minicontrol_message_send(minictrl, MINICTRL_EVENT_APP_PAUSE) == 1) {
		_D("minictrl %p is paused", minictrl);
		evas_object_data_set(minictrl, MINICTRL_PRIVATE_KEY_STATE, (void*)MINICTRL_STATE_PAUSE);
	} else {
		_E("failed to pause %p", minictrl);
	}
}

static void _resume_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *minictrl = data;
	ret_if(minictrl == NULL);

	int state = (int)evas_object_data_get(minictrl, MINICTRL_PRIVATE_KEY_STATE);
	if (state == MINICTRL_STATE_RESUME) {
		return ;
	}

	if (minicontrol_message_send(minictrl, MINICTRL_EVENT_APP_RESUME) == 1) {
		_D("minictrl %p is resumed", minictrl);
		evas_object_data_set(minictrl, MINICTRL_PRIVATE_KEY_STATE, (void*)MINICTRL_STATE_RESUME);
	} else {
		_E("failed to resume %p", minictrl);
	}
}

static int _prepare(clock_h clock)
{
	int pid = 0;

	clock_util_provider_launch(clock->pkgname, &pid, clock->configure);
	_W("pid:%d", pid);

	if (pid >= 0) {
		clock->pid = pid;
		return CLOCK_RET_OK;
	}

	return CLOCK_RET_FAIL;
}

static int _config(clock_h clock, int configure)
{
	int pid = 0;

	clock_util_provider_launch(clock->pkgname, &pid, configure);
	if (pid >= 0) {
		if (clock->pid != pid) {
			_E("pid is changed, %d %d", clock->pid, pid);
		}
		clock->pid = pid;
		return CLOCK_RET_OK;
	}

	return CLOCK_RET_FAIL;
}

static int _create(clock_h clock)
{
	Evas_Object *page = NULL;
	Evas_Object *obj = NULL;
	Evas_Object *scroller = _scroller_get();

	if (clock->view) {
		Evas_Object *mini_obj = clock_view_get_item(clock->view);
		if (mini_obj) {
			_size_set(mini_obj, clock->w, clock->h);
			evas_object_show(mini_obj);
		}
		return CLOCK_RET_OK;
	}

	obj = minicontrol_viewer_add(scroller, clock->view_id);
	if (obj != NULL) {
		_size_set(obj, clock->w, clock->h);
		evas_object_show(obj);

		page = clock_view_add(scroller, obj);
		if (page != NULL) {
			clock->view = (void *)page;

			evas_object_smart_callback_add(page, CLOCK_SMART_SIGNAL_PAUSE, _pause_cb, obj);
			evas_object_smart_callback_add(page, CLOCK_SMART_SIGNAL_RESUME, _resume_cb, obj);

			return CLOCK_RET_OK;
		}
	} else {
		_E("Failed to add viewer - %s", clock->view_id);
	}

	return CLOCK_RET_FAIL;
}

static int _attached_cb(clock_h clock)
{
	Evas_Object *page = clock->view;

	int visibility = 1;
	if (clock_service_event_state_get(CLOCK_EVENT_DEVICE_LCD) == CLOCK_EVENT_DEVICE_LCD_OFF) {
		visibility = 0;
	}
	if (clock_service_event_state_get(CLOCK_EVENT_APP) == CLOCK_EVENT_APP_PAUSE) {
		visibility = 0;
	}

	if (visibility == 1) {
		evas_object_smart_callback_call(page, CLOCK_SMART_SIGNAL_RESUME, page_get_item(page));
	} else {
		if (clock->configure != CLOCK_CONF_CLOCK_CONFIGURATION) {
			evas_object_smart_callback_call(page, CLOCK_SMART_SIGNAL_PAUSE, page_get_item(page));
		}
	}

	return CLOCK_RET_OK;
}

static int _destroy(clock_h clock)
{
	Evas_Object *page = clock->view;

	if (page != NULL) {
		evas_object_smart_callback_del(page, CLOCK_SMART_SIGNAL_PAUSE, _pause_cb);
		evas_object_smart_callback_del(page, CLOCK_SMART_SIGNAL_RESUME, _resume_cb);
		page_destroy(page);
		clock->view = NULL;
	}

	clock_util_terminate_clock_by_pid(clock->pid);

	return CLOCK_RET_OK;
}

clock_inf_s clock_inf_minictrl = {
	.async = 1,
	.use_dead_monitor = 1,
	.prepare = _prepare,
	.config = _config,
	.create = _create,
	.attached_cb = _attached_cb,
	.destroy = _destroy,
};

static Eina_Bool _view_ready_idler_cb(void *data)
{
	_D("");

	if (s_info.waiting_timer != NULL) {
		s_info.waiting_timer = NULL;
	}

	clock_h clock = data;
	retv_if(clock == NULL, ECORE_CALLBACK_CANCEL);

	clock_service_event_handler(clock, CLOCK_EVENT_VIEW_READY);

	_D("");

	return ECORE_CALLBACK_CANCEL;
}

HAPI void clock_inf_minictrl_event_hooker(int action, int pid, const char *minictrl_id, int is_realized, int width, int height)
{
	clock_h clock = NULL;
	ret_if(minictrl_id == NULL);

	switch (action) {
		case MINICONTROL_ACTION_START:
			clock = clock_manager_clock_get(CLOCK_CANDIDATE);
			if (clock != NULL) {
				if (clock->pid == pid) {
					_D("candidate clock:%s is now ready", clock->pkgname);

					if (clock->view_id == NULL) {
						clock->view_id = strdup(minictrl_id);
					}
					clock->w = width;
					clock->h = height;

					//ecore_idler_add(_view_ready_idler_cb, clock);
					if (s_info.waiting_timer != NULL) {
						ecore_timer_del(s_info.waiting_timer);
						s_info.waiting_timer = NULL;
					}
					if (is_realized == 1) {
						_W("clock is already realized");
						clock_service_event_handler(clock, CLOCK_EVENT_VIEW_READY);
					} else {
						_W("not realized, waiting 1.0 sec");
						/*
						 * To load minicontrol ASAP
						 */
						 if (clock->app_type != CLOCK_APP_TYPE_WEBAPP) {
						 	_create(clock);
						 }
						s_info.waiting_timer = ecore_timer_add(1.0f, _view_ready_idler_cb, clock);
					}
				} else {
					_E("the clock isn't what we want:%d %s", pid, minictrl_id);
				}
			} else {
				_E("[Exceptional, no candiate clock");
			}
			break;
		case MINICONTROL_ACTION_RESIZE:
			clock = clock_manager_clock_get(CLOCK_ATTACHED);
			if (clock != NULL) {
				if (clock->view_id == NULL) {
					/**
					 * THIS CASE MUST HAS NOT TO BE HAPPEND.
					 * But for the safety, I just handled this too.
					 */
					_E("Attached clock: has no view ID(%s)", clock->pkgname);
					if (clock->pid == pid) {
						_D("But, PID is matched: %d", clock->pid);
						clock->view_id = strdup(minictrl_id);
						if (!clock->view_id) {
							_E("strdup: Fail to strdup - %d", errno);
						} else {
							clock->w = width;
							clock->h = height;

							clock_service_event_handler(clock, CLOCK_EVENT_VIEW_RESIZED);
						}
					}
				} else if (strcmp(clock->view_id, minictrl_id) == 0) {
					_D("Attached clock: %s is updated", clock->pkgname);

					clock->w = width;
					clock->h = height;

					clock_service_event_handler(clock, CLOCK_EVENT_VIEW_RESIZED);
				}
			} else {
				_W("clock: %s isn't attached yet", minictrl_id);
			}
			clock = clock_manager_clock_get(CLOCK_CANDIDATE);
			if (clock != NULL) {
				if (clock->view_id == NULL) {
					if (clock->pid == pid) {
						_D("Clock has no view ID, but PID is matched: %d %s", clock->pid, clock->pkgname);
						clock->view_id = strdup(minictrl_id);
						if (!clock->view_id) {
							_E("Fail to strdup(minictrl_id) - %d", errno);
						} else {
							clock->w = width;
							clock->h = height;
						}
					} else {
						_D("Candidate PID is not matched? %d", clock->pid);
					}
				} else if (strcmp(clock->view_id, minictrl_id) == 0) {
					_D("candidate clock:%s is updated", clock->pkgname);
					clock->w = width;
					clock->h = height;
				}
			}
			break;
		case MINICONTROL_ACTION_STOP:
			break;
		case MINICONTROL_ACTION_REQUEST:
			if (width == MINICONTROL_REQ_FREEZE_SCROLL_VIEWER) {
				clock_service_event_handler(NULL, CLOCK_EVENT_SCROLLER_FREEZE_ON);
			} else if (width == MINICONTROL_REQ_UNFREEZE_SCROLL_VIEWER) {
				clock_service_event_handler(NULL, CLOCK_EVENT_SCROLLER_FREEZE_OFF);
			} else {
				_W("Not a case");
			}
			break;
		default:
			break;
	}
}
