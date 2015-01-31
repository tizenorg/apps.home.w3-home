#include <Elementary.h>
#include <Evas.h>
#include <stdbool.h>
#include <vconf.h>
#include <bundle.h>
#include <aul.h>
#include <efl_assist.h>
#include <dlog.h>
#include <appcore-common.h>

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
#include "effect.h"
#include "dbus.h"
#include "tutorial.h"
#include "clock_service.h"
#include "apps/apps_main.h"

#define CLOCK_PRIVATE_DATA_KEY_VISIBILITY "cl_p_v_d_v"
#define CLOCK_PRIVATE_DATA_KEY_HIDDEN_BUTTON "cl_p_c_hb"
#define CLOCK_PRIVATE_DATA_KEY_HIDDEN_BUTTON_DISPLAY_STATE "cl_p_c_hb_ds_st"
#define CLOCK_PRIVATE_DATA_KEY_DOWN_X "cl_p_c_d_x"
#define CLOCK_PRIVATE_DATA_KEY_DOWN_Y "cl_p_c_d_y"
#define CLOCK_PRIVATE_DATA_KEY_DISPLAY_CUE "cl_p_c_di_cu"
#define CLOCK_PRIVATE_DATA_KEY_HIDING_TIMER "cl_p_c_hi_ti"

#define DRAWER_HIDING_TIME 3.0f

struct evet_handler_s {
	int event;
	void (*process) (Evas_Object *, int , void *data);
};

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
static void _mouse_flickup_cb(void *data, Evas_Object *scroller, void *event_info);
#endif
static void _drawer_hide(Evas_Object *page, int is_show);

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

void clock_view_indicator_show(int is_show)
{
	int pid = 0;
	bundle *b = bundle_create();
	if (b != NULL) {
		if (is_show >= INDICATOR_SHOW) {
			bundle_add(b, "__WINDICATOR_OP__", "__SHOW_MOMENT_VIEW__");
			if(is_show == INDICATOR_SHOW_WITH_EFFECT) {
				bundle_add(b, "__EFFECT__", "ON");
			}
		} else {
			bundle_add(b, "__WINDICATOR_OP__", "__HIDE_MOMENT_VIEW__");
			if(is_show == INDICATOR_HIDE_WITH_EFFECT) {
				bundle_add(b, "__EFFECT__", "ON");
			}
		}

		pid = aul_launch_app("org.tizen.windicator", b);
		_D("aul_launch_app: %s(%d)", "org.tizen.windicator", pid);
	}
}

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
static void _drawer_hiding_timer_del(Evas_Object *page)
{
	Ecore_Timer *tm = evas_object_data_del(page, CLOCK_PRIVATE_DATA_KEY_HIDING_TIMER);
	if (tm != NULL) {
		ecore_timer_del(tm);
	}
}

Eina_Bool _drawer_hiding_timer_cb(void *data)
{
	Evas_Object *page = data;
	retv_if(page == NULL, ECORE_CALLBACK_CANCEL);

	_drawer_hiding_timer_del(page);
	_drawer_hide(page, INDICATOR_HIDE);

	return ECORE_CALLBACK_CANCEL;
}

void _drawer_hiding_timer_set(Evas_Object *page)
{
	ret_if(page == NULL);

	_drawer_hiding_timer_del(page);

	Ecore_Timer *tm = ecore_timer_add(DRAWER_HIDING_TIME, _drawer_hiding_timer_cb, page);
	evas_object_data_set(page, CLOCK_PRIVATE_DATA_KEY_HIDING_TIMER, tm);
}

static int _is_drawer_displayed(Evas_Object *page)
{
	retv_if(page == NULL, 0);

	return (int)evas_object_data_get(page, CLOCK_PRIVATE_DATA_KEY_HIDDEN_BUTTON);
}

static void _drawer_displayed_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *page = data;
	ret_if(page == NULL);

	evas_object_data_set(page, CLOCK_PRIVATE_DATA_KEY_HIDDEN_BUTTON_DISPLAY_STATE, (void*)1);
}

