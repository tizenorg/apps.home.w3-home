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



#include <Ecore_X.h>
#include <Elementary.h>
#include <efl_assist.h>
#include <utilX.h>
#include <vconf.h>

#include "conf.h"
#include "log.h"
#include "util.h"
#include "layout_info.h"
#include "effect.h"
#include "key.h"
#include "main.h"
#include "page_info.h"
#include "page.h"
#include "scroller_info.h"
#include "scroller.h"
#include "tutorial.h"
#include "tutorial_info.h"
#include "clock_service.h"
#include "apps/apps_main.h"

#define PRIVATE_DATA_KEY_TUTORIAL_PRESSED "p"
#define PRIVATE_DATA_KEY_TUTORIAL_DOWN_X "d_x"
#define PRIVATE_DATA_KEY_TUTORIAL_DOWN_Y "d_y"
#define PRIVATE_DATA_KEY_TUTORIAL_STEP "p_n"
#define PRIVATE_DATA_KEY_TUTORIAL_INFO "p_t_i"
#define PRIVATE_DATA_KEY_TUTORIAL_TEXT "p_t"
#define PRIVATE_DATA_KEY_TUTORIAL_ENABLE_INDICATOR "p_t_e_i"
#define PRIVATE_DATA_KEY_TUTORIAL_TRANSIENT_TIMER "pdk_ttt"

#define TUTORIAL_THRESHOLD_ABLE_X 150
#define TUTORIAL_THRESHOLD_ABLE_Y 150

#define TUTORIAL_THRESHOLD_DISABLE_X 250
#define TUTORIAL_THRESHOLD_DISABLE_Y 150

#define STEP_APPS ((void *) 1)
#define APPS_FIRST_LIST 0



static struct {
	Evas_Object *win;
	Ecore_Timer *transient_timer;
	Ecore_Timer *step_eight_timer;
	Eina_List *transient_list;
} _tutorial_info = {
	.win = NULL,
	.transient_timer = NULL,
	.step_eight_timer = NULL,
	.transient_list = NULL,
};



static void _append_transient_list(Ecore_X_Window win, Ecore_X_Window for_win)
{
	_W("%p is transient for %p", win, for_win);
	ecore_x_icccm_transient_for_set(win, for_win);

	_tutorial_info.transient_list
			= eina_list_append(_tutorial_info.transient_list, (void *) win);
}



static void _destroy_transient_list(void)
{
	void *xwin;

	if (!_tutorial_info.transient_list) return;

	EINA_LIST_FREE(_tutorial_info.transient_list, xwin) {
		_W("%p is not transient", xwin);
		ecore_x_icccm_transient_for_unset((Ecore_X_Window) xwin);
	}
}



static Eina_Bool _set_transient_for_timer(void *data)
{
	Ecore_X_Window home_xwin;
	Ecore_X_Window tutorial_xwin;
	Ecore_X_Window xwin = (Ecore_X_Window)data;

	retv_if(!xwin, ECORE_CALLBACK_CANCEL);

	_D("Set normal type window");

	home_xwin = elm_win_xwindow_get(main_get_info()->win);
	retv_if(!home_xwin, ECORE_CALLBACK_CANCEL);

	tutorial_xwin = elm_win_xwindow_get(_tutorial_info.win);
	retv_if(!tutorial_xwin, ECORE_CALLBACK_CANCEL);

	/* unset transient */
	_destroy_transient_list();

	/* set tutorial window to normal type */
	ecore_x_netwm_window_type_set(tutorial_xwin, ECORE_X_WINDOW_TYPE_NORMAL);

	/* set transient */
	_append_transient_list(tutorial_xwin, xwin);
	_append_transient_list(xwin, home_xwin);

	_tutorial_info.transient_timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}



#define TIMER_TIME_SET_TRANSIENT 1.0f
HAPI void tutorial_set_transient_for(Ecore_X_Window xwin)
{
	ret_if(!xwin);

	if (_tutorial_info.transient_timer) {
		ecore_timer_del(_tutorial_info.transient_timer);
		_tutorial_info.transient_timer = NULL;
	}

	_tutorial_info.transient_timer = ecore_timer_add(TIMER_TIME_SET_TRANSIENT, _set_transient_for_timer, (void *)xwin);
	if (!_tutorial_info.transient_timer) _E("Cannot add a timer");
}



HAPI int tutorial_is_first_boot(void)
{
	int tutorial_enabled = 0;

	if (vconf_get_int(VCONF_KEY_HOME_IS_TUTORIAL_ENABLED_TO_RUN, &tutorial_enabled) != 0) {
		_E("Cannot get the vconf for %s", VCONF_KEY_HOME_IS_TUTORIAL_ENABLED_TO_RUN);
	}

	/* 1 : first try, 0 : others */
	_D("Tutorial is [%d]", tutorial_enabled);

	return tutorial_enabled;
}



HAPI int tutorial_is_exist(void)
{
	layout_info_s *layout_info = NULL;

	layout_info = evas_object_data_get(main_get_info()->layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, 0);

	return layout_info->tutorial? 1 : 0;
}



static void _tutorial_destroy_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	_D("Tutorial will be removed");
	if (tutorial_is_exist()) {
		tutorial_destroy(data);
	}
}



static char *_done_button_info_cb(void *data, Evas_Object *obj)
{
	char *text = NULL;

	text = strdup(_("IDS_ST_BUTTON_OK"));
	retv_if(!text, NULL);

	return text;
}



static void _destroy_done_button(Evas_Object *tutorial)
{
	Evas_Object *button = NULL;
	ret_if(!tutorial);

	button = elm_object_part_content_unset(tutorial, "button");
	ret_if(!button);

	evas_object_del(button);
}



static void _done_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *tutorial = data;
	ret_if(!tutorial);

	elm_object_signal_emit(tutorial, "show", "page9,pressed");
	effect_play_vibration();

	/* only tutorial use */
	if (vconf_set_int(VCONF_KEY_HOME_IS_TUTORIAL_ENABLED_TO_RUN, 0) != 0) {
		_E("Critical, cannot set the private vconf key");
		return;
	}

	 _D("End the tutorial");

	 _destroy_done_button(tutorial);
}



