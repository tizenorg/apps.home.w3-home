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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <widget_service.h>
#include <widget_errno.h>

#include <dlog.h>
#include <system_settings.h>

#include <Elementary.h>

#include "add-viewer.h"
#include "add-viewer_pkgmgr.h"
#include "add-viewer_ucol.h"
#include "add-viewer_package.h"
#include "add-viewer_debug.h"
#include "add-viewer_util.h"
#include "conf.h"

#include "bg.h"

#if defined(LOG_TAG)
#undef LOG_TAG
#endif
#define LOG_TAG "ADD_VIEWER"
#define ADD_VIEWER_CLASS_NAME "add-viewer"

int errno;

static struct {
	Evas_Smart_Class sc;
	Evas_Smart *smart;
	Eina_List *add_viewer_list;
	int enable_dnd;
} s_info = {
	.sc = EVAS_SMART_CLASS_INIT_NAME_VERSION(ADD_VIEWER_CLASS_NAME),
	.smart = NULL,
	.add_viewer_list = NULL,
	.enable_dnd = 0,
};

struct widget_data {
	Evas *e;
	Evas_Object *stage;

	Evas_Object *add_viewer;
	Evas_Object *parent;

	Evas_Object *scroller;

	Evas_Object *bg;
};

struct click {
	struct widget_data *widget_data;
	struct add_viewer_package *package;
	int size;
	Ecore_Timer *long_press_timer;
	int x;
	int y;

	struct dnd {
		Evas_Object *obj;
	} dnd;

	struct geo {
		Evas_Object *obj;
		int x;
		int y;
	} geo;
};

static Evas_Object *winset_preview_add(struct widget_data *widget_data, Evas_Object *parent, struct add_viewer_package *package, const char *name, int type, int no_event);

static inline void append_padding(Evas_Object *box, int padding)
{
	Evas_Object *pad;

	pad = elm_box_add(box);
	if (!pad) {
		ErrPrint("Failed to create a pad\n");
		return;
	}

	evas_object_resize(pad, ADD_VIEWER_PREVIEW_WIDTH, padding);
	evas_object_size_hint_min_set(pad, ADD_VIEWER_PREVIEW_WIDTH, padding);
	evas_object_show(pad);
	elm_box_pack_end(box, pad);
}

static Eina_Bool normal_loader_cb(struct widget_data *widget_data, void *container)
{
	struct add_viewer_package *package;
	Evas_Object *thumb_item;
	Eina_List *l;
	char *name;
	char *filter;

	l = (Eina_List *)evas_object_data_get(container, "list");
	if (!l) {
		l = add_viewer_package_list_handle();
		if (!l) {
			goto cancel;
		}

		evas_object_data_set(container, "list", l);
	}

	package = add_viewer_package_list_item(l);

	l = add_viewer_package_list_next(l);
	evas_object_data_set(container, "list", l);
	if (!package) {
		ErrPrint("Package list is not valid\n");
		goto out;
	}

	filter = evas_object_data_get(container, "filter");
	if (filter) {
		name = (char *)add_viewer_package_list_name(package);
		if (add_viewer_ucol_case_search(name, filter) < 0) {
			goto out;
		}

		name = add_viewer_util_highlight_keyword(name, filter);
		DbgPrint("Filtered: %s\n", name);
	} else {
		name = elm_entry_utf8_to_markup(add_viewer_package_list_name(package));
	}

	thumb_item = winset_preview_add(widget_data, container, package, name, WIDGET_SIZE_TYPE_2x2, 0);
	free(name);
	evas_object_data_set(thumb_item, "package", package);
	elm_box_pack_end(container, thumb_item);

out:
	if (l) {
		return ECORE_CALLBACK_RENEW;
	}
	DbgPrint("Loading is finished\n");

cancel:
	(void)evas_object_data_del(container, "list");
	(void)evas_object_data_del(container, "loader");

	l = elm_box_children_get(container);
	if (!l) {
		/* TODO: "No content" show */
	} else {
		eina_list_free(l);
		/* TODO: "No content" hide */

		append_padding(container, 96);
	}

	return ECORE_CALLBACK_CANCEL;
}

