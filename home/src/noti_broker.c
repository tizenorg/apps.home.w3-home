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
#include <dlfcn.h>
#include <bundle.h>
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
#include "key.h"
#include "tutorial.h"
#include "noti_broker.h"
#include "apps/apps_main.h"
#include "critical_log.h"

#define NOTI_BROKER_ERROR_NONE 0
#define NOTI_BROKER_ERROR_FAIL -1
#define NOTI_BROKER_PLUGIN_PATH "/usr/lib/libnoti-sample.so"

/*
 * Events, To notify events to plugin
 */
const int EVENT_SOURCE_SCROLLER = 0x00000000;
const int EVENT_SOURCE_VIEW = 0x00000001;
const int EVENT_SOURCE_EDITING = 0x00000002;

const int EVENT_RET_NONE = 0x00000000;
const int EVENT_RET_CONTINUE = 0x00000004;
const int EVENT_RET_STOP = 0x00000008;

#define EVENT_TYPE_ANIM 0x00000000
const int EVENT_TYPE_ANIM_START = EVENT_TYPE_ANIM | 0x0;
const int EVENT_TYPE_ANIM_STOP = EVENT_TYPE_ANIM | 0x1;
#define EVENT_TYPE_DRAG 0x00001000
const int EVENT_TYPE_DRAG_START = EVENT_TYPE_DRAG | 0x0;
const int EVENT_TYPE_DRAG_STOP = EVENT_TYPE_DRAG | 0x1;
#define EVENT_TYPE_EDGE 0x00002000
const int EVENT_TYPE_EDGE_LEFT = EVENT_TYPE_EDGE | 0x0;
const int EVENT_TYPE_EDGE_RIGHT = EVENT_TYPE_EDGE | 0x1;
#define EVENT_TYPE_EDIT 0x00004000
const int EVENT_TYPE_EDIT_START = EVENT_TYPE_EDIT | 0x0;
const int EVENT_TYPE_EDIT_STOP = EVENT_TYPE_EDIT | 0x1;
#define EVENT_TYPE_NOTI 0x00008000
const int EVENT_TYPE_NOTI_DELETE = EVENT_TYPE_NOTI | 0x0;
const int EVENT_TYPE_NOTI_DELETE_ALL = EVENT_TYPE_NOTI | 0x1;
#define EVENT_TYPE_MOUSE 0x00010000
const int EVENT_TYPE_MOUSE_DOWN = EVENT_TYPE_MOUSE | 0x0;
const int EVENT_TYPE_MOUSE_UP = EVENT_TYPE_MOUSE | 0x1;
#define EVENT_TYPE_SCROLL 0x00020000
const int EVENT_TYPE_SCROLLING = EVENT_TYPE_SCROLL | 0x0;
#define EVENT_TYPE_KEY 0x00040000
const int EVENT_TYPE_KEY_BACK = EVENT_TYPE_KEY | 0x0;
#define EVENT_TYPE_APPS 0x00080000
const int EVENT_TYPE_APPS_SHOW = EVENT_TYPE_APPS | 0x0;
const int EVENT_TYPE_APPS_HIDE = EVENT_TYPE_APPS | 0x1;

/*
 * Categories, To discrete views
 */
const int CATEGORY_NOTIFICATION = 0x00000001;
const int CATEGORY_DASHBOARD = 0x00000002;

/*
 * Functions, To handle requests from plugin
 */