static void _drawer_hided_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *page = data;
	ret_if(page == NULL);

	evas_object_data_del(page, CLOCK_PRIVATE_DATA_KEY_HIDDEN_BUTTON_DISPLAY_STATE);
}
#endif

static void _drawer_show(Evas_Object *page, int indicator_show)
{
	ret_if(page == NULL);
	Evas_Object *item = page_get_item(page);
	ret_if(item == NULL);
	Evas_Object *layout = _layout_get();
	ret_if(layout == NULL);

#ifdef ENABLE_INDICATOR_BRIEFING_VIEW
	clock_view_indicator_show(indicator_show);
#else
	if (clock_service_event_state_get(CLOCK_EVENT_SIM) == CLOCK_EVENT_SIM_NOT_INSERTED) {
		elm_object_signal_emit(item , "hidden.show.short", "clock_bg");
	} else {
		elm_object_signal_emit(item , "hidden.show", "clock_bg");
	}
	evas_object_data_set(page, CLOCK_PRIVATE_DATA_KEY_HIDDEN_BUTTON, (void*)1);
	evas_object_smart_callback_del(layout, LAYOUT_SMART_SIGNAL_FLICK_UP, _mouse_flickup_cb);
	evas_object_smart_callback_add(layout, LAYOUT_SMART_SIGNAL_FLICK_UP, _mouse_flickup_cb, page);
	clock_view_indicator_show(indicator_show);

	if (util_feature_enabled_get(FEATURE_CLOCK_HIDDEN_BUTTON_TIMER) == 1) {
		_drawer_hiding_timer_set(page);
	}
	clock_view_hidden_event_handler(page, CLOCK_EVENT_VIEW_HIDDEN_SHOW);
#endif
}

static void _drawer_hide(Evas_Object *page, int indicator_show)
{
	ret_if(page == NULL);
	Evas_Object *item = page_get_item(page);
	ret_if(item == NULL);
	Evas_Object *layout = _layout_get();
	ret_if(layout == NULL);

#ifdef ENABLE_INDICATOR_BRIEFING_VIEW
	clock_view_indicator_show(indicator_show);
#else
	evas_object_data_del(page, CLOCK_PRIVATE_DATA_KEY_HIDDEN_BUTTON);
	evas_object_smart_callback_del(layout, LAYOUT_SMART_SIGNAL_FLICK_UP, _mouse_flickup_cb);
	elm_object_signal_emit(item , "hidden.hide", "clock_bg");
	clock_view_indicator_show(indicator_show);
	_drawer_hiding_timer_del(page);
	clock_view_hidden_event_handler(page, CLOCK_EVENT_VIEW_HIDDEN_HIDE);
#endif
}

static void _screen_reader_enable_set(Evas_Object *page, int is_on)
{
	ret_if(page == NULL);
	Evas_Object *item = page_get_item(page);
	ret_if(item == NULL);

	if (is_on == 1) {
		elm_object_signal_emit(item, "screenreader,on", "clock_bg");
		if (util_feature_enabled_get(FEATURE_APPS) == 1) {
			elm_object_signal_emit(item, "screenreader,apps,on", "clock_bg");
		}
	} else {
		elm_object_signal_emit(item, "screenreader,off", "clock_bg");
		elm_object_signal_emit(item, "screenreader,apps,off", "clock_bg");
	}
}

static int _visibility_get(Evas_Object *page)
{
	retv_if(page == NULL, 0);

	return (int)evas_object_data_get(page, CLOCK_PRIVATE_DATA_KEY_VISIBILITY);
}

static void  _visibility_set(Evas_Object *page, int visibility)
{
	ret_if(page == NULL);

	evas_object_data_set(page, CLOCK_PRIVATE_DATA_KEY_VISIBILITY, (void *)visibility);
}


