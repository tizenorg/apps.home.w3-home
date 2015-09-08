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
#include <widget_service.h>
#include <widget_service_internal.h>
#include <widget_errno.h>
#include <stdbool.h>
#include <dlog.h>
#include <bundle.h>
#include <efl_assist.h>
#include <Ecore_X.h>
#include <widget.h>
#include <vconf.h>
#include <app_preference.h>
#include <widget_viewer_evas_internal.h> /* feed_access_event */
#include <widget_viewer_evas.h>

/**
 * WIDGET_ACCESS_STATUS_XXX
 */
#include <widget_service_internal.h>

#include "util.h"
#include "db.h"
#include "index.h"
#include "layout_info.h"
#include "log.h"
#include "main.h"
#include "page_info.h"
#include "scroller_info.h"
#include "scroller.h"
#include "page.h"
#include "edit.h"
#include "add-viewer_package.h"
#include "add-viewer_pkgmgr.h"


static struct info {
	Eina_Bool is_scrolling;
	Eina_List *pended_event_list;
} s_info = {
	.is_scrolling = EINA_FALSE,
	.pended_event_list = NULL,
};



#define MAX_TTL 10
#define TAG_SCROLL       "l.s" // Scroller
#define TAG_UPDATED      "l.u" // Widget Updated
#define TAG_FAULT_HL     "f.h" // Faulted Highlight (emulate)
#define FAULTED_HL_TIMER 0.00001f



struct pended_access_event {
	Evas_Object *widget;
	Elm_Access_Action_Info info;
	void (*cb)(Evas_Object *obj, int ret, void *data);
	void *data;
	int ttl;
};



static const char *action_type_string(int type)
{
	switch (type) {
	case ELM_ACCESS_ACTION_HIGHLIGHT:
		return "HL";
	case ELM_ACCESS_ACTION_UNHIGHLIGHT:
		return "UNHL";
	case ELM_ACCESS_ACTION_HIGHLIGHT_NEXT:
		return "NEXT";
	case ELM_ACCESS_ACTION_HIGHLIGHT_PREV:
		return "PREV";
	case ELM_ACCESS_ACTION_VALUE_CHANGE:
		return "VALUE_CHANGE";
	case ELM_ACCESS_ACTION_MOUSE:
		return "MOUSE";
	case ELM_ACCESS_ACTION_BACK:
		return "BACK";
	case ELM_ACCESS_ACTION_OVER:
		return "OVER";
	case ELM_ACCESS_ACTION_READ:
		return "READ";
	case ELM_ACCESS_ACTION_ENABLE:
		return "ENABLE";
	case ELM_ACCESS_ACTION_DISABLE:
		return "DISABLE";
	case ELM_ACCESS_ACTION_ACTIVATE:
		return "ACTIVATE";
	case ELM_ACCESS_ACTION_SCROLL:
		return "SCROLL";
	default:
		return "Unknown";
	}
}

static void push_pended_event_list(Evas_Object *obj, Elm_Access_Action_Info *info, void (*cb)(Evas_Object *obj, int ret, void *data), void *data)
{
	struct pended_access_event *_info;

	_info = malloc(sizeof(*_info));
	if (!_info) {
		_E("malloc:%s\n", strerror(errno));
		return;
	}

	memcpy(&_info->info, info, sizeof(*info));
	_info->widget = obj;
	_info->cb = cb;
	_info->data = data;
	_info->ttl = MAX_TTL;

	s_info.pended_event_list = eina_list_append(s_info.pended_event_list, _info);
}



static void try_again_send_event(struct pended_access_event *info)
{
	if (info->ttl < 0) {
		_E("Event TTL reach to the end");
		free(info);
		return;
	}

	s_info.pended_event_list = eina_list_prepend(s_info.pended_event_list, info);
}



static struct pended_access_event *pop_pended_event_list(void)
{
	struct pended_access_event *info;

	info = eina_list_nth(s_info.pended_event_list, 0);
	if (info) {
		s_info.pended_event_list = eina_list_remove(s_info.pended_event_list, info);
		info->ttl--;
	}

	return info;
}



