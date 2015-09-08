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

#include <Ecore_X.h>
#include <Elementary.h>
#include <stdbool.h>
#include <stdlib.h>
#include <utilX.h>
#include <dlog.h>
#include <bundle.h>
#include <efl_assist.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>

#include "layout_info.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "key.h"
#include "minictrl.h"
#include "page_info.h"
#include "scroller_info.h"
#include "scroller.h"



#define CB_LIST_MAX 1
static struct {
	Eina_Bool pressed;
	Ecore_Event_Handler *press_handler;
	Ecore_Event_Handler *release_handler;

	Eina_Bool register_handler;
	Ecore_Timer *long_press;
	Eina_Bool home_grabbed;
	Eina_List *cbs_list[CB_LIST_MAX];
} key_info = {
	.pressed = 0,
	.press_handler = NULL,
	.release_handler = NULL,

	.register_handler = EINA_FALSE,
	.long_press = NULL,
	.home_grabbed = EINA_FALSE,
	.cbs_list = {NULL, },
};



typedef struct {
	key_cb_ret_e (*result_cb)(void *);
	void *result_data;
} key_cb_s;



HAPI w_home_error_e key_register_cb(
	int type,
	key_cb_ret_e (*result_cb)(void *), void *result_data)
{
	retv_if(NULL == result_cb, W_HOME_ERROR_INVALID_PARAMETER);

	key_cb_s *cb = calloc(1, sizeof(key_cb_s));
	retv_if(NULL == cb, W_HOME_ERROR_FAIL);

	cb->result_cb = result_cb;
	cb->result_data = result_data;

	key_info.cbs_list[type] = eina_list_prepend(key_info.cbs_list[type], cb);
	retv_if(NULL == key_info.cbs_list[type], W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}



HAPI void key_unregister_cb(
	int type,
	key_cb_ret_e (*result_cb)(void *))
{

	const Eina_List *l;
	const Eina_List *n;
	key_cb_s *cb;
	EINA_LIST_FOREACH_SAFE(key_info.cbs_list[type], l, n, cb) {
		continue_if(NULL == cb);
		if (result_cb != cb->result_cb) continue;
		key_info.cbs_list[type] = eina_list_remove(key_info.cbs_list[type], cb);
		free(cb);
		return;
	}
}



static void _execute_cbs(int type)
{
	const Eina_List *l;
	const Eina_List *n;
	key_cb_s *cb;
	EINA_LIST_FOREACH_SAFE(key_info.cbs_list[type], l, n, cb) {
		continue_if(NULL == cb);
		continue_if(NULL == cb->result_cb);
		if (KEY_CB_RET_STOP == cb->result_cb(cb->result_data)) {
			_W("back key execution has been stopped");
			break;
		}
	}
}



#define HOME_KEY_ESC "XF86PowerOff"
static Eina_Bool _key_release_cb(void *data, int type, void *event)
{
	Evas_Event_Key_Up *ev = event;

	retv_if(EINA_FALSE == key_info.register_handler, ECORE_CALLBACK_PASS_ON);
	retv_if(NULL == ev, ECORE_CALLBACK_PASS_ON);

	_D("Key(%s) released %d", ev->keyname, key_info.pressed);

	if (key_info.pressed == EINA_FALSE) {
		return ECORE_CALLBACK_PASS_ON;
	}

	if (!strcmp(ev->keyname, KEY_BACK)) {
		_execute_cbs(KEY_TYPE_BACK);
	} else if (!strcmp(ev->keyname, KEY_MENU)) {
		_execute_cbs(KEY_TYPE_BEZEL_UP);
	}

	key_info.pressed = EINA_FALSE;

	return ECORE_CALLBACK_PASS_ON;
}



static Eina_Bool _key_press_cb(void *data, int type, void *event)
{
	Evas_Event_Key_Down *ev = event;

	retv_if(EINA_FALSE == key_info.register_handler, ECORE_CALLBACK_PASS_ON);
	retv_if(NULL == ev, ECORE_CALLBACK_PASS_ON);

	key_info.pressed = EINA_TRUE;
	_D("Key pressed %d", key_info.pressed);

	retv_if(APP_STATE_PAUSE == main_get_info()->state, ECORE_CALLBACK_PASS_ON);

	return ECORE_CALLBACK_PASS_ON;
}



HAPI void key_register(void)
{
	if (!key_info.release_handler) {
		key_info.release_handler = ecore_event_handler_add(ECORE_EVENT_KEY_UP, _key_release_cb, NULL);
		if (!key_info.release_handler) {
			_E("Failed to register a key up event handler");
		}
	}

	if (!key_info.press_handler) {
		key_info.press_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _key_press_cb, NULL);
		if (!key_info.press_handler) {
			_E("Failed to register a key down event handler");
		}
	}

	if (utilx_grab_key(ecore_x_display_get(), elm_win_xwindow_get(main_get_info()->win), KEY_MENU, TOP_POSITION_GRAB) != 0) {
		_E("Failed to grab KEY_MENU");
	}

	key_info.pressed = EINA_FALSE;
	key_info.register_handler = EINA_TRUE;
}



HAPI void key_unregister(void)
{
	if (key_info.long_press) {
		ecore_timer_del(key_info.long_press);
		key_info.long_press = NULL;
	}

	if (key_info.release_handler) {
		ecore_event_handler_del(key_info.release_handler);
		key_info.release_handler = NULL;
	}

	if (key_info.press_handler) {
		ecore_event_handler_del(key_info.press_handler);
		key_info.press_handler = NULL;
	}

	if (utilx_ungrab_key(ecore_x_display_get(), elm_win_xwindow_get(main_get_info()->win), KEY_MENU) != 0) {
		_E("Failed to ungrab KEY_MENU");
	}

	key_info.register_handler = EINA_FALSE;
}



HAPI void key_cb_execute(int type)
{
	_execute_cbs(type);
}

// End of a file
