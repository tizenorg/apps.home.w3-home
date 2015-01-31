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
#include <vconf.h>
#include <bundle.h>
#include <efl_assist.h>
#include <dlog.h>
#include <dynamicbox_service.h>
#include <dynamicbox_errno.h>
#include <dynamicbox.h>
#include <aul.h>

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
#include "dbus.h"
#include "dynamicbox_evas.h"
#include "dbox.h"
#include "clock_service.h"

#define DEFAULT_INIT_REFRESH 5
#define DEFAULT_INIT_TIMER 2.5f
#define TAG_FORCE "c.f"		// Clock Force
#define TAG_REFRESH "c.r"	// Clock Refresh
#define TAG_RETRY "c.t"		// Clock reTry

static struct info {
	int initialized; /* Initialize the event callback */
	Eina_List *pkg_list; /* Deleted package list */
	Eina_List *create_list;
	Eina_List *freeze_list;
	Evas_Object *first_clock;
} s_info = {
	.initialized = 0,
	.pkg_list = NULL,
	.create_list = NULL,
	.freeze_list = NULL,
	.first_clock = NULL,
};

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

static void pumping_clock(Evas_Object *obj)
{
	clock_h clock;

	clock = clock_manager_clock_get(CLOCK_CANDIDATE);
	if (clock && clock->interface == CLOCK_INF_DBOX && clock->state == CLOCK_STATE_WAITING) {
		clock_service_event_handler(clock, CLOCK_EVENT_VIEW_READY);
	} else {
		/**
		 * Clock is created, but it is not my favor,
		 * Destroy it
		 */
		s_info.create_list = eina_list_remove(s_info.create_list, obj);
		evas_object_del(obj);
	}
}

static Eina_Bool force_updated_cb(void *data)
{
	Evas_Object *obj = data;

	evas_object_data_del(obj, TAG_REFRESH);
	evas_object_data_del(obj, TAG_FORCE);

	dbox_set_update_callback(obj, NULL);

	pumping_clock(obj);

	return ECORE_CALLBACK_CANCEL;
}

static int updated_cb(Evas_Object *obj)
{
	int refresh;

	refresh = (int)evas_object_data_get(obj, TAG_REFRESH);
	refresh--;

	if (refresh <= 0) {
		Ecore_Timer *force_timer;

		evas_object_data_del(obj, TAG_REFRESH);
		force_timer = evas_object_data_del(obj, TAG_FORCE);
		if (force_timer) {
			ecore_timer_del(force_timer);
		}

		pumping_clock(obj);
		return ECORE_CALLBACK_CANCEL;
	}

	evas_object_data_set(obj, TAG_REFRESH, (void *)refresh);
	return ECORE_CALLBACK_RENEW;
}

static int scroll_cb(Evas_Object *obj, int hold)
{
	_W("Scroll control: %s", hold ? "hold" : "release");

	if (hold) {
		clock_service_event_handler(NULL, CLOCK_EVENT_SCROLLER_FREEZE_ON);
	} else {
		clock_service_event_handler(NULL, CLOCK_EVENT_SCROLLER_FREEZE_OFF);
	}

	return ECORE_CALLBACK_RENEW;
}

static void user_create_cb(struct dynamicbox_evas_raw_event_info *info, void *data)
{
	Eina_List *l;
	Eina_List *n;
	Evas_Object *obj;
	const char *dbox_id;

	EINA_LIST_FOREACH_SAFE(s_info.create_list, l, n, obj) {
		dbox_id = evas_object_dynamicbox_dbox_id(obj);
		if (!dbox_id) {
			/* This is not possible */
			continue;
		}

		if (strcmp(dbox_id, info->pkgname)) {
			continue;
		}

		if (info->error != DBOX_STATUS_ERROR_NONE) {
			/* Failed to create a new clock */
			/* TODO: Feeds fault event to clock service */
			_E("Failed to create a new clock: %s (%x)", info->pkgname, info->error);
			s_info.create_list = eina_list_remove(s_info.create_list, obj);
			evas_object_del(obj);
		} else {
			evas_object_dynamicbox_resume(obj);
		}

		break;
	}
}