static void widget_pended_event_consumer(Evas_Object *obj, int ret, void *data)
{
	struct pended_access_event *info;

	info = pop_pended_event_list();
	if (info) {
		int ret;
		ret = widget_viewer_evas_feed_access_event(info->widget, info->info.action_type, &info->info, info->cb, info->data);
		if (ret == WIDGET_ERROR_RESOURCE_BUSY) {
			_E("Failed to send pended event (%s), %X", action_type_string(info->info.action_type), ret);
			try_again_send_event(info);
		} else {
			free(info);
		}
	}
}



static void send_access_event(Evas_Object *widget, Elm_Access_Action_Info *info, int must_be_sent, void (*cb)(Evas_Object *obj, int ret, void *data), void *data)
{
	int ret;

	ret = widget_viewer_evas_feed_access_event(widget, info->action_type, info, cb, data);
	if (ret == WIDGET_ERROR_RESOURCE_BUSY && must_be_sent) {
		_D("Pending event (%s)", action_type_string(info->action_type));
		push_pended_event_list(widget, info, cb, data);
	}
}



static void del_cb(void *data, Evas *e, Evas_Object *widget, void *event_info)
{
	const char *widget_id;
	struct pended_access_event *info;
	Eina_List *l;
	Eina_List *n;

	EINA_LIST_FOREACH_SAFE(s_info.pended_event_list, l, n, info) {
		if (info->widget == widget) {
			s_info.pended_event_list = eina_list_remove(s_info.pended_event_list, info);
			free(info);
		}
	}

	widget_id = widget_viewer_evas_get_widget_id(widget);
	if (widget_id) {
		struct add_viewer_package *pkginfo;

		pkginfo = add_viewer_package_find(widget_id);
		if (!pkginfo) {
			_E("Add viewer has no info: %s", widget_id);
		} else {
			add_viewer_package_set_skip(pkginfo, 0);
		}
	} else {
		_E("Has no widget_id?");
	}
}



static void _widget_created_cb(void *data, Evas_Object *obj, void *event_info)
{
}



static void _widget_updated_cb(void *data, Evas_Object *obj, void *event_info)
{
	int (*updated)(Evas_Object *obj);

	updated = evas_object_data_get(obj, TAG_UPDATED);
	if (!updated) {
		return;
	}

	if (updated(obj) == ECORE_CALLBACK_CANCEL) {
		evas_object_data_del(obj, TAG_UPDATED);
	}
}



static void _widget_control_scroll_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct widget_evas_event_info *ev = event_info;
	int (*scroll)(Evas_Object *obj, int hold);

	scroll = evas_object_data_get(obj, TAG_SCROLL);
	if (!scroll) {
		return;
	}

	if (scroll(obj, (ev->error == WIDGET_ERROR_NONE && ev->event == WIDGET_EVENT_HOLD_SCROLL)) == ECORE_CALLBACK_CANCEL) {
		evas_object_data_del(obj, TAG_SCROLL);
	}
}



HAPI Evas_Object *widget_create(Evas_Object *parent, const char *id, const char *subid, double period)
{
	Evas_Object *widget = NULL;
	char *pkgname = NULL;
	struct add_viewer_package *pkginfo;

	retv_if(!id, NULL);

	pkgname = widget_service_get_widget_id(id);
	if (pkgname) {
		free(pkgname);
	} else {
		_D("%s is not installed in the pkgmgr DB", id);
		return NULL;
	}

	widget = widget_viewer_evas_add_widget(parent, id, subid, period);
	retv_if(!widget, NULL);

	pkginfo = add_viewer_package_find(id);
	if (!pkginfo) {
		_E("add-viewer info none: %s", id);
	} else {
		if (add_viewer_package_is_skipped(pkginfo)) {
			_E("Package marked as skip");
		}

		add_viewer_package_set_skip(pkginfo, 1);
		_D("Mark added: %s", id);
	}

	evas_object_event_callback_add(widget, EVAS_CALLBACK_DEL, del_cb, NULL);
	evas_object_smart_callback_add(widget, WIDGET_SMART_SIGNAL_WIDGET_CREATED, _widget_created_cb, NULL);
	evas_object_smart_callback_add(widget, WIDGET_SMART_SIGNAL_UPDATED, _widget_updated_cb, NULL);
	evas_object_smart_callback_add(widget, WIDGET_SMART_SIGNAL_CONTROL_SCROLLER, _widget_control_scroll_cb, NULL);
	return widget;
}



