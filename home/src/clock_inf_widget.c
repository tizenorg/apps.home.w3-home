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
#include <dlog.h>
#include <widget_service.h>
#include <widget_service_internal.h>
#include <widget_errno.h>
#include <widget.h>
#include <aul.h>
#include <app.h>
#include <widget_viewer_evas.h>
#include <widget_viewer_evas_internal.h>

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
#include "widget.h"
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
	if (clock && clock->interface == CLOCK_INF_WIDGET && clock->state == CLOCK_STATE_WAITING) {
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

	widget_set_update_callback(obj, NULL);

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

static void user_create_cb(struct widget_evas_raw_event_info *info, void *data)
{
	Eina_List *l;
	Eina_List *n;
	Evas_Object *obj;
	const char *widget_id;

	EINA_LIST_FOREACH_SAFE(s_info.create_list, l, n, obj) {
		widget_id = widget_viewer_evas_get_widget_id(obj);
		if (!widget_id) {
			/* This is not possible */
			continue;
		}

		if (strcmp(widget_id, info->pkgname)) {
			continue;
		}

		if (info->error != WIDGET_ERROR_NONE) {
			/* Failed to create a new clock */
			/* TODO: Feeds fault event to clock service */
			_E("Failed to create a new clock: %s (%x)", info->pkgname, info->error);
			s_info.create_list = eina_list_remove(s_info.create_list, obj);
			evas_object_del(obj);
		} else {
			widget_viewer_evas_resume_widget(obj);
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

static void user_del_cb(struct widget_evas_raw_event_info *info, void *data)
{
	clock_h clock;
	char *pkgname;

	clock = clock_manager_clock_get(CLOCK_CANDIDATE);

	if (info->error == WIDGET_ERROR_FAULT && !clock) {
		const char *widget_id = NULL;

		if (info->widget) {
			widget_id = widget_viewer_evas_get_widget_id(info->widget);
		}
		_D("Faulted: %s, Current: %s", info->pkgname, widget_id);

		clock = clock_manager_clock_get(CLOCK_ATTACHED);
		if (clock) {
			if (clock->view_id && widget_id && !strcmp(widget_id, clock->view_id)) {
				int retry;

				retry = (int)evas_object_data_get(info->widget, TAG_RETRY);
				retry--;
				if (retry <= 0) {
					// No more recovery count remained
					clock_service_event_handler(clock, CLOCK_EVENT_APP_PROVIDER_ERROR_FATAL);
				} else {
					widget_viewer_evas_activate_faulted_widget(info->widget);
					evas_object_data_set(info->widget, TAG_RETRY, (void *)retry);
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

	if (clock && clock->interface == CLOCK_INF_WIDGET && clock->state == CLOCK_STATE_WAITING) {
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
		widget_viewer_evas_thaw_visibility(obj);
	}

	return W_HOME_ERROR_NONE;
}

static int _prepare(clock_h clock)
{
	char *widget_id = widget_service_get_widget_id(clock->pkgname);
	Eina_List *l;
	char *pkgname;
	int ret = CLOCK_RET_OK;

	retv_if(widget_id == NULL, CLOCK_RET_FAIL);

	clock->view_id = widget_id;

	if (!s_info.initialized) {
		/* When could we delete these callbacks? */
		if (main_register_cb(APP_STATE_RESUME, home_resumed_cb, NULL) != W_HOME_ERROR_NONE) {
			_E("Unable to register app state change callback");
		}
		widget_viewer_evas_set_raw_event_callback(WIDGET_VIEWER_EVAS_RAW_DELETE, user_del_cb, NULL);
		widget_viewer_evas_set_raw_event_callback(WIDGET_VIEWER_EVAS_RAW_CREATE, user_create_cb, NULL);
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
				if (clock_attached->interface == CLOCK_INF_WIDGET) {
					_D("Need detroying previous one");
					ret |= CLOCK_RET_NEED_DESTROY_PREVIOUS;
				}
			}
			_D("Async loading enabled");
			break;
		}
	}

	if (ret == CLOCK_RET_OK) {
		clock_h clock_attached;

		/**
		 * Launch a clock process first to save the preparation time for 3D clock
		 */
		clock_attached = clock_manager_clock_get(CLOCK_ATTACHED);
		if (clock_attached /* && clock_attached->interface == CLOCK_INF_WIDGET */) {
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
					const char *first_widget_id;
					first_widget_id = widget_viewer_evas_get_widget_id(s_info.first_clock);
					if (!first_widget_id || strcmp(clock->view_id, first_widget_id)) {
						_D("WIDGET ID is not matched: %s", first_widget_id);
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
					LOGD("Create Widget: %s\n", clock->view_id);
					obj = widget_create(scroller, clock->view_id, content, WIDGET_VIEWER_EVAS_DEFAULT_PERIOD);
					if (obj) {
						Ecore_Timer *force_refresh_timer;

						if (content) {
							widget_viewer_evas_freeze_visibility(obj, WIDGET_VISIBILITY_STATUS_SHOW_FIXED);
						}
						evas_object_data_set(obj, TAG_REFRESH, (void *)DEFAULT_INIT_REFRESH);
						evas_object_data_set(obj, TAG_RETRY, (void *)clock_service_get_retry_count());

						widget_set_update_callback(obj, updated_cb);
						widget_set_scroll_callback(obj, scroll_cb);

						force_refresh_timer = ecore_timer_add(DEFAULT_INIT_TIMER, force_updated_cb, obj);
						if (!force_refresh_timer) {
							_E("Failed to create refresh timer\n");
						}

						evas_object_data_set(obj, TAG_FORCE, force_refresh_timer);
						widget_viewer_evas_disable_preview(obj);
						widget_viewer_evas_disable_loading(obj);
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
				}
			}
		}
	}

	_D("Prepared: %s", clock->view_id);
	return ret;
}

static int _create(clock_h clock)
{
	int ret = CLOCK_RET_FAIL;
	Evas_Object *obj = NULL;
	Evas_Object *scroller;
	const char *widget_id;
	Eina_List *l;
	Eina_List *n;

	scroller = _scroller_get();
	if (!scroller) {
		_E("Failed to get current scroller");
		return CLOCK_RET_FAIL;
	}

	obj = NULL;
	EINA_LIST_FOREACH_SAFE(s_info.create_list, l, n, obj) {
		widget_id = widget_viewer_evas_get_widget_id(obj);
		if (widget_id) {
			if (!strcmp(widget_id, clock->view_id)) {
				_D("Prepared clock found (%s)", widget_id);
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
			const char *first_widget_id;
			first_widget_id = widget_viewer_evas_get_widget_id(s_info.first_clock);
			if (!first_widget_id || strcmp(clock->view_id, first_widget_id)) {
				_D("LBID is not matched: %s", first_widget_id);
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
			obj = widget_create(scroller, clock->view_id, content, WIDGET_VIEWER_EVAS_DEFAULT_PERIOD);
			if (obj) {
				if (content) {
					widget_viewer_evas_freeze_visibility(obj, WIDGET_VISIBILITY_STATUS_SHOW_FIXED);
				}
				evas_object_data_set(obj, TAG_RETRY, (void *)clock_service_get_retry_count());
				widget_set_scroll_callback(obj, scroll_cb);
				widget_viewer_evas_disable_preview(obj);
				widget_viewer_evas_disable_loading(obj);
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
				_E("Fail to strdup(pkgname) - %d", errno);
			}

			if (widget_viewer_evas_is_visibility_frozen(obj)) {
				if (main_get_info()->state == APP_STATE_RESUME) {
					_D("Thaw freezed object: %s", pkgname);
					widget_viewer_evas_thaw_visibility(obj);
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
		if (widget_viewer_evas_is_faulted(item)) {
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

clock_inf_s clock_inf_widget = {
	.async = 0,
	.use_dead_monitor = 0,
	.prepare = _prepare,
	.config = NULL,
	.create = _create,
	.destroy = _destroy,
};

/* End of a file */
