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

#include <aul.h>
#include <app.h>
#include <appsvc.h>
#include <bundle.h>
#include <efl_assist.h>
#include <Elementary.h>
#include <Evas.h>
#include <dlog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vconf.h>

#include "conf.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "popup.h"



#define APPID_WC_POPUP "org.tizen.weconn-popup"
#define WC_POPUP_EXTRA_DATA_KEY	"http://samsung.com/appcontrol/data/connection_type"
static void _popup_weconn_gear_to_home(void)
{
	app_control_h service = NULL;
	ret_if(APP_CONTROL_ERROR_NONE != app_control_create(&service));
	ret_if(NULL == service);

	app_control_add_extra_data(service, WC_POPUP_EXTRA_DATA_KEY, "mobile_data_noti");

	app_control_set_app_id(service, APPID_WC_POPUP);
	int ret = app_control_send_launch_request(service, NULL, NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		_E("Failed to launch:%d", ret);
	}

	app_control_destroy(service);
}

static void _popup_after_tutorial(void)
{
	_W("popup:misc");
	_popup_weconn_gear_to_home();
}

void popup_show(int stage)
{
	if (stage == POPUP_STAGE_AFTER_TUTORIAL) {
		_popup_after_tutorial();
	}
}