HAPI void widget_destroy(Evas_Object *widget)
{
	ret_if(!widget);

	evas_object_smart_callback_del(widget, WIDGET_SMART_SIGNAL_WIDGET_CREATED, _widget_created_cb);
	evas_object_smart_callback_del(widget, WIDGET_SMART_SIGNAL_UPDATED, _widget_updated_cb);
	evas_object_smart_callback_del(widget, WIDGET_SMART_SIGNAL_CONTROL_SCROLLER, _widget_control_scroll_cb);
	evas_object_del(widget);
}



/* The content is updated */
static void _widget_extra_updated_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *page = data;
	layout_info_s *layout_info = NULL;
	scroller_info_s *scroller_info = NULL;
	page_info_s *page_info = NULL;
	const char *content_info = NULL;

	int center_index = 0;
	int page_index = 0;
	int ordering;

	ret_if(!page);

	_D("Widget is updated");

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);
	ret_if(!page_info->item);

	layout_info = evas_object_data_get(page_info->layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	content_info = widget_viewer_evas_get_content_info(page_info->item);
	ret_if(!content_info);

	free(page_info->subid);
	page_info->subid = strdup(content_info);
	ret_if(!page_info->subid);

	/* do not need to update DB on the edit mode */
	if (layout_info->edit) return;

	scroller_info = evas_object_data_get(page_info->scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);

	center_index = scroller_seek_page_position(page_info->scroller, scroller_info->center);
	page_index = scroller_seek_page_position(page_info->scroller, page);
	ordering = page_index - center_index - 1;

	db_update_item_by_ordering(page_info->id, content_info, ordering);

	_SD("Widget is updated to [%s:%s:%d]", page_info->id, content_info, ordering);
}



static void _widget_remove(Evas_Object *page)
{
	Evas_Object *proxy_page = NULL;
	Evas_Object *page_current = NULL;
	Evas_Object *scroller = NULL;
	page_info_s *page_info = NULL;

	/* We have to delete a proxy page on the edit mode */
	proxy_page = evas_object_data_get(page, DATA_KEY_PROXY_PAGE);
	if (proxy_page) {
		scroller_info_s *scroller_info = NULL;
		page_info_s *proxy_page_info = NULL;

		proxy_page_info = evas_object_data_get(proxy_page, DATA_KEY_PAGE_INFO);
		ret_if(!proxy_page_info);

		scroller = proxy_page_info->scroller;
		ret_if(!scroller);

		scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
		ret_if(!scroller_info);
		ret_if(!scroller_info->parent);

		scroller_pop_page(proxy_page_info->scroller, proxy_page);
		edit_destroy_proxy_page(proxy_page);

		page_current = scroller_get_focused_page(scroller);
		ret_if(!page_current);

		index_bring_in_page(scroller_info->index[PAGE_DIRECTION_RIGHT], page_current);
		edit_change_focus(scroller, page_current);
		edit_arrange_plus_page(scroller_info->parent);
	}

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	_D("Widget(%s) is removed", page_info->id);

	scroller = page_info->scroller;
	ret_if(!scroller);

	scroller_pop_page(page_info->scroller, page);
	page_destroy(page);
	if (!main_get_info()->is_tts) {
		page_arrange_plus_page(scroller, 0);
	}
}



static void _widget_create_aborted_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct widget_evas_event_info *ev = event_info;
	Evas_Object *page = data;
	Eina_List *page_info_list = NULL;
	layout_info_s *layout_info = NULL;
	page_info_s *page_info = NULL;

	ret_if(!page);

	_D("Widget is aborted to create");

	/* We have to get the page_info before removing it */
	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	layout_info = evas_object_data_get(page_info->layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	if (ev->error == WIDGET_ERROR_ALREADY_EXIST) {
		_SW("Already exists %s", ev->widget_app_id);
		_widget_remove(page);
	} else if (ev->error == WIDGET_ERROR_DISABLED) {
		_SW("Disable %s", ev->widget_app_id);
		_widget_remove(page);
		page_info_list = scroller_write_list(layout_info->scroller);
		if (page_info_list) {
			db_read_list(page_info_list);
			page_info_list_destroy(page_info_list);
		} else {
			db_remove_all_item();
		}
	}
}



