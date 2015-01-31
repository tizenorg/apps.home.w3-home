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

#ifndef __W_HOME_EFFECT_H__
#define __W_HOME_EFFECT_H__

extern w_home_error_e effect_init(void);
extern void effect_fini(void);

extern void effect_set_sound_status(int status);
extern int effect_get_sound_status(void);

extern void effect_play_sound(void);
extern void effect_play_vibration(void);

#endif //__W_HOME_EFFECT_H__

// End of a file