static void _visibility_vconf_set(int visibility)
{
	if (main_get_info()->state == APP_STATE_PAUSE) {
		visibility = 0;
	}
}

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
static void _mouse_flickup_cb(void *data, Evas_Object *scroller, void *event_info)
{
	_D("flickup");
	Evas_Object *page = data;
	ret_if(page == NULL);

	if (_is_drawer_displayed(page) == 1) {
		_drawer_hide(page, INDICATOR_HIDE);
	}
}

static void _mouse_down_cb(void *data, Evas * evas, Evas_Object * obj, void *event_info)
{
	Evas_Object *page = obj;
	Evas_Object *layout = data;
	Evas_Event_Mouse_Down *ev = event_info;
	ret_if(page == NULL);
	ret_if(layout == NULL);
	ret_if(ev == NULL);

	if (_is_drawer_displayed(page) == 1) {
		Evas_Object *drawer = elm_object_part_content_get(layout, "hidden_item");
		if (drawer != NULL) {
			int drawer_y = 0;
			int drawer_h = 0;
			evas_object_geometry_get(drawer, NULL, &drawer_y, NULL, &drawer_h);

			if (ev->canvas.y > (drawer_h + drawer_y)) {
				_drawer_hide(page, INDICATOR_HIDE);
			} else {
				if (util_feature_enabled_get(FEATURE_CLOCK_HIDDEN_BUTTON_TIMER) == 1) {
					_drawer_hiding_timer_set(page);
				}
			}
		}
	}
}
#endif

static void _mouse_up_cb(void *data, Evas * evas, Evas_Object * obj, void *event_info)
{
	Evas_Object *page = obj;
	Evas_Event_Mouse_Up *ev = event_info;
	ret_if(page == NULL);
	ret_if(ev == NULL);

	if ((int)evas_object_data_get(page, DATA_KEY_PAGE_ONHOLD_COUNT) == 1) {
		evas_object_data_del(page, DATA_KEY_PAGE_ONHOLD_COUNT);
		ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
		_E("event onhold flag set on clock page");
	}
}

static void _mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	static int is_first_call = -1;
	int x = 0, visibility = 0;
	int win_w = 0;
	Evas_Object *page = obj;
	Evas_Object *item = page_get_item(obj);
	ret_if(page == NULL);
	ret_if(item == NULL);

	if (is_first_call == -1) {
		Evas_Object *scroller = _scroller_get();
		if (scroller != NULL) {
			scroller_region_show_by_push_type(scroller, SCROLLER_PUSH_TYPE_CENTER, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT);
		}
		is_first_call = 1;
	}

	if (obj != NULL) {
		win_w = main_get_info()->root_w;
		evas_object_geometry_get(obj, &x, NULL, NULL, NULL);

		if ((x >= 0 && x < win_w) || (x <= 0 && x > -win_w)) {
			visibility = 1;
		} else {
			visibility = 0;
		}

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
		if(x >= 10 || x <= -10) {
			if (_is_drawer_displayed(page) == 1) {
				_drawer_hide(page, INDICATOR_HIDE);
			}
		}
#endif

		if (_visibility_get(page) != visibility) {
			_D("clock page:%p visibility changed:%s", obj, (visibility == 1) ? "show" : "hide");

			_visibility_vconf_set(visibility);
			_visibility_set(page, visibility);

			if (clock_service_event_state_get(CLOCK_EVENT_DEVICE_LCD) == CLOCK_EVENT_DEVICE_LCD_OFF) {
				visibility = 0;
				return;
			}
			if (clock_service_event_state_get(CLOCK_EVENT_APP) == CLOCK_EVENT_APP_PAUSE) {
				visibility = 0;
				return;
			}

			if (visibility == 0) {
				evas_object_smart_callback_call(page, CLOCK_SMART_SIGNAL_PAUSE, item);
			} else if (visibility == 1) {
				if (evas_object_data_get(page, CLOCK_PRIVATE_DATA_KEY_DISPLAY_CUE) != NULL) {
					clock_view_cue_display_set(page, 1, 0);
					evas_object_data_del(page, CLOCK_PRIVATE_DATA_KEY_DISPLAY_CUE);
				}
				evas_object_smart_callback_call(page, CLOCK_SMART_SIGNAL_RESUME, item);
			}
		}
	}
}