static void _widget_period_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct widget_evas_event_info *ev = event_info;
	_SD("Period changed: %s -> %lf", ev->widget_app_id, widget_viewer_evas_get_period(obj));
	/*
	 * TODO: Update the period information - Sync with DB?
	 */
}



static void _widget_faulted_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct widget_evas_event_info *ev = event_info;

	_SD("Widget is faulted - %s", ev->widget_app_id);
}



static void _widget_deleted_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct widget_evas_event_info *ev = event_info;
	Evas_Object *page = data;
	page_info_s *page_info = NULL;

	ret_if(!page);

	_D("Widget is deleted");

	if (ev->error == WIDGET_ERROR_FAULT) {
		/* DBox is faulted. */
		_SE("Widget is faulted - %s", ev->widget_app_id);
		return;
	}

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	if (!widget_viewer_evas_is_faulted(page_info->item)) {
		_widget_remove(page);
	}
}



static void _widget_access_action_ret_cb(Evas_Object *obj, int ret, void *data)
{
	Evas_Object *page = data;
	page_info_s *page_info;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	switch (ret) {
	case WIDGET_ACCESS_STATUS_FIRST:
	case WIDGET_ACCESS_STATUS_LAST:
	{
		Elm_Access_Action_Info action_info;
		memset(&action_info, 0, sizeof(action_info));
		action_info.action_type = ELM_ACCESS_ACTION_UNHIGHLIGHT;
		push_pended_event_list(page_info->item, &action_info, widget_pended_event_consumer, NULL);
	}
	case WIDGET_ACCESS_STATUS_ERROR: /* In case of error, we have to set focus on our page */
		page_info->need_to_unhighlight = EINA_FALSE;
		page_info->highlight_changed = EINA_FALSE;
		page_info->need_to_read = EINA_TRUE;

		/* Update highlight */
		_D("The result of access action is (%d) %s", ret, page_info->highlighted ? "hl" : "unhl");
		elm_access_highlight_set(page_info->focus);
		break;
	case WIDGET_ACCESS_STATUS_DONE:
	case WIDGET_ACCESS_STATUS_READ:
	default:
		if (page_info->highlighted) {
			page_info->need_to_unhighlight = EINA_TRUE;
			if (!page_info->highlight_changed) {
				page_info->highlight_changed = EINA_TRUE;
				_D("Need to unhighlight");
				elm_access_highlight_set(page_info->focus);
			} else {
				_D("Do not change the highlight");
			}
		} else {
			_D("page_info->highlight EINA_FALSE");
		}
	}

	widget_pended_event_consumer(obj, ret, data);
}



static Eina_Bool _highlight_action_cb(void *data, Evas_Object *focus, Elm_Access_Action_Info *action_info)
{
	Evas_Object *page = data;
	page_info_s *page_info;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	if (!page_info) {
		_E("Page info is not valid\n");
		return EINA_FALSE;
	}

	if (page_info->highlighted == EINA_FALSE) {
		page_info->highlighted = EINA_TRUE;
		page_info->need_to_read = EINA_TRUE;
		_D("Turn on highlight");
		return EINA_FALSE;
	}

	if (page_info->need_to_unhighlight) {
		page_info->need_to_unhighlight = EINA_FALSE;
		_D("Turn off highlight");
		return EINA_TRUE;
	} else {
		_D("Turn on highlight");
		return EINA_FALSE;
	}
}