static char *remove_from_pkglist(const char *pkgname)
{
	char *item;
	Eina_List *l;
	Eina_List *n;

	if (!pkgname) {
		return NULL;
	}

	item = NULL;
	EINA_LIST_FOREACH_SAFE(s_info.pkg_list, l, n, item) {
		if (item && !strcmp(item, pkgname)) {
			s_info.pkg_list = eina_list_remove(s_info.pkg_list, item);
			_D("Manually cleared from package list (%s)", item);
			break;
		}
		item = NULL;
	}

	return item;
}

static void user_del_cb(struct dynamicbox_evas_raw_event_info *info, void *data)
{
	clock_h clock;
	char *pkgname;

	clock = clock_manager_clock_get(CLOCK_CANDIDATE);

	if (info->error == DBOX_STATUS_ERROR_FAULT && !clock) {
		const char *dbox_id = NULL;

		if (info->dynamicbox) {
			dbox_id = evas_object_dynamicbox_dbox_id(info->dynamicbox);
		}
		_D("Faulted: %s, Current: %s", info->pkgname, dbox_id);

		clock = clock_manager_clock_get(CLOCK_ATTACHED);
		if (clock) {
			if (clock->view_id && dbox_id && !strcmp(dbox_id, clock->view_id)) {
				int retry;

				retry = (int)evas_object_data_get(info->dynamicbox, TAG_RETRY);
				retry--;
				if (retry <= 0) {
					// No more recovery count remained
					clock_service_event_handler(clock, CLOCK_EVENT_APP_PROVIDER_ERROR_FATAL);
				} else {
					evas_object_dynamicbox_activate(info->dynamicbox);
					evas_object_data_set(info->dynamicbox, TAG_RETRY, (void *)retry);
					_D("There is no waiting clock. Try to recover from fault (%d)", retry);
				}
				return;
			}

			pkgname = remove_from_pkglist(info->pkgname);
			if (!pkgname) {
				_D("Unknown Box");
			} else {
				free(pkgname);
			}
		} else {
			_E("There is no attached clock");
		}

		return;
	}

	_D("%s is deleted", info->pkgname);
	pkgname = remove_from_pkglist(info->pkgname);
	if (!pkgname) {
		return;
	}

	if (clock && clock->interface == CLOCK_INF_DBOX && clock->state == CLOCK_STATE_WAITING) {
		if (clock->view_id && !strcmp(pkgname, clock->view_id)) {
			clock_service_event_handler(clock, CLOCK_EVENT_VIEW_READY);
		}
	}

	free(pkgname);
}

static void del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_D("Remove from freeze list");
	s_info.freeze_list = eina_list_remove(s_info.freeze_list, obj);
}

static w_home_error_e home_resumed_cb(void *data)
{
	Evas_Object *obj;

	_D("Thaw all freezed objects");
	EINA_LIST_FREE(s_info.freeze_list, obj) {
		evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL, del_cb);
		evas_object_dynamicbox_thaw_visibility(obj);
	}

	return W_HOME_ERROR_NONE;
}

