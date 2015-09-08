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

#ifndef __W_HOME_TUTORIAL_H__
#define __W_HOME_TUTORIAL_H__

enum {
	TUTORIAL_STEP_INIT = 0,
	TUTORIAL_STEP_ONE,
	TUTORIAL_STEP_TWO,
	TUTORIAL_STEP_THREE,
	TUTORIAL_STEP_FOUR,
	TUTORIAL_STEP_FIVE,
	TUTORIAL_STEP_SIX,
	TUTORIAL_STEP_SEVEN,
	TUTORIAL_STEP_MAX,
};

extern void tutorial_set_transient_for(Ecore_X_Window xwin);
extern int tutorial_is_first_boot(void);
extern int tutorial_is_exist(void);
extern Evas_Object *tutorial_create(Evas_Object *layout);
extern void tutorial_destroy(Evas_Object *tutorial);
extern int tutorial_is_apps(Ecore_X_Window xwin);
extern int tutorial_is_indicator(Ecore_X_Window xwin);

#endif /* __W_HOME_TUTORIAL_H__ */
