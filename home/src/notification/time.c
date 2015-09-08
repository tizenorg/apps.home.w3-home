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



#include <unistd.h>
#include <glib.h>
#include <system_settings.h>

#include <utils_i18n.h>
#include <string.h>

#include "util.h"
#include "log.h"

#define BUF_FORMATTER 64



static struct {
	char *timezone;
	char *locale;

	i18n_udatepg_h generator;
	i18n_udate_format_h formatter_time;
	i18n_udate_format_h formatter_ampm;
	i18n_udate_format_h formatter_time_24;
	i18n_udate_format_h formatter_date;

	int timeformat;
	Eina_Bool is_pre;
} notification_time_s = {
	.timezone = NULL,
	.locale = NULL,

	.generator = NULL,
	.formatter_time = NULL,
	.formatter_ampm = NULL,
	.formatter_time_24 = NULL,
	.formatter_date = NULL,

	.timeformat = 0,
	.is_pre = EINA_FALSE,
};



static char *_get_timezone(void)
{
	char *timezone = NULL;

	int ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_TIMEZONE, &timezone);
	if (ret < 0) {
		_E("fail to get the locale time zone value(%d)", ret);
	}

	if (!timezone) {
		_E("system settings fail to get value: time zone");
	}
	_D("time zone : %s", timezone);

	return timezone;
}



static char *_get_locale(void)
{
	char *locale = NULL;
	int ret = -1;

	ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, &locale);
	if (ret < 0) {
		_E("fail to get the locale country value(%d)", ret);
		return strdup("en_US");
	}

	if (!locale) {
		_E("system settings fail to get value: region format");
		return strdup("en_US");
	}
	_D("locale is %s", locale);

	return locale;
}



static int _get_timeformat(void)
{
	bool val = false;
	int ret = -1;

	ret = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &val);
	if (ret < 0) {
		_E("fail to get the timeformat(%d)", ret);
	}

	return val;
}



static i18n_udatepg_h _get_generator(void)
{
	i18n_udatepg_h generator = NULL;
	int status = I18N_ERROR_INVALID_PARAMETER;

	status = i18n_ulocale_set_default(NULL);
	retv_if(status != I18N_ERROR_NONE, NULL);

	status = i18n_udatepg_create(notification_time_s.locale, &generator);
	if (status != I18N_ERROR_NONE) {
		_E("udatepg_creation is failed");
		return NULL;
	}

	_D("Get a generator success");

	return generator;
}



static i18n_udate_format_h _get_time_formatter(void)
{
	i18n_uchar u_pattern[BUF_FORMATTER] = {0, };
	i18n_uchar u_timezone[BUF_FORMATTER] = {0, };
	i18n_uchar u_best_pattern[BUF_FORMATTER] = {0, };
	i18n_udate_format_h formatter = NULL;

	char a_best_pattern[BUF_FORMATTER] = {0, };
	char *a_best_pattern_fixed = NULL;

	int32_t u_best_pattern_capacity;
	int32_t best_pattern_len;
	int status = I18N_ERROR_INVALID_PARAMETER;

	/* only 12 format */
	if (!i18n_ustring_copy_ua_n(u_pattern, "h:mm", sizeof(u_pattern))) {
		_E("ustring_copy() is failed.");
		return NULL;
	}

	u_best_pattern_capacity =
			(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));

	status = i18n_udatepg_get_best_pattern(notification_time_s.generator, u_pattern, i18n_ustring_get_length(u_pattern),
								u_best_pattern, u_best_pattern_capacity, &best_pattern_len);
	if (status != I18N_ERROR_NONE) {
		_E("get best pattern() failed(%d)", status);
		return NULL;
	}

	/* remove am/pm of best pattern */
	retv_if(!i18n_ustring_copy_au(a_best_pattern, u_best_pattern), NULL);
	_D("best pattern [%s]", a_best_pattern);

	a_best_pattern_fixed = strtok(a_best_pattern, "a");
	a_best_pattern_fixed = strtok(a_best_pattern_fixed, " ");
	_D("best pattern fixed [%s]", a_best_pattern_fixed);

	if (a_best_pattern_fixed) {
		/* exception - da_DK */
		if (strncmp(notification_time_s.locale, "da_DK", 5) == 0
		|| strncmp(notification_time_s.locale, "mr_IN", 5) == 0){

			char *a_best_pattern_changed = g_strndup("h:mm", 4);
			_D("best pattern is changed [%s]", a_best_pattern_changed);
			if (a_best_pattern_changed) {
				i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_changed);
				g_free(a_best_pattern_changed);
			}
		}
		else {
			retv_if(!i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_fixed), NULL);
		}
	}

	/* change char to UChar */
	retv_if(!i18n_ustring_copy_n(u_pattern, u_best_pattern, sizeof(u_pattern)), NULL);

	/* get formatter */
	i18n_ustring_copy_ua_n(u_timezone, notification_time_s.timezone, sizeof(u_timezone));
	status = i18n_udate_create(I18N_UDATE_PATTERN, I18N_UDATE_PATTERN, notification_time_s.locale, u_timezone, -1,
							u_pattern, -1, &formatter);
	if (!formatter) {
		_E("time_create() is failed.%d", status);

		return NULL;
	}

	_D("getting time formatter success");
	return formatter;
}



