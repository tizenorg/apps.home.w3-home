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

#include <badge.h>
#include <Elementary.h>
#include <stdbool.h>
#include <unicode/unum.h>
#include <unicode/ustring.h>
#include <vconf.h>

#include "log.h"
#include "util.h"
#include "apps/scroller_info.h"
#include "apps/item_badge.h"
#include "apps/item_info.h"
#include "apps/apps_main.h"


#define PRIVATE_DATA_KEY_IDLE_TIMER "p_it"



HAPI int item_badge_count(Evas_Object *item)
{
	item_info_s *item_info = NULL;
	unsigned int is_display = 0;
	unsigned int count = 0;
	badge_error_e err = BADGE_ERROR_NONE;

	retv_if(!item, 0);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, 0);

	err = badge_get_display(item_info->appid, &is_display);
	if (BADGE_ERROR_NONE != err) _APPS_E("cannot get badge display");

	if (!is_display) return 0;

	err = badge_get_count(item_info->appid, &count);
	if (BADGE_ERROR_NONE != err) _APPS_E("cannot get badge count");

	_APPS_SD("Badge for app %s : %u", item_info->appid, count);

	return (int) count;
}



HAPI int item_badge_is_registered(const char *appid)
{
	bool existing = 0;
	badge_error_e err = BADGE_ERROR_NONE;

	retv_if(!appid, 0);

	err = badge_is_existing(appid, &existing);
	if (BADGE_ERROR_NONE != err) _APPS_E("cannot know whether the badge for %s is or not.", appid);

	return existing? 1 : 0;
}



static Eina_Bool _idler_cb(void *data)
{
	Evas_Object *item = data;
	int count;

	retv_if(!item, ECORE_CALLBACK_CANCEL);

	count = item_badge_count(item);
	if (count) item_badge_show(item, count);
	else item_badge_hide(item);

	evas_object_data_del(item, PRIVATE_DATA_KEY_IDLE_TIMER);

	return ECORE_CALLBACK_CANCEL;
}



HAPI apps_error_e item_badge_init(Evas_Object *item)
{
	Ecore_Idler *idle_timer = NULL;
	item_info_s *item_info = NULL;
	int is_registered;

	retv_if(!item, APPS_ERROR_INVALID_PARAMETER);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	retv_if(!item_info, APPS_ERROR_FAIL);

	is_registered = item_badge_is_registered(item_info->appid);
	if (!is_registered) {
		return APPS_ERROR_NONE;
	}

	idle_timer = evas_object_data_get(item, PRIVATE_DATA_KEY_IDLE_TIMER);
	if (idle_timer) {
		ecore_idler_del(idle_timer);
		evas_object_data_del(item, PRIVATE_DATA_KEY_IDLE_TIMER);
	}

	idle_timer = ecore_idler_add(_idler_cb, item);
	retv_if(NULL == idle_timer, APPS_ERROR_FAIL);
	evas_object_data_set(item, PRIVATE_DATA_KEY_IDLE_TIMER, idle_timer);

	return APPS_ERROR_NONE;
}



HAPI void item_badge_destroy(Evas_Object *item)
{
	Ecore_Idler *idle_timer = NULL;
	item_info_s *item_info = NULL;

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	idle_timer = evas_object_data_get(item, PRIVATE_DATA_KEY_IDLE_TIMER);
	if (idle_timer) {
		_APPS_D("Badge handler for app %s is not yet ready", item_info->appid);
		evas_object_data_del(item, PRIVATE_DATA_KEY_IDLE_TIMER);
		ecore_idler_del(idle_timer);
		return;
	}

	item_badge_hide(item);
}