static int reload_list_cb(struct add_viewer_package *package, void *data)
{
	Eina_List *children;
	Eina_List *l;
	Evas_Object *tmp;
	Ecore_Timer *timer;
	Evas_Object *container;
	struct widget_data *widget_data = data;

	container = elm_object_content_get(widget_data->scroller);
	if (!container) {
		return WIDGET_ERROR_FAULT;
	}

	/*!
	 * \note
	 * Delete all thumbnail objects
	 */
	children = elm_box_children_get(container);
	EINA_LIST_FREE(children, tmp) {
		elm_box_unpack(container, tmp);
		evas_object_del(tmp);
	}

	timer = evas_object_data_del(container, "loader");
	if (timer) {
		ecore_timer_del(timer);
		(void)evas_object_data_del(container, "list");
	}

	append_padding(container, ADD_VIEWER_PREVIEW_PAD_TOP);

	l = add_viewer_package_list_handle();
	if (l) {
		elm_object_part_text_set(widget_data->bg, "empty", "");
		while (normal_loader_cb(widget_data, container) == ECORE_CALLBACK_RENEW);
	} else {
		elm_object_part_text_set(widget_data->bg, "empty", _("IDS_ST_BODY_EMPTY"));
	}

	/* To set the first focus */
	evas_object_smart_callback_call(widget_data->scroller, "scroll", NULL);

	return WIDGET_ERROR_NONE;
}

static void widget_add(Evas_Object *add_viewer)
{
	struct widget_data *widget_data;

	widget_data = calloc(1, sizeof(*widget_data));
	if (!widget_data) {
		ErrPrint("Failed to allocate heap: %d\n", errno);
		return;
	}

	widget_data->e = evas_object_evas_get(add_viewer);
	if (!widget_data->e) {
		ErrPrint("Failed to get Evas\n");
		free(widget_data);
		return;
	}

	widget_data->stage = evas_object_rectangle_add(widget_data->e);
	if (!widget_data->stage) {
		ErrPrint("Failed to create a stage\n");
		free(widget_data);
		return;
	}

	widget_data->add_viewer = add_viewer;

	evas_object_color_set(widget_data->stage, 255, 255, 255, 255);
	evas_object_smart_data_set(widget_data->add_viewer, widget_data);
	evas_object_smart_member_add(widget_data->stage, widget_data->add_viewer);

	s_info.add_viewer_list = eina_list_append(s_info.add_viewer_list, widget_data);

	add_viewer_package_list_add_event_callback(NULL, PACKAGE_LIST_EVENT_RELOAD, reload_list_cb, widget_data);
	/* widget_data->parent is not yet initialized */
}

/**
 * \note
 * This callback can be called while initializing a widget.
 * It means, the element of a structure could be null.
 * so we have to validate it before delete it.
 */
static void widget_del(Evas_Object *add_viewer)
{
	struct widget_data *widget_data;

	widget_data = evas_object_smart_data_get(add_viewer);
	if (!widget_data) {
		ErrPrint("Invalid widget\n");
		return;
	}

	add_viewer_package_list_del_event_callback(NULL, PACKAGE_LIST_EVENT_RELOAD, reload_list_cb, widget_data);

	s_info.add_viewer_list = eina_list_remove(s_info.add_viewer_list, widget_data);

	if (widget_data->scroller) {
		evas_object_smart_member_del(widget_data->scroller);
		evas_object_del(widget_data->scroller);
	}

	if (widget_data->stage) {
		evas_object_smart_member_del(widget_data->stage);
		evas_object_del(widget_data->stage);
	}

	if (widget_data->bg) {
		evas_object_smart_member_del(widget_data->bg);
		evas_object_del(widget_data->bg);
	}

	free(widget_data);
}

static void widget_move(Evas_Object *add_viewer, Evas_Coord x, Evas_Coord y)
{
	struct widget_data *widget_data;

	widget_data = evas_object_smart_data_get(add_viewer);
	if (!widget_data) {
		ErrPrint("Invalid widget\n");
		return;
	}

	evas_object_move(widget_data->scroller, x, y);
	evas_object_move(widget_data->stage, x, y);
	evas_object_move(widget_data->bg, x, y);
}

static void widget_resize(Evas_Object *add_viewer, Evas_Coord w, Evas_Coord h)
{
	struct widget_data *widget_data;

	widget_data = evas_object_smart_data_get(add_viewer);
	if (!widget_data) {
		ErrPrint("Invalid widget\n");
		return;
	}

	evas_object_resize(widget_data->scroller, w, h);
	evas_object_resize(widget_data->stage, w, h);
	evas_object_resize(widget_data->bg, w, h);
}

static void widget_show(Evas_Object *add_viewer)
{
	struct widget_data *widget_data;

	widget_data = evas_object_smart_data_get(add_viewer);
	if (!widget_data) {
		ErrPrint("Invalid widget\n");
		return;
	}

	evas_object_show(widget_data->stage);
}

