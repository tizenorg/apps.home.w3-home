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

#include <feedback.h>
#include <vconf.h>
#include <Evas.h>
#include <bundle.h>
#include <efl_assist.h>
#include <dlog.h>

#include "log.h"
#include "util.h"
#include "main.h"
#include "effect.h"


static struct {
	int sound_status;
	int is_feedback_initialized;
} effect_info = {
	.sound_status = 0,
	.is_feedback_initialized = 0,
};



HAPI inline void effect_set_sound_status(int status)
{
	effect_info.sound_status = status;
}



HAPI inline int effect_get_sound_status(void)
{
	return effect_info.sound_status;
}



HAPI w_home_error_e effect_init(void)
{
	retv_if(1 == effect_info.is_feedback_initialized, W_HOME_ERROR_NONE);
	retv_if(0 != feedback_initialize(), W_HOME_ERROR_FAIL);
	retv_if(vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &effect_info.sound_status) < 0, W_HOME_ERROR_FAIL);

	effect_info.is_feedback_initialized = 1;

	return W_HOME_ERROR_NONE;
}



HAPI void effect_fini(void)
{
	ret_if(0 == effect_info.is_feedback_initialized);
	ret_if(0 != feedback_deinitialize());

	effect_info.is_feedback_initialized = 0;
}



HAPI void effect_play_sound(void)
{
	int state = main_get_info()->state;

	if (APP_STATE_PAUSE == state)
	{
		_D("Do not play sound");
		return;
	}

	if (effect_info.is_feedback_initialized == 0) {
		effect_init();
	}

	ret_if(vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &effect_info.sound_status) < 0);

	if (effect_info.sound_status) {
		feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TOUCH_TAP);
	}
}



HAPI void effect_play_vibration(void)
{
	int state = main_get_info()->state;

	if (APP_STATE_PAUSE == state)
	{
		_D("Do not play vibration");
		return;
	}

	if (effect_info.is_feedback_initialized == 0) {
		effect_init();
	}

	feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_HOLD);
}



// End of a file