static void _badge_change_cb(unsigned int action, const char *appid, unsigned int count, void *data)
{
	Evas_Object *layout = data;
	Evas_Object *item = NULL;
	unsigned int is_display = 0;
	badge_error_e err = BADGE_ERROR_NONE;

	_APPS_SD("Badge changed, action : %u, appid : %s, count : %u", action, appid, count);

	ret_if(!layout);
	ret_if(!appid);

	Evas_Object *scroller = evas_object_data_get(layout, DATA_KEY_SCROLLER);
	ret_if(!scroller);

	scroller_info_s *scroller_info = evas_object_data_get(scroller, DATA_KEY_SCROLLER_INFO);
	ret_if(!scroller_info);
	ret_if(!scroller_info->list);

	item = apps_item_info_list_get_item(scroller_info->list, appid);
	if (NULL == item) return;

	if (BADGE_ACTION_REMOVE == action) {
		count = 0;
		is_display = 0;
	} else {
		err = badge_get_display(appid, &is_display);
		if (BADGE_ERROR_NONE != err) _APPS_E("cannot get badge display");
		if (!is_display) count = 0;
	}

	if (count) item_badge_show(item, count);
	else item_badge_hide(item);
}



HAPI void item_badge_register_changed_cb(Evas_Object *layout)
{
	badge_error_e err;

	err = badge_register_changed_cb(_badge_change_cb, layout);
	ret_if(BADGE_ERROR_NONE != err);
}



HAPI void item_badge_unregister_changed_cb(void)
{
	badge_error_e err;

	err = badge_unregister_changed_cb(_badge_change_cb);
	ret_if(BADGE_ERROR_NONE != err);
}



static char *_get_count_str_from_icu(int count)
{
	char *p = NULL;
	char *locale = NULL;

	char *ret_str = NULL;
	UErrorCode status = U_ZERO_ERROR;
	uint32_t number = count;
	UNumberFormat *num_fmt;
	UChar result[20] = { 0, };
	char res[20] = { 0, };
	int32_t len = (int32_t) (sizeof(result) / sizeof((result)[0]));

	locale = vconf_get_str(VCONFKEY_REGIONFORMAT);
	retv_if(!locale, NULL);

	if(locale[0] != '\0') {
		p = strstr(locale, ".UTF-8");
		if (p) *p = 0;
	}

	num_fmt = unum_open(UNUM_DEFAULT, NULL, -1, locale, NULL, &status);
	unum_format(num_fmt, number, result, len, NULL, &status);
	u_austrcpy(res, result);
	unum_close(num_fmt);

	ret_str = strdup(res);
	free(locale);
	return ret_str;
}



#define BADGE_SIGNAL_LEN 16
#define MAX_BADGE_COUNT 999
HAPI void item_badge_show(Evas_Object *item, int count)
{
	item_info_s *item_info = NULL;
	char *str = NULL;
	char badge_signal[16];

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);

	if (count > MAX_BADGE_COUNT) count = MAX_BADGE_COUNT;

	str = _get_count_str_from_icu(count);
	elm_object_part_text_set(item_info->item_inner, "badge_txt", str);

	if (count <= 0) {
		snprintf(badge_signal, sizeof(badge_signal), "badge,off");
	} else if (count < 10) {
		snprintf(badge_signal, sizeof(badge_signal), "badge,on,1");
	} else if (count < 100) {
		snprintf(badge_signal, sizeof(badge_signal), "badge,on,2");
	} else {
		snprintf(badge_signal, sizeof(badge_signal), "badge,on,3");
	}
	elm_object_signal_emit(item_info->item_inner, badge_signal, "item");

	free(str);
}



HAPI void item_badge_hide(Evas_Object *item)
{
	item_info_s *item_info = NULL;

	ret_if(!item);

	item_info = evas_object_data_get(item, DATA_KEY_ITEM_INFO);
	ret_if(!item_info);
	elm_object_signal_emit(item_info->item_inner, "badge,off", "item");
}



HAPI void item_badge_change_language(Evas_Object *item)
{
	int count = 0;

	ret_if(!item);

	count = item_badge_count(item);
	if (count) item_badge_show(item, count);
	else item_badge_hide(item);
}



HAPI void item_badge_remove(const char *pkgid)
{
	ret_if(!pkgid);

	_APPS_SD("pkgid: [%s]", pkgid);

	badge_remove(pkgid);
}


// End of a file