static void widget_hide(Evas_Object *add_viewer)
{
	struct widget_data *widget_data;

	widget_data = evas_object_smart_data_get(add_viewer);
	if (!widget_data) {
		ErrPrint("Invalid widget\n");
		return;
	}

	evas_object_hide(widget_data->stage);
}

static void widget_color_set(Evas_Object *add_viewer, int r, int g, int b, int a)
{
	struct widget_data *widget_data;

	widget_data = evas_object_smart_data_get(add_viewer);
	if (!widget_data) {
		ErrPrint("Invalid widget\n");
		return;
	}

	evas_object_color_set(widget_data->stage, r, g, b, a);
}

static void widget_clip_set(Evas_Object *add_viewer, Evas_Object *clip)
{
	struct widget_data *widget_data;

	widget_data = evas_object_smart_data_get(add_viewer);
	if (!widget_data) {
		ErrPrint("Invalid widget\n");
		return;
	}

	evas_object_clip_set(widget_data->stage, clip);
}

static void widget_clip_unset(Evas_Object *add_viewer)
{
	struct widget_data *widget_data;

	widget_data = evas_object_smart_data_get(add_viewer);
	if (!widget_data) {
		ErrPrint("Invalid widget\n");
		return;
	}

	evas_object_clip_unset(widget_data->stage);
}

HAPI void evas_object_add_viewer_init(void)
{
	add_viewer_ucol_init();
	(void)add_viewer_package_init();

}

HAPI void evas_object_add_viewer_fini(void)
{
	(void)add_viewer_package_fini();
	add_viewer_ucol_fini();
}

static Eina_Bool register_access_object_for_edje_part(Evas_Object *object, const char *part_name)
{
	Evas_Object *edje;
	Evas_Object *content;
	Elm_Access_Action_Info info;
	Elm_Access_Action_Type action;

	edje = evas_object_data_get(object, "edje");
	if (!edje) {
		return EINA_TRUE;
	}

	content = elm_object_part_content_get(edje, part_name);
	if (!content) {
		return EINA_TRUE;
	}

	memset(&info, 0, sizeof(info));

	action = ELM_ACCESS_ACTION_HIGHLIGHT_NEXT;
	info.highlight_cycle = EINA_FALSE;
	return elm_access_action(content, action, &info);
}

static inline int make_clicked_event(Evas_Object *part_obj)
{
	Evas_Object *ao;
	Evas *e;
	int x;
	int y;
	int w;
	int h;
	double timestamp;

	ao = evas_object_data_get(part_obj, "access,object");
	if (!ao) {
		ErrPrint("Access object is not exists\n");
		return EINA_FALSE;
	}

	e = evas_object_evas_get(part_obj);
	if (!e) {
		ErrPrint("evas is not valid\n");
		return EINA_FALSE;
	}

	evas_object_geometry_get(part_obj, &x, &y, &w, &h);
	x += w / 2;
	y += h / 2;

	timestamp = ecore_time_get();

	evas_event_feed_mouse_move(e, x, y, timestamp, NULL);
	evas_event_feed_mouse_down(e, 1, EVAS_BUTTON_NONE, timestamp + 10, NULL);
	evas_event_feed_mouse_move(e, x, y, timestamp + 15, NULL);
	evas_event_feed_mouse_up(e, 1, EVAS_BUTTON_NONE, timestamp + 20, NULL);

	return EINA_TRUE;
}

static Eina_Bool activate_cb(void *part_name, Evas_Object *ao, Elm_Access_Action_Info *action_info)
{
	Eina_Bool ret;
	Evas_Object *part_object;

	part_object = evas_object_data_get(ao, "part,object");
	if (!part_object) {
		return EINA_FALSE;
	}

	if (part_name) {
		ret = register_access_object_for_edje_part(part_object, part_name);
	} else {
		ret = make_clicked_event(part_object);
	}

	return ret;
}

static char *_access_tab_to_add_cb(void *data, Evas_Object *obj)
{
	char *tmp;

	tmp = strdup(_("IDS_KM_BODY_DOUBLE_TAP_TO_ADD"));
	if (!tmp) {
		ErrPrint("tmp is not exist\n");
		return NULL;
	}

	return tmp;
}


