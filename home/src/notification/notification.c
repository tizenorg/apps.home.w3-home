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
#include <notification.h>
#include <notification_internal.h>

#include "util.h"
#include "log.h"
#include "main.h"
#include "page_info.h"
#include "page.h"
#include "scroller_info.h"
#include "scroller.h"
#include "notification/detail.h"
#include "notification/notification.h"
#include "notification/simple.h"
#include "notification/summary.h"
#include "notification/time.h"

#define PRIAVTE_DATA_KEY_PKGNAME "pdkpn"



static void _remove_detail_item_event_cb(void *event_info, void *data)
{
	detail_item_s *detail_item_info = event_info;
	Evas_Object *scroller = data;
	Evas_Object *page = NULL;
	Evas_Object *focused_page = NULL;
	int count = 0;

	ret_if(!detail_item_info);
	ret_if(!detail_item_info->pkgname);

	_D("Receive an event to remove an item");

	focused_page = scroller_get_focused_page(scroller);
	if (!focused_page)
		_E("There is no focused page");

	page = summary_get_page(scroller, detail_item_info->pkgname);
	ret_if(!page);

	count = detail_list_count_pkgname(detail_item_info->pkgname);
	if (count > 0) {
		detail_item_s *latest_info = NULL;
		Evas_Object *item = NULL;

		latest_info = detail_list_get_latest_info(detail_item_info->pkgname);
		ret_if(!latest_info);

		item = page_get_item(page);
		summary_destroy_item(item);

		_D("Refresh the summary item");

		item = summary_create_item(page
				, latest_info->pkgname
				, latest_info->icon
				, latest_info->title
				, latest_info->content
				, count
				, &latest_info->time);
		ret_if(!item);
		page_set_item(page, item);
	} else {
		_D("Remove the summary page");
		summary_destroy_page(page);
	}

	scroller_bring_in_page(scroller, focused_page, SCROLLER_FREEZE_OFF, SCROLLER_BRING_TYPE_ANIMATOR);
}



/*
 * typedef enum _notification_op_data_type {
 * NOTIFICATION_OP_DATA_MIN = 0,   // Default
 * NOTIFICATION_OP_DATA_TYPE,  // Operation type
 * NOTIFICATION_OP_DATA_PRIV_ID,   // Private ID
 * NOTIFICATION_OP_DATA_NOTI,  // Notification handler
 * NOTIFICATION_OP_DATA_EXTRA_INFO_1,  // Reserved
 * NOTIFICATION_OP_DATA_EXTRA_INFO_2,  // Reserved
 * NOTIFICATION_OP_DATA_MAX,   // Max flag
 * } notification_op_data_type_e;
 */
/*
 * typedef enum _notification_op_type {
 * NOTIFICATION_OP_NONE = 0,	// Default
 * NOTIFICATION_OP_INSERT = 1,	// Notification inserted
 * NOTIFICATION_OP_UPDATE,	// Notification updated
 * NOTIFICATION_OP_DELETE,	// Notification deleted
 * NOTIFICATION_OP_DELETE_ALL,	// Notifications deleted
 * NOTIFICATION_OP_REFRESH,	// Deprecated
 * NOTIFICATION_OP_SERVICE_READY,	// Notification service is ready
 * } notification_op_type_e;
*/
static void _detailed_changed_cb(void *data, notification_type_e type, notification_op *op_list, int num_op)
{
	Evas_Object *win = NULL;
	Evas_Object *scroller = data;
	Evas_Object *page = NULL;

	detail_item_s *detail_item_info = NULL;
	notification_h noti = NULL;

	char *domain = NULL;
	char *dir = NULL;
	char *pkgname = NULL;
	char *icon = NULL;
	char *title = NULL;
	char *content = NULL;

	int op_type = 0;
	int applist = NOTIFICATION_DISPLAY_APP_ALL;
	int i = 0;
	int count = 0;
	int flags = 0;
	int priv_id = 0;
	time_t time;

	ret_if(!scroller);
	ret_if(!op_list);
	ret_if(num_op < 1);

	for (; i < num_op; i++) {
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_PRIV_ID, &priv_id);
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_TYPE, &op_type);
		notification_op_get_data(op_list + i, NOTIFICATION_OP_DATA_NOTI, &noti);

		/* FIXME : This API will be deprecated on Tizen 2.3 */
		notification_get_text_domain(noti, &domain, &dir);
		if (!domain && !dir) {
			bindtextdomain(domain, dir);
		}

		notification_get_pkgname(noti, &pkgname);
		notification_get_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, &icon);
		notification_get_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, &title);
		notification_get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, &content);
		notification_get_time_from_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, &time);

		_D("Notification operation type : %d", op_type);

		switch (op_type) {
		case NOTIFICATION_OP_INSERT:
		case NOTIFICATION_OP_UPDATE:
			detail_item_info = detail_list_append_info(priv_id, pkgname, icon, title, content, time);
			break_if(!detail_item_info);

			count = detail_list_count_pkgname(pkgname);
			if (count > 1) {
				page = summary_get_page(scroller, pkgname);
				if (page) summary_destroy_page(page);
			}
			page = summary_create_page(scroller, pkgname, icon, title, content, count, &time);
			ret_if(!page);
			scroller_push_page(scroller, page, SCROLLER_PUSH_TYPE_CENTER_LEFT);

			if (main_get_info()->state == APP_STATE_PAUSE) {
				win = simple_create_win(icon, title);
				if (!win) {
					_E("cannot create a notification window");
				}
			}
			break;
		case NOTIFICATION_OP_DELETE:
			detail_item_info = detail_list_remove_list(priv_id);
			break_if(!detail_item_info);
			_remove_detail_item_event_cb(detail_item_info, scroller);
			detail_list_remove_info(detail_item_info->priv_id);
			break;
		case NOTIFICATION_OP_DELETE_ALL:
			_E("We don't support NOTIFICATION_OP_DELETE_ALL");
			break;
		default:
			_D("default case");
			break;
		}

		notification_get_display_applist(noti, &applist);
		if (applist & NOTIFICATION_DISPLAY_APP_TICKER) {
			_D("App ticker is displayed");
		}

		notification_get_property(noti, &flags);
		if (flags & NOTIFICATION_PROP_DISABLE_TICKERNOTI) {
			_D("Disable ticker notifications");
		}
	}

	return;
}



HAPI int notification_init(Evas_Object *scroller)
{
	int ret = NOTIFICATION_ERROR_NONE;

	retv_if(!scroller, W_HOME_ERROR_INVALID_PARAMETER);

	ret = notification_register_detailed_changed_cb(_detailed_changed_cb, scroller);
	retv_if(NOTIFICATION_ERROR_NONE != ret, W_HOME_ERROR_FAIL);

	notification_time_init();
	detail_register_event_cb(DETAIL_EVENT_REMOVE_ITEM, _remove_detail_item_event_cb, scroller);

	return W_HOME_ERROR_NONE;
}



HAPI void notification_fini(Evas_Object *scroller)
{
	int ret = NOTIFICATION_ERROR_NONE;

	detail_unregister_event_cb(DETAIL_EVENT_REMOVE_ITEM, _remove_detail_item_event_cb);
	notification_time_fini();

	ret = notification_unregister_detailed_changed_cb(_detailed_changed_cb, scroller);
	ret_if(NOTIFICATION_ERROR_NONE != ret);
}
