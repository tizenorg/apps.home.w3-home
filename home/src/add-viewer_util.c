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

#include <stdio.h>
#include <unistd.h>

#include <Elementary.h>
#include <dlog.h>
#include <device/display.h>
#include <appsvc.h>
#include <widget_service.h>
#include <app.h>
#if defined(USE_APP_MANAGER)
#include <app_manager.h>
#endif

#include <efl_assist.h>

#include "add-viewer_debug.h"
#include "add-viewer_package.h"
#include "add-viewer_util.h"
#include "add-viewer_ucol.h"

#if defined(LOG_TAG)
#undef LOG_TAG
#endif
#define LOG_TAG "ADD_VIEWER"

#define MATCH_COLOR "#ee00eeff"

static struct {
	char *setup;
	struct {
		int r;
		int g;
		int b;
		int a;
	} matched_color;
} s_info = {
	.setup = NULL,
	.matched_color = {
		.r = 0xee,
		.g = 0x00,
		.b = 0xee,
		.a = 0xff,
	},
};

struct result_data {
	struct add_viewer_package *package;
	int size;
};

extern int appsvc_allow_transient_app(bundle *b, Ecore_X_Window id);

#if defined(USE_APP_MANAGER)
static void app_ctx_cb(app_context_h app_context, app_context_event_e event, void *user_data)
{
	char *pkgname = NULL;

	if (app_context_get_app_id(app_context, &pkgname) != APP_MANAGER_ERROR_NONE) {
		ErrPrint("Failed to get pkgname\n");
		return;
	}

	if (!pkgname) {
		ErrPrint("Failed to get package name\n");
		return;
	}

	switch (event) {
	case APP_CONTEXT_EVENT_LAUNCHED:
		break;
        case APP_CONTEXT_EVENT_TERMINATED:
		/*!
		 * \note
		 * This must be called later than response callback
		 */
		if (s_info.setup && !strcmp(pkgname, s_info.setup)) {
			ErrPrint("Setup app is terminated [%s]\n", s_info.setup);
			free(s_info.setup);
			s_info.setup = NULL;
			elm_exit();
		}
		break;
	default:
		break;
	}

	free(pkgname);
	return;
}
#else
#include <aul.h>

extern int aul_listen_app_dead_signal(int (*func)(int signal, void *data), void *data);

int _dead_cb(int pid, void *data)
{
	char pkgname[256];

	if (aul_app_get_pkgname_bypid(pid, pkgname, sizeof(pkgname)) != AUL_R_OK) {
		ErrPrint("Failed to get pkgname for %d\n", pid);
		return 0;
	}

	if (s_info.setup && !strcmp(pkgname, s_info.setup)) {
		ErrPrint("Setup app is terminated [%s]\n", s_info.setup);
		free(s_info.setup);
		s_info.setup = NULL;
		elm_exit();
	}

	return 0;
}
#endif

HAPI int add_viewer_util_init(void)
{
#if defined(USE_APP_MANAGER)
	app_manager_set_app_context_event_cb(app_ctx_cb, NULL);
#else
	aul_listen_app_dead_signal(_dead_cb, NULL);
#endif
	return 0;
}

HAPI int add_viewer_util_fini(void)
{
#if defined(USE_APP_MANAGER)
	app_manager_unset_app_context_event_cb();
#endif
	return 0;
}

static void response_callback(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
        struct result_data *res_data = user_data;
        char *content_info = NULL;

	if (s_info.setup) {
		free(s_info.setup);
		s_info.setup = NULL;
	}

	if (result != APP_CONTROL_RESULT_SUCCEEDED) {
		ErrPrint("Operation is canceled: %d\n", result);
		elm_exit();
		return;
	}

        app_control_get_extra_data(reply, EXTRA_KEY_CONTENT_INFO , &content_info);

	/**
	 * \TODO
	 * Use this content_info to add a new widget
	 */

        free(res_data);

	/**
	 * If the box is successfully added, do we need to terminate this?
	 */
}