HAPI void winset_access_object_add(Evas_Object *parent, Evas_Object *layout, const char *size, const char *name)
{
	Evas_Object *ao;
	Evas_Object *part;
	char *text;
	int len;

	len = strlen(name) + (size ? strlen(size) : 0) + 2;
	text = malloc(len);
	if (!text) {
		ErrPrint("Heap : %d\n", errno);
		return;
	}

	strcpy(text, name);

	ao = evas_object_data_get(layout, "access,object");
	if (ao) {
		elm_access_info_set(ao, ELM_ACCESS_INFO, text);
		free(text);
		return;
	}

	Evas_Object *edje;
	edje = elm_layout_edje_get(layout);
	part = (Evas_Object *)edje_object_part_object_get(edje, "preview,dbg");
	ao = elm_access_object_register(part, layout);
	if (!ao) {
		free(text);
		return;
	}

	elm_access_info_set(ao, ELM_ACCESS_INFO, text);
	free(text);

	elm_access_info_set(ao, ELM_ACCESS_TYPE, _("IDS_IDLE_HEADER_WIDGET"));

	elm_access_info_cb_set(ao, ELM_ACCESS_CONTEXT_INFO, _access_tab_to_add_cb, NULL);

	elm_object_focus_custom_chain_append(layout, ao, NULL);

	evas_object_data_set(ao, "part,object", layout);
	evas_object_data_set(ao, "parent", parent);

	elm_access_action_cb_set(ao, ELM_ACCESS_ACTION_ACTIVATE, activate_cb, NULL);

	evas_object_data_set(layout, "access,object", ao);
}

static void _change_focus(Evas_Object *scroller, Evas_Object *focus_widget)
{
	Evas_Object *pre_focus = NULL;

	pre_focus = evas_object_data_get(scroller, "focused");
	if (pre_focus == focus_widget) return;

	elm_object_signal_emit(focus_widget, "show", "line");
	evas_object_data_set(scroller, "focused", focus_widget);

	if (pre_focus && pre_focus != focus_widget) {
		elm_object_signal_emit(pre_focus, "hide", "line");
	}
}

static void _widget_scroll_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *box = data;
	Evas_Object *scroller = obj;
	Evas_Object *focus_widget = NULL;
	Eina_List *list = NULL;
	int h_page = 0;
	Evas_Coord y;

	DbgPrint("========== Widget Scroll CB is called (%p)\n", obj);

	if (!scroller) {
		ErrPrint("Failed to load the widget scroller\n");
		return;
	}

	if (!box) {
		return;
	}

	list = elm_box_children_get(box);
	if (!list) {
		return;
	}

	evas_object_geometry_get(box, NULL, &y, NULL, NULL);
	y -= (ADD_VIEWER_PAGE_HEIGHT >> 1);
	h_page = -(y / ADD_VIEWER_PAGE_HEIGHT);

	focus_widget = eina_list_nth(list, h_page + 1);
	eina_list_free(list);

	if (!focus_widget) {
		ErrPrint("Failed to get the focused page in scroller\n");
		return;
	}

	_change_focus(scroller, focus_widget);
}