static Eina_Bool _unhighlight_action_cb(void *data, Evas_Object *focus, Elm_Access_Action_Info *action_info)
{
	Evas_Object *page = data;
	page_info_s *page_info;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	if (!page_info) {
		_E("Page info is not valid\n");
		return EINA_FALSE;
	}

	if (page_info->highlighted == EINA_TRUE) {
		Elm_Access_Action_Info _action_info;

		_D("Reset highlight flags");
		page_info->highlighted = EINA_FALSE;
		page_info->highlight_changed = EINA_FALSE;

		memset(&_action_info, 0, sizeof(_action_info));
		if (s_info.is_scrolling) {
			if (page_info->is_scrolled_object) {
				page_info->is_scrolled_object = EINA_FALSE;

				if (widget_viewer_evas_is_faulted(page_info->item)) {
					_D("Faulted");
					return EINA_FALSE;
				}

				// We have to cancelate the scroll event.
				_action_info.mouse_type = 2; // MOUSE_UP
				_action_info.action_type = ELM_ACCESS_ACTION_SCROLL;
				send_access_event(page_info->item, &_action_info, 1, widget_pended_event_consumer, NULL);

				_action_info.mouse_type = 0;
				_action_info.action_type = ELM_ACCESS_ACTION_UNHIGHLIGHT;
				send_access_event(page_info->item, &_action_info, 1, widget_pended_event_consumer, NULL);
				_D("Reset scroll event");
			}
		} else {
			// Need to turn of highlight
			_action_info.action_type = ELM_ACCESS_ACTION_UNHIGHLIGHT;
			send_access_event(page_info->item, &_action_info, 1, widget_pended_event_consumer, NULL);
		}
		return EINA_FALSE;
	}

	_D("Unhighlighted");
	return EINA_TRUE;
}



static Eina_Bool _access_action_activate_cb(void *data, Evas_Object *focus, Elm_Access_Action_Info *action_info)
{
	Evas_Object *page = data;
	page_info_s *page_info;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	if (!page_info) {
		_E("Page info is not valid\n");
		/* Do not change the highlight in this case, so return EINA_TRUE */
		return EINA_FALSE;
	}

	if (page_info->highlighted == EINA_FALSE) {
		_D("Highlight is not exists");
		return EINA_FALSE;
	}

	if (widget_viewer_evas_is_faulted(page_info->item)) {
		_D("Activate Widget\n");
		widget_viewer_evas_activate_faulted_widget(page_info->item);
	} else {
		_D("Access action(%s) for focus(%p <> %p) is called", action_type_string(action_info->action_type), focus, page_info->focus);
		send_access_event(page_info->item, action_info, 1, _widget_access_action_ret_cb, page);
		/* Highligh off and then reset highlight from return callback _widget_access_action_ret_cb() */
	}

	return EINA_TRUE;
}



static Eina_Bool _access_action_scroll_cb(void *data, Evas_Object *focus, Elm_Access_Action_Info *action_info)
{
	Evas_Object *page = data;
	page_info_s *page_info;
	int must = 0;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	if (!page_info) {
		_E("Page info is not valid\n");
		/* Do not change the highlight in this case, so return EINA_TRUE */
		return EINA_FALSE;
	}

	if (page_info->highlighted == EINA_FALSE) {
		_D("Highlight is not exists");
		return EINA_FALSE;
	}

	if (action_info->mouse_type == 0) { // MOUSE_DOWN
		s_info.is_scrolling = EINA_TRUE;
		page_info->is_scrolled_object = EINA_TRUE;
		must = 1;
	} else if (action_info->mouse_type == 2) { // MOUSE_UP
		s_info.is_scrolling = EINA_FALSE;

		if (!page_info->is_scrolled_object) {
			return EINA_FALSE;
		}

		page_info->is_scrolled_object = EINA_FALSE;
		must = 1;
	} else if (s_info.is_scrolling != EINA_TRUE) {
		return EINA_FALSE;
	}

	if (widget_viewer_evas_is_faulted(page_info->item)) {
		_D("Faulted box, do not send any events");
		return EINA_FALSE;
	}

	send_access_event(page_info->item, action_info, must, widget_pended_event_consumer, NULL);

	/* Highligh off and then reset highlight from return callback _widget_access_action_ret_cb() */
	return EINA_FALSE;
}



