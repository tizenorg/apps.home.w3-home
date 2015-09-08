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

#ifndef __W_HOME_KEY_H__
#define __W_HOME_KEY_H__

typedef enum {
	KEY_CB_RET_CONTINUE = 0,
	KEY_CB_RET_STOP,
} key_cb_ret_e;

typedef enum {
	KEY_TYPE_BACK = 0,
	KEY_TYPE_HOME,
	KEY_TYPE_BEZEL_UP,
	KEY_TYPE_MAX,
} key_type_e;

/* Key callbacks are managed as a stack[LIFO] */
extern w_home_error_e key_register_cb(
	int type,
	key_cb_ret_e (*result_cb)(void *), void *result_data);

extern void key_unregister_cb(
	int type,
	key_cb_ret_e (*result_cb)(void *));

extern void key_register(void);
extern void key_unregister(void);
extern void key_cb_execute(int type);

#endif //__W_HOME_KEY_H__

// End of a file