static void _view_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	int idx_request = (int)event_info;
	Evas_Object *page = obj;

	if (idx_request == CLOCK_VIEW_REQUEST_DRAWER_HIDE) {
		_drawer_hide(page, INDICATOR_HIDE);
	} else if (idx_request == CLOCK_VIEW_REQUEST_DRAWER_SHOW) {
		_drawer_show(page, INDICATOR_SHOW);
	}
}

static void _event_handler_view_resized(Evas_Object *page, int event, void *data)
{
	clock_h clock = data;
	ret_if(data == NULL);

	Evas_Object *layout = page_get_item(page);
	ret_if(layout == NULL);

	Evas_Object *item = elm_object_part_content_get(layout, "item");
	ret_if(item == NULL);

	evas_object_resize(item, clock->w, clock->h);
	evas_object_size_hint_min_set(item, clock->w, clock->h);
	evas_object_size_hint_max_set(item, clock->w, clock->h);
}

static void _event_handler_app_state(Evas_Object *page, int event, void *data)
{
	int visibility = 0;
	Evas_Object *item = page_get_item(page);

	visibility = _visibility_get(page);
	_visibility_vconf_set(visibility);

	if (event == CLOCK_EVENT_APP_RESUME) {
		if (_visibility_get(page) == 0) {
			evas_object_data_set(page, CLOCK_PRIVATE_DATA_KEY_DISPLAY_CUE, (void*)1);
		} else {
			clock_view_cue_display_set(page, 1, 0);
			evas_object_smart_callback_call(page, CLOCK_SMART_SIGNAL_RESUME, item);
		}

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
		if (_is_drawer_displayed(page) == 1) {
			if (tutorial_is_exist() == 1) {
				clock_view_indicator_show(INDICATOR_SHOW);
			}
		}
#endif
	} else if (event == CLOCK_EVENT_APP_PAUSE) {
		evas_object_smart_callback_call(page, CLOCK_SMART_SIGNAL_PAUSE, item);
#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
		if (_is_drawer_displayed(page) == 1) {
			if (tutorial_is_exist() == 0) {
				_drawer_hide(page, INDICATOR_HIDE);
			} else {
				clock_view_indicator_show(INDICATOR_HIDE);
			}
		}
#endif
	} else if (event == CLOCK_EVENT_APP_LANGUAGE_CHANGED) {
#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
		clock_view_hidden_event_handler(page, CLOCK_EVENT_APP_LANGUAGE_CHANGED);
#endif
	}
}

static void _event_handler_lcd(Evas_Object *page, int event, void *data)
{
	if (_visibility_get(page) == 1 && event == CLOCK_EVENT_DEVICE_LCD_ON) {
		clock_view_cue_display_set(page, 1, 0);
		evas_object_data_del(page, CLOCK_PRIVATE_DATA_KEY_DISPLAY_CUE);
	} else if (event == CLOCK_EVENT_DEVICE_LCD_ON) {
		evas_object_data_set(page, CLOCK_PRIVATE_DATA_KEY_DISPLAY_CUE, (void*)1);
#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
		if (_is_drawer_displayed(page) == 1) {
			clock_view_indicator_show(INDICATOR_SHOW);
		}
#endif
	} else if (event == CLOCK_EVENT_DEVICE_LCD_OFF) {
		clock_view_cue_display_set(page, 0, 0);
#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
		if (_is_drawer_displayed(page) == 1) {
			if (tutorial_is_exist() == 0) {
				_drawer_hide(page, INDICATOR_HIDE);
			} else {
				clock_view_indicator_show(INDICATOR_HIDE);
			}
		}
#endif
	}
}

