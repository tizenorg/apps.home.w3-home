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

#if !defined(SECURE_LOGD)
#define SECURE_LOGD LOGD
#endif

#if !defined(SECURE_LOGW)
#define SECURE_LOGW LOGW
#endif

#if !defined(SECURE_LOGE)
#define SECURE_LOGE LOGE
#endif

#if !defined(S_)
#define S_(str) dgettext("sys_string", str)
#endif

#if !defined(T_)
#define T_(str) dgettext(PACKAGE, str)
#endif

#if !defined(N_)
#define N_(str) (str)
#endif

#if !defined(_)
#define _(str) gettext(str)
#endif


#if !defined(DbgPrint)
#define DbgPrint(format, arg...)	SECURE_LOGD(format, ##arg)
#endif

#if !defined(ErrPrint)
#define ErrPrint(format, arg...)	SECURE_LOGE(format, ##arg)
#endif

#if !defined(WarnPrint)
#define WarnPrint(format, arg...)	SECURE_LOGW(format, ##arg)
#endif

#define HAPI __attribute__((visibility("hidden")))

#if !defined(SERVICE_OPERATION_DBOX_SETUP)
#define SERVICE_OPERATION_DBOX_SETUP "http://tizen.org/appcontrol/operation/appwidget/configuration"
#endif

#if !defined(SERVICE_OPERATION_SAMSUNG_DBOX_SETUP_MULTIPLE)
#define SERVICE_OPERATION_SAMSUNG_DBOX_SETUP_MULTIPLE "http://samsung.com/appcontrol/operation/appwidget/configuration_multiple"
#endif

#if !defined(EXTRA_KEY_PROVIDER_NAME)
#define EXTRA_KEY_PROVIDER_NAME "http://tizen.org/appcontrol/data/provider_name"
#endif

#if !defined(EXTRA_KEY_CONTENT_INFO)
#define EXTRA_KEY_CONTENT_INFO "http://tizen.org/appcontrol/data/user_info"
#endif

#if !defined(EXTRA_KEY_CONTENT_INFO_LIST)
#define EXTRA_KEY_CONTENT_INFO_LIST "http://samsung.com/appcontrol/data/user_info_list"
#endif

#if !defined(EXTRA_KEY_CONTENT_INFO_COUNT)
#define EXTRA_KEY_CONTENT_INFO_COUNT "http://samsung.com/appcontrol/data/max_items"
#endif

#define EDJE_FILE RESDIR"/edje/preview.edj"
#define UNKNOWN_ICON RESDIR"/images/unknown.png"

/* End of a file */