static int widget_data_setup(struct widget_data *widget_data, Evas_Object *parent)
{
	Evas_Object *box;

	widget_data->parent = parent;

	widget_data->bg = elm_layout_add(widget_data->parent);
	if (!widget_data->bg) {
		return WIDGET_ERROR_FAULT;
	}

	if (elm_layout_file_set(widget_data->bg, EDJE_FILE, "bg") != EINA_TRUE) {
		evas_object_del(widget_data->bg);
		widget_data->bg = NULL;
		return WIDGET_ERROR_FAULT;
	}

	widget_data->scroller = elm_scroller_add(widget_data->parent);
	if (!widget_data->scroller) {
		evas_object_del(widget_data->bg);
		widget_data->bg = NULL;
		return WIDGET_ERROR_FAULT;
	}

	box = elm_box_add(widget_data->scroller);
	if (!box) {
		evas_object_del(widget_data->bg);
		widget_data->bg = NULL;

		evas_object_del(widget_data->scroller);
		widget_data->scroller = NULL;
		return WIDGET_ERROR_FAULT;
	}

	elm_box_horizontal_set(box, EINA_FALSE);
	elm_box_homogeneous_set(box, EINA_FALSE);
	evas_object_size_hint_fill_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_box_padding_set(box, ADD_VIEWER_PREVIEW_PAD_LEFT, ADD_VIEWER_PREVIEW_PAD_TOP);
	elm_box_align_set(box, 0.0, 0.0);
	evas_object_show(box);

	elm_scroller_policy_set(widget_data->scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_scroller_propagate_events_set(widget_data->scroller, EINA_TRUE);
	elm_scroller_bounce_set(widget_data->scroller, EINA_FALSE, EINA_TRUE);
	elm_object_style_set(widget_data->scroller, "effect");

	elm_scroller_page_size_set(widget_data->scroller, ADD_VIEWER_PAGE_WIDTH, ADD_VIEWER_PAGE_HEIGHT);

	elm_object_content_set(widget_data->scroller, box);

	evas_object_smart_callback_add(widget_data->scroller, "scroll", _widget_scroll_cb, box);

	evas_object_show(widget_data->scroller);
	evas_object_show(widget_data->bg);

	evas_object_smart_member_add(widget_data->bg, widget_data->add_viewer);
	evas_object_clip_set(widget_data->bg, widget_data->stage);

	evas_object_smart_member_add(widget_data->scroller, widget_data->add_viewer);
	evas_object_clip_set(widget_data->scroller, widget_data->stage);
	return WIDGET_ERROR_NONE;
}

static void del_cb(void *data, Evas *e, Evas_Object *container, void *event_info)
{
	free(data);
}

static Eina_Bool long_press_cb(void *data)
{
	struct click *cbdata = data;
	int x;
	int y;

	evas_object_geometry_get(cbdata->geo.obj, &x, &y, NULL, NULL);

	if (cbdata->geo.x == x && cbdata->geo.y == y) {
		const char *name;
		Evas_Coord w;
		Evas_Coord h;
		struct add_viewer_event_info info = {
			.pkg_info = {
				.widget_id = add_viewer_package_list_pkgname(cbdata->package),
				.content = NULL,
				.size_type = cbdata->size,
			},
			.move = {
				.obj = NULL,
			},
		};

		name = add_viewer_package_list_name(cbdata->package);

		info.move.obj = winset_preview_add(cbdata->widget_data, cbdata->geo.obj, cbdata->package, name, cbdata->size, 1);
		if (!info.move.obj) {
			ErrPrint("Failed to create a preview object\n");
		} else {
			/* Register the DnD object to Click CB Data to move it from move function */
			cbdata->dnd.obj = info.move.obj;

			evas_object_smart_member_add(cbdata->dnd.obj, cbdata->widget_data->add_viewer);
			evas_object_clip_set(cbdata->dnd.obj, cbdata->widget_data->stage);

			elm_object_signal_emit(cbdata->geo.obj, "reset", "preview,dbg");

			evas_object_hide(cbdata->widget_data->scroller);
			evas_object_hide(cbdata->widget_data->bg);

			evas_object_resize(cbdata->dnd.obj, 222, 336);
			evas_object_geometry_get(cbdata->dnd.obj, NULL, NULL, &w, &h);

			evas_object_move(cbdata->dnd.obj, cbdata->x - (w >> 1), cbdata->y - (h >> 1));

			evas_object_smart_callback_call(cbdata->widget_data->add_viewer, "dnd", &info);

		}
	} else {
		DbgPrint("Object is moved\n");
	}

	cbdata->long_press_timer = NULL;
	return ECORE_CALLBACK_CANCEL;
}

static void add_to_home_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	struct click *cbdata = data;
	struct add_viewer_event_info info = {
		.move = {
			.obj = NULL,
		},

		.pkg_info = {
			.widget_id = add_viewer_package_list_pkgname(cbdata->package),
			.content = NULL,
			.size_type = cbdata->size,
			.duplicated = add_viewer_package_is_skipped(cbdata->package),
			.image = elm_object_part_content_unset(obj, "preview"),
		}
	};

	evas_object_smart_callback_call(cbdata->widget_data->add_viewer, "selected", &info);

	elm_access_say(_("IDS_TTS_BODY_ITEM_ADDED"));
}

static void preview_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	struct click *cbdata = data;
	Evas_Event_Mouse_Down *down = event_info;

	evas_object_geometry_get(obj, &cbdata->geo.x, &cbdata->geo.y, NULL, NULL);
	cbdata->geo.obj = obj;

	if (s_info.enable_dnd) {
		int delay;
		double fdelay;

		delay = SYSTEM_SETTINGS_TAP_AND_HOLD_DELAY_SHORT; /* default 0.5 sec */
		if (system_settings_get_value_int(SYSTEM_SETTINGS_KEY_TAP_AND_HOLD_DELAY, &delay) != 0) {
			delay = SYSTEM_SETTINGS_TAP_AND_HOLD_DELAY_SHORT;
		}

		fdelay = ((double)delay / 1000.0f);
		DbgPrint("Long press: %lf\n", fdelay);

		cbdata->long_press_timer = ecore_timer_add(fdelay, long_press_cb, cbdata);
		if (!cbdata->long_press_timer) {
			ErrPrint("Failed to add timer\n");
		}
	}

	cbdata->x = down->canvas.x;
	cbdata->y = down->canvas.y;
}