static Evas_Object *_create_done_button(Evas_Object *tutorial)
{
	Evas_Object *button = NULL;

	retv_if(!tutorial, NULL);

	_D("Add done button");

	button = elm_button_add(tutorial);
	retv_if(!button, NULL);

	elm_object_style_set(button, "focus");
	elm_object_part_content_set(tutorial, "button", button);
	evas_object_show(button);

	elm_object_part_text_set(tutorial, "button_text", _("IDS_ST_BUTTON_OK"));
	elm_object_domain_translatable_part_text_set(tutorial, "button_text", PROJECT, "IDS_ST_BUTTON_OK");

	elm_access_info_cb_set(button, ELM_ACCESS_INFO, _done_button_info_cb, tutorial);
	evas_object_smart_callback_add(button, "clicked", _done_button_clicked_cb, tutorial);
	elm_object_signal_callback_add(tutorial, "done", "button", _tutorial_destroy_cb, tutorial);

	return button;
}



static char *_text_start(void)
{
	char *text = NULL;
	text = strdup(_("IDS_WMGR_BODY_WELCOME_E_TAP_THE_BUTTON_BELOW_TO_LEARN_HOW_TO_USE_YOUR_GEAR_ABB"));
	retv_if(!text, NULL);

	return text;
}



static char *_text_structure(void)
{
	char *text = NULL;
	text = strdup(_("IDS_WNOTI_BODY_THIS_IS_THE_MAIN_STRUCTURE_OF_THE_HOME_SCREEN"));
	retv_if(!text, NULL);

	return text;
}



static char *_text_init(void)
{
	char *text = NULL;
	text = strdup(_("IDS_HELP_POP_SWIPE_RIGHT_TO_VIEW_NOTIFICATIONS_ABB"));
	retv_if(!text, NULL);

	return text;
}



static char *_text_one(void)
{
	char *text = NULL;
	text = strdup(_("IDS_HELP_POP_SWIPE_LEFT_TO_GO_BACK_TO_THE_CLOCK_ABB"));
	retv_if(!text, NULL);

	return text;
}



static char *_text_two(void)
{
	char *text = NULL;
	text = strdup(_("IDS_HELP_POP_SWIPE_LEFT_TO_VIEW_WIDGETS_ABB"));
	retv_if(!text, NULL);

	return text;
}



static char *_text_three(void)
{
	char *text = NULL;
	text = strdup(_("IDS_HELP_POP_SWIPE_RIGHT_TO_GO_BACK_TO_THE_CLOCK_ABB"));
	retv_if(!text, NULL);

	return text;
}



static char *_text_four(void)
{
	char *text = NULL;
	text = strdup(_("IDS_WNOTI_BODY_SWIPE_BOTTOM_EDGE_UP_TO_VIEW_APPS_ON_CLOCK_ABB"));
	retv_if(!text, NULL);
	return text;
}



static char *_text_five(void)
{
	char *text = NULL;
	text = strdup(_("IDS_WNOTI_BODY_SWIPE_TOP_EDGE_DOWN_TO_GO_BACK_ABB"));
	retv_if(!text, NULL);

	return text;
}



static char *_text_six(void)
{
	char *text = NULL;
	text = strdup(_("IDS_WNOTI_BODY_SWIPE_TOP_EDGE_DOWN_TO_SEE_INDICATOR_ICONS_ON_CLOCK_ABB"));
	retv_if(!text, NULL);
	return text;
}



static char *_text_seven(void)
{
	char *text = NULL;
	text = strdup(_("IDS_WNOTI_BODY_SWIPE_UPWARDS_TO_GO_BACK_TO_CLOCK_ABB"));
	retv_if(!text, NULL);

	return text;
}



static char *_text_eight(void)
{
	char *text = NULL;
	text = strdup(_("IDS_WMGR_POP_THATS_IT_E_GET_INTO_GEAR_E"));
	retv_if(!text, NULL);

	return text;
}



static char *_access_info_cb(void *data, Evas_Object *obj)
{
	char *(*_text)(void);
	Evas_Object *tutorial = data;
	retv_if(!tutorial, NULL);

	_text = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT);
	retv_if(!_text, NULL);

	return _text();
}



static void _down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ei = event_info;
	Evas_Object *tutorial = data;
	ret_if(!tutorial);

	int x = ei->output.x;
	int y = ei->output.y;

	_D("Tutorial mouse down (%d,  %d)", x, y);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_PRESSED, (void *) 1);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_DOWN_X, (void *) x);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_DOWN_Y, (void *) y);
}



static Eina_Bool _step_eight_timer_cb(void *data)
{
	tutorial_info_s *tutorial_info = NULL;
	int is_wide_character = 0;
	Evas_Object *tutorial = data;

	_tutorial_info.step_eight_timer = NULL;

	retv_if(!tutorial, ECORE_CALLBACK_CANCEL);

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	retv_if(!tutorial_info, ECORE_CALLBACK_CANCEL);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "center");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "center");
	} else {
		elm_object_signal_emit(tutorial, "standard", "center");
	}

	elm_object_signal_emit(tutorial, "show", "page9");
	elm_object_part_text_set(tutorial, "text", _("IDS_WMGR_POP_THATS_IT_E_GET_INTO_GEAR_E"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_WMGR_POP_THATS_IT_E_GET_INTO_GEAR_E");

	elm_access_highlight_set(tutorial_info->text_focus);

	if (!_create_done_button(tutorial)) _E("failed to add the add done button");

	return ECORE_CALLBACK_CANCEL;
}



#define TIMER_TIME_STEP_EIGHT 0.3f
static void _step_eight(Evas_Object *tutorial, int vec_x, int vec_y)
{
	Ecore_X_Window tutorial_xwin;
	Ecore_X_Window home_xwin;
	tutorial_info_s *tutorial_info = NULL;

	ret_if(!tutorial);

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	_D("Step 8");

	if (!(abs(vec_y) > (TUTORIAL_THRESHOLD_ABLE_Y - 70)
		&& abs(vec_x) < TUTORIAL_THRESHOLD_DISABLE_X
		&& vec_y < 0))
	{
		_D("Exit step 8");
		return;
	}

	/* Hide indicator */
	clock_view_indicator_show(INDICATOR_HIDE);

	home_xwin = elm_win_xwindow_get(main_get_info()->win);
	ret_if(!home_xwin);
	tutorial_xwin = elm_win_xwindow_get(tutorial_info->win);
	ret_if(!tutorial_xwin);

	/* unset transient */
	_destroy_transient_list();

	/* set transient : home & tutorial */
	_append_transient_list(tutorial_xwin, home_xwin);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP, NULL);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_eight);

	if (_tutorial_info.step_eight_timer) {
		ecore_timer_del(_tutorial_info.step_eight_timer);
		_tutorial_info.step_eight_timer = NULL;
	}

	elm_object_signal_emit(tutorial, "hide", "page8");

	_tutorial_info.step_eight_timer = ecore_timer_add(TIMER_TIME_STEP_EIGHT, _step_eight_timer_cb, tutorial);
	if (!_tutorial_info.step_eight_timer) _E("Cannot add a timer");
}