static i18n_udate_format_h _get_time_formatter_24(void)
{
	i18n_uchar u_pattern[BUF_FORMATTER] = {0,};
	i18n_uchar u_timezone[BUF_FORMATTER] = {0,};
	i18n_uchar u_best_pattern[BUF_FORMATTER] = {0,};
	i18n_udate_format_h formatter = NULL;

	char a_best_pattern[BUF_FORMATTER] = {0.};
	char *a_best_pattern_fixed = NULL;

	int32_t u_best_pattern_capacity;
	int32_t best_pattern_len;
	int status = I18N_ERROR_INVALID_PARAMETER;

	/* only 12 format */
	if (!i18n_ustring_copy_ua_n(u_pattern, "H:mm", sizeof(u_pattern))) {
		_E("ustring_copy() is failed.");
		return NULL;
	}

	u_best_pattern_capacity =
			(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));

	status = i18n_udatepg_get_best_pattern(notification_time_s.generator, u_pattern, i18n_ustring_get_length(u_pattern),
								u_best_pattern, u_best_pattern_capacity, &best_pattern_len);
	if (status != I18N_ERROR_NONE) {
		_E("get best pattern() failed(%d)", status);
		return NULL;
	}

	/* remove am/pm of best pattern */
	retv_if(!i18n_ustring_copy_au(a_best_pattern, u_best_pattern), NULL);
	_D("best pattern [%s]", a_best_pattern);

	a_best_pattern_fixed = strtok(a_best_pattern, "a");
	a_best_pattern_fixed = strtok(a_best_pattern_fixed, " ");
	_D("best pattern fixed [%s]", a_best_pattern_fixed);

	if (a_best_pattern_fixed) {
		/* exception - pt_BR(HH'h'mm), id_ID, da_DK */
		if (strncmp(a_best_pattern_fixed, "HH'h'mm", 7) == 0
			|| strncmp(notification_time_s.locale, "id_ID", 5) == 0
			|| strncmp(notification_time_s.locale, "da_DK", 5) == 0
			|| strncmp(notification_time_s.locale, "mr_IN", 5) == 0) {

			char *a_best_pattern_changed = g_strndup("HH:mm", 5);
			_D("best pattern is changed [%s]", a_best_pattern_changed);
			if (a_best_pattern_changed) {
				i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_changed);
				g_free(a_best_pattern_changed);
			}
		}
		else {
			retv_if(!i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_fixed), NULL);
		}
	}

	/* change char to UChar */
	retv_if(!i18n_ustring_copy_n(u_pattern, u_best_pattern, sizeof(u_pattern)), NULL);

	/* get formatter */
	i18n_ustring_copy_ua_n(u_timezone, notification_time_s.timezone, sizeof(u_timezone));
	status = i18n_udate_create(I18N_UDATE_PATTERN, I18N_UDATE_PATTERN, notification_time_s.locale, u_timezone, -1,
							u_pattern, -1, &formatter);
	if (!formatter) {
		_E("time24_create() is failed.");
		return NULL;
	}

	_D("Getting time formatter success");

	return formatter;
}