#define FUNCTION_DEF(X) const int X = E_##X
#define FUNCTION_VAL(X) E_##X
enum {
	FUNCTION_VAL(FUNCTION_NONE) = 0x0,
	FUNCTION_VAL(FUNCTION_PAGE_CREATE) = 0x00000001,
	FUNCTION_VAL(FUNCTION_PAGE_DESTROY) = 0x00000002,
	FUNCTION_VAL(FUNCTION_PAGE_ADD) = 0x00000004,
	FUNCTION_VAL(FUNCTION_PAGE_REMOVE) = 0x00000008,
	FUNCTION_VAL(FUNCTION_PAGE_SHOW) = 0x00000010,
	FUNCTION_VAL(FUNCTION_PAGE_RELOAD) = 0x00000020,
	FUNCTION_VAL(FUNCTION_PAGE_ITEM_SET) = 0x00000040,
	FUNCTION_VAL(FUNCTION_PAGE_ITEM_GET) = 0x00000080,
	FUNCTION_VAL(FUNCTION_SCROLLER_LEFT_PAGE_GET) = 0x00000100,
	FUNCTION_VAL(FUNCTION_SCROLLER_RIGHT_PAGE_GET) = 0x00000200,
	FUNCTION_VAL(FUNCTION_SCROLLER_FOCUSED_PAGE_GET) = 0x00000400,
	FUNCTION_VAL(FUNCTION_WINDOW_ACTIVATE) = 0x00000800,
	FUNCTION_VAL(FUNCTION_WINDOW_GET) = 0x00001000,
	FUNCTION_VAL(FUNCTION_PAGE_SHOW_NO_DELAY) = 0x00002000,
	FUNCTION_VAL(FUNCTION_PAGE_FOCUS_OBJECT_GET) = 0x00004000,
	FUNCTION_VAL(FUNCTION_PAGE_REORDER) = 0x00008000,
};
FUNCTION_DEF(FUNCTION_NONE);
FUNCTION_DEF(FUNCTION_PAGE_CREATE);
FUNCTION_DEF(FUNCTION_PAGE_DESTROY);
FUNCTION_DEF(FUNCTION_PAGE_ADD);
FUNCTION_DEF(FUNCTION_PAGE_REMOVE);
FUNCTION_DEF(FUNCTION_PAGE_SHOW);
FUNCTION_DEF(FUNCTION_PAGE_SHOW_NO_DELAY);
FUNCTION_DEF(FUNCTION_PAGE_RELOAD);
FUNCTION_DEF(FUNCTION_PAGE_REORDER);
FUNCTION_DEF(FUNCTION_PAGE_ITEM_SET);
FUNCTION_DEF(FUNCTION_PAGE_ITEM_GET);
FUNCTION_DEF(FUNCTION_PAGE_FOCUS_OBJECT_GET);
FUNCTION_DEF(FUNCTION_SCROLLER_LEFT_PAGE_GET);
FUNCTION_DEF(FUNCTION_SCROLLER_RIGHT_PAGE_GET);
FUNCTION_DEF(FUNCTION_SCROLLER_FOCUSED_PAGE_GET);
FUNCTION_DEF(FUNCTION_WINDOW_ACTIVATE);
FUNCTION_DEF(FUNCTION_WINDOW_GET);


struct broker_function {
	int function;
	int (*process) (const char *id, int category, void *view, void *data, void *result);
};

typedef struct _Noti_Broker_Plugin_Handler {
	int (*init) (void *parent, void *data);
	int (*fini) (void);
	int (*event) (int source, int event, void *data);
} Noti_Broker_Plugin_Handler;

static struct _s_info {
	int is_loaded;
	int is_initialized;
	void *dl_handler;
	Noti_Broker_Plugin_Handler handle;
} s_info = {
	.is_loaded = 0,
	.is_initialized = 0,
	.dl_handler = NULL,
	.handle = {NULL,NULL,NULL},
};

static Evas_Object *_layout_get(void)
{
	Evas_Object *win = main_get_info()->win;
	retv_if(win == NULL, NULL);

	return evas_object_data_get(win, DATA_KEY_LAYOUT);
}

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

/*!
 * To handle requests from plugin
 */
