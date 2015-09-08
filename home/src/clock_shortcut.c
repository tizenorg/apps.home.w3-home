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
#include <stdbool.h>
#include <stdio.h>
#include <bundle.h>
#include <aul.h>
#include <vconf.h>
#include <efl_assist.h>
#include <dlog.h>
#include <message_port.h>
#include <pkgmgr-info.h>
#include <app_preference.h>
#include <app.h>

#include "conf.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "effect.h"
#include "clock_service.h"
#include "page_info.h"
#include "page.h"

#define MUSIC_PLAYER_APPID "org.tizen.w-music-player"
#define MUSIC_PLAYER_VCONF_STATE "memory/private/org.tizen.w-music-player/player_state"
#define CLOCK_SHORTCUT_EDJ_FILE EDJEDIR"/clock_shortcut.edj"
#define CLOCK_SHORTCUT_DATA_KEY_APPID "c_s_d_i"
#define CLOCK_SHORTCUT_MESSAGE_PORT_ID "Home.Clock.Shortcut.Music"
#define CLOCK_SHORTCUT_MESSAGE_KEY_STATE "state"
#define CLOCK_SHORTCUT_MESSAGE_KEY_ICON "icon"
#define CLOCK_SHORTCUT_MESSAGE_STATE_SHOW "show"
#define CLOCK_SHORTCUT_MESSAGE_STATE_HIDE "hide"

typedef struct {
	int type;
	char *appid;
	char *icon;
} shortcut_info_s;

static struct {
	int local_port_id;
	Eina_List *list;
} s_info = {
	.local_port_id = -1,
	.list = NULL,
};

static Evas_Object *_attached_clock_view_get(void)
{
	clock_h clock = clock_manager_clock_get(CLOCK_ATTACHED);
	retv_if(clock == NULL, NULL);

	return clock->view;
}

static Evas_Object *_shortcut_layout_get(Evas_Object *page_a) {
	Evas_Object *page = (page_a == NULL) ? _attached_clock_view_get() : page_a;
	retv_if(page == NULL, NULL);

	Evas_Object *item = page_get_item(page);
	if (item != NULL) {
		return elm_object_part_content_get(item, "shortcut");
	}

	return NULL;
}

static void _info_free(shortcut_info_s *info)
{
	ret_if(info == NULL);

	free(info->appid);
	free(info->icon);
}

static shortcut_info_s *_list_lastest_info_get(void)
{
	retv_if(s_info.list == NULL, NULL);

	return eina_list_nth(s_info.list, 0);
}

static shortcut_info_s * _list_info_get(const char *appid)
{
	Eina_List *l = NULL;
	shortcut_info_s *info = NULL;
	retv_if(appid == NULL, NULL);

	EINA_LIST_FOREACH(s_info.list, l, info) {
		if (info != NULL) {
			if (info->appid != NULL) {
				if (strcmp(info->appid, appid) == 0) {
					return info;
				}
			}
		}
	}

	return NULL;
}

static void _list_info_add(shortcut_info_s *info)
{
	s_info.list = eina_list_prepend(s_info.list, info);
}

static void _list_info_del(shortcut_info_s *info)
{
	s_info.list = eina_list_remove(s_info.list, info);
	_info_free(info);
}

static void _list_clean(void)
{
	shortcut_info_s *info = NULL;

	EINA_LIST_FREE(s_info.list, info) {
		if (info != NULL) {
			_info_free(info);
		}
	}
}

static char *_get_app_name(const char *appid)
{
	pkgmgrinfo_appinfo_h appinfo_h = NULL;
	char *tmp = NULL;
	char *name = NULL;

	retv_if(0 > pkgmgrinfo_appinfo_get_appinfo(appid, &appinfo_h), NULL);
	goto_if(PMINFO_R_OK != pkgmgrinfo_appinfo_get_label(appinfo_h, &tmp), ERROR);

	if (tmp) {
		name = strdup(tmp);
		goto_if(NULL == name, ERROR);
	}

	pkgmgrinfo_appinfo_destroy_appinfo(appinfo_h);
	return name;

ERROR:
	pkgmgrinfo_appinfo_destroy_appinfo(appinfo_h);
	return NULL;
}

/*!
 * preload music player state
 */
static int _mp_state_get(void)
{
	int val = -1;

	if(vconf_get_int(MUSIC_PLAYER_VCONF_STATE, &val) < 0) {
		_E("Failed to get MUSIC_PLAYER_VCONF_STATE");
		return 0;
	}
	retv_if(val != 1, 0);

	retv_if(aul_app_is_running(MUSIC_PLAYER_APPID) <= 0, 0);

	return 1;
}

/*!
 * shortcut icon
 */