static void _step_seven(Evas_Object *tutorial, int vec_x, int vec_y)
{
	tutorial_info_s *tutorial_info = NULL;
	Ecore_X_Window tutorial_xwin;
	int is_wide_character = 0;

	ret_if(!tutorial);

	_D("Step 7");

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP, _step_eight);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_seven);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_ENABLE_INDICATOR, (void *) 0);

	tutorial_xwin = elm_win_xwindow_get(tutorial_info->win);
	ret_if(!tutorial_xwin);

	/* unset transient : home & tutorial */
	_destroy_transient_list();

	/* set tutorial window to noti type */
	ecore_x_netwm_window_type_set(tutorial_xwin, ECORE_X_WINDOW_TYPE_NOTIFICATION);
	utilx_set_system_notification_level(ecore_x_display_get(), tutorial_xwin, UTILX_NOTIFICATION_LEVEL_LOW);

	/* Show indicator */
	clock_view_indicator_show(INDICATOR_SHOW);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "sub");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "sub");
	} else {
		elm_object_signal_emit(tutorial, "standard", "sub");
	}

	elm_object_signal_emit(tutorial, "hide", "page7");
	elm_object_signal_emit(tutorial, "show", "page8");
	elm_object_part_text_set(tutorial, "text", _("IDS_WNOTI_BODY_SWIPE_UPWARDS_TO_GO_BACK_TO_CLOCK_ABB"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_WNOTI_BODY_SWIPE_UPWARDS_TO_GO_BACK_TO_CLOCK_ABB");
	elm_access_highlight_set(tutorial_info->text_focus);
}



static void _step_six(Evas_Object *tutorial, int vec_x, int vec_y)
{
	tutorial_info_s *tutorial_info = NULL;
	int is_wide_character = 0;

	ret_if(!tutorial);

	_D("Step 6");

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP, NULL);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_six);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_ENABLE_INDICATOR, (void *) 1);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "sub");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "sub");
	} else {
		elm_object_signal_emit(tutorial, "standard", "sub");
	}

	elm_object_signal_emit(tutorial, "up", "sub");
	elm_object_signal_emit(tutorial, "hide", "page6");
	elm_object_signal_emit(tutorial, "show", "page7");
	elm_object_part_text_set(tutorial, "text", _("IDS_WNOTI_BODY_SWIPE_TOP_EDGE_DOWN_TO_SEE_INDICATOR_ICONS_ON_CLOCK_ABB"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_WNOTI_BODY_SWIPE_TOP_EDGE_DOWN_TO_SEE_INDICATOR_ICONS_ON_CLOCK_ABB");
}



static void _step_five(Evas_Object *tutorial, int vec_x, int vec_y)
{
	tutorial_info_s *tutorial_info = NULL;
	Eina_List *instance_list = NULL;
	instance_info_s *info  = NULL;
	Ecore_X_Window tutorial_xwin;
	int is_wide_character = 0;
	int down_y = (int) evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_DOWN_Y);

	ret_if(!tutorial);

	_D("Step 5");

	if (main_get_info()->is_tts) {
		if (!(abs(vec_y) > TUTORIAL_THRESHOLD_ABLE_Y
			&& abs(vec_x) < TUTORIAL_THRESHOLD_DISABLE_X
			&& vec_y < 0))
		{
			_D("Exit step 5");
			return;
		}
	} else {
		if (!(abs(vec_y) > (TUTORIAL_THRESHOLD_ABLE_Y - 70)
			&& abs(vec_x) < TUTORIAL_THRESHOLD_DISABLE_X
			&& down_y >= main_get_info()->root_h - BEZEL_MOVE_THRESHOLD))
		{
			_D("Exit step 5");
			return;
		}
	}

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP, STEP_APPS);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_five);

	/* add event to checking apps state */
	instance_list = apps_main_get_info()->instance_list;
	if (!instance_list) {
		_D("Instance list is not exist");
		apps_main_init();
	}
	info = eina_list_nth(instance_list, APPS_FIRST_LIST);
	ret_if(!info);
	ret_if(!info->win);

	if (main_get_info()->is_tts) {
		ret_if(!info->layout);

		elm_object_tree_focus_allow_set(info->layout, EINA_FALSE);
		_D("tree_focus_allow_set layout(%p) as FALSE", info->layout);
	}

	tutorial_xwin = elm_win_xwindow_get(tutorial_info->win);
	ret_if(!tutorial_xwin);

	/* unset transient : home & tutorial */
	_destroy_transient_list();

	/* set tutorial window to noti type */
	ecore_x_netwm_window_type_set(tutorial_xwin, ECORE_X_WINDOW_TYPE_NOTIFICATION);
	utilx_set_system_notification_level(ecore_x_display_get(), tutorial_xwin, UTILX_NOTIFICATION_LEVEL_LOW);

	_D("Tutorial win (%p)", tutorial_xwin);

	apps_main_launch(APPS_LAUNCH_SHOW);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "sub");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "sub");
	} else {
		elm_object_signal_emit(tutorial, "standard", "sub");
	}

	elm_object_signal_emit(tutorial, "up", "sub");
	elm_object_signal_emit(tutorial, "hide", "page5");
	elm_object_signal_emit(tutorial, "show", "page6");
	elm_object_part_text_set(tutorial, "text", _("IDS_WNOTI_BODY_SWIPE_TOP_EDGE_DOWN_TO_GO_BACK_ABB"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_WNOTI_BODY_SWIPE_TOP_EDGE_DOWN_TO_GO_BACK_ABB");
}



static void _step_four(Evas_Object *tutorial, int vec_x, int vec_y)
{
	tutorial_info_s *tutorial_info = NULL;
	int is_wide_character = 0;

	ret_if(!tutorial);

	_D("Step 4");

	if (!(vec_x > TUTORIAL_THRESHOLD_ABLE_X
		&& abs(vec_y) < TUTORIAL_THRESHOLD_DISABLE_Y))
	{
		_D("Exit step 4");
		return;
	}

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP, _step_five);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_four);

	scroller_bring_in_page(tutorial_info->scroller, tutorial_info->scroller_info->center, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "sub");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "sub");
	} else {
		elm_object_signal_emit(tutorial, "standard", "sub");
	}

	elm_object_signal_emit(tutorial, "hide", "page4");
	elm_object_signal_emit(tutorial, "show", "page5");
	elm_object_part_text_set(tutorial, "text", _("IDS_WNOTI_BODY_SWIPE_BOTTOM_EDGE_UP_TO_VIEW_APPS_ON_CLOCK_ABB"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_WNOTI_BODY_SWIPE_BOTTOM_EDGE_UP_TO_VIEW_APPS_ON_CLOCK_ABB");
	elm_access_highlight_set(tutorial_info->text_focus);
}



static void _step_three(Evas_Object *tutorial, int vec_x, int vec_y)
{
	Evas_Object *widget_page = NULL;
	tutorial_info_s *tutorial_info = NULL;
	int is_wide_character = 0;

	ret_if(!tutorial);

	_D("Step 3");

	if (!(vec_x < -TUTORIAL_THRESHOLD_ABLE_X
		&& abs(vec_y) < TUTORIAL_THRESHOLD_DISABLE_Y))
	{
		_D("Exit step 3");
		return;
	}

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP, _step_four);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_three);

	widget_page = scroller_get_right_page(tutorial_info->scroller, tutorial_info->scroller_info->center);
	ret_if(!widget_page);
	ret_if(!page_is_appended(widget_page));
	scroller_bring_in_page(tutorial_info->scroller, widget_page, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "sub");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "sub");
	} else {
		elm_object_signal_emit(tutorial, "standard", "sub");
	}

	elm_object_signal_emit(tutorial, "hide", "page3");
	elm_object_signal_emit(tutorial, "show", "page4");
	elm_object_part_text_set(tutorial, "text", _("IDS_HELP_POP_SWIPE_RIGHT_TO_GO_BACK_TO_THE_CLOCK_ABB"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_HELP_POP_SWIPE_RIGHT_TO_GO_BACK_TO_THE_CLOCK_ABB");
	elm_access_highlight_set(tutorial_info->text_focus);
}



static void _step_two(Evas_Object *tutorial, int vec_x, int vec_y)
{
	tutorial_info_s *tutorial_info = NULL;
	int is_wide_character = 0;

	ret_if(!tutorial);

	_D("Step 2");

	if (!(vec_x < -TUTORIAL_THRESHOLD_ABLE_X
		&& abs(vec_y) < TUTORIAL_THRESHOLD_DISABLE_Y))
	{
		_D("Exit step 2");
		return;
	}

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP, _step_three);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_two);

	scroller_bring_in_page(tutorial_info->scroller, tutorial_info->scroller_info->center, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "sub");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "sub");
	} else {
		elm_object_signal_emit(tutorial, "standard", "sub");
	}

	elm_object_signal_emit(tutorial, "hide", "page2");
	elm_object_signal_emit(tutorial, "show", "page3");
	elm_object_part_text_set(tutorial, "text", _("IDS_HELP_POP_SWIPE_LEFT_TO_VIEW_WIDGETS_ABB"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_HELP_POP_SWIPE_LEFT_TO_VIEW_WIDGETS_ABB");
	elm_access_highlight_set(tutorial_info->text_focus);
}



static void _step_one(Evas_Object *tutorial, int vec_x, int vec_y)
{
	Evas_Object *noti_page = NULL;
	tutorial_info_s *tutorial_info = NULL;
	int is_wide_character = 0;

	ret_if(!tutorial);

	_D("Step 1");

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	if (!(vec_x > TUTORIAL_THRESHOLD_ABLE_X
		&& abs(vec_y) < TUTORIAL_THRESHOLD_DISABLE_Y))
	{
		_D("Exit step 1");
		scroller_bring_in_page(tutorial_info->scroller, tutorial_info->scroller_info->center, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);
		return;
	}

	noti_page = scroller_get_left_page(tutorial_info->scroller, tutorial_info->scroller_info->center);
	if (!noti_page || !page_is_appended(noti_page)) {
		_D("There is no notification");
		scroller_bring_in_page(tutorial_info->scroller, tutorial_info->scroller_info->center, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);
		return;
	}
	scroller_bring_in_page(tutorial_info->scroller, noti_page, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP, _step_two);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_one);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "sub");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "sub");
	} else {
		elm_object_signal_emit(tutorial, "standard", "sub");
	}

	elm_object_signal_emit(tutorial, "hide", "page1");
	elm_object_signal_emit(tutorial, "show", "page2");
	elm_object_part_text_set(tutorial, "text", _("IDS_HELP_POP_SWIPE_LEFT_TO_GO_BACK_TO_THE_CLOCK_ABB"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_HELP_POP_SWIPE_LEFT_TO_GO_BACK_TO_THE_CLOCK_ABB");
	elm_access_highlight_set(tutorial_info->text_focus);
}



static void _up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Move *ei = event_info;
	tutorial_info_s *tutorial_info = data;

	Evas_Object *tutorial = NULL;
	void (*_step)(Evas_Object *, int , int);
	int down_x, down_y;
	int cur_x, cur_y;
	int vec_x, vec_y;

	ret_if(!tutorial_info);
	tutorial = tutorial_info->tutorial;

	if (!evas_object_data_del(tutorial, PRIVATE_DATA_KEY_TUTORIAL_PRESSED)) return;

	/* can use down_x or down_y in the _step functions */
	down_x = (int) evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_DOWN_X);
	down_y = (int) evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_DOWN_Y);

	cur_x = ei->cur.output.x;
	cur_y = ei->cur.output.y;

	_D("Tutorial mouse up (%d, %d)", cur_x, cur_y);

	vec_x = cur_x - down_x;
	vec_y = cur_y - down_y;

	_step = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP);
	if (!_step) {
		_D("Step is done");
		return;
	} else if (STEP_APPS == _step) {
		_D("Step is holding");
		return;
	}
	_step(tutorial, vec_x, vec_y);

	evas_object_data_del(tutorial, PRIVATE_DATA_KEY_TUTORIAL_DOWN_X);
	evas_object_data_del(tutorial, PRIVATE_DATA_KEY_TUTORIAL_DOWN_Y);
}