static int _handler_page_create(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *page = NULL;
	Evas_Object *scroller = _scroller_get();
	retv_if(scroller == NULL, NOTI_BROKER_ERROR_FAIL);

	page_removable_e removable = PAGE_REMOVABLE_ON;

	if (category == CATEGORY_DASHBOARD) {
		removable = PAGE_REMOVABLE_OFF;
	}

	page = page_create(scroller
						, (Evas_Object*)view
						, NULL, NULL
						, main_get_info()->root_w, main_get_info()->root_h
						, PAGE_CHANGEABLE_BG_OFF, removable);
	if (page != NULL && result != NULL) {
		*((Evas_Object **)result) = page;
		page_set_effect(page, page_effect_none, page_effect_none);

		return NOTI_BROKER_ERROR_NONE;
	}

	return NOTI_BROKER_ERROR_FAIL;
}

static int _handler_page_destroy(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *page = view;

	page_destroy(page);

	return NOTI_BROKER_ERROR_NONE;
}

static int _handler_page_add(const char *id, int category, void *view, void *data, void *result)
{
	int ret = W_HOME_ERROR_NONE;
	Evas_Object *page = view;
	Evas_Object *scroller = _scroller_get();;
	retv_if(page == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(scroller == NULL, NOTI_BROKER_ERROR_FAIL);

	if (category == CATEGORY_NOTIFICATION) {
		ret = scroller_push_page(scroller, page, SCROLLER_PUSH_TYPE_CENTER_LEFT);
	} else if (category == CATEGORY_DASHBOARD) {
		ret = scroller_push_page(scroller, page, SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_LEFT);
	}

	return (ret == W_HOME_ERROR_NONE) ? NOTI_BROKER_ERROR_NONE : NOTI_BROKER_ERROR_FAIL;
}

static int _handler_page_remove(const char *id, int category, void *view, void *data, void *result)
{
	int ret = NOTI_BROKER_ERROR_NONE;
	Evas_Object *page = view;
	Evas_Object *scroller = _scroller_get();
	retv_if(page == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(scroller == NULL, NOTI_BROKER_ERROR_FAIL);

	if (scroller_pop_page(scroller, page) == NULL) {
		_E("Failed to remove page(%p)", page);
		ret = NOTI_BROKER_ERROR_FAIL;
	}
	evas_object_hide(page);

	return ret;
}

static int _handler_page_show(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *page = view;
	Evas_Object *scroller = _scroller_get();
	retv_if(scroller == NULL, NOTI_BROKER_ERROR_FAIL);

	if (tutorial_is_exist() == 1) {
		_E("tutorial is exist, can't bring the page");
		return NOTI_BROKER_ERROR_FAIL;
	}

	scroller_bring_in_page(scroller, page, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_ANIMATOR);

	return NOTI_BROKER_ERROR_NONE;
}

static int _handler_page_show_no_delay(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *page = view;
	Evas_Object *scroller = _scroller_get();
	retv_if(scroller == NULL, NOTI_BROKER_ERROR_FAIL);

	scroller_bring_in_page(scroller, page, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT_SHOW);

	return NOTI_BROKER_ERROR_NONE;
}

static int _handler_page_reload(const char *id, int category, void *view, void *data, void *result)
{
	int ret = W_HOME_ERROR_NONE;
	Evas_Object *page = view;
	Evas_Object *scroller = _scroller_get();
	retv_if(scroller == NULL, NOTI_BROKER_ERROR_FAIL);

	if (category == CATEGORY_NOTIFICATION) {
		ret = scroller_push_page(scroller, page, SCROLLER_PUSH_TYPE_CENTER_LEFT);
	} else if (category == CATEGORY_DASHBOARD) {
		if (scroller_pop_page(scroller, page) == NULL) {
			_E("Failed to pop page(%p)", page);
		}
		ret = scroller_push_page(scroller, page, SCROLLER_PUSH_TYPE_CENTER_NEIGHBOR_LEFT);
	}

	return (ret == W_HOME_ERROR_NONE) ? NOTI_BROKER_ERROR_NONE : NOTI_BROKER_ERROR_FAIL;
}

static int _handler_page_reorder(const char *id, int category, void *view, void *data, void *result)
{
	int ret = W_HOME_ERROR_NONE;
	Eina_List *list = data;
	Evas_Object *scroller = _scroller_get();
	retv_if(list == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(scroller == NULL, NOTI_BROKER_ERROR_FAIL);

	scroller_reorder_with_list(scroller, list, PAGE_DIRECTION_LEFT);

	return (ret == W_HOME_ERROR_NONE) ? NOTI_BROKER_ERROR_NONE : NOTI_BROKER_ERROR_FAIL;
}

static int _handler_page_item_set(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *page = view;
	Evas_Object *item = data;
	retv_if(page == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(item == NULL, NOTI_BROKER_ERROR_FAIL);

	page_set_item(page, item);

	return NOTI_BROKER_ERROR_NONE;
}

static int _handler_page_item_get(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *page = view;
	retv_if(page == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(result == NULL, NOTI_BROKER_ERROR_FAIL);

	*((Evas_Object **)result) = page_get_item(page);

	return NOTI_BROKER_ERROR_NONE;
}

static int _handler_page_focus_object_get(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *page = view;
	page_info_s *page_info = NULL;
	retv_if(page == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(result == NULL, NOTI_BROKER_ERROR_FAIL);

	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(page_info == NULL, NOTI_BROKER_ERROR_FAIL);
	*((Evas_Object **)result) = page_info->focus;

	return NOTI_BROKER_ERROR_NONE;
}

static int _handler_scroller_left_page_get(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *page = view;
	Evas_Object *scroller = _scroller_get();
	retv_if(page == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(scroller == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(result == NULL, NOTI_BROKER_ERROR_FAIL);

	*((Evas_Object **)result) = scroller_get_left_page(scroller, page);

	return NOTI_BROKER_ERROR_NONE;
}

static int _handler_scroller_right_page_get(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *page = view;
	Evas_Object *scroller = _scroller_get();
	retv_if(page == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(scroller == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(result == NULL, NOTI_BROKER_ERROR_FAIL);

	*((Evas_Object **)result) = scroller_get_right_page(scroller, page);

	return NOTI_BROKER_ERROR_NONE;
}

static int _handler_scroller_focused_page_get(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *scroller = _scroller_get();
	retv_if(scroller == NULL, NOTI_BROKER_ERROR_FAIL);
	retv_if(result == NULL, NOTI_BROKER_ERROR_FAIL);

	*((Evas_Object **)result) = scroller_get_focused_page(scroller);

	return NOTI_BROKER_ERROR_NONE;
}

static int _handler_window_activate(const char *id, int category, void *view, void *data, void *result)
{
	Evas_Object *win = main_get_info()->win;
	retv_if(win == NULL, NOTI_BROKER_ERROR_FAIL);

	if (tutorial_is_exist() == 1) {
		_E("tutorial is exist, can't activate home window");
		return NOTI_BROKER_ERROR_FAIL;
	}

	elm_win_activate(win);

	if (apps_main_is_visible() == EINA_TRUE) {
		apps_main_launch(APPS_LAUNCH_HIDE);
	}

	return NOTI_BROKER_ERROR_NONE;
}

static int _handler_window_get(const char *id, int category, void *view, void *data, void *result)
{
	retv_if(result == NULL, NOTI_BROKER_ERROR_FAIL);

	*((Evas_Object **)result) = main_get_info()->win;

	return NOTI_BROKER_ERROR_NONE;
}

int noti_broker_request(int function, const char *id, int category, void *view, void *data, void *result)
{
	//do something
	Evas_Object *scroller = _scroller_get();
	retv_if(scroller == NULL, -1);

	if (function == FUNCTION_PAGE_CREATE ||
		function == FUNCTION_PAGE_DESTROY ||
		function == FUNCTION_PAGE_ADD ||
		function == FUNCTION_PAGE_REMOVE ||
		function == FUNCTION_PAGE_SHOW ||
		function == FUNCTION_PAGE_RELOAD ||
		function == FUNCTION_PAGE_REORDER) {
		_W("%x %s %d %p %p %p", function, id, category, view, data, result);
	}

	static struct broker_function fn_table[] = {
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_CREATE),
			.process = _handler_page_create,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_DESTROY),
			.process = _handler_page_destroy,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_ADD),
			.process = _handler_page_add,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_REMOVE),
			.process = _handler_page_remove,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_SHOW),
			.process = _handler_page_show,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_RELOAD),
			.process = _handler_page_reload,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_REORDER),
			.process = _handler_page_reorder,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_ITEM_SET),
			.process = _handler_page_item_set,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_ITEM_GET),
			.process = _handler_page_item_get,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_SCROLLER_LEFT_PAGE_GET),
			.process = _handler_scroller_left_page_get,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_SCROLLER_RIGHT_PAGE_GET),
			.process = _handler_scroller_right_page_get,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_SCROLLER_FOCUSED_PAGE_GET),
			.process = _handler_scroller_focused_page_get,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_WINDOW_ACTIVATE),
			.process = _handler_window_activate,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_WINDOW_GET),
			.process = _handler_window_get,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_SHOW_NO_DELAY),
			.process = _handler_page_show_no_delay,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_PAGE_FOCUS_OBJECT_GET),
			.process = _handler_page_focus_object_get,
		},
		{
			.function = FUNCTION_VAL(FUNCTION_NONE),
			.process = NULL,
		},
	};

	int i = 0;
	int ret = NOTI_BROKER_ERROR_NONE;
	for (i = 0; fn_table[i].function != FUNCTION_VAL(FUNCTION_NONE); i++) {
		if (function == fn_table[i].function && fn_table[i].process != NULL) {
			ret = fn_table[i].process(id, category, view, data, result);
		}
	}

	return ret;
}