static i18n_udate_format_h _get_date_formatter(void)
{
	i18n_uchar u_timezone[BUF_FORMATTER] = {0, };
	i18n_uchar u_skeleton[BUF_FORMATTER] = {0, };
	i18n_uchar u_best_pattern[BUF_FORMATTER] = {0, };
	i18n_udate_format_h formatter = NULL;

	int32_t u_best_pattern_capacity;
	int32_t skeleton_len = 0;
	int status = I18N_ERROR_INVALID_PARAMETER;

	i18n_ustring_copy_ua_n(u_skeleton, "MMMEd", strlen("MMMEd"));
	skeleton_len = i18n_ustring_get_length(u_skeleton);

	u_best_pattern_capacity =
					(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));
	status = i18n_udatepg_get_best_pattern(notification_time_s.generator, u_skeleton, skeleton_len,
								u_best_pattern, u_best_pattern_capacity, &status);
	if (status != I18N_ERROR_NONE) {
		_E("get best pattern() failed(%d)", status);
		return NULL;
	}

	if (strncmp(notification_time_s.locale, "fi_FI", 5) == 0) {
		char *a_best_pattern_changed = g_strndup("ccc, d. MMM", 11);
		_D("date formatter best pattern is changed [%s]", a_best_pattern_changed);
		if (a_best_pattern_changed) {
			i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_changed);
			g_free(a_best_pattern_changed);
		}
	}

	i18n_ustring_copy_ua_n(u_timezone, notification_time_s.timezone, sizeof(u_timezone));
	status = i18n_udate_create(I18N_UDATE_PATTERN, I18N_UDATE_PATTERN, notification_time_s.locale, u_timezone, -1, u_best_pattern, -1, &formatter);

	if (!formatter) {
		_E("udate_create() is failed.");
		return NULL;
	}

	_D("getting date formatter success");

	return formatter;

}



static i18n_udate_format_h _get_ampm_formatter(void)
{
	i18n_uchar u_timezone[BUF_FORMATTER] = {0, };
	i18n_uchar u_skeleton[BUF_FORMATTER] = {0, };
	i18n_uchar u_best_pattern[BUF_FORMATTER] = {0, };
	i18n_udate_format_h formatter = NULL;

	char a_best_pattern[BUF_FORMATTER] = {0, };
	int32_t skeleton_len = 0;
	int32_t u_best_pattern_capacity;
	int status = I18N_ERROR_INVALID_PARAMETER;

	i18n_ustring_copy_ua_n(u_skeleton, "hhmm", strlen("hhmm"));
	skeleton_len = i18n_ustring_get_length(u_skeleton);

	u_best_pattern_capacity =
					(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));
	status = i18n_udatepg_get_best_pattern(notification_time_s.generator, u_skeleton, skeleton_len,
								u_best_pattern, u_best_pattern_capacity, &status);
	if (status != I18N_ERROR_NONE) {
		_E("get best pattern() failed(%d)", status);
		return NULL;
	}

	i18n_ustring_copy_au(a_best_pattern, u_best_pattern);
	i18n_ustring_copy_ua(u_best_pattern, "a");

	if (a_best_pattern[0] == 'a') {
		notification_time_s.is_pre = EINA_TRUE;
	} else {
		notification_time_s.is_pre = EINA_FALSE;
	}

	/* get formatter */
	i18n_ustring_copy_ua_n(u_timezone, notification_time_s.timezone, sizeof(u_timezone));
	status = i18n_udate_create(I18N_UDATE_PATTERN, I18N_UDATE_PATTERN, notification_time_s.locale, u_timezone, -1, u_best_pattern, -1, &formatter);
	if (!formatter) {
		_E("ampm_create() is failed.");
		return NULL;
	}

	_D("getting ampm formatter success");

	return formatter;
}



static void _set_formatters(void)
{
	notification_time_s.generator = _get_generator();
	notification_time_s.formatter_time = _get_time_formatter();
	notification_time_s.formatter_ampm = _get_ampm_formatter();
	notification_time_s.formatter_time_24 = _get_time_formatter_24();
	notification_time_s.formatter_date = _get_date_formatter();
}



static void _remove_formatters(void)
{
	if (notification_time_s.generator) {
		i18n_udatepg_destroy(notification_time_s.generator);
		notification_time_s.generator = NULL;
	}

	if (notification_time_s.formatter_time) {
		i18n_udate_destroy(notification_time_s.formatter_time);
		notification_time_s.formatter_time = NULL;
	}

	if (notification_time_s.formatter_ampm) {
		i18n_udate_destroy(notification_time_s.formatter_ampm);
		notification_time_s.formatter_ampm = NULL;
	}

	if (notification_time_s.formatter_time_24) {
		i18n_udate_destroy(notification_time_s.formatter_time_24);
		notification_time_s.formatter_time_24 = NULL;
	}

	if (notification_time_s.formatter_date) {
		i18n_udate_destroy(notification_time_s.formatter_date);
		notification_time_s.formatter_date = NULL;
	}
}