static void _event_handler_backkey(Evas_Object *page, int event, void *data)
{
	Evas_Object *scroller = _scroller_get();
	ret_if(scroller == NULL);

	int clock_visibility = _visibility_get(page);

	if (clock_visibility == 0) {
		_D("show a clock");
		scroller_bring_in_by_push_type(scroller, SCROLLER_PUSH_TYPE_CENTER, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_ANIMATOR);
	} else {
		if (util_feature_enabled_get(FEATURE_CLOCK_HIDDEN_BUTTON) == 0) {
			return;
		}

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
		_D("show a hidden button");
		if (_is_drawer_displayed(page) == 1) {
			return;
		}
#endif

		_drawer_show(page, INDICATOR_SHOW);
	}

	return;
}

static void _event_handler_screen_reader(Evas_Object *page, int event, void *data)
{
	if (event == CLOCK_EVENT_SCREEN_READER_ON) {
		_screen_reader_enable_set(page, 1);
	} else if (event == CLOCK_EVENT_SCREEN_READER_OFF) {
		_screen_reader_enable_set(page, 0);
	}
}

static void _event_handler_modem_status(Evas_Object *page, int event, void *data)
{
	clock_view_indicator_event_handler(page, event);
}

static void _event_handler_blocking_status(Evas_Object *page, int event, void *data)
{
	clock_view_indicator_event_handler(page, event);
}

static void _event_handler_sim_status(Evas_Object *page, int event, void *data)
{
#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
	clock_view_hidden_event_handler(page, event);
#endif
}

static char *_access_info_clock_cb(void *data, Evas_Object *obj)
{
	int hour = 0;
	int minutes = 0;
	time_t tt;
	struct tm st;
	char buf[256] = {0,};

	enum appcore_time_format timeformat = APPCORE_TIME_FORMAT_UNKNOWN;

	tt = time(NULL);
	localtime_r(&tt, &st);

	if(appcore_get_timeformat(&timeformat) != 0) {
		timeformat = APPCORE_TIME_FORMAT_24;
	}

	if(timeformat == APPCORE_TIME_FORMAT_24) {
		hour = st.tm_hour;
		minutes = st.tm_min;

		snprintf(buf, sizeof(buf) - 1, _("IDS_LCKSCN_BODY_IT_IS_P1SD_CP2SD_T_TTS"), hour, minutes);
	} else {
		strftime(buf, sizeof(buf), "%l", &st);
		hour = atoi(buf);
		minutes = st.tm_min;

		if(st.tm_hour >= 0 && st.tm_hour < 12) {
			snprintf(buf, sizeof(buf) - 1, _("IDS_TTS_BODY_IT_IS_PD_CPD_AM"), hour, minutes);
		} else {
			snprintf(buf, sizeof(buf) - 1, _("IDS_TTS_BODY_IT_IS_PD_CPD_PM"), hour, minutes);
		}
	}

	_D("TTS current time : %s", buf);

	return strdup(buf);
}

static void _apps_clicked_cb(void *cbdata, Evas_Object *obj, void *event_info)
{
	if (util_feature_enabled_get(FEATURE_APPS) == 1) {
		apps_main_launch(APPS_LAUNCH_SHOW);
	}

	effect_play_sound();
}

static char *_access_info_apps_cb(void *data, Evas_Object *obj)
{
	char *info = _("IDS_IDLE_BODY_APPS");

	if (info != NULL) {
		return strdup(info);
	}

	return NULL;
}

static char *_access_context_apps_cb(void *data, Evas_Object *obj)
{
	char *info = _("IDS_HELP_BODY_DOUBLE_TAP_TO_OPEN_APP_TRAY_TTS");

	if (info != NULL) {
		return strdup(info);;
	}

	return NULL;
}

static void _del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Object *page = obj;
	Evas_Object *layout = data;
	Evas_Object *obj_sub = NULL;
	ret_if(page == NULL);
	ret_if(layout == NULL);

	/*
	 * clean-up event callback
	 */
	evas_object_event_callback_del(page
			, EVAS_CALLBACK_MOVE, _mouse_move_cb);
	evas_object_event_callback_del(page
			, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb);
	evas_object_smart_callback_del(page
			, CLOCK_SMART_SIGNAL_VIEW_REQUEST, _view_request_cb);

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
	evas_object_event_callback_del(page
			, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb);
	evas_object_smart_callback_del(layout, LAYOUT_SMART_SIGNAL_FLICK_UP, _mouse_flickup_cb);
	elm_object_signal_callback_del(layout, "drawer,displayed", "clock_bg", _drawer_displayed_cb);
	elm_object_signal_callback_del(layout, "drawer,hided", "clock_bg", _drawer_hided_cb);