static void _scroller_evas_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	int event_type = (int)data;

	noti_broker_event_fire_to_plugin(EVENT_SOURCE_SCROLLER, event_type, event_info);
}

static void _layout_smart_cb(void *data, Evas_Object *scroller, void *event_info)
{
	int event_type = (int)data;

	noti_broker_event_fire_to_plugin(EVENT_SOURCE_VIEW, event_type, event_info);
}

static void _scroller_smart_cb(void *data, Evas_Object *scroller, void *event_info)
{
	int event_type = (int)data;

	noti_broker_event_fire_to_plugin(EVENT_SOURCE_SCROLLER, event_type, event_info);
}

static key_cb_ret_e _noti_broker_back_cb(void *data)
{
	int ret = EVENT_RET_CONTINUE;
	if (tutorial_is_exist() == 1) {
		return KEY_CB_RET_CONTINUE;
	}

	ret = noti_broker_event_fire_to_plugin(EVENT_SOURCE_VIEW, EVENT_TYPE_KEY_BACK, NULL);
	if (ret == EVENT_RET_STOP) {
		_W("stop back key execution");
		return KEY_CB_RET_STOP;
	}

	_W("continue the back key execution");
	return KEY_CB_RET_CONTINUE;
}

static void _evas_object_events_register(Evas_Object *layout, Evas_Object *scroller)
{
	evas_object_event_callback_add(scroller, EVAS_CALLBACK_MOUSE_DOWN, _scroller_evas_cb, (void*)EVENT_TYPE_MOUSE_DOWN);
	evas_object_event_callback_add(scroller, EVAS_CALLBACK_MOUSE_UP, _scroller_evas_cb, (void*)EVENT_TYPE_MOUSE_UP);

	evas_object_smart_callback_add(scroller, "scroll,anim,start", _scroller_smart_cb, (void*)EVENT_TYPE_ANIM_START);
	evas_object_smart_callback_add(scroller, "scroll,anim,stop", _scroller_smart_cb, (void*)EVENT_TYPE_ANIM_STOP);
	evas_object_smart_callback_add(scroller, "scroll,drag,start", _scroller_smart_cb, (void*)EVENT_TYPE_DRAG_START);
	evas_object_smart_callback_add(scroller, "scroll,drag,stop", _scroller_smart_cb, (void*)EVENT_TYPE_DRAG_STOP);
	evas_object_smart_callback_add(scroller, "edge,left", _scroller_smart_cb, (void*)EVENT_TYPE_EDGE_LEFT);
	evas_object_smart_callback_add(scroller, "edge,right", _scroller_smart_cb, (void*)EVENT_TYPE_EDGE_RIGHT);
	evas_object_smart_callback_add(scroller, "scroll", _scroller_smart_cb, (void*)EVENT_TYPE_SCROLL);

	evas_object_smart_callback_add(layout, LAYOUT_SMART_SIGNAL_EDIT_ON,
		_layout_smart_cb, (void *)EVENT_TYPE_EDIT_START);
	evas_object_smart_callback_add(layout, LAYOUT_SMART_SIGNAL_EDIT_OFF,
		_layout_smart_cb, (void *)EVENT_TYPE_EDIT_STOP);

	key_register_cb(KEY_TYPE_BACK, _noti_broker_back_cb, NULL);
}