static key_cb_ret_e _tutorial_back_cb(void *data)
{
	Evas_Object *tutorial = data;

	retv_if(!tutorial, KEY_CB_RET_STOP);

	_D("Back key is released");

	/* If the Apps is down, then proceed the next step */
	if(evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_ENABLE_INDICATOR)) {
		clock_h clock = NULL;
		clock = clock_manager_clock_get(CLOCK_ATTACHED);
		retv_if(!clock, KEY_CB_RET_CONTINUE);
		retv_if(!clock->view, KEY_CB_RET_CONTINUE);

		_step_seven(tutorial, 0, 0);

		return KEY_CB_RET_CONTINUE;
	}
	return KEY_CB_RET_STOP;
}



static w_home_error_e _resume_cb(void *data)
{
	Ecore_X_Window tutorial_xwin;
	Ecore_X_Window home_xwin;
	Evas_Object *tutorial = data;
	void (*_step)(Evas_Object *tutorial, int vec_x, int vec_y);

	retv_if(!tutorial, W_HOME_ERROR_FAIL);

	_D("Resume window");

	_step = evas_object_data_get(data, PRIVATE_DATA_KEY_TUTORIAL_STEP);
	if (_step_one == _step) {
		tutorial_info_s *tutorial_info = NULL;

		tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
		retv_if(!tutorial_info, W_HOME_ERROR_FAIL);

		scroller_bring_in_page(tutorial_info->scroller, tutorial_info->scroller_info->center, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);
		elm_object_focus_set(tutorial_info->text_focus, EINA_TRUE);
	} else if (STEP_APPS == _step) {
		tutorial_info_s *tutorial_info = NULL;

		tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
		retv_if(!tutorial_info, W_HOME_ERROR_FAIL);

		home_xwin = elm_win_xwindow_get(main_get_info()->win);
		retv_if(!home_xwin, W_HOME_ERROR_FAIL);
		tutorial_xwin = elm_win_xwindow_get(tutorial_info->win);
		retv_if(!tutorial_xwin, W_HOME_ERROR_FAIL);

		/* unset transient */
		_destroy_transient_list();

		/* set transient : home & tutorial */
		_append_transient_list(tutorial_xwin, home_xwin);

		_step_six(tutorial, 0, 0);
	}

	return W_HOME_ERROR_NONE;
}