static Eina_Bool delayed_faulted_action_cb(void *data)
{
	Evas_Object *page = data;
	page_info_s *page_info;
	Elm_Access_Action_Info action_info;
	int ret = ECORE_CALLBACK_RENEW;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	if (!page_info) {
		return ECORE_CALLBACK_CANCEL;
	}

	memset(&action_info, 0, sizeof(action_info));

	if ((int)evas_object_data_get(page, TAG_FAULT_HL) == 1) {
		action_info.action_type = ELM_ACCESS_ACTION_HIGHLIGHT;
		/**
		 * Call elm_access_action with "HIGHLIGHT", will invoke the ELM_ACCESS_ACTION_READ callback.
		 * It will invoke the _access_action_forward_cb,
		 */
		elm_access_action(page_info->focus, action_info.action_type, &action_info);

		evas_object_data_del(page, TAG_FAULT_HL);
		page_info->faulted_hl_timer = NULL;
		ret = ECORE_CALLBACK_CANCEL;
	} else if ((int)evas_object_data_get(page, TAG_FAULT_HL) == 0) {
		action_info.action_type = ELM_ACCESS_ACTION_UNHIGHLIGHT;
		elm_access_action(page_info->focus, action_info.action_type, &action_info);

		evas_object_data_set(page, TAG_FAULT_HL, (void *)1);
	}

	return ret;
}



/**
 * ELM_ACCESS_ACTION_HIGHLIGHT_NEXT
 * ELM_ACCESS_ACTION_HIGHLIGHT_PREV
 * ELM_ACCESS_ACTION_VALUE_CHANGE
 * ELM_ACCESS_ACTION_BACK
 * ELM_ACCESS_ACTION_READ
 * ELM_ACCESS_ACTION_ENABLE
 * ELM_ACCESS_ACTION_DISABLE
 */
static Eina_Bool _access_action_forward_cb(void *data, Evas_Object *focus, Elm_Access_Action_Info *action_info)
{
	Evas_Object *page = data;
	page_info_s *page_info;
	Evas_Coord x, y;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	if (!page_info) {
		_E("Page info is not valid\n");
		/* Do not change the highlight in this case, so return EINA_TRUE */
		return EINA_FALSE;
	}

	if (page_info->highlighted == EINA_FALSE) {
		_D("Highlight is not exists");
		return EINA_FALSE;
	}

	evas_object_geometry_get(page_info->item, &x, &y, NULL, NULL);

	if (s_info.is_scrolling || x || y) {
		_D("I don't want to do anything! %dx%d", x, y);
		return EINA_FALSE;
	}

	if (widget_viewer_evas_is_faulted(page_info->item)) {
		switch (action_info->action_type) {
		case ELM_ACCESS_ACTION_READ:
			break;
		case ELM_ACCESS_ACTION_HIGHLIGHT_NEXT:
		case ELM_ACCESS_ACTION_HIGHLIGHT_PREV:
			if (!page_info->faulted_hl_timer) {
				page_info->faulted_hl_timer = ecore_timer_add(FAULTED_HL_TIMER, delayed_faulted_action_cb, page);
				if (!page_info->faulted_hl_timer) {
					_E("Faulted box, faulted timer failed");
				} else {
					_D("Faulted box, Add highlight emulate timer");
				}
			} else {
				_D("Faulted box, Keep highlight emulate timer");
			}
			break;
		default:
			break;
		}
		page_info->need_to_read = EINA_TRUE;

		return EINA_TRUE;
	}

	_D("Access action(%s) for focus(%p <> %p) is called, by(%d)", action_type_string(action_info->action_type), focus, page_info->focus, action_info->action_by);
	send_access_event(page_info->item, action_info, 1, _widget_access_action_ret_cb, page);

	/* Highligh off and then reset highlight from return callback _widget_access_action_ret_cb() */
	return EINA_TRUE;
}



static Eina_Bool _access_action_mouse_cb(void *data, Evas_Object *focus, Elm_Access_Action_Info *action_info)
{
	Evas_Object *page = data;
	page_info_s *page_info;
	int must;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	if (!page_info) {
		_E("Page info is not valid\n");
		/* Do not change the highlight in this case, so return EINA_TRUE */
		return EINA_FALSE;
	}

	if (page_info->highlighted == EINA_FALSE) {
		_D("Highlight is not exists");
		return EINA_FALSE;
	}

	if (widget_viewer_evas_is_faulted(page_info->item)) {
		_D("Faulted box, do not send any events");
		return EINA_FALSE;
	}

	if (s_info.is_scrolling) {
		_D("I don't want to do anything!");
		return EINA_FALSE;
	}

	must = action_info->mouse_type != 1;

	_D("Access action(%s) for focus(%p <> %p) is called", action_type_string(action_info->action_type), focus, page_info->focus);
	send_access_event(page_info->item, action_info, must, widget_pended_event_consumer, NULL);

	/* Highligh off and then reset highlight from return callback _widget_access_action_ret_cb() */
	return EINA_TRUE;
}



