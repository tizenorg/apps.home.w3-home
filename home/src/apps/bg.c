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
#include <vconf.h>

#include "log.h"
#include "util.h"



#define VCONFKEY_WMS_BG_MODE "db/wms/home_bg_mode"
#define VCONFKEY_WMS_BG_PALETTE "db/wms/home_bg_palette"
#define BG_TYPE_COLOR 0
#define BG_TYPE_IMAGE 1
#define BG_TYPE_GALLERY 2

#if 0 // There is no conformant in the wearable.
static Evas_Object *_create_conformant(Evas_Object *win)
{
	Evas_Object *conformant = NULL;
	conformant = (Evas_Object *) app_get_preinitialized_conformant();
	if (!conformant) conformant = elm_conformant_add(win);
	retv_if(NULL == conformant, NULL);

	evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, conformant);

	elm_object_signal_emit(conformant, "elm,state,indicator,overlap", "elm");
	elm_object_signal_emit(conformant, "elm,state,virtualkeypad,disable", "");
	elm_object_signal_emit(conformant, "elm,state,clipboard,disable", "");

	evas_object_data_set(conformant, DATA_KEY_WIN, win);
	evas_object_data_set(win, DATA_KEY_CONFORMANT, conformant);
	evas_object_show(conformant);

	return conformant;
}



void _destroy_conformant(Evas_Object *win)
{
	Evas_Object *conformant = NULL;
	ret_if(!win);

	conformant = evas_object_data_del(win, DATA_KEY_CONFORMANT);
	ret_if(!conformant);

	evas_object_data_del(conformant, DATA_KEY_WIN);
	evas_object_del(conformant);
}
#endif



#define DEFAULT_HOME_BG_PATH "/opt/share/settings/Wallpapers/Home_default.png"
char *_bg_filename(void)
{
	char *buf;

	buf = vconf_get_str(VCONFKEY_BGSET);
	if (!buf) {
		buf = strdup(DEFAULT_HOME_BG_PATH);
	}

	return buf;
}


static int _get_bg_type(void)
{
	/* 0 : Color, 1 : Image, 2 : Gallery */
	int val = -1;

	if(vconf_get_int(VCONFKEY_WMS_BG_MODE, &val) < 0) {
		_APPS_E("Failed to get WMS_HOME_BG_MODE");
		return BG_TYPE_IMAGE;
	}

	return val;
}



static void _change_bg(keynode_t *node, void *data)
{
	int bg_type = BG_TYPE_IMAGE;
	Evas_Object *bg = data;
	char *buf = NULL;

	ret_if(!bg);

	bg_type = _get_bg_type();
	_APPS_D("bg_type:%d", bg_type);
	switch(bg_type)
	{
		case BG_TYPE_COLOR:
		{
			buf = vconf_get_str(VCONFKEY_WMS_BG_PALETTE);

			if (buf) {
				int rgb = strtoul(buf, NULL, 16);
				elm_bg_color_set(bg,
					(rgb >> 16) & 0xFF,
					(rgb >> 8) & 0xFF,
					(rgb) & 0xFF);
				elm_bg_file_set(bg, NULL, NULL);
			}
		}
		break;
		case BG_TYPE_IMAGE:
		case BG_TYPE_GALLERY:
		{
			buf = _bg_filename();
			ret_if(!buf);

			_APPS_D("Change the background[%s]", buf);

			elm_bg_file_set(bg, buf, NULL);
		}
		break;
		default:
			_APPS_E("bg_type[%d] is unknown", bg_type);
			break;
	}

	free(buf);
}



static void _bg_down_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
	_APPS_D("BG is pressed");
}



static void _bg_up_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
	_APPS_D("BG is released");
}



Evas_Object *apps_bg_create(Evas_Object *win, int w, int h)
{
	Evas_Object *layout = NULL;
	Evas_Object *bg = NULL;

	layout = evas_object_data_get(win, DATA_KEY_LAYOUT);
	retv_if(!layout, NULL);

#if 0
	bg = app_get_preinitialized_background();
	if (!bg) bg = elm_bg_add(win);
#else
	bg = elm_bg_add(win);
#endif
	retv_if(!bg, NULL);

	elm_bg_option_set(bg, ELM_BG_OPTION_SCALE);
	elm_object_part_content_set(layout, "container", bg);

	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(bg, w, h);
	evas_object_show(bg);

	evas_object_data_set(win, DATA_KEY_BG, bg);
	evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_DOWN, _bg_down_cb, NULL);
	evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_UP, _bg_up_cb, NULL);

	if (vconf_notify_key_changed(VCONFKEY_WMS_BG_MODE, _change_bg, bg) < 0) {
		_APPS_E("Failed to register the VCONFKEY_WMS_BG_MODE callback");
	}

	/* Changing the bg after setting DATA_KEY_BG */
	_change_bg(NULL, bg);

	return bg;
}



void apps_bg_destroy(Evas_Object *win)
{
	Evas_Object *bg = NULL;

	if (vconf_ignore_key_changed(VCONFKEY_WMS_BG_MODE, _change_bg) < 0) {
		_APPS_E("Failed to ignore the VCONFKEY_WMS_BG_MODE callback");
	}

	evas_object_event_callback_del(bg, EVAS_CALLBACK_MOUSE_DOWN, _bg_down_cb);
	evas_object_event_callback_del(bg, EVAS_CALLBACK_MOUSE_UP, _bg_up_cb);
	bg = evas_object_data_del(win, DATA_KEY_BG);
	ret_if(!bg);
	evas_object_del(bg);
}



// End of a file