static i18n_uchar *_uastrcpy(const char *chars)
{
	int len = 0;
	i18n_uchar *str = NULL;

	len = strlen(chars);
	str = (i18n_uchar *) malloc(sizeof(i18n_uchar) *(len + 1));
	if (!str) {
		return NULL;
	}

	i18n_ustring_copy_ua(str, chars);

	return str;
}



static void _set_timezone(const char *timezone)
{
	int ec = I18N_ERROR_NONE;
	i18n_uchar *str = NULL;

	ret_if(!timezone);

	str = _uastrcpy(timezone);

	ec = i18n_ucalendar_set_default_timezone(str);
	if (ec != I18N_ERROR_NONE) {
		_E("ucal_setDefaultTimeZone() FAILED");
	}
	free(str);
}



static void _time_changed_cb(system_settings_key_e key, void *data)
{
	_D("System setting key : %d", key);

	if (notification_time_s.locale) {
		free(notification_time_s.locale);
		notification_time_s.locale = NULL;
	}
	notification_time_s.locale = _get_locale();

	if (notification_time_s.timezone) {
		free(notification_time_s.timezone);
		notification_time_s.timezone = NULL;
	}
	notification_time_s.timezone = _get_timezone();
	notification_time_s.timeformat = _get_timeformat();
	_set_timezone(notification_time_s.timezone);

	_D("[%d][%s][%s]", notification_time_s.timeformat, notification_time_s.locale, notification_time_s.timezone);

	_remove_formatters();
	_set_formatters();
}



static void _register_settings(void)
{
	int ret = -1;

	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_TIME_CHANGED, _time_changed_cb, NULL);
	if (ret < 0) {
		_E("Failed to set time changed cb.(%d)", ret);
	}

	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, _time_changed_cb, NULL);
	if (ret < 0) {
		_E("Failed to set language setting's changed cb.(%d)", ret);
	}
	notification_time_s.locale = _get_locale();

	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEZONE, _time_changed_cb, NULL);
	if (ret < 0) {
		_E("Failed to set time zone change cb(%d)", ret);
	}
	notification_time_s.timezone = _get_timezone();

	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, _time_changed_cb, NULL);
	if (ret < 0) {
		_E("Failed to set time format about 24 hour changed cb(%d).", ret);
	}
	notification_time_s.timeformat = _get_timeformat();
}



static void _unregister_settings(void)
{
	int ret = -1;

	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_TIME_CHANGED);
	if (ret < 0) {
		_E("Failed to unset time changed(%d).", ret);
	}

	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE);
	if (ret < 0) {
		_E("Failed to unset locale language(%d).", ret);
	}

	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEZONE);
	if (ret < 0) {
		_E("Failed to unset time zone changed cb(%d).", ret);
	}

	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR);
	if (ret < 0) {
		_E("Failed to unset locale time format about 24 hour(%d).", ret);
	}
}



static int _get_date(time_t *intime, char *buf, int buf_len)
{
	i18n_udate u_time = (i18n_udate)(*intime) * 1000;
	i18n_uchar u_formatted_str[BUF_FORMATTER] = {0, };
	int32_t u_formatted_str_capacity;
	int32_t formatted_str_len = -1;
	int status = I18N_ERROR_INVALID_PARAMETER;

	u_formatted_str_capacity =
			(int32_t)(sizeof(u_formatted_str) / sizeof((u_formatted_str)[0]));

	status = i18n_udate_format_date(notification_time_s.formatter_date, u_time, u_formatted_str, u_formatted_str_capacity, NULL, &formatted_str_len);
	if (status != I18N_ERROR_NONE) {
		_E("udat_format() failed");
		return -1;
	}

	if (formatted_str_len <= 0) {
		_E("formatted_str_len is less than 0");
	}

	buf = i18n_ustring_copy_au_n(buf, u_formatted_str, (int32_t)buf_len);
	_SD("date:(%d)[%s][%d]", formatted_str_len, buf, *intime);

	return 0;
}



