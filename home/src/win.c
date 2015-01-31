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

#include <app.h>
#include <Ecore_X.h>
#include <Elementary.h>
#include <Evas.h>
#include <efl_assist.h>
#include <dlog.h>

#include "log.h"
#include "util.h"
#include "main.h"



#ifdef BOOT_OPTIMIZATION
static w_home_error_e _rotate_cb(main_s *info, int angle)
{
	_D("Enter _rotate, angle is %d", angle);
	info->angle = angle;

	switch (angle) {
		case 0:
			_D("Portrait normal");
			info->is_rotated = 0;
			break;
		case 90:
			_D("Landscape reverse");
			info->is_rotated = 1;
			break;
		case 180:
			_D("Portrait reverse");
			info->is_rotated = 0;
			break;
		case 270:
			_D("Landscape normal");
			info->is_rotated = 1;
			break;
		default:
			_E("Cannot reach here, angle is %d", angle);
	}

	/* Rotate all objects */

	return W_HOME_ERROR_NONE;
}



static void _rotation_changed_cb(void *data, Evas_Object *obj, void *event)
{
	main_s *info = data;
	int changed_ang;

	ret_if(!info);

	changed_ang = elm_win_rotation_get(info->win);
	if (changed_ang != info->angle) {
		_rotate_cb(info, changed_ang);
	}
}
#endif



HAPI Evas_Object *win_create(const char *name)
{
	Evas_Object *win = NULL;
	main_s *info = NULL;
	Ecore_X_Window xwin;

	info = main_get_info();
	retv_if(!info, NULL);

	/* Open GL backend */
	elm_config_preferred_engine_set("opengl_x11");
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &info->root_w, &info->root_h);

#if 0 // Doesn't support app_get_preinitialized_window in the wearables
	win = (Evas_Object *) app_get_preinitialized_window(name);
	if (!win) win = elm_win_add(NULL, name, ELM_WIN_BASIC);
#else
	win = elm_win_add(NULL, name, ELM_WIN_BASIC);
#endif
	retv_if(!win, NULL);

	elm_win_title_set(win, name);
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_alpha_set(win, EINA_FALSE);
	elm_win_indicator_mode_set(win, ELM_WIN_INDICATOR_HIDE);
	elm_win_indicator_opacity_set(win, ELM_WIN_INDICATOR_BG_TRANSPARENT);

#ifdef BOOT_OPTIMIZATION
	/* Set available rotations */
	if (elm_win_wm_rotation_supported_get(win)) {
		const int rots[4] = {0, 90, 180, 270};
		elm_win_wm_rotation_available_rotations_set(win, (const int *) &rots, 4);
		_D("Set available rotations, {0, 90, 180, 270}");
	}
	evas_object_smart_callback_add(win, "wm,rotation,changed", _rotation_changed_cb, info);
#endif

	evas_object_resize(win, info->root_w, info->root_h);
	evas_object_show(win);
	info->win = win;
	info->e = evas_object_evas_get(win);

	xwin = elm_win_xwindow_get(win);
	ecore_x_vsync_animator_tick_source_set(xwin);

	return win;
}



HAPI void win_destroy(Evas_Object *win)
{
	main_s *info = NULL;

	ret_if(!win);

	info = main_get_info();
	ret_if(!info);

	evas_object_del(win);
	info->win = NULL;
}



// End of a file