static void _highlighted_enabled_cb(void *data, Evas_Object *obj, void *event_info)
{
	tutorial_info_s *tutorial_info = data;
	ret_if(!tutorial_info);
	ret_if(!tutorial_info->text_focus);

	_D("highlight enabled");

	elm_access_highlight_set(tutorial_info->text_focus);
}



static void _highlighted_disabled_cb(void *data, Evas_Object *obj, void *event_info)
{
	_D("highlight disabled");
}



static void _tutorial_start_2_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	tutorial_info_s *tutorial_info = NULL;
	int is_wide_character = 0;
	Evas_Object *tutorial = data;
	ret_if(!tutorial);

	_D("Show page 1");

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP, _step_one);
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_init);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "sub");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "sub");
	} else {
		elm_object_signal_emit(tutorial, "standard", "sub");
	}

	elm_object_signal_emit(tutorial, "hide", "page0");
	elm_object_signal_emit(tutorial, "show", "page1");
	elm_object_part_text_set(tutorial, "text", _("IDS_HELP_POP_SWIPE_RIGHT_TO_VIEW_NOTIFICATIONS_ABB"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_HELP_POP_SWIPE_RIGHT_TO_VIEW_NOTIFICATIONS_ABB");
	elm_access_highlight_set(tutorial_info->text_focus);
}



static char *_start_button_info_cb(void *data, Evas_Object *obj)
{
	char *text = NULL;

	text = strdup(_("IDS_ST_BUTTON_NEXT"));
	retv_if(!text, NULL);

	return text;
}



static void _destroy_start_2_button(Evas_Object *tutorial)
{
	Evas_Object *button = NULL;
	ret_if(!tutorial);

	button = elm_object_part_content_unset(tutorial, "button");
	ret_if(!button);

	evas_object_del(button);
}



static void _start_2_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	tutorial_info_s *tutorial_info = NULL;

	Evas_Object *tutorial = data;
	ret_if(!tutorial);

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	scroller_bring_in_page(tutorial_info->scroller, tutorial_info->scroller_info->center, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);

	elm_object_signal_emit(tutorial, "show", "start2,pressed");
	effect_play_vibration();

	 _D("Start the tutorial");

	_destroy_start_2_button(tutorial);
}