Eina_Bool _init_timeout_cb(void *data)
{
	if (s_info.is_initialized == 0) {
		_W("noti broker isn't initialized");
		noti_broker_init();
	}

	return ECORE_CALLBACK_CANCEL;
}

/*!
 * constructor/deconstructor
 */
HAPI void noti_broker_load(void)
{
	char *errmsg = NULL;
	void *dl_handle = NULL;

	_W("loading noti broker plugin start");
	dl_handle = dlopen(NOTI_BROKER_PLUGIN_PATH, RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND);
	_W("loading noti broker plugin done");

	errmsg = dlerror();
	if (errmsg) {
		_E("dlerror(can be ignored): %s\n", errmsg);
		CRITICAL_LOG("dlopen failed: %s\n", errmsg);
	}
	ret_if(dl_handle == NULL);

	s_info.handle.init = dlsym(dl_handle, "noti_board_plugin_init");
	if (s_info.handle.init == NULL) {
		_E("Failed to find noti_board_plugin_init");
		goto ERR;
	}

	s_info.handle.fini = dlsym(dl_handle, "noti_board_plugin_fini");
	if (s_info.handle.fini == NULL) {
		_E("Failed to find noti_board_plugin_fini");
		goto ERR;
	}

	s_info.handle.event = dlsym(dl_handle, "noti_board_plugin_event");
	if (s_info.handle.event == NULL) {
		_E("Failed to find noti_board_plugin_event");
		goto ERR;
	}

	s_info.is_loaded = 1;
	s_info.dl_handler = dl_handle;

	ecore_timer_add(10.0f, _init_timeout_cb, NULL);

	return ;

ERR:
	if (dl_handle) dlclose(dl_handle);
}

HAPI void noti_broker_init(void)
{
	Evas_Object *layout = _layout_get();
	Evas_Object *scroller = _scroller_get();
	ret_if(s_info.is_loaded == 0);
	ret_if(s_info.is_initialized == 1);
	ret_if(layout == NULL);
	ret_if(scroller == NULL);

	s_info.is_initialized = 1;
	s_info.handle.init(scroller, scroller);
	_evas_object_events_register(layout, scroller);
}

HAPI void noti_broker_fini(void)
{
	ret_if(s_info.is_loaded == 0);
	s_info.is_loaded = 0;

	s_info.handle.fini();
	s_info.is_initialized = 0;

	if (s_info.dl_handler != NULL) {
		dlclose(s_info.dl_handler);
	}
}

/*!
 * To notify events to plugin
 */
HAPI int noti_broker_event_fire_to_plugin(int source, int event, void *data)
{
	retv_if(s_info.is_loaded == 0, EVENT_RET_NONE);

	if (event == EVENT_TYPE_APPS_SHOW ||
		event == EVENT_TYPE_APPS_HIDE) {
		_W("source:%d event:%x", source, event);
	}

	return s_info.handle.event(source, event, data);
}