static char *_shortcut_icon_owner_get(Evas_Object *shortcut_layout)
{
	char *owner_appid = NULL;
	Evas_Object *icon = NULL;
	retv_if(shortcut_layout == NULL, NULL);

	icon = elm_object_part_content_get(shortcut_layout, "shortcut,icon");
	retv_if(icon == NULL, NULL);

	owner_appid = (char *)evas_object_data_get(icon, CLOCK_SHORTCUT_DATA_KEY_APPID);

	return owner_appid;
}

static int _shortcut_icon_is_owner_same(Evas_Object *shortcut_layout, const char *appid)
{
	Evas_Object *icon = NULL;
	retv_if(shortcut_layout == NULL, 0);
	retv_if(appid == NULL, 0);

	icon = elm_object_part_content_get(shortcut_layout, "shortcut,icon");
	retv_if(icon == NULL, 0);

	char *owner_appid = (char *)evas_object_data_get(icon, CLOCK_SHORTCUT_DATA_KEY_APPID);
	if (owner_appid != NULL) {
		if (strcmp(owner_appid, appid) == 0) {
			return 1;
		}
	}

	return 0;
}

static void _shortcut_icon_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	char *owner_appid = (char *)evas_object_data_del(obj, CLOCK_SHORTCUT_DATA_KEY_APPID);
	free(owner_appid);
}

static Evas_Object *_shortcut_icon_create(Evas_Object *parent, const char *file_a)
{
	_D("");
	const char *file = NULL;;
	Evas_Object *icon = NULL;

	if (file_a != NULL) {
		if (access(file_a, R_OK) != 0) {
			_E("Failed to access an icon(%s)", file_a);
		} else {
			file = file_a;
		}
	}

	if (file == NULL) {
		icon = elm_layout_add(parent);
		retv_if(icon == NULL, NULL);

		if (elm_layout_file_set(icon, CLOCK_SHORTCUT_EDJ_FILE, "shortcut_default_icon") != EINA_TRUE) {
			_E("failed to set default icon file");
		}
		evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(icon);
	} else {
		icon = elm_image_add(parent);
		retv_if(icon == NULL, NULL);

		elm_image_resizable_set(icon, EINA_TRUE, EINA_TRUE);
		if (elm_image_file_set(icon, file, NULL) == EINA_FALSE) {
			_E("Failed to set image file:%s", file);
		}
		evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(icon);
	}

	return icon;
}

/*!
 * shortcut layout
 */
static void _shortcut_layout_icon_set(Evas_Object *shortcut_layout, Evas_Object *icon)
{
	Evas_Object *tmp = NULL;
	ret_if(shortcut_layout == NULL);
	ret_if(icon == NULL);

	tmp = elm_object_part_content_unset(shortcut_layout, "shortcut,icon");
	if (tmp != NULL) {
		evas_object_del(tmp);
	}

	elm_object_part_content_set(shortcut_layout, "shortcut,icon", icon);
}

#define MUSIC_STATUS_PLAY "play"
#define MUSIC_STATUS_STOP "stop"
static void _shortcut_layout_vconf_set(int is_visible)
{
	char *appid = NULL;
	char value[BUFSZE] = { 0, };

	Evas_Object *shortcut_layout = _shortcut_layout_get(NULL);
	ret_if(shortcut_layout == NULL);

	if ((appid = _shortcut_icon_owner_get(shortcut_layout)) == NULL) {
		_E("Failed to get appid");
	}

	if (is_visible) {
		snprintf(value, sizeof(value), "%s;%s", appid, MUSIC_STATUS_PLAY);
	} else {
		snprintf(value, sizeof(value), "%s;%s", appid, MUSIC_STATUS_STOP);
	}

	if (preference_set_string(VCONFKEY_MUSIC_STATUS, value) != 0) {
		_E("Failed to set the vconfkey, %s", value);
	}
}

static void _shortcut_layout_visible_set(Evas_Object *shortcut_layout, int is_visible)
{
	if (is_visible == 1) {
		elm_object_signal_emit(shortcut_layout, "icon,show", "prog");
		_D("clock shortcut become visible");
	} else {
		elm_object_signal_emit(shortcut_layout, "icon,hide", "prog");
		_D("clock shortcut become invisible");
	}
}