static Evas_Object *_create_start_2_button(Evas_Object *tutorial)
{
	Evas_Object *button = NULL;

	retv_if(!tutorial, NULL);

	_D("Add start 2 button");

	button = elm_button_add(tutorial);
	retv_if(!button, NULL);

	elm_object_style_set(button, "focus");
	elm_object_part_content_set(tutorial, "button", button);
	evas_object_show(button);

	elm_object_part_text_set(tutorial, "button_text", _("IDS_ST_BUTTON_NEXT"));
	elm_object_domain_translatable_part_text_set(tutorial, "button_text", PROJECT, "IDS_ST_BUTTON_NEXT");

	elm_access_info_cb_set(button, ELM_ACCESS_INFO, _start_button_info_cb, tutorial);
	evas_object_smart_callback_add(button, "clicked", _start_2_button_clicked_cb, tutorial);
	elm_object_signal_callback_add(tutorial, "start2", "button", _tutorial_start_2_cb, tutorial);

	return button;
}



static void _tutorial_start_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	tutorial_info_s *tutorial_info = NULL;
	int is_wide_character = 0;
	Evas_Object *tutorial = data;
	ret_if(!tutorial);

	_D("Show structure image");

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_structure);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "sub");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "sub");
	} else {
		elm_object_signal_emit(tutorial, "standard", "sub");
	}

	elm_object_signal_emit(tutorial, "show", "structure_page");
	elm_object_part_text_set(tutorial, "text", _("IDS_WNOTI_BODY_THIS_IS_THE_MAIN_STRUCTURE_OF_THE_HOME_SCREEN"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_WNOTI_BODY_THIS_IS_THE_MAIN_STRUCTURE_OF_THE_HOME_SCREEN");
	elm_access_highlight_set(tutorial_info->text_focus);

	if (!_create_start_2_button(tutorial)) _E("failed to add the add start 2 button");
}



static char *_close_button_info_cb(void *data, Evas_Object *obj)
{
	char *text = NULL;

	text = strdup(_("IDS_ST_BUTTON_EXIT"));
	retv_if(!text, NULL);

	return text;
}



static void _destroy_start_button(Evas_Object *tutorial)
{
	Evas_Object *button = NULL;
	ret_if(!tutorial);

	button = elm_object_part_content_unset(tutorial, "button");
	ret_if(!button);

	evas_object_del(button);
}



static void _destroy_close_button(Evas_Object *tutorial)
{
	Evas_Object *button = NULL;
	ret_if(!tutorial);

	button = elm_object_part_content_unset(tutorial, "close_button");
	ret_if(!button);

	evas_object_del(button);
}



static void _start_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	tutorial_info_s *tutorial_info = NULL;

	Evas_Object *tutorial = data;
	ret_if(!tutorial);

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	scroller_bring_in_page(tutorial_info->scroller, tutorial_info->scroller_info->center, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);

	elm_object_signal_emit(tutorial, "show", "start,pressed");
	effect_play_vibration();

	 _D("Show structure page");

	_destroy_start_button(tutorial);
	_destroy_close_button(tutorial);
}



static Evas_Object *_create_start_button(Evas_Object *tutorial)
{
	Evas_Object *button = NULL;

	retv_if(!tutorial, NULL);

	_D("Add start button");

	button = elm_button_add(tutorial);
	retv_if(!button, NULL);

	elm_object_style_set(button, "focus");
	elm_object_part_content_set(tutorial, "button", button);
	evas_object_show(button);

	elm_object_part_text_set(tutorial, "button_text", _("IDS_ST_BUTTON_NEXT"));
	elm_object_domain_translatable_part_text_set(tutorial, "button_text", PROJECT, "IDS_ST_BUTTON_NEXT");

	elm_access_info_cb_set(button, ELM_ACCESS_INFO, _start_button_info_cb, tutorial);
	evas_object_smart_callback_add(button, "clicked", _start_button_clicked_cb, tutorial);
	elm_object_signal_callback_add(tutorial, "start", "button", _tutorial_start_cb, tutorial);

	return button;
}



static void _close_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	tutorial_info_s *tutorial_info = NULL;

	Evas_Object *tutorial = data;
	ret_if(!tutorial);

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	/* Scroller has to be unfreezed before using scroller_bring_in_page */
	scroller_unfreeze(tutorial_info->scroller);
	scroller_bring_in_page(tutorial_info->scroller, tutorial_info->scroller_info->center, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT);

	_destroy_start_button(tutorial);
	_destroy_close_button(tutorial);

	tutorial_destroy(tutorial);

	/* only tutorial use */
	if (vconf_set_int(VCONF_KEY_HOME_IS_TUTORIAL_ENABLED_TO_RUN, 0) != 0) {
		_E("Critical, cannot set the private vconf key");
		return;
	}
}



static Evas_Object *_create_close_button(Evas_Object *tutorial)
{
	Evas_Object *button = NULL;

	retv_if(!tutorial, NULL);

	_D("Add start button");

	button = elm_button_add(tutorial);
	retv_if(!button, NULL);

	elm_object_style_set(button, "focus");
	elm_object_part_content_set(tutorial, "close_button", button);
	evas_object_show(button);

	elm_access_info_cb_set(button, ELM_ACCESS_INFO, _close_button_info_cb, tutorial);
	evas_object_smart_callback_add(button, "clicked", _close_button_clicked_cb, tutorial);

	return button;
}



