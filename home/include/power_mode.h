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

#ifndef __POWER_MODE_H
#define __POWER_MODE_H

typedef enum {
	POWER_MODE_COOLDOWN_ON,
	POWER_MODE_COOLDOWN_OFF,
	POWER_MODE_ENHANCED_ON,
	POWER_MODE_ENHANCED_OFF,
	POWER_MODE_EVENT_MAX,
} power_mode_event_type_e;

#define COOLDOWN_MODE_RELEASE 1
#define COOLDOWN_MODE_LIMITATION 2
#define COOLDOWN_MODE_WARNING 3

extern int power_mode_register_cb(int type, void (*result_cb)(void *, void *), void *result_data);
extern void power_mode_unregister_cb(int type, void (*result_cb)(void *, void *));
extern void power_mode_init(void);
extern void power_mode_ui_init(void);
extern void power_mode_fini(void);
extern int emergency_mode_enabled_get(void);
extern int cooldown_mode_enabled_get(void);
extern int cooldown_mode_warning_get(void);

#endif