static int _prepare(clock_h clock)
{
	char *dbox_id = dynamicbox_service_dbox_id(clock->pkgname);
	Eina_List *l;
	char *pkgname;
	int ret = CLOCK_RET_OK;

	retv_if(dbox_id == NULL, CLOCK_RET_FAIL);

	clock->view_id = dbox_id;

	if (!s_info.initialized) {
		/* When could we delete these callbacks? */
		if (main_register_cb(APP_STATE_RESUME, home_resumed_cb, NULL) != W_HOME_ERROR_NONE) {
			_E("Unable to register app state change callback");
		}
		evas_object_dynamicbox_set_raw_event_callback(DYNAMICBOX_EVAS_RAW_DELETE, user_del_cb, NULL);
		evas_object_dynamicbox_set_raw_event_callback(DYNAMICBOX_EVAS_RAW_CREATE, user_create_cb, NULL);
		s_info.initialized = 1;
	}

	/**
	 * If the previous one is already installed,
	 * Wait for destroying previous one first.
	 */
	EINA_LIST_FOREACH(s_info.pkg_list, l, pkgname) {
		if (!strcmp(pkgname, clock->view_id)) {
			ret = CLOCK_RET_ASYNC;
			clock_h clock_attached = clock_manager_clock_get(CLOCK_ATTACHED);
			if (clock_attached) {
				if (clock_attached->interface == CLOCK_INF_DBOX) {
					_D("Need detroying previous one");
					ret |= CLOCK_RET_NEED_DESTROY_PREVIOUS;
				}
			}
			_D("Async loading enabled");
			break;
		}
	}

	if (ret == CLOCK_RET_OK) {
		bundle *b;
		clock_h clock_attached;
		int pid = -1;

		/**
		 * Launch a clock process first to save the preparation time for 3D clock
		 */
		b = bundle_create();
		if (b) {
			bundle_add(b, "__APP_SVC_OP_TYPE__", APP_CONTROL_OPERATION_MAIN);
			clock_util_setting_conf_bundle_add(b, clock->configure);

			/**
			 * Add more bundles to prepare setup screen of style clock
			 */
			home_dbus_cpu_booster_signal_send();
			pid = aul_launch_app(clock->view_id, b);
			if (pid < 0) {
				_E("Failed to do pre-launch for clock %s", clock->view_id);
				/**
				 * Even if we failed to launch a clock,
				 * The master will launch it for us.
				 * So from here, we don't need to care regarding this failure
				 */
			} else {
				_D("Clock pre-launched: %s", clock->view_id);
			}

			bundle_free(b);
		}

		clock_attached = clock_manager_clock_get(CLOCK_ATTACHED);
		if (clock_attached /* && clock_attached->interface == CLOCK_INF_DBOX */) {
			Evas_Object *scroller;
			/**
			 * Try to create a new box first.
			 * If it is created, then replace it with the old clock.
			 * If we fails to create a new clock from here,
			 * Take normal path.
			 *
			 * @TODO
			 * Do I have to care the previously create clock here? is it possible?
			 */
			scroller = _scroller_get();
			if (scroller) {
				Evas_Object *obj = NULL;

				if (s_info.first_clock) {
					const char *first_dbox_id;
					first_dbox_id = evas_object_dynamicbox_dbox_id(s_info.first_clock);
					if (!first_dbox_id || strcmp(clock->view_id, first_dbox_id)) {
						_D("LBID is not matched: %s", first_dbox_id);
						evas_object_del(s_info.first_clock);
					} else {
						_D("Use the first clock");
						obj = s_info.first_clock;
					}
					s_info.first_clock = NULL;
				}
				if (!obj) {
					const char *content;
					content = clock_util_setting_conf_content(clock->configure);
					obj = dbox_create(scroller, clock->view_id, content, DYNAMICBOX_EVAS_DEFAULT_PERIOD);
					if (obj) {
						Ecore_Timer *force_refresh_timer;

						if (content) {
							evas_object_dynamicbox_freeze_visibility(obj, DBOX_SHOW);
						}
						evas_object_data_set(obj, TAG_REFRESH, (void *)DEFAULT_INIT_REFRESH);
						evas_object_data_set(obj, TAG_RETRY, (void *)clock_service_get_retry_count());

						dbox_set_update_callback(obj, updated_cb);
						dbox_set_scroll_callback(obj, scroll_cb);

						force_refresh_timer = ecore_timer_add(DEFAULT_INIT_TIMER, force_updated_cb, obj);
						if (!force_refresh_timer) {
							_E("Failed to create refresh timer\n");
						}
						evas_object_data_set(obj, TAG_FORCE, force_refresh_timer);
						evas_object_dynamicbox_disable_preview(obj);
						evas_object_dynamicbox_disable_loading(obj);
						evas_object_resize(obj, main_get_info()->root_w, main_get_info()->root_h);
						evas_object_size_hint_min_set(obj, main_get_info()->root_w, main_get_info()->root_h);
						evas_object_show(obj);
					}
				}
				if (obj) {
					/* Move this to out of screen */
					evas_object_move(obj, main_get_info()->root_w, main_get_info()->root_h);

					s_info.create_list = eina_list_append(s_info.create_list, obj);
					ret = CLOCK_RET_ASYNC;
				} else {
					/* Failed to create a clock */
					if (pid > 0) {
						_E("Terminate: %d", pid);
						aul_terminate_pid(pid);
					}
				}
			} else {
				if (pid > 0) {
					_E("Terminate: %d", pid);
					aul_terminate_pid(pid);
				}
			}
		}
	}

	_D("Prepared: %s", clock->view_id);
	return ret;
}