static int _get_ampm(time_t *intime, char *buf, int buf_len, int *ampm_len)
{
	i18n_udate u_time = (i18n_udate) (*intime) * 1000;
	i18n_uchar u_formatted_str[BUF_FORMATTER] = {0,};
	int32_t u_formatted_str_capacity;
	int32_t formatted_str_len = -1;
	int status = I18N_ERROR_INVALID_PARAMETER;

	retv_if(!notification_time_s.formatter_ampm, -1);

	/* calculate formatted string capacity */
	u_formatted_str_capacity =
			(int32_t)(sizeof(u_formatted_str) / sizeof((u_formatted_str)[0]));

	/* fomatting date using formatter */
	status = i18n_udate_format_date(notification_time_s.formatter_ampm, u_time, u_formatted_str, u_formatted_str_capacity, NULL, &formatted_str_len);
	if (status != I18N_ERROR_NONE) {
		_E("udat_format() failed");
		return -1;
	}

	if (formatted_str_len <= 0) {
		_E("formatted_str_len is less than 0");
	}


	(*ampm_len) = i18n_ustring_get_length(u_formatted_str);

	buf = i18n_ustring_copy_au_n(buf, u_formatted_str, (int32_t)buf_len);
	_SD("ampm:(%d)[%s][%d]", formatted_str_len, buf, *intime);

	return 0;
}



static int _get_time(time_t *intime, char *buf, int buf_len, Eina_Bool is_time_24)
{
	i18n_udate u_time = (i18n_udate) (*intime) * 1000;
	i18n_uchar u_formatted_str[BUF_FORMATTER] = {0,};
	int32_t u_formatted_str_capacity;
	int32_t formatted_str_len = -1;
	int status = I18N_ERROR_INVALID_PARAMETER;

	/* calculate formatted string capacity */
	u_formatted_str_capacity =
			(int32_t)(sizeof(u_formatted_str) / sizeof((u_formatted_str)[0]));

	/* fomatting date using formatter */
	if (is_time_24) {
		retv_if(notification_time_s.formatter_time_24 == NULL, -1);
		status = i18n_udate_format_date(notification_time_s.formatter_time_24, u_time, u_formatted_str, u_formatted_str_capacity, NULL, &formatted_str_len);
	} else {
		retv_if(notification_time_s.formatter_time == NULL, -1);
		status = i18n_udate_format_date(notification_time_s.formatter_time, u_time, u_formatted_str, u_formatted_str_capacity, NULL, &formatted_str_len);
	}
	if (status != I18N_ERROR_NONE) {
		_E("udat_format() failed");
		return -1;
	}

	if (formatted_str_len <= 0)
		_E("formatted_str_len is less than 0");

	buf = i18n_ustring_copy_au_n(buf, u_formatted_str, (int32_t)buf_len);
	_SD("time:(%d)[%s][%d]", formatted_str_len, buf, *intime);

	return 0;
}



w_home_error_e notification_time_to_string(time_t *time, char *string, int len)
{
	char utc_date[BUFSZE] = {0, };
	char utc_time[BUFSZE] = {0, };
	char utc_ampm[BUFSZE] = {0, };
	char *time_str = NULL;

	int ampm_length = 0;
	Eina_Bool is_24hour = EINA_FALSE;

	retv_if(!string, W_HOME_ERROR_FAIL);

	_get_date(time, utc_date, sizeof(utc_date));
	if (notification_time_s.timeformat) {
		_get_time(time, utc_time, sizeof(utc_time), EINA_TRUE);
		is_24hour = EINA_TRUE;
	} else {
		_get_ampm(time, utc_ampm, sizeof(utc_ampm), &ampm_length);
		_get_time(time, utc_time, sizeof(utc_time), EINA_FALSE);
		is_24hour = EINA_FALSE;
	}

	_D("utc_time=%s, utc_ampm=[%d]%s", utc_time, ampm_length, utc_ampm);

	if (is_24hour == EINA_TRUE){
		time_str = g_strdup(utc_time);
	} else {
		if (notification_time_s.is_pre == EINA_TRUE) {
			time_str = g_strdup_printf("%s %s", utc_ampm, utc_time);
		} else {
			time_str = g_strdup_printf("%s %s", utc_time, utc_ampm);
		}
	}
	_D("time_str=%s", time_str);

	strncpy(string, time_str, len);
	string[len-1] = '\0';
	g_free(time_str);

	return W_HOME_ERROR_NONE;
}



void notification_time_init(void)
{
	_register_settings();
	_set_formatters();
}



void notification_time_fini(void)
{
	_remove_formatters();
	_unregister_settings();
}