static Evas_Object *_create_window(void)
{
	Evas_Object *win = NULL;

	win = elm_win_add(NULL, "Tutorial", ELM_WIN_BASIC);
	retv_if(!win, NULL);

	elm_win_title_set(win, "Tutorial");
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_alpha_set(win, EINA_TRUE);
	elm_win_indicator_mode_set(win, ELM_WIN_INDICATOR_HIDE);
	elm_win_indicator_opacity_set(win, ELM_WIN_INDICATOR_BG_TRANSPARENT);
	elm_win_prop_focus_skip_set(win, EINA_TRUE);
	elm_win_role_set(win, "no-effect");

	evas_object_resize(win, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_show(win);

	return win;
}



static void _destroy_window(Evas_Object *win)
{
	ret_if(!win);

	evas_object_del(win);
}



#define FILE_TUTORIAL_EDJ EDJEDIR"/tutorial.edj"
#define GROUP_TUTORIAL "tutorial"
static Evas_Object *_create_layout(Evas_Object *win)
{
	Evas_Object *tutorial = NULL;
	Eina_Bool ret;

	retv_if(!win, NULL);

	tutorial = elm_layout_add(win);
	retv_if(!tutorial, NULL);

	ret = elm_layout_file_set(tutorial, FILE_TUTORIAL_EDJ, GROUP_TUTORIAL);
	if (EINA_FALSE == ret) return NULL;

	evas_object_size_hint_weight_set(tutorial, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(tutorial, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_resize(tutorial, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_show(tutorial);

	return tutorial;
}



static void _destroy_layout(Evas_Object *tutorial)
{
	ret_if(!tutorial);

	evas_object_data_del(tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP);
	evas_object_data_del(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT);
	evas_object_data_del(tutorial, PRIVATE_DATA_KEY_TUTORIAL_ENABLE_INDICATOR);

	evas_object_del(tutorial);
}



static Evas_Object *_create_bg(Evas_Object *tutorial)
{
	Evas *evas = NULL;
	Evas_Object *bg = NULL;

	retv_if(!tutorial, NULL);

	evas = evas_object_evas_get(tutorial);
	retv_if(!evas, NULL);

	bg = evas_object_rectangle_add(evas);
	retv_if(!bg, NULL);

	evas_object_size_hint_min_set(bg, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_size_hint_max_set(bg, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_resize(bg, main_get_info()->root_w, main_get_info()->root_h);
	elm_object_part_content_set(tutorial, "bg_area", bg);
	evas_object_show(bg);

	return bg;
}



static void _destroy_bg(Evas_Object *bg)
{
	ret_if(!bg);

	evas_object_del(bg);
}



static void _destroy_focus_buttons(tutorial_info_s *tutorial_info)
{
	ret_if(!tutorial_info);

	if (tutorial_info->layout_focus) {
		evas_object_del(tutorial_info->layout_focus);
	}

	if (tutorial_info->text_focus) {
		evas_object_del(tutorial_info->text_focus);
	}
}



static void _create_focus_buttons(Evas_Object *tutorial)
{
	tutorial_info_s *tutorial_info = NULL;
	Evas_Object *focus = NULL;

	ret_if(!tutorial);

	tutorial_info = evas_object_data_get(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);

	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_TEXT, _text_start);

	/* Layout button for TTS*/
	focus = elm_button_add(tutorial);
	ret_if(!focus);

	elm_object_style_set(focus, "focus");
	elm_object_part_content_set(tutorial, "focus,tutorial", focus);
	evas_object_event_callback_add(focus, EVAS_CALLBACK_MOUSE_DOWN, _down_cb, tutorial);
	evas_object_event_callback_add(focus, EVAS_CALLBACK_MOUSE_UP, _up_cb, tutorial_info);
	elm_object_focus_allow_set(focus, EINA_FALSE);
	elm_access_object_unregister(focus);
	tutorial_info->layout_focus = focus;

	/*text button for TTS */
	focus = elm_button_add(tutorial);
	goto_if(!focus, ERROR);

	elm_object_style_set(focus, "focus");
	elm_object_part_content_set(tutorial, "focus,text", focus);
	elm_access_info_cb_set(focus, ELM_ACCESS_TYPE, _access_info_cb, tutorial);
	elm_object_focus_allow_set(focus, EINA_TRUE);
	elm_object_focus_set(focus, EINA_TRUE);
	tutorial_info->text_focus = focus;

	return;

ERROR:
	_destroy_focus_buttons(tutorial_info);
}



HAPI Evas_Object *tutorial_create(Evas_Object *layout)
{
	Evas_Object *win = NULL;
	Evas_Object *tutorial = NULL;
	Evas_Object *bg = NULL;
	Evas_Object *scroller = NULL;

	layout_info_s *layout_info = NULL;
	scroller_info_s *scroller_info = NULL;
	tutorial_info_s *tutorial_info = NULL;
	Ecore_X_Window tutorial_xwin;
	Ecore_X_Window home_xwin;
	int ret = 0;
	int is_wide_character = 0;

	retv_if(!layout, NULL);

	_D("Creat tutorial");

	if ((ret = vconf_set_int(VCONF_KEY_HOME_IS_TUTORIAL, 1)) != 0) {
		_E("Critical, cannot set the public vconf key");
		return NULL;
	}

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, NULL);

	tutorial_info = calloc(1, sizeof(tutorial_info_s));
	retv_if(!tutorial_info, NULL);

	tutorial_info->layout = layout;

	scroller = evas_object_data_get(layout, DATA_KEY_SCROLLER);
	goto_if(!scroller, ERROR);
	tutorial_info->scroller = scroller;

	scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	goto_if(!scroller_info, ERROR);
	tutorial_info->scroller_info = scroller_info;

	win = _create_window();
	goto_if(!win, ERROR);
	tutorial_info->win = win;
	_tutorial_info.win = win;

	tutorial = _create_layout(win);
	goto_if(!tutorial, ERROR);
	tutorial_info->tutorial = tutorial;

	bg = _create_bg(tutorial);
	goto_if(!bg, ERROR);
	tutorial_info->bg = bg;

	/* We have to set tutorial_info right after calloc & tutorial */
	evas_object_data_set(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO, tutorial_info);

	if (main_get_info()->is_tts) {
		elm_object_tree_focus_allow_set(main_get_info()->layout, EINA_FALSE);
		_D("tree_focus_allow_set layout(%p) as FALSE", main_get_info()->layout);
	}
	scroller_unhighlight(scroller);

	is_wide_character = main_get_info()->is_wide_character;
	if (is_wide_character == TUTORIAL_LANGUAGE_MALI) {
		elm_object_signal_emit(tutorial, "small", "center");
	} else if (is_wide_character == TUTORIAL_LANGUAGE_GEORGIA) {
		elm_object_signal_emit(tutorial, "medium", "center");
	} else {
		elm_object_signal_emit(tutorial, "standard", "center");
	}

	elm_object_signal_emit(tutorial, "start", "start");
	elm_object_part_text_set(tutorial, "text", _("IDS_WMGR_BODY_WELCOME_E_TAP_THE_BUTTON_BELOW_TO_LEARN_HOW_TO_USE_YOUR_GEAR_ABB"));
	elm_object_domain_translatable_part_text_set(tutorial, "text", PROJECT, "IDS_WMGR_BODY_WELCOME_E_TAP_THE_BUTTON_BELOW_TO_LEARN_HOW_TO_USE_YOUR_GEAR_ABB");
	if (!_create_start_button(tutorial)) _E("failed to add the add start button");
	if (!_create_close_button(tutorial)) _E("failed to add the add close button");

	_create_focus_buttons(tutorial);

	evas_object_smart_callback_add(win, "access,highlight,enabled", _highlighted_enabled_cb, tutorial_info);
	evas_object_smart_callback_add(win, "access,highlight,disabled", _highlighted_disabled_cb, tutorial_info);

	scroller_freeze(scroller);
	scroller_bring_in_page(scroller, scroller_info->center, SCROLLER_FREEZE_ON, SCROLLER_BRING_TYPE_INSTANT);

	tutorial_xwin = elm_win_xwindow_get(win);
	goto_if(!tutorial_xwin, ERROR);

	home_xwin = elm_win_xwindow_get(main_get_info()->win);
	goto_if(!home_xwin, ERROR);

	/* set transient : home & tutorial */
	_append_transient_list(tutorial_xwin, home_xwin);

	key_register_cb(KEY_TYPE_BACK, _tutorial_back_cb, tutorial);
	main_register_cb(APP_STATE_RESUME, _resume_cb, tutorial);

	layout_info->tutorial = tutorial;

	return tutorial;

ERROR:
	/* We don't need to destroy bg and so on because we don't create */
	if (tutorial) _destroy_layout(tutorial);
	if (win) _destroy_window(win);
	if (tutorial_info) free(tutorial_info);

	return NULL;
}



HAPI void tutorial_destroy(Evas_Object *tutorial)
{
	layout_info_s *layout_info = NULL;
	tutorial_info_s *tutorial_info = NULL;
	Eina_List *instance_list = NULL;
	instance_info_s *info  = NULL;
	layout_info_s *home_layout_info = NULL;
	scroller_info_s *scroller_info = NULL;

	_D("Destroy tutorial");

	ret_if(!tutorial);

	if(_tutorial_info.transient_timer) {
		ecore_timer_del(_tutorial_info.transient_timer);
		_tutorial_info.transient_timer = NULL;
	}

	if(_tutorial_info.step_eight_timer) {
		ecore_timer_del(_tutorial_info.step_eight_timer);
		_tutorial_info.step_eight_timer = NULL;
	}

	tutorial_info = evas_object_data_del(tutorial, PRIVATE_DATA_KEY_TUTORIAL_INFO);
	ret_if(!tutorial_info);
	ret_if(!tutorial_info->layout);

	layout_info = evas_object_data_get(tutorial_info->layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	main_unregister_cb(APP_STATE_RESUME, _resume_cb);
	key_unregister_cb(KEY_TYPE_BACK, _tutorial_back_cb);

	instance_list = apps_main_get_info()->instance_list;
	ret_if(!instance_list);

	info = eina_list_nth(instance_list, APPS_FIRST_LIST);
	ret_if(!info);
	ret_if(!info->win);

	if (main_get_info()->is_tts) {
		elm_object_tree_focus_allow_set(main_get_info()->layout, EINA_TRUE);
		_D("tree_focus_allow_set layout(%p) as TRUE", main_get_info()->layout);

		ret_if(!info->layout);
		elm_object_tree_focus_allow_set(info->layout, EINA_TRUE);
		_D("tree_focus_allow_set layout(%p) as TRUE", info->layout);
		home_layout_info = evas_object_data_get(main_get_info()->layout, DATA_KEY_LAYOUT_INFO);
		scroller_info = evas_object_data_get(home_layout_info->scroller, DATA_KEY_SCROLLER_INFO);
		page_focus(scroller_info->center);
	}

	scroller_highlight(tutorial_info->scroller);

	_destroy_focus_buttons(tutorial_info);
	_destroy_bg(tutorial_info->bg);
	_destroy_layout(tutorial_info->tutorial);

	_destroy_transient_list();
	_destroy_window(tutorial_info->win);

	scroller_unfreeze(tutorial_info->scroller);
	free(tutorial_info);

	layout_info->tutorial = NULL;

	if (vconf_set_int(VCONF_KEY_HOME_IS_TUTORIAL, 0) != 0) {
		_E("Critical, cannot set the public vconf key");
		return;
	}
}



#define TITLE_APPS "__APPS__"
HAPI int tutorial_is_apps(Ecore_X_Window xwin)
{
	layout_info_s *layout_info = NULL;
	void (*_step_func)(Evas_Object *, int, int);
	char *title = NULL;

	title = ecore_x_icccm_title_get(xwin);
	retv_if(!title, 0);

	if (strcmp(title, TITLE_APPS)) {
		_D("This is not Apps");
		free(title);
		return 0;
	}
	free(title);

	layout_info = evas_object_data_get(main_get_info()->layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, 0);

	_step_func = evas_object_data_get(layout_info->tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP);
	retv_if(!_step_func, 0);

	return _step_func == STEP_APPS ? 1 : 0;
}



#define TITLE_INDICATOR "__MOMENT_VIEW__"
HAPI int tutorial_is_indicator(Ecore_X_Window xwin)
{
	layout_info_s *layout_info = NULL;
	void (*_step_func)(Evas_Object *, int, int);
	char *title = NULL;

	title = ecore_x_icccm_title_get(xwin);
	retv_if(!title, 0);

	if (strcmp(title, TITLE_INDICATOR)) {
		_D("This is not Indicator");
		free(title);
		return 0;
	}
	free(title);

	layout_info = evas_object_data_get(main_get_info()->layout, DATA_KEY_LAYOUT_INFO);
	retv_if(!layout_info, 0);

	_step_func = evas_object_data_get(layout_info->tutorial, PRIVATE_DATA_KEY_TUTORIAL_STEP);
	retv_if(!_step_func, 0);

	return _step_func == _step_eight ? 1 : 0;
}

// End of file