static void preview_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	struct click *cbdata = data;

	if (cbdata->long_press_timer) {
		DbgPrint("Long press timer object found\n");
		ecore_timer_del(cbdata->long_press_timer);
		cbdata->long_press_timer = NULL;
		add_to_home_cb(data, NULL, NULL, NULL);
	} else {
		if (cbdata->dnd.obj) {
			add_to_home_cb(data, NULL, NULL, NULL);
			evas_object_smart_member_del(cbdata->dnd.obj);
			evas_object_del(cbdata->dnd.obj);
			cbdata->dnd.obj = NULL;

			evas_object_show(cbdata->widget_data->scroller);
			evas_object_show(cbdata->widget_data->bg);
		}
	}
	DbgPrint("Return\n");
}

static void preview_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	struct click *cbdata = data;
	Evas_Event_Mouse_Move *move = event_info;
	int dx;
	int dy;
	int x;
	int y;

	evas_object_geometry_get(obj, &x, &y, NULL, NULL);

        dx = move->cur.canvas.x - move->prev.canvas.x;
        dy = move->cur.canvas.y - move->prev.canvas.y;

	if ((abs(dx) > 5 || abs(dy) > 2 || cbdata->geo.x != x || cbdata->geo.y != y) && cbdata->long_press_timer) {
		ecore_timer_del(cbdata->long_press_timer);
		cbdata->long_press_timer = NULL;
	}

	cbdata->x = move->cur.canvas.x;
	cbdata->y = move->cur.canvas.y;

	if (cbdata->dnd.obj) {
		Evas_Coord w;
		Evas_Coord h;

		evas_object_geometry_get(cbdata->dnd.obj, NULL, NULL, &w, &h);

		evas_object_move(cbdata->dnd.obj, move->cur.canvas.x - (w >> 1), move->cur.canvas.y - (h >> 1));
	}
}

static void _operator_name_slide_mode_set(Evas_Object *name)
{
	Evas_Object *name_edje;
	Evas_Object *tb;
	Evas_Coord tb_w=0;

	if (name == NULL) {
		ErrPrint("paramter error!");
	}

	elm_label_slide_mode_set(name, ELM_LABEL_SLIDE_MODE_NONE);

	name_edje = elm_layout_edje_get(name);
	if (!name_edje) {
		ErrPrint("Failed to get label edje");
		return;
	}

	tb = (Evas_Object*)edje_object_part_object_get(name_edje, "elm.text");
	if (!tb) {
		ErrPrint("Failed to get label tb");
		return;
	}

	evas_object_textblock_size_native_get(tb, &tb_w, NULL);

	if((tb_w>0) && (tb_w>ELM_SCALE_SIZE(ADD_VIEWER_TEXT_WIDTH))) {
		elm_label_slide_mode_set(name, ELM_LABEL_SLIDE_MODE_AUTO);
	}
	elm_label_slide_go(name);
}

