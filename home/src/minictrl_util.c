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

#include <Elementary.h>
#include <Evas.h>
#include <stdbool.h>
#include <glib.h>
#include <aul.h>
#include <vconf.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
#include <efl_assist.h>
#include <dlog.h>

#include "conf.h"
#include "layout.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "page_info.h"
#include "scroller_info.h"
#include "page.h"
#include "scroller.h"
#include "minictrl.h"

HAPI char *minicontrol_appid_get_by_pid(int pid)
{
	int ret = AUL_R_OK;
	char *dup_pkgname = NULL;
	char pkgname[512 + 1] = { 0, };

	ret = aul_app_get_pkgname_bypid(pid, pkgname, sizeof(pkgname));
	if (ret == AUL_R_OK) {
		if (strlen(pkgname) <= 0) {
			return NULL;
		}
	}

	dup_pkgname = strdup(pkgname);
	if (!dup_pkgname) {
		_E("Failed to dup string");
	}

	return dup_pkgname;
}

HAPI void minicontrol_magic_set(Evas_Object *obj) {
	if (obj != NULL) {
		evas_object_data_set(obj, MINICTRL_DATA_KEY_MAGIC, (void *)MINICONTROL_MAGIC);
	}
}

HAPI int minicontrol_magic_get(Evas_Object *obj) {
	if (obj != NULL) {
		return (int)evas_object_data_get(obj, MINICTRL_DATA_KEY_MAGIC);
	}

	return 0x0;
}

HAPI void minicontrol_name_set(Evas_Object *obj, const char *name) {
	char *name_old = NULL;

	name_old = evas_object_data_get(obj, MINICTRL_DATA_KEY_NAME);
	if (name_old != NULL) {
		evas_object_data_del(obj, MINICTRL_DATA_KEY_NAME);
		free(name_old);
	}

	if (obj != NULL && name != NULL) {
		evas_object_data_set(obj, MINICTRL_DATA_KEY_NAME, (void *)strdup(name));
	}
}

HAPI char *minicontrol_name_get(Evas_Object *obj) {
	if (obj != NULL) {
		return (char *)evas_object_data_get(obj, MINICTRL_DATA_KEY_NAME);
	}

	return NULL;
}

HAPI void minicontrol_category_set(Evas_Object *obj, int category) {
	if (obj != NULL) {
		evas_object_data_set(obj, MINICTRL_DATA_KEY_CATEGORY, (void *)category);
	}
}

HAPI int minicontrol_category_get(Evas_Object *obj) {
	if (obj != NULL) {
		return (int)evas_object_data_get(obj, MINICTRL_DATA_KEY_CATEGORY);
	}

	return 0;
}

HAPI void minicontrol_pid_set(Evas_Object *obj, int pid) {
	if (obj != NULL) {
		evas_object_data_set(obj, MINICTRL_DATA_KEY_PID, (void *)pid);
	}
}

HAPI int minicontrol_pid_get(Evas_Object *obj) {
	if (obj != NULL) {
		return (int)evas_object_data_get(obj, MINICTRL_DATA_KEY_PID);
	}

	return 0;
}

HAPI void minicontrol_visibility_set(Evas_Object *obj, int visibility) {
	if (obj != NULL) {
		evas_object_data_set(obj, MINICTRL_DATA_KEY_VISIBILITY, (void *)visibility);
	}
}

HAPI int minicontrol_visibility_get(Evas_Object *obj) {
	if (obj != NULL) {
		return (int)evas_object_data_get(obj, MINICTRL_DATA_KEY_VISIBILITY);
	}

	return 0;
}

HAPI int minicontrol_message_send(Evas_Object *minictrl, char *message)
{
	Ecore_Evas *ee = NULL;
	Evas_Object *plug_img = NULL;
	retv_if(message == NULL, 0);
	retv_if(minictrl == NULL, 0);

	plug_img = elm_plug_image_object_get(minictrl);
	retv_if(plug_img == NULL, 0);
	ee = ecore_evas_object_ecore_evas_get(plug_img);
	retv_if(ee == NULL, 0);
	ecore_evas_msg_parent_send(ee, 0, 0, message, strlen(message) + 1);

	return 1;
}