#endif
	clock_view_indicator_show(INDICATOR_HIDE);

	obj_sub = elm_object_part_content_get(layout, "clock_bg");
	if (obj_sub != NULL) {
		evas_object_del(obj_sub);
	}
	obj_sub = elm_object_part_content_get(layout, "event_blocker");
	if (obj_sub != NULL) {
		evas_object_del(obj_sub);
	}
}

Evas_Object *clock_view_add(Evas_Object *parent, Evas_Object *item)
{
	Eina_Bool ret = EINA_TRUE;
	Evas_Object *page = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *bg = NULL;
	Evas_Object *evb = NULL;
	retv_if(parent == NULL, NULL);

	layout = elm_layout_add(parent);
	goto_if(layout == NULL, ERR);
	ret = elm_layout_file_set(layout, PAGE_CLOCK_EDJE_FILE, "clock_page");
	goto_if(ret == EINA_FALSE, ERR);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(layout);

	bg = evas_object_rectangle_add(main_get_info()->e);
	goto_if(bg == NULL, ERR);
	evas_object_size_hint_min_set(bg, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_size_hint_max_set(bg, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_color_set(bg, 0, 0, 0, 0);
	evas_object_show(bg);
	elm_object_part_content_set(layout, "clock_bg", bg);

	evb = evas_object_rectangle_add(main_get_info()->e);
	goto_if(evb == NULL, ERR);
	evas_object_size_hint_min_set(evb, main_get_info()->root_w, main_get_info()->root_h);
	evas_object_size_hint_weight_set(evb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_repeat_events_set(evb, 1);
	evas_object_color_set(evb, 0, 0, 0, 0);
	evas_object_show(evb);
	elm_object_part_content_set(layout, "event_blocker", evb);

	if (item != NULL) {
		elm_access_object_unregister(item);
	}

	Evas_Object *focus_apps = elm_button_add(layout);
	if (focus_apps != NULL) {
		elm_object_style_set(focus_apps, "focus");
		elm_access_info_cb_set(focus_apps, ELM_ACCESS_INFO, _access_info_apps_cb, NULL);
		elm_access_info_cb_set(focus_apps, ELM_ACCESS_CONTEXT_INFO, _access_context_apps_cb, NULL);
		evas_object_smart_callback_add(focus_apps, "clicked", _apps_clicked_cb, NULL);
		elm_object_part_content_set(layout, "focus.bottom.cue", focus_apps);
	}

	if (item != NULL) {
		elm_object_part_content_set(layout, "item", item);
	}

	page = page_create(parent
					, layout
					, NULL, NULL
					, main_get_info()->root_w, main_get_info()->root_h
					, PAGE_CHANGEABLE_BG_OFF, PAGE_REMOVABLE_OFF);
	goto_if(page == NULL, ERR);

	_visibility_set(page, 1);
	_visibility_vconf_set(1);

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
#ifdef RUN_ON_DEVICE
	Evas_Object *hidden_button = clock_view_hidden_add(page);
	if (hidden_button != NULL) {
		elm_object_part_content_set(layout, "hidden_item", hidden_button);
	} else {
		_E("Failed to add top hidden object");
	}
#endif
#endif

	Evas_Object *cue = clock_view_cue_add(page);
	if (cue != NULL) {
		elm_object_part_content_set(layout, "bottom_cue", cue);
	} else {
		_E("failed to add bottom cue");
	}

	clock_shortcut_view_add(page);
	clock_view_indicator_add(page);

	page_set_effect(page, page_effect_none, page_effect_none);
	evas_object_event_callback_add(page
			, EVAS_CALLBACK_DEL, _del_cb, layout);
	evas_object_event_callback_add(page
			, EVAS_CALLBACK_MOVE, _mouse_move_cb, layout);
	evas_object_event_callback_add(page
			, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, layout);

	evas_object_smart_callback_add(page
			, CLOCK_SMART_SIGNAL_VIEW_REQUEST, _view_request_cb, layout);

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
	evas_object_event_callback_add(page
			, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, layout);
	elm_object_signal_callback_add(layout, "drawer,displayed", "clock_bg", _drawer_displayed_cb, page);
	elm_object_signal_callback_add(layout, "drawer,hided", "clock_bg", _drawer_hided_cb, page);
#endif

	if (clock_service_event_state_get(CLOCK_EVENT_SCREEN_READER) == CLOCK_EVENT_SCREEN_READER_ON) {
		_screen_reader_enable_set(page, 1);
	} else {
		_screen_reader_enable_set(page, 0);
	}

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
	clock_view_hidden_event_handler(page, clock_service_event_state_get(CLOCK_EVENT_POWER));
#endif

	clock_view_indicator_event_handler(page, CLOCK_EVENT_CATEGORY(CLOCK_EVENT_MODEM));

	page_info_s *page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	if (page_info != NULL && page_info->focus != NULL) {
		elm_access_info_cb_set(page_info->focus, ELM_ACCESS_INFO, _access_info_clock_cb, NULL);

		elm_object_focus_next_object_set(page_info->focus, focus_apps, ELM_FOCUS_PREVIOUS);
		elm_access_highlight_next_set(page_info->focus, ELM_HIGHLIGHT_DIR_PREVIOUS, focus_apps);

		elm_object_focus_next_object_set(page_info->focus, focus_apps, ELM_FOCUS_NEXT);
		elm_access_highlight_next_set(page_info->focus, ELM_HIGHLIGHT_DIR_NEXT, focus_apps);

		elm_object_focus_next_object_set(focus_apps, page_info->focus, ELM_FOCUS_PREVIOUS);
		elm_access_highlight_next_set(focus_apps, ELM_HIGHLIGHT_DIR_PREVIOUS, page_info->focus);

		elm_object_focus_next_object_set(focus_apps, page_info->focus, ELM_FOCUS_NEXT);
		elm_access_highlight_next_set(focus_apps, ELM_HIGHLIGHT_DIR_NEXT, page_info->focus);

		page_info->focus_prev = focus_apps;
		page_info->focus_next = focus_apps;
	}

	return page;

ERR:
	if (bg != NULL) evas_object_del(bg);
	if (evb != NULL) evas_object_del(evb);
	if (layout != NULL) evas_object_del(layout);

	return NULL;
}

Evas_Object *clock_view_get_item(Evas_Object *view)
{
	Evas_Object *layout;
	Evas_Object *item;

	layout = page_get_item(view);
	if (!layout) {
		return NULL;
	}

	item = elm_object_part_content_get(layout, "item");
	return item;
}

int clock_view_attach(Evas_Object *page)
{
	int ret = CLOCK_RET_FAIL;
	Evas_Object *scroller = _scroller_get();
	retv_if(page == NULL, CLOCK_RET_FAIL);
	retv_if(scroller == NULL, CLOCK_RET_FAIL);

	page_info_s *page_info = NULL;
	page_info = evas_object_data_get(page, DATA_KEY_PAGE_INFO);
	retv_if(page_info == NULL, CLOCK_RET_FAIL);

	if (scroller_push_page(scroller, page, SCROLLER_PUSH_TYPE_CENTER) == W_HOME_ERROR_NONE) {
		ret = CLOCK_RET_OK;
	}

	if (clock_service_event_state_get(CLOCK_EVENT_SCREEN_READER) == CLOCK_EVENT_SCREEN_READER_ON) {
		if (page_info->focus != NULL) {
			elm_object_focus_set(page_info->focus, EINA_TRUE);
			elm_access_highlight_set(page_info->focus);
		}
	}

	return ret;
}

int clock_view_deattach(Evas_Object *page)
{
	Evas_Object *scroller = _scroller_get();
	retv_if(page == NULL, CLOCK_RET_FAIL);
	retv_if(scroller == NULL, CLOCK_RET_FAIL);

	if (scroller_pop_page(scroller, page) != NULL) {
		return CLOCK_RET_OK;
	}

	return CLOCK_RET_FAIL;
}

void clock_view_show(Evas_Object *page)
{
	Evas_Object *scroller = _scroller_get();
	ret_if(page == NULL);
	ret_if(scroller == NULL);

	scroller_region_show_page(scroller, page, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_INSTANT);
}

Evas_Object *clock_view_empty_add(void)
{
	Evas_Object *scroller = _scroller_get();
	retv_if(scroller == NULL, NULL);

	Evas_Object *page = clock_view_add(scroller, NULL);

	return page;
}

void clock_view_event_handler(clock_h clock, int event, int need_relay)
{
	ret_if(clock == NULL);
	Evas_Object *page = clock->view;

	_D("event:%x received", event);

	static struct evet_handler_s fn_table[] = {
		{
			.event = CLOCK_EVENT_VIEW_RESIZED,
			.process = _event_handler_view_resized,
		},
		{
			.event = CLOCK_EVENT_DEVICE_BACK_KEY,
			.process = _event_handler_backkey,
		},
		{
			.event = CLOCK_EVENT_DEVICE_LCD_ON,
			.process = _event_handler_lcd,
		},
		{
			.event = CLOCK_EVENT_DEVICE_LCD_OFF,
			.process = _event_handler_lcd,
		},
		{
			.event = CLOCK_EVENT_APP_PAUSE,
			.process = _event_handler_app_state,
		},
		{
			.event = CLOCK_EVENT_APP_RESUME,
			.process = _event_handler_app_state,
		},
		{
			.event = CLOCK_EVENT_APP_LANGUAGE_CHANGED,
			.process = _event_handler_app_state,
		},
		{
			.event = CLOCK_EVENT_SCREEN_READER_ON,
			.process = _event_handler_screen_reader,
		},
		{
			.event = CLOCK_EVENT_SCREEN_READER_OFF,
			.process = _event_handler_screen_reader,
		},
		{
			.event = CLOCK_EVENT_SAP_ON,
			.process = _event_handler_modem_status,
		},
		{
			.event = CLOCK_EVENT_SAP_OFF,
			.process = _event_handler_modem_status,
		},
		{
			.event = CLOCK_EVENT_MODEM_ON,
			.process = _event_handler_modem_status,
		},
		{
			.event = CLOCK_EVENT_MODEM_OFF,
			.process = _event_handler_modem_status,
		},
		{
			.event = CLOCK_EVENT_DND_ON,
			.process = _event_handler_blocking_status,
		},
		{
			.event = CLOCK_EVENT_DND_OFF,
			.process = _event_handler_blocking_status,
		},
		{
			.event = CLOCK_EVENT_SIM_INSERTED,
			.process = _event_handler_sim_status,
		},
		{
			.event = CLOCK_EVENT_SIM_NOT_INSERTED,
			.process = _event_handler_sim_status,
		},
		{
			.event = 0,
			.process = NULL,
		},
	};
	ret_if(page == NULL);

	int i = 0;
	for (i = 0; fn_table[i].process != NULL; i++) {
		if (event == fn_table[i].event && fn_table[i].process != NULL) {
			fn_table[i].process(page, fn_table[i].event, clock);
		}
	}
}

int clock_view_display_state_get(Evas_Object *page, int view_type)
{
	retv_if(page == NULL, 0);

#ifndef ENABLE_INDICATOR_BRIEFING_VIEW
	if (view_type == CLOCK_VIEW_TYPE_DRAWER) {
		if (evas_object_data_get(page, CLOCK_PRIVATE_DATA_KEY_HIDDEN_BUTTON_DISPLAY_STATE) != NULL) {
			return 1;
		} else {
			return 0;
		}
	}
#endif

	return 0;
}
