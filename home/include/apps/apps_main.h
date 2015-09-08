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

#ifndef __APPS_MAIN_H__
#define __APPS_MAIN_H__

#include <app.h>
#include <Ecore_Evas.h>
#include <efl_assist.h>
#include <Elementary.h>
#include <Evas.h>
#include <stdbool.h>

#include "util.h"

typedef struct {
	/* Multi-window : Every windows are written in the instance_list. */
	Eina_List *instance_list;
	Elm_Theme *theme;
	Ea_Theme_Color_Table *color_theme;
	Eina_List *font_theme;
	int first_boot;
	int updated;
	double scale;
	int tts;
	bool longpress_edit_disable;
} apps_main_s;

typedef struct {
	int state;
	int root_w;
	int root_h;
	int booting_state;
	int angle;
	int is_rotated;
	Eina_List *cbs_list[APPS_APP_STATE_MAX];
	Evas *e;
	Ecore_Evas *ee;
	Evas_Object *win;
	Evas_Object *layout;
	char *content;
} instance_info_s;


enum {
	APPS_LAUNCH_INIT = 0,
	APPS_LAUNCH_SHOW,
	APPS_LAUNCH_EDIT,
	APPS_LAUNCH_HIDE,
	APPS_LAUNCH_MAX,
};

HAPI apps_main_s *apps_main_get_info(void);

HAPI apps_error_e apps_main_register_cb(
	instance_info_s *info,
	int state,
	apps_error_e (*result_cb)(void *), void *result_data);

HAPI void apps_main_unregister_cb(
	instance_info_s *info,
	int state,
	apps_error_e (*result_cb)(void *));

HAPI void apps_main_init();
HAPI void apps_main_fini();
HAPI void apps_main_launch(int launch_type);
HAPI void apps_main_pause();
HAPI void apps_main_resume();
HAPI void apps_main_language_chnage();
HAPI void apps_main_theme_chnage();
HAPI Eina_Bool apps_main_is_visible();

HAPI void apps_main_list_backup();
HAPI void apps_main_list_restore();
HAPI void apps_main_list_reset();

HAPI void apps_main_list_tts(int is_tts);

HAPI void apps_main_show_count_add(void);
HAPI int apps_main_show_count_get(void);

#endif //__APPS_MAIN_H__

// End of a file
