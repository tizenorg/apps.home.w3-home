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

#ifndef __W_HOME_MAIN_H__
#define __W_HOME_MAIN_H__

typedef struct {
	Evas_Object *win;
	Evas *e;
	Evas_Object *layout;
	Ecore_Event_Handler *handler;
	Ea_Theme_Color_Table *color_theme;
	Eina_List *font_theme;
	Elm_Theme *theme;

	int first_boot;
	int state;
	int root_w;
	int root_h;
	int booting_state;
	int is_mapbuf;
	int is_rotated;
	int angle;
	int apps_pid;
	int is_tts;
	int panning_sensitivity;
	int setup_wizard;
	int is_alpm_clock_enabled;
	int is_lcd_on;
	int is_win_visible;
	int is_wide_character;

	Eina_List *cbs_list[APP_STATE_MAX];

	Ecore_Timer *safety_init_timer;
	int is_service_initialized;
} main_s;

extern main_s *main_get_info(void);

extern void main_inc_booting_state(void);
extern void main_dec_booting_state(void);
extern int main_get_booting_state(void);

extern w_home_error_e main_register_cb(
		int state,
		w_home_error_e (*result_cb)(void *), void *result_data);
extern void main_unregister_cb(
		int state,
		w_home_error_e (*result_cb)(void *));

#endif /* __W_HOME_MAIN_H__ */