static int _config(clock_h clock, int configure)
{
	int pid = 0;
	int ret;

	ret = dynamicbox_service_trigger_update(clock->view_id, NULL, NULL, NULL, clock_util_setting_conf_content(configure), 1);
	if (ret == DBOX_STATUS_ERROR_NONE) {
		_D("Trigger update with content string");
	}

	clock_util_provider_launch(clock->view_id, &pid, configure);
	if (pid >= 0) {
		_E("configured, %d", pid);
		return CLOCK_RET_OK;
	}

	return CLOCK_RET_FAIL;
}

static int _create(clock_h clock)
{
	int ret = CLOCK_RET_FAIL;
	Evas_Object *obj = NULL;
	Evas_Object *scroller;
	const char *dbox_id;
	Eina_List *l;
	Eina_List *n;

	scroller = _scroller_get();
	if (!scroller) {
		_E("Failed to get current scroller");
		return CLOCK_RET_FAIL;
	}

	obj = NULL;
	EINA_LIST_FOREACH_SAFE(s_info.create_list, l, n, obj) {
		dbox_id = evas_object_dynamicbox_dbox_id(obj);
		if (dbox_id) {
			if (!strcmp(dbox_id, clock->view_id)) {
				_D("Prepared clock found (%s)", dbox_id);
				s_info.create_list = eina_list_remove(s_info.create_list, obj);
				break;
			}
		}

		obj = NULL;
	}

	/**
	 * In this case, try to a new clock from here again.
	 * There is something problem, but we have to care this too.
	 */
	if (!obj) {
		_D("Prepare stage is skipped");
		if (s_info.first_clock) {
			const char *first_dbox_id;
			first_dbox_id = evas_object_dynamicbox_dbox_id(s_info.first_clock);
			if (!first_dbox_id || strcmp(clock->view_id, first_dbox_id)) {
				_D("LBID is not matched: %s", first_dbox_id);
				evas_object_del(s_info.first_clock);
			} else {
				_D("Use the first clock");
				obj = s_info.first_clock;
			}
			s_info.first_clock = NULL;
		}

		if (!obj) {
			const char *content;
			content = clock_util_setting_conf_content(clock->configure);
			obj = dbox_create(scroller, clock->view_id, content, DYNAMICBOX_EVAS_DEFAULT_PERIOD);
			if (obj) {
				if (content) {
					evas_object_dynamicbox_freeze_visibility(obj, DBOX_SHOW);
				}
				evas_object_data_set(obj, TAG_RETRY, (void *)clock_service_get_retry_count());
				dbox_set_scroll_callback(obj, scroll_cb);
				evas_object_dynamicbox_disable_preview(obj);
				evas_object_dynamicbox_disable_loading(obj);
				evas_object_resize(obj, main_get_info()->root_w, main_get_info()->root_h);
				evas_object_size_hint_min_set(obj, main_get_info()->root_w, main_get_info()->root_h);
				evas_object_show(obj);
			}
		}

		if (obj) {
			evas_object_move(obj, 0, 0);
		}
	}

	if (obj != NULL) {
		Evas_Object *page;

		_D("Create clock: %s", clock->view_id);
		page = clock_view_add(scroller, obj);
		if (page != NULL) {
			char *pkgname;

			clock->view = (void *)page;
			ret = CLOCK_RET_OK;

			pkgname = strdup(clock->view_id);
			if (pkgname) {
				s_info.pkg_list = eina_list_append(s_info.pkg_list, pkgname);
			} else {
				_E("strdup: %s\n", strerror(errno));
			}

			if (evas_object_dynamicbox_visibility_is_freezed(obj)) {
				if (main_get_info()->state == APP_STATE_RESUME) {
					_D("Thaw freezed object: %s", pkgname);
					evas_object_dynamicbox_thaw_visibility(obj);
				} else {
					_D("Push freezed object: %s", pkgname);
					evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, del_cb, NULL);
					s_info.freeze_list = eina_list_append(s_info.freeze_list, obj);
				}
			}
		} else {
			evas_object_del(obj);
		}
	} else {
		_E("Failed to add clock - %s", clock->view_id);
	}

	return ret;
}

