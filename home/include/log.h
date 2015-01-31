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

#ifndef __W_HOME_LOG_H__
#define __W_HOME_LOG_H__

#include <unistd.h>
#include <dlog.h>

#undef LOG_TAG
#define LOG_TAG "W_HOME"

#if !defined(_D)
#define _D(fmt, arg...) LOGD(fmt"\n", ##arg)
#endif

#if !defined(_W)
#define _W(fmt, arg...) LOGW(fmt"\n", ##arg)
#endif

#if !defined(_E)
#define _E(fmt, arg...) LOGE(fmt"\n", ##arg)
#endif

#if !defined(_SD)
#define _SD(fmt, arg...) SECURE_LOGD(fmt"\n", ##arg)
#endif

#if !defined(_SW)
#define _SW(fmt, arg...) SECURE_LOGW(fmt"\n", ##arg)
#endif

#if !defined(_SE)
#define _SE(fmt, arg...) SECURE_LOGE(fmt"\n", ##arg)
#endif

#if !defined(_T)
#define _T(package) SECURE_LOG(LOG_DEBUG, "LAUNCH", "[%s:Menuscreen:launch:done]", package);
#endif

/* Apps Debug message */
#define LOG_TAG_APPS "APPS"

#if !defined(_APPS_D)
#define _APPS_D(fmt, arg...)  LOG(LOG_DEBUG, LOG_TAG_APPS, " "fmt, ##arg)
#endif

#if !defined(_APPS_W)
#define _APPS_W(fmt, arg...) LOG(LOG_WARN, LOG_TAG_APPS, " "fmt, ##arg)
#endif

#if !defined(_APPS_E)
#define _APPS_E(fmt, arg...)  LOG(LOG_ERROR, LOG_TAG_APPS, " "fmt, ##arg)
#endif

#if !defined(_APPS_SD)
#define _APPS_SD(fmt, arg...)  SECURE_LOG(LOG_DEBUG, LOG_TAG_APPS, " "fmt, ##arg)
#endif

#if !defined(_APPS_SW)
#define _APPS_SW(fmt, arg...) SECURE_LOG(LOG_WARN, LOG_TAG_APPS, " "fmt, ##arg)
#endif

#if !defined(_APPS_SE)
#define _APPS_SE(fmt, arg...)  SECURE_LOG(LOG_ERROR, LOG_TAG_APPS, " "fmt, ##arg)
#endif

#ifdef ADD_FILE_LOG
#define _F(fmt, arg...) do { \
	FILE *fp;\
	fp = fopen("/opt/usr/apps/org.tizen.w-home/data/logs", "a+");\
	if (NULL == fp) break;\
	fprintf(fp, "[%s:%d] "fmt"\n", __func__, __LINE__, ##arg); \
	fclose(fp);\
} while (0)
#else
#define _F(fmt, arg...) ;
#endif

#define retvm_if_timer(timer, expr, val, fmt, arg...) do { \
	if (expr) { \
		_E(fmt, ##arg); \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		timer = NULL; \
		return (val); \
	} \
} while (0)

#define retvm_if(expr, val, fmt, arg...) do { \
	if(expr) { \
		_E(fmt, ##arg); \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		return val; \
	} \
} while (0)

#define retv_if(expr, val) do { \
	if(expr) { \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		return (val); \
	} \
} while (0)

#define retm_if(expr, fmt, arg...) do { \
	if(expr) { \
		_E(fmt, ##arg); \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		return; \
	} \
} while (0)

#define ret_if(expr) do { \
	if(expr) { \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		return; \
	} \
} while (0)

#define goto_if(expr, val) do { \
	if(expr) { \
		_E("(%s) -> goto", #expr); \
		goto val; \
	} \
} while (0)

#define break_if(expr) { \
	if(expr) { \
		_E("(%s) -> break", #expr); \
		break; \
	} \
}

#define continue_if(expr) { \
	if(expr) { \
		_E("(%s) -> continue", #expr); \
		continue; \
	} \
}

#endif				/* __W_HOME_LOG_H__ */