static char *_access_widget_read_cb(void *data, Evas_Object *obj)
{
	Evas_Object *page = data;
	page_info_s *page_info = NULL;
	char *tmp;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	if (widget_viewer_evas_is_widget(page_get_item(page)) == 1) {
		if (page_info->need_to_read == EINA_FALSE) {
			/*
			 * need_to_read will be toggled by _access_page_num_cb function
			 * so this function should not touch it.
			 */
			 return NULL;
		}

		if (page_info->need_to_unhighlight == EINA_TRUE) {
			return NULL;
		}
	}

	tmp = strdup(_("IDS_IDLE_HEADER_WIDGET"));
	retv_if(!tmp, NULL);
	return tmp;
}


static char *_access_page_name_cb(void *data, Evas_Object *obj)
{
	Evas_Object *page = data;
	page_info_s *page_info = NULL;
	char *title;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	if (widget_viewer_evas_is_widget(page_get_item(page)) == 1) {
		if (page_info->need_to_read == EINA_FALSE) {
			/*
			 * need_to_read will be toggled by _access_page_num_cb function
			 * so this function should not touch it.
			 */
			return NULL;
		}

		if (page_info->need_to_unhighlight == EINA_TRUE) {
			return NULL;
		}
	}

	if (page_info->title) {
		title = strdup(page_info->title);
		retv_if(!title, NULL);
		return title;
	}

	/* EFL will free this title. */
	title = page_read_title(page);
	retv_if(!title, NULL);

	return title;
}



static char *_access_context_info_cb(void *data, Evas_Object *obj)
{
	Evas_Object *page = data;
	page_info_s *page_info = NULL;
	char *ctx = NULL;

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(!page_info, NULL);

	if (widget_viewer_evas_is_faulted(page_info->item)) {
		if (page_info->need_to_read == EINA_FALSE) {
			return NULL;
		}

		if (page_info->need_to_unhighlight == EINA_TRUE) {
			return NULL;
		}
		page_info->need_to_read = EINA_FALSE;
		ctx = strdup(_("IDS_HS_BODY_UNABLE_TO_LOAD_DATA_TAP_TO_RETRY"));
	}

	return ctx;
}



HAPI void widget_add_callback(Evas_Object *widget, Evas_Object *page)
{
	page_info_s *page_info = NULL;

	ret_if(!widget);
	ret_if(!page);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	ret_if(!page_info);

	/* Only handling the focus set operation to page_focus */
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_HIGHLIGHT, _highlight_action_cb, page);
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_UNHIGHLIGHT, _unhighlight_action_cb, page);

	/* Deliever events to the widget */
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT, _access_action_forward_cb, page);
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_HIGHLIGHT_PREV, _access_action_forward_cb, page);
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_VALUE_CHANGE, _access_action_forward_cb, page);
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_BACK, _access_action_forward_cb, page);
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_READ, _access_action_forward_cb, page);
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_ENABLE, _access_action_forward_cb, page);
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_DISABLE, _access_action_forward_cb, page);

	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_ACTIVATE, _access_action_activate_cb, page);

	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_SCROLL, _access_action_scroll_cb, page);
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_MOUSE, _access_action_mouse_cb, page);
	elm_access_action_cb_set(page_info->focus, ELM_ACCESS_ACTION_OVER, _access_action_mouse_cb, page);

	elm_access_info_cb_set(page_info->focus, ELM_ACCESS_INFO, _access_page_name_cb, page);
	elm_access_info_cb_set(page_info->focus, ELM_ACCESS_TYPE, _access_widget_read_cb, page);
	elm_access_info_cb_set(page_info->focus, ELM_ACCESS_CONTEXT_INFO, _access_context_info_cb, page);

	evas_object_smart_callback_add(widget, WIDGET_SMART_SIGNAL_EXTRA_INFO_UPDATED, _widget_extra_updated_cb, page);
	evas_object_smart_callback_add(widget, WIDGET_SMART_SIGNAL_WIDGET_DELETED, _widget_deleted_cb, page);
	evas_object_smart_callback_add(widget, WIDGET_SMART_SIGNAL_WIDGET_FAULTED, _widget_faulted_cb, page);
	evas_object_smart_callback_add(widget, WIDGET_SMART_SIGNAL_PERIOD_CHANGED, _widget_period_changed_cb, page);
	evas_object_smart_callback_add(widget, WIDGET_SMART_SIGNAL_WIDGET_CREATE_ABORTED, _widget_create_aborted_cb, page);
}