static int _destroy(clock_h clock)
{
	Evas_Object *page;
	Evas_Object *item;

	page = clock->view;
	if (!page) {
		_E("Clock doesn't have a view");
		return CLOCK_RET_FAIL;
	}

	_D("Delete clock: %s", clock->view_id);
	item = clock_view_get_item(page);
	if (item) {
		if (evas_object_dynamicbox_is_faulted(item)) {
			char *pkgname;

			_D("Faulted box: Clean up pkg_list manually");
			pkgname = remove_from_pkglist(clock->view_id);
			if (pkgname) {
				_D("Faulted box(%s) is removed", pkgname);
				free(pkgname);
			}
		}
	} else {
		_D("Item is not exists");
	}

	page_destroy(page);

	return CLOCK_RET_OK;
}

clock_inf_s clock_inf_dbox = {
	.async = 0,
	.use_dead_monitor = 0,
	.prepare = _prepare,
	.config = _config,
	.create = _create,
	.destroy = _destroy,
};

int clock_inf_prepare_first_clock(const char *pkgname)
{
	char *dbox_id;
	Evas_Object *scroller;

	if (!pkgname) {
		_D("pkgname is NULL");
		return CLOCK_RET_OK;
	}

	if (s_info.first_clock) {
		// Already created
		_D("Already created");
		return CLOCK_RET_OK;
	}

	scroller = _scroller_get();
	if (!scroller) {
		_D("Scroller is NIL");
		return CLOCK_RET_FAIL;
	}

	dbox_id = dynamicbox_service_dbox_id(pkgname);
	if (!dbox_id) {
		_D("Dynamicbox Package id is not valid: %s", pkgname);
		return CLOCK_RET_OK;
	}

	s_info.first_clock = dbox_create(scroller, dbox_id, NULL, DYNAMICBOX_EVAS_DEFAULT_PERIOD);
	free(dbox_id);
	if (!s_info.first_clock) {
		_E("Failed to create a dbox (%s)", dbox_id);
		return CLOCK_RET_FAIL;
	}

	//evas_object_dynamicbox_disable_preview(s_info.first_clock);
	dbox_set_scroll_callback(s_info.first_clock, scroll_cb);
	evas_object_data_set(s_info.first_clock, TAG_RETRY, (void *)clock_service_get_retry_count());
	evas_object_dynamicbox_disable_overlay_text(s_info.first_clock);
	evas_object_move(s_info.first_clock, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_resize(s_info.first_clock, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_size_hint_min_set(s_info.first_clock, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_show(s_info.first_clock);
	_D("First clock is prepared");
	return CLOCK_RET_OK;
}

/* End of a file */