HAPI int add_viewer_util_add_to_home(struct add_viewer_package *package, int size, int use_noti)
{
	int ret = -EINVAL;
	char *setup;

	setup = widget_service_get_app_id_of_setup_app(add_viewer_package_list_pkgname(package));
	if (setup) {
		app_control_h service;
		struct result_data *res_data;

		if (s_info.setup) {
			ErrPrint("Setup is already launched: %s\n", s_info.setup);
			free(s_info.setup);
			s_info.setup = NULL;
		}

		DbgPrint("Setup App: %s\n", setup);

		res_data = malloc(sizeof(*res_data));
		if (!res_data) {
			free(setup);
			return -ENOMEM;
		}

		res_data->package = package;
		res_data->size = size;

		ret = app_control_create(&service);
		if (ret != APP_CONTROL_ERROR_NONE) {
			free(setup);
			free(res_data);
			return -EFAULT;
		}

		ret = app_control_set_app_id(service, setup);
		if (ret != APP_CONTROL_ERROR_NONE) {
			free(setup);
			free(res_data);
			app_control_destroy(service);
			return -EFAULT;
		}

		ret = app_control_set_operation(service, SERVICE_OPERATION_WIDGET_SETUP);
		if (ret != APP_CONTROL_ERROR_NONE) {
			free(setup);
			free(res_data);
			app_control_destroy(service);
			return -EFAULT;
		}

		ret = app_control_send_launch_request(service, response_callback, res_data);

		app_control_destroy(service);

		if (ret != APP_CONTROL_ERROR_NONE) {
			free(res_data);
			free(setup);
		} else {
			s_info.setup = setup;
		}
	} else {
		/**
		 * \TODO
		 * Create a new widget without content_info
		 */
	}

	return ret;
}

HAPI int add_viewer_util_is_lcd_off(void)
{
	display_state_e state;

	if (device_display_get_state(&state) != 0) {
		ErrPrint("Idle lock state is not valid\n");
		state = DISPLAY_STATE_NORMAL; /* UNLOCK */
	}

	return state == DISPLAY_STATE_SCREEN_OFF;
}

HAPI char *add_viewer_util_highlight_keyword(const char *name, const char *filter)
{
	char *highlighted_name;
	int filter_len;
	int name_len;
	int target_idx = 0;
	int tag_len;
	int ret_len;
	const int amp_len = strlen("&amp;");
	const char *org_name = name;

	if (!name) {
		ErrPrint("Cannot get name");
		return NULL;
	}

	if (!filter) {
		ErrPrint("Cannot get filter");
		return NULL;
	}

	filter_len = strlen(filter);
	name_len = strlen(name);
	tag_len = strlen("<font color="MATCH_COLOR"></font>");
	ret_len = name_len + 1;

	highlighted_name = malloc(ret_len);
	if (!highlighted_name) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return NULL;
	}

	while (*name) {
		if (*name == '&') {
			char *tmp;

			ret_len += amp_len;
			tmp = realloc(highlighted_name, ret_len);
			if (!tmp) {
				ErrPrint("Heap: Fail to realloc - %d \n", errno);
				free(highlighted_name);
				return NULL;
			}
			highlighted_name = tmp;

			/*!
			 * Manipulate tagging
			 */
			strncpy(highlighted_name + target_idx, "&amp;",  amp_len);
			target_idx += amp_len;
			name++;
		} else if ((name_len - (int)(name - org_name)) < filter_len) {
			int idx_len;

			idx_len = name_len - (int)(name - org_name);

			strncpy(highlighted_name + target_idx, name, idx_len);
			target_idx += idx_len;
			name += idx_len;
		} else if (!add_viewer_ucol_case_ncompare(name, filter, filter_len)) {
			char *tmp;

			ret_len += tag_len;
			tmp = realloc(highlighted_name, ret_len);
			if (!tmp) {
				ErrPrint("Heap: %s\n", strerror(errno));
				free(highlighted_name);
				return NULL;
			}
			highlighted_name = tmp;

			/*!
			 * Manipulate tagging
			 */
			tmp = malloc(filter_len + 1);
			if (!tmp) {
				ErrPrint("Heap: Fail to malloc - %d \n", errno);
				free(highlighted_name);
				return NULL;
			}
			strncpy(tmp, name, filter_len); /* Keep original string */
			tmp[filter_len] = '\0';

			target_idx += snprintf(highlighted_name + target_idx, ret_len - target_idx,
					"<font color=#%02x%02x%02x%02x>%s</font>",
					s_info.matched_color.r,
					s_info.matched_color.g,
					s_info.matched_color.b,
					s_info.matched_color.a,
					tmp
			);
			free(tmp);
			name += filter_len;
		} else {
			int idx_len;

			idx_len = add_viewer_util_get_utf8_len(*name);
			strncpy(highlighted_name + target_idx, name, idx_len);
			target_idx += idx_len;
			name += idx_len;
		}
	}

	highlighted_name[target_idx] = '\0';
	return highlighted_name;
}

HAPI int add_viewer_util_get_utf8_len(char ch)
{
	int idx_len;
	if ((ch & 0x80) == 0x00) {
		idx_len = 1;
	} else if ((ch & 0xE0) == 0xC0) {
		idx_len = 2;
	} else if ((ch & 0xF0) == 0xE0) {
		idx_len = 3;
	} else if ((ch & 0xFC) == 0xF8) {
		idx_len = 4;
	} else if ((ch & 0xFE) == 0xFC) {
		idx_len = 5;
	} else if ((ch & 0xFF) == 0xFE) {
		idx_len = 6;
	} else {
		idx_len = 1;
	}

	return idx_len;
}

/* End of a file */
