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
#include <efl_assist.h>
#include <dlog.h>
#include <bundle.h>

#include "log.h"
#include "util.h"
#include "main.h"
#include "bg.h"
#include "dbus.h"

#define VCONFKEY_WMS_BG_MODE "db/wms/home_bg_mode"
#define VCONFKEY_WMS_BG_PALETTE "db/wms/home_bg_palette"
#define VCONFKEY_WMS_BG_GALLERY "db/wms/home_bg_gallery"
#define BG_DEFAULT_ALPHA 127



typedef enum {
	BG_TYPE_COLOR = 0,
	BG_TYPE_IMAGE,
	BG_TYPE_GALLERY,
	BG_TYPE_MAX,
} bg_type_e;



static bg_type_e _get_bg_type(void)
{
	/* 0 : Color, 1 : Image, 2 : Gallery */
	int val = -1;

	if(vconf_get_int(VCONFKEY_WMS_BG_MODE, &val) < 0) {
		_E("Failed to get WMS_HOME_BG_MODE");
		return BG_TYPE_IMAGE;
	}

	return val;
}



HAPI void bg_set_rgb(Evas_Object *bg, const char *buf)
{
	int rgb;
	int r;
	int g;
	int b;

	ret_if(!bg);
	ret_if(!buf);

	rgb = strtoul(buf, NULL, 16);

	r = (rgb & 0x00FF0000) >> 16;
	g = (rgb & 0x0000FF00) >> 8;
	b = rgb & 0x000000FF;

	elm_bg_color_set(bg, r, g, b);
	elm_bg_file_set(bg, NULL, NULL);
}



#define DEFAULT_BG_PATH     "/opt/share/settings/Wallpapers/Home_default.png"
static void _change_bg(keynode_t *node, void *data)
{
	bg_type_e bg_type = BG_TYPE_IMAGE;
	Evas_Object *win = data;
	Evas_Object *bg;
	char *buf = NULL;

	if (node) {
		_D("BG is changed by vconf");
	}

	bg = evas_object_data_get(win, DATA_KEY_BG);
	if (!bg) {
		_E("Cannot get bg");
		return;
	}

	bg_type = _get_bg_type();
	switch (bg_type) {
	case BG_TYPE_COLOR:
		buf = vconf_get_str(VCONFKEY_WMS_BG_PALETTE);
		break_if(!buf);

		bg_set_rgb(bg, buf);
		break;
	case BG_TYPE_IMAGE:
	case BG_TYPE_GALLERY:
		buf = vconf_get_str(VCONFKEY_BGSET);
		buf = (buf == NULL) ? strdup(DEFAULT_BG_PATH) : buf;
		break_if(!buf);

		if (elm_bg_file_set(bg, buf, NULL) == EINA_FALSE) {
			_E("Failed to set BG file:%s", buf);
		} else {
			_D("BG is changed to %s", buf);
		}
		_W("file size:%lld", ecore_file_size(buf));
		break;
	default:
		_E("Cannot reach here");
		break;
	}
	_W("BG : [%d, %s]", bg_type, buf);
	free(buf);

	home_dbus_lcd_on_signal_send(EINA_FALSE);
}



static void _bg_down_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
	_D("BG is pressed");
}



static void _bg_up_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
	_D("BG is released");
}



HAPI Evas_Object *bg_create(Evas_Object *win)
{
	Evas_Object *bg = NULL;

#if 0
	bg = app_get_preinitialized_background();
	if (!bg) bg = elm_bg_add(win);
#else
	bg = elm_bg_add(win);
#endif
	retv_if(!bg, NULL);

	elm_bg_option_set(bg, ELM_BG_OPTION_SCALE);
	elm_win_resize_object_add(win, bg);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(bg);
	evas_object_data_set(win, DATA_KEY_BG, bg);
	evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_DOWN, _bg_down_cb, NULL);
	evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_UP, _bg_up_cb, NULL);

	if (vconf_notify_key_changed(VCONFKEY_WMS_BG_MODE, _change_bg, win) < 0) {
		_E("Failed to register the VCONFKEY_WMS_BG_MODE callback");
	}

	/* Changing the bg after setting DATA_KEY_BG */
	_change_bg(NULL, win);

	return bg;
}



HAPI void bg_destroy(Evas_Object *win)
{
	Evas_Object *bg = NULL;

	if (vconf_ignore_key_changed(VCONFKEY_WMS_BG_MODE, _change_bg) < 0) {
		_E("Failed to ignore the VCONFKEY_WMS_BG_MODE callback");
	}

	evas_object_event_callback_del(bg, EVAS_CALLBACK_MOUSE_DOWN, _bg_down_cb);
	evas_object_event_callback_del(bg, EVAS_CALLBACK_MOUSE_UP, _bg_up_cb);
	bg = evas_object_data_del(win, DATA_KEY_BG);
	ret_if(!bg);
	evas_object_del(bg);
}



HAPI void bg_register_object(Evas_Object *obj)
{
	Evas_Object *bg;

	bg = elm_object_part_content_get(obj, "bg");
	if (bg) {
		evas_object_color_set(bg, 0, 0, 0, 0);
	}
}



// End of a file