HAPI void widget_del_callback(Evas_Object *widget)
{
	ret_if(!widget);

	evas_object_smart_callback_del(widget, WIDGET_SMART_SIGNAL_EXTRA_INFO_UPDATED, _widget_extra_updated_cb);
	evas_object_smart_callback_del(widget, WIDGET_SMART_SIGNAL_WIDGET_DELETED, _widget_deleted_cb);
	evas_object_smart_callback_del(widget, WIDGET_SMART_SIGNAL_WIDGET_FAULTED, _widget_faulted_cb);
	evas_object_smart_callback_del(widget, WIDGET_SMART_SIGNAL_PERIOD_CHANGED, _widget_period_changed_cb);
	evas_object_smart_callback_del(widget, WIDGET_SMART_SIGNAL_WIDGET_CREATE_ABORTED, _widget_create_aborted_cb);
}



HAPI void widget_set_update_callback(Evas_Object *obj, int (*updated)(Evas_Object *obj))
{
	evas_object_data_set(obj, TAG_UPDATED, updated);
}



HAPI void widget_set_scroll_callback(Evas_Object *obj, int (*scroll)(Evas_Object *obj, int hold))
{
	evas_object_data_set(obj, TAG_SCROLL, scroll);
}



static int uninstall_cb(const char *pkgname, enum pkgmgr_status status, double value, void *data)
{
	layout_info_s *layout_info;
	scroller_info_s *scroller_info;
	page_info_s *page_info;
	Eina_List *page_list;
	Evas_Object *page;

	if (status != PKGMGR_STATUS_START) {
		return 0;
	}

	if (!main_get_info()->layout) {
		return 0;
	}

	layout_info = evas_object_data_get(main_get_info()->layout, DATA_KEY_LAYOUT_INFO);
	if (!layout_info) {
		return 0;
	}

	scroller_info = evas_object_data_get(layout_info->scroller, DATA_KEY_SCROLLER_INFO);
	if (!scroller_info) {
		return 0;
	}

	if (!scroller_info->box) {
		return 0;
	}

	page_list = elm_box_children_get(scroller_info->box);
	EINA_LIST_FREE(page_list, page) {
		page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
		if (!page_info) {
			continue;
		}

		if (page_info->direction != PAGE_DIRECTION_RIGHT) {
			continue;
		}

		if (!page_info->item) {
			continue;
		}

		if (!widget_viewer_evas_is_faulted(page_info->item)) {
			continue;
		}

		_D("Faulted package: %s", widget_viewer_evas_get_widget_id(page_info->item));
		_widget_remove(page);
	}

	return 0;
}



HAPI void widget_init(Evas_Object *win)
{
	int val;

	add_viewer_pkgmgr_add_event_callback(PKGMGR_EVENT_UNINSTALL, uninstall_cb, NULL);

	widget_viewer_evas_set_option(WIDGET_VIEWER_EVAS_AUTO_RENDER_SELECTION, 1);

	if (preference_get_int("memory/private/org.tizen.w-home/auto_feed", &val) >= 0) {
		widget_viewer_evas_set_option(WIDGET_VIEWER_EVAS_EVENT_AUTO_FEED, val);
	}

	if (preference_get_int("memory/private/org.tizen.w-home/sensitive_move", &val) >= 0) {
		widget_viewer_evas_set_option(WIDGET_VIEWER_EVAS_SENSITIVE_MOVE, val);
	}

	widget_viewer_evas_init(win);
	widget_viewer_evas_set_option(WIDGET_VIEWER_EVAS_SCROLL_X, 1);
}



HAPI void widget_fini(void)
{
	widget_viewer_evas_fini();
	add_viewer_pkgmgr_del_event_callback(PKGMGR_EVENT_UNINSTALL, uninstall_cb, NULL);
}



// End of a file
