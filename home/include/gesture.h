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

#ifndef __W_HOME_GESTURE_H__
#define __W_HOME_GESTURE_H__

#define HOME_GESTURE_WRISTUP_OFF 0
#define HOME_GESTURE_WRISTUP_CLOCK 1
#define HOME_GESTURE_WRISTUP_LASTVIEW 2

#define ALPM_MANAGER_STATUS_SHOW "show"
#define ALPM_MANAGER_STATUS_ALPM_HIDE "ALPMHide"
#define ALPM_MANAGER_STATUS_SIMPLE_HIDE "SimpleHide"

typedef enum {
	BEZEL_UP = 0,
	BEZEL_DOWN,
} gesture_type_e;

extern void home_gesture_init(void);
extern void home_gesture_fini(void);

extern int home_gesture_wristup_setting_get(void);
extern void gesture_home_window_effect_set(int is_enable);

extern void gesture_execute_cbs(int mode);

extern w_home_error_e gesture_register_cb(
		int mode,
		void (*result_cb)(void *), void *result_data);

extern void gesture_unregister_cb(
		int mode,
		void (*result_cb)(void *));

#endif // __W_HOME_GESTURE_H__
