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

#include <feedback.h>
#include <vconf.h>

#include "log.h"
#include "util.h"
#include "apps/apps_main.h"



static struct {
	int sound_status;
	int is_feedback_initialized;
} effect_info = {
	.sound_status = 0,
	.is_feedback_initialized = 0,
};



HAPI inline void apps_effect_set_sound_status(int status)
{
	effect_info.sound_status = status;
}



HAPI inline int apps_effect_get_sound_status(void)
{
	return effect_info.sound_status;
}



HAPI apps_error_e apps_effect_init(void)
{
	retv_if(1 == effect_info.is_feedback_initialized, APPS_ERROR_NONE);
	retv_if(0 != feedback_initialize(), APPS_ERROR_FAIL);
	retv_if(vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &effect_info.sound_status) < 0, APPS_ERROR_FAIL);

	effect_info.is_feedback_initialized = 1;

	return APPS_ERROR_NONE;
}



HAPI void apps_effect_fini(void)
{
	ret_if(0 == effect_info.is_feedback_initialized);
	ret_if(0 != feedback_deinitialize());

	effect_info.is_feedback_initialized = 0;
}



HAPI void apps_effect_play_sound(void)
{
	if (effect_info.is_feedback_initialized == 0) {
		apps_effect_init();
	}

	ret_if(vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &effect_info.sound_status) < 0);

	if (effect_info.sound_status) {
		feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TOUCH_TAP);
	}
	else {
		_APPS_E("effect_info.sound_status: [%d]", effect_info.sound_status);
	}
}



HAPI void apps_effect_play_vibration(void)
{
	if (effect_info.is_feedback_initialized == 0) {
		apps_effect_init();
	}

	feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_HOLD);
}



// End of a file