static void _shortcut_layout_clicked_cb(void *cbdata, Evas_Object *obj, void *event_info)
{
	int pid = 0;
	int app_type = APP_TYPE_NATIVE;
	bundle *b = NULL;
	Evas_Object *shortcut_layout = cbdata;
	ret_if(shortcut_layout == NULL);

	Evas_Object *icon = elm_object_part_content_get(shortcut_layout, "shortcut,icon");
	ret_if(icon == NULL);

	char *owner_appid = (char *)evas_object_data_get(icon, CLOCK_SHORTCUT_DATA_KEY_APPID);
	ret_if(owner_appid == NULL);

	app_type = util_get_app_type(owner_appid);

	if (app_type == APP_TYPE_NATIVE) {
		b = bundle_create();
		if (b != NULL) {
			bundle_add(b, "__APP_SVC_OP_TYPE__", APP_CONTROL_OPERATION_MAIN);

			pid = aul_launch_app(owner_appid, b);
			_D("aul_launch_app: %s(%d)", owner_appid, pid);

			bundle_free(b);
		}
	} else if (app_type == APP_TYPE_WEB) {
		pid = aul_open_app(owner_appid);
	}

	if (pid < 0) {
		_E("Failed to launch %s(%d)", owner_appid, pid);
	}

	effect_play_sound();
}

static char *_access_info_cb(void *data, Evas_Object *obj)
{
	Evas_Object *shortcut_layout = data;
	retv_if(shortcut_layout == NULL, NULL);

	Evas_Object *icon = elm_object_part_content_get(shortcut_layout, "shortcut,icon");
	retv_if(icon == NULL, NULL);

	char *owner_appid = (char *)evas_object_data_get(icon, CLOCK_SHORTCUT_DATA_KEY_APPID);
	retv_if(owner_appid == NULL, NULL);

	char *name = _get_app_name(owner_appid);
	if (name == NULL) {
		return strdup(owner_appid);
	}

	return name;
}