static Evas_Object *winset_preview_add(struct widget_data *widget_data, Evas_Object *parent, struct add_viewer_package *package, const char *name, int type, int no_event)
{
	const char *size_str;
	const char *icon_group;
	Evas_Object *preview;
	Evas_Object *thumbnail;
	Evas_Object *label;
	int w;
	int h;
	int ret;
	int idx;
	char *filename;
	Evas_Object *bg;
	char buf[512] = {0, };

	filename = widget_service_get_preview_image_path(add_viewer_package_list_pkgname(package), type);

	switch (type) {
	case WIDGET_SIZE_TYPE_2x2:
		size_str = "preview,2x2";
		icon_group = "default,2x2";
		idx = 8;
		break;
	case WIDGET_SIZE_TYPE_1x1:
	case WIDGET_SIZE_TYPE_2x1:
	case WIDGET_SIZE_TYPE_4x1:
	case WIDGET_SIZE_TYPE_4x2:
	case WIDGET_SIZE_TYPE_4x3:
	case WIDGET_SIZE_TYPE_4x4:
	case WIDGET_SIZE_TYPE_4x5:
	case WIDGET_SIZE_TYPE_4x6:
	case WIDGET_SIZE_TYPE_EASY_1x1:
	case WIDGET_SIZE_TYPE_EASY_3x1:
	case WIDGET_SIZE_TYPE_EASY_3x3:
	default:
		/* Unsupported */
		free(filename);
		return NULL;
	}

	preview = elm_layout_add(parent);
	if (!preview) {
		ErrPrint("Failed to add a new layout\n");
		free(filename);
		return NULL;
	}

	ret = elm_layout_file_set(preview, EDJE_FILE, size_str);
	if (ret != EINA_TRUE) {
		ErrPrint("Failed to load a layout file\n");
		evas_object_del(preview);
		free(filename);
		return NULL;
	}

	bg = evas_object_rectangle_add(widget_data->e);
	if (bg) {
		elm_object_part_content_set(preview, "bg", bg);
		bg_register_object(preview);
	}

	/* Load image */
	if (filename) {
		thumbnail = evas_object_image_filled_add(evas_object_evas_get(preview));
		if (!thumbnail) {
			ErrPrint("Failed to add an image\n");
			evas_object_del(preview);
			free(filename);
			return NULL;
		}

		evas_object_image_file_set(thumbnail, filename, NULL);
		ret = evas_object_image_load_error_get(thumbnail);
		if (ret != EVAS_LOAD_ERROR_NONE) {
			ErrPrint("Failed to set file: %s\n", filename);
			evas_object_image_file_set(thumbnail, UNKNOWN_ICON, NULL);
			ret = evas_object_image_load_error_get(thumbnail);
			if (ret != EVAS_LOAD_ERROR_NONE) {
				ErrPrint("Failed to set file: %s\n", UNKNOWN_ICON);
				evas_object_del(thumbnail);
				evas_object_del(preview);
				free(filename);
				return NULL;
			}
		}
		evas_object_image_size_get(thumbnail, &w, &h);
		evas_object_image_fill_set(thumbnail, 0, 0, w, h);
		free(filename);
	} else {
		Evas_Object *icon_image;

		filename = (char *)add_viewer_package_list_icon(package);
		name = add_viewer_package_list_name(package);

		DbgPrint("Image file: [%s] (%s)\n", filename, name);
		icon_image = evas_object_image_filled_add(evas_object_evas_get(preview));
		if (!icon_image) {
			evas_object_del(preview);
			return NULL;
		}

		evas_object_image_file_set(icon_image, filename, NULL);
		ret = evas_object_image_load_error_get(icon_image);
		if (ret != EVAS_LOAD_ERROR_NONE) {
			evas_object_image_file_set(icon_image, UNKNOWN_ICON, NULL);
			ret = evas_object_image_load_error_get(icon_image);
			if (ret != EVAS_LOAD_ERROR_NONE) {
				ErrPrint("Failed to set file\n");
				evas_object_del(icon_image);
				evas_object_del(preview);
				return NULL;
			}
		}
		evas_object_image_size_get(icon_image, &w, &h);
		evas_object_image_fill_set(icon_image, 0, 0, w, h);
		//evas_object_image_preload(icon_image, EINA_TRUE);

		thumbnail = elm_layout_add(parent);
		if (!thumbnail) {
			evas_object_del(icon_image);
			evas_object_del(preview);
			ErrPrint("Failed to create a layout\n");
			return NULL;
		}

		if (elm_layout_file_set(thumbnail, EDJE_FILE, icon_group) != EINA_TRUE) {
			ErrPrint("Failed to load a file\n");
			evas_object_del(icon_image);
			evas_object_del(preview);
			evas_object_del(thumbnail);
			return NULL;
		}

		elm_object_part_content_set(thumbnail, "icon", icon_image);
		elm_object_part_text_set(thumbnail, "text", name);
	}

	elm_object_part_content_set(preview, "preview", thumbnail);

	if (!name) {
		name = add_viewer_package_list_name(package);
	}

	label = elm_label_add(preview);
	if (!label) {
		ErrPrint("Failed to create the label\n");
		evas_object_del(preview);
		evas_object_del(thumbnail);
		return NULL;
	}
	if (name) {
		snprintf(buf, sizeof(buf), "<align=center><color=#FFFFFF>%s</color></align>", name);
	} else {
		snprintf(buf, sizeof(buf), "<align=center><color=#FFFFFF>%s</color></align>", " ");
	}
	elm_object_text_set(label, buf);
	elm_object_style_set(label, "slide_short");
	elm_label_wrap_width_set(label, ELM_SCALE_SIZE(ADD_VIEWER_TEXT_WIDTH));
	evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);

	_operator_name_slide_mode_set(label);
	evas_object_show(label);
	elm_object_part_content_set(preview, "title", label);

	//elm_object_part_text_set(preview, "title", name);

	DbgPrint("[%s] Image %dx%d\n", name, w, h);
	edje_object_size_min_calc(elm_layout_edje_get(preview), &w, &h);
	evas_object_size_hint_min_set(preview, w, h);
	DbgPrint("[%s] Image min %dx%d\n", name, w, h);

	if (!no_event) {
		struct click *click_cbdata;

		click_cbdata = calloc(1, sizeof(*click_cbdata));
		if (!click_cbdata) {
			ErrPrint("Heap: %d \n", errno);
			evas_object_del(thumbnail);
			evas_object_del(preview);
			return NULL;
		}

		click_cbdata->package = package;
		click_cbdata->size = type;
		click_cbdata->widget_data = widget_data;
		elm_object_signal_callback_add(preview, "clicked", "preview", add_to_home_cb, click_cbdata);
		evas_object_event_callback_add(preview, EVAS_CALLBACK_MOUSE_DOWN, preview_down_cb, click_cbdata);
		evas_object_event_callback_add(preview, EVAS_CALLBACK_MOUSE_MOVE, preview_move_cb, click_cbdata);
		evas_object_event_callback_add(preview, EVAS_CALLBACK_MOUSE_UP, preview_up_cb, click_cbdata);
		evas_object_event_callback_add(preview, EVAS_CALLBACK_DEL, del_cb, click_cbdata);

		winset_access_object_add(parent, preview, size_str + idx, add_viewer_package_list_name(package));
	}
	elm_object_signal_emit(preview, "hide,im", "line");
	elm_object_signal_emit(preview, add_viewer_package_is_skipped(package) ? "show" : "hide", "duplicated");
	edje_object_message_signal_process(elm_layout_edje_get(preview));

	evas_object_show(preview);
	return preview;
}

