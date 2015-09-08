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

#include <app_control.h>
#include <widget_service.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <Elementary.h>

#include "log.h"
#include "util.h"
#include "db.h"
#include "page.h"
#include "widget.h"
#include "wms.h"
#include "conf.h"
#include "layout_info.h"
#include "xml.h"
#include "page_info.h"
#include "scroller_info.h"
#include "scroller.h"
#include "apps/apps_main.h"
#include "apps/xml.h"
#include "main.h"



void wms_change_apps_order(int value)
{
	switch(value)
	{
		case W_HOME_WMS_BACKUP:
			_D("Backup");
			apps_main_list_backup();

			//Have to set the DONE, 3, to notify that backup is completed.
			vconf_set_int(VCONF_KEY_WMS_APPS_ORDER, W_HOME_WMS_DONE);

			break;
		case W_HOME_WMS_RESOTRE:
			_D("Restore");
			wms_unregister_setup_wizard_vconf();
			apps_main_list_restore();
			break;
		case W_HOME_WMS_DONE:
			_D("done");
			//No need to do anything
			break;
		default:
			_E("Invalid value:[%d]", value);
			break;
	}
}



#define FAVORITES_ORDER_XML_PATH DATADIR"/favorites_order.xml"
void wms_change_favorite_order(int value)
{
	Evas_Object *layout = NULL;
	layout_info_s *layout_info = NULL;
	Eina_List *page_info_list = NULL;

	layout = evas_object_data_get(main_get_info()->win, DATA_KEY_LAYOUT);
	ret_if(!layout);

	layout_info = evas_object_data_get(layout, DATA_KEY_LAYOUT_INFO);
	ret_if(!layout_info);

	switch(value)
	{
		case W_HOME_WMS_BACKUP:
			_D("Favorite Backup");

			page_info_list = scroller_write_list(layout_info->scroller);
			if (page_info_list) {
				xml_read_list(FAVORITES_ORDER_XML_PATH, page_info_list);
			}

			//Have to set the DONE, 3, to notify that backup is completed.
			vconf_set_int(VCONF_KEY_WMS_FAVORITES_ORDER, W_HOME_WMS_DONE);

			break;
		case W_HOME_WMS_RESOTRE:
			_D("Favorite Restore");
			wms_unregister_setup_wizard_vconf();

			page_info_list = xml_write_list(FAVORITES_ORDER_XML_PATH);
			if (page_info_list) {
				scroller_read_favorites_list(layout_info->scroller, page_info_list);
			}
			break;
		case W_HOME_WMS_DONE:
			_D("done");
			//No need to do anything
			break;
		default:
			_E("Invalid value:[%d]", value);
			break;
	}
}


static void _favorites_list_reset()
{
	Eina_List *page_info_list = NULL;
	Eina_List *tmp_list = NULL;
	const Eina_List *l, *ln;
	page_info_s *page_info = NULL;
	char *pkgname = NULL;

	page_info_list = db_write_list();
	ret_if(!page_info_list);

	EINA_LIST_FOREACH_SAFE(page_info_list, l, ln, page_info) {
		continue_if(!page_info);
		continue_if(!page_info->id);

		pkgname = widget_service_get_widget_id(page_info->id);
		if (pkgname) {
			tmp_list = eina_list_append(tmp_list, page_info);
			free(pkgname);
		} else {
			_D("%s is not installed in the pkgmgr DB", page_info->id);
		}
	}

	xml_read_list(FAVORITES_ORDER_XML_PATH, tmp_list);
}



static void _order_reset()
{
	_D("order reset");

	//Home
	_favorites_list_reset();
	vconf_set_int(VCONF_KEY_WMS_FAVORITES_ORDER, W_HOME_WMS_DONE);

	//apps
	apps_main_list_reset();
	vconf_set_int(VCONF_KEY_WMS_APPS_ORDER, W_HOME_WMS_DONE);
}


static void _change_setup_wizard_cb(keynode_t *node, void *data)
{
	int value = -1;

	if(vconf_get_bool(VCONFKEY_SETUP_WIZARD_FIRST_BOOT,  &value) < 0) {
		_E("Failed to get VCONFKEY_WMS_FAVORITES_ORDER");
		return;
	}

	_SD("value is changed: [%d]", value);

	if(value == 0) {
		if (vconf_ignore_key_changed(VCONFKEY_SETUP_WIZARD_FIRST_BOOT, _change_setup_wizard_cb) < 0) {
			_E("Failed to ignore the setup wizard firstboot callback");
		}

		_order_reset();
	}
}


void wms_register_setup_wizard_vconf()
{
	main_s *main_info = main_get_info();
	ret_if(!main_info);

	int value = -1;

	vconf_get_bool(VCONFKEY_SETUP_WIZARD_FIRST_BOOT, &value);

	_SD("first boot: [%d]", value);

	if(value == 1) {
		if (vconf_notify_key_changed(VCONFKEY_SETUP_WIZARD_FIRST_BOOT, _change_setup_wizard_cb, NULL) < 0) {
			_E("Failed to register the setup wizard firstboot callback");
		} else {
			main_info->setup_wizard = 1;
		}
	}
}


void wms_unregister_setup_wizard_vconf()
{
	main_s *main_info = main_get_info();
	ret_if(!main_info);

	_SD("first boot cb: [%d]", main_info->setup_wizard);

	if(main_info->setup_wizard) {
		if (vconf_ignore_key_changed(VCONFKEY_SETUP_WIZARD_FIRST_BOOT, _change_setup_wizard_cb) < 0) {
			_E("Failed to ignore the setup wizard firstboot callback");
		}
		main_info->setup_wizard = 0;
	}
}


static void _launch_gear_manager_response_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	Evas_Object *parent = (Evas_Object*)user_data;
	ret_if(!parent);

	_APPS_D("result: %d", result);

	if (result == APP_CONTROL_RESULT_SUCCEEDED) {
		util_create_toast_popup(parent, _("IDS_HS_TPOP_DOWNLOAD_GEAR_APPLICATIONS_USING_MOBILE_DEVICE_ABB"));
	}
	else {
		_APPS_E("Operation is failed: %d", result);
	}
}

#define W_MANAGER_SERVICE_APP_ID "org.tizen.w-manager-service"
void wms_launch_gear_manager(Evas_Object *parent, char *link)
{
	char *type = "gear";
	app_control_h service = NULL;

	ret_if(!link);
	ret_if(APP_CONTROL_ERROR_NONE != app_control_create(&service));
	ret_if(!service);

	app_control_set_operation(service, APP_CONTROL_OPERATION_DEFAULT);
	app_control_set_app_id(service, W_MANAGER_SERVICE_APP_ID);
	app_control_add_extra_data(service, "type", type);
	app_control_add_extra_data(service, "deeplink", link);

	int ret = app_control_send_launch_request(service, _launch_gear_manager_response_cb, parent);
	if (APP_CONTROL_ERROR_NONE != ret) {
		_APPS_E("launch fail:[%d]", ret);
	}

	app_control_destroy(service);
}


// End of a file