static void _shortcut_layout_create(Evas_Object *clock_layout)
{
	_D("");
	Evas_Object *shortcut_layout = NULL;
	ret_if(clock_layout == NULL);

	shortcut_layout = elm_layout_add(clock_layout);
	ret_if(shortcut_layout == NULL);

	if (elm_layout_file_set(shortcut_layout, CLOCK_SHORTCUT_EDJ_FILE, "shortcut") == EINA_TRUE) {
		evas_object_size_hint_weight_set(clock_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_object_part_content_set(clock_layout, "shortcut", shortcut_layout);

		Evas_Object *focus = elm_button_add(shortcut_layout);
		if (focus != NULL) {
			elm_object_style_set(focus, "focus");
			elm_access_info_cb_set(focus, ELM_ACCESS_INFO, _access_info_cb, shortcut_layout);
			evas_object_smart_callback_add(focus, "clicked",
				_shortcut_layout_clicked_cb, shortcut_layout);
			elm_object_part_content_set(shortcut_layout, "focus", focus);
		}
		evas_object_show(clock_layout);
	} else {
		_E("Failed to add shortcut layout");
	}
}

static void _shortcut_layout_destroy(Evas_Object *clock_layout)
{
	ret_if(clock_layout == NULL);

	Evas_Object *obj = elm_object_part_content_unset(clock_layout, "shortcut");
	ret_if(obj == NULL);

	evas_object_del(obj);
}

static Evas_Object *_shortcut_add(Evas_Object *page, shortcut_info_s *info)
{
	retv_if(info == NULL, NULL);
	retv_if(info->appid == NULL, NULL);
	Evas_Object *shortcut_layout = _shortcut_layout_get(page);
	retv_if(shortcut_layout == NULL, NULL);

	Evas_Object *icon = _shortcut_icon_create(shortcut_layout, info->icon);
	if (icon != NULL) {
		evas_object_data_set(icon, CLOCK_SHORTCUT_DATA_KEY_APPID, strdup(info->appid));
		evas_object_event_callback_add(icon, EVAS_CALLBACK_DEL,
			_shortcut_icon_del_cb, shortcut_layout);
		_shortcut_layout_icon_set(shortcut_layout, icon);
		_shortcut_layout_vconf_set(1);
		_shortcut_layout_visible_set(shortcut_layout, 1);

		return icon;
	}

	return NULL;
}

static void _clock_shortcut_message_received_cb(int local_port_id, const char *remote_app_id, const char *remote_port, bool trusted_port, bundle* msg, void *user_data)
{
	const char *state = NULL;
	const char *icon_path = NULL;
	ret_if(remote_app_id == NULL);
	ret_if(msg == NULL);
	Evas_Object *shortcut_layout = _shortcut_layout_get(NULL);
	ret_if(shortcut_layout == NULL);

	state = bundle_get_val(msg, CLOCK_SHORTCUT_MESSAGE_KEY_STATE);
	ret_if(state == NULL);
	icon_path = bundle_get_val(msg, CLOCK_SHORTCUT_MESSAGE_KEY_ICON);

	_W("%s %s %s", remote_app_id, state, icon_path);

	shortcut_info_s *info = NULL;

	if (strcmp(state, CLOCK_SHORTCUT_MESSAGE_STATE_SHOW) == 0) {
		info = (shortcut_info_s *)calloc(1, sizeof(shortcut_info_s));
		ret_if(!info);
		info->appid = strdup(remote_app_id);
		goto_if(!info->appid, ERROR);
		if (icon_path != NULL) {
			info->icon = strdup(icon_path);
			goto_if(!info->icon, ERROR);
		}
		if (_list_info_get(remote_app_id) == NULL) {
			_list_info_add(info);
			_shortcut_add(NULL, _list_lastest_info_get());
		} else {
			goto ERROR;
		}
	} else {
		if (_shortcut_icon_is_owner_same(shortcut_layout, remote_app_id) == 1) {
			_shortcut_layout_vconf_set(0);
			_shortcut_layout_visible_set(shortcut_layout, 0);
		}
		_list_info_del(_list_info_get(remote_app_id));
	}
	return;

ERROR:
	if (info->icon) free(info->icon);
	if (info->appid) free(info->appid);
	if (info) free(info);
}

static void _mp_state_vconf_changed_cb(keynode_t *node, void *data)
{
	Evas_Object *shortcut_layout = _shortcut_layout_get(data);
	ret_if(shortcut_layout == NULL);

	shortcut_info_s *info = NULL;

	if (_mp_state_get() == 1) {
		info = (shortcut_info_s *)calloc(1, sizeof(shortcut_info_s));
		ret_if(!info);
		info->appid = strdup(MUSIC_PLAYER_APPID);
		goto_if(!info->appid, ERROR);
		info->icon = NULL;
		if (_list_info_get(MUSIC_PLAYER_APPID) == NULL) {
			_list_info_add(info);
			_shortcut_add(data, _list_lastest_info_get());
		} else {
			goto ERROR;
		}
	} else {
		if (_shortcut_icon_is_owner_same(shortcut_layout, MUSIC_PLAYER_APPID) == 1) {
			_shortcut_layout_vconf_set(0);
			_shortcut_layout_visible_set(shortcut_layout, 0);
		}
		_list_info_del(_list_info_get(MUSIC_PLAYER_APPID));
	}
	return;

ERROR:
	if (info->appid) free(info->appid);
	if (info) free(info);
	return;
}

HAPI void clock_shortcut_view_add(Evas_Object *page)
{
	ret_if(page == NULL);

	Evas_Object *clock_layout = page_get_item(page);

	_shortcut_layout_destroy(clock_layout);
	_shortcut_layout_create(clock_layout);

	shortcut_info_s *info = NULL;
	if (_mp_state_get() == 1) {
		info = (shortcut_info_s *)calloc(1, sizeof(shortcut_info_s));
		ret_if (!info);
		info->appid = strdup(MUSIC_PLAYER_APPID);
		goto_if(!info->appid, ERROR);
		info->icon = NULL;
		if (_list_info_get(MUSIC_PLAYER_APPID) == NULL) {
			_list_info_add(info);
		} else {
			goto ERROR;
		}
	}

	info = _list_lastest_info_get();
	if (info != NULL) {
		_shortcut_add(page, info);
	}
	return;

ERROR:
	if (info->appid) free(info->appid);
	if (info) free(info);
	return;
}

HAPI void clock_shortcut_init(void)
{
	int port_id = 0;

	if(vconf_notify_key_changed(MUSIC_PLAYER_VCONF_STATE, _mp_state_vconf_changed_cb, NULL) < 0) {
		_E("Failed to register the music player state vconf cb");
	}

	if (s_info.local_port_id < 0) {
		if ((port_id = message_port_register_local_port(CLOCK_SHORTCUT_MESSAGE_PORT_ID,
			_clock_shortcut_message_received_cb, NULL)) <= 0) {
			_E("Failed to register clock shortcut message port cb");
		}
		_E("port_id:%d", port_id);
		s_info.local_port_id = port_id;
	}
}

HAPI void clock_shortcut_fini(void)
{
	if(vconf_notify_key_changed(MUSIC_PLAYER_VCONF_STATE, _mp_state_vconf_changed_cb, NULL) < 0) {
		_E("Failed to unregister the music player state vconf cb");
	}

	if (s_info.local_port_id >= 0) {
		if (message_port_unregister_local_port(s_info.local_port_id) != MESSAGE_PORT_ERROR_NONE) {
			_E("Failed to unregister clock shortcut message port cb");
		}
	}

	_list_clean();
}

HAPI void clock_shortcut_app_dead_cb(int pid)
{
	char *appid = NULL;
	Evas_Object *shortcut_layout = _shortcut_layout_get(NULL);
	ret_if(shortcut_layout == NULL);

	if ((appid = _shortcut_icon_owner_get(shortcut_layout)) != NULL) {
		if (aul_app_is_running(appid) <= 0) {
			_E("%s is dead, hiding shortcut", appid);
			_shortcut_layout_vconf_set(0);
			_shortcut_layout_visible_set(shortcut_layout, 0);
			_list_info_del(_list_info_get(appid));
		}
	}
}
