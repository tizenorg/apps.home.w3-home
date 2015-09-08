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

#ifndef __APPS_EFFECT_H__
#define __APPS_EFFECT_H__

#include "util.h"

HAPI apps_error_e apps_effect_init(void);
HAPI void apps_effect_fini(void);

HAPI void apps_effect_set_sound_status(int status);
HAPI int apps_effect_get_sound_status(void);

HAPI void apps_effect_play_sound(void);
HAPI void apps_effect_play_vibration(void);

#endif //__APPS_EFFECT_H__

// End of a file