HAPI Evas_Object *evas_object_add_viewer_add(Evas_Object *parent)
{
	struct widget_data *widget_data;
	Evas_Object *add_viewer;
	Evas *e;

	if (!s_info.smart) {
		s_info.sc.add = widget_add;
		s_info.sc.del = widget_del;
		s_info.sc.move = widget_move;
		s_info.sc.resize = widget_resize;
		s_info.sc.show = widget_show;
		s_info.sc.hide = widget_hide;
		s_info.sc.color_set = widget_color_set;
		s_info.sc.clip_set = widget_clip_set;
		s_info.sc.clip_unset = widget_clip_unset;

		s_info.smart = evas_smart_class_new(&s_info.sc);
		if (!s_info.smart) {
			ErrPrint("Failed to create a new smart class\n");
			return NULL;
		}
	}

	e = evas_object_evas_get(parent);
	if (!e) {
		ErrPrint("Failed to get \"Evas\"\n");
		return NULL;
	}

	/*
	 * Invoke widget_add callback
	 */
	add_viewer = evas_object_smart_add(e, s_info.smart);
	if (!add_viewer) {
		ErrPrint("Failed to create a new object\n");
		return NULL;
	}

	widget_data = evas_object_smart_data_get(add_viewer);
	if (!widget_data) {
		ErrPrint("Failed to get smart data\n");
		evas_object_del(add_viewer);
		return NULL;
	}

	if (widget_data_setup(widget_data, parent) < 0) {
		ErrPrint("Failed to initiate the widget_data\n");
		evas_object_del(add_viewer);
		return NULL;
	}

	reload_list_cb(NULL, widget_data);

	return add_viewer;
}

HAPI void evas_object_add_viewer_conf_set(int type, int flag)
{
	switch (type) {
	case ADD_VIEWER_CONF_DND:
		s_info.enable_dnd = flag;
		break;
	default:
		break;
	}
}

HAPI int evas_object_add_viewer_access_action(Evas_Object *obj, int type, void *info)
{
	struct widget_data *widget_data;

	if (!evas_object_smart_type_check(obj, ADD_VIEWER_CLASS_NAME)) {
		return WIDGET_ERROR_INVALID_PARAMETER;
	}

	widget_data = evas_object_smart_data_get(obj);
	if (!widget_data) {
		return WIDGET_ERROR_FAULT;
	}

	elm_access_action(widget_data->scroller, type, info);

	return WIDGET_ERROR_NONE;
}

HAPI int evas_object_add_viewer_reload(void)
{
	Eina_List *l;
	Eina_List *n;
	struct widget_data *widget_data;

	add_viewer_package_reload_name();

	EINA_LIST_FOREACH_SAFE(s_info.add_viewer_list, l, n, widget_data) {
		reload_list_cb(NULL, widget_data);
	}

	return 0;
}

/* End of a file */
