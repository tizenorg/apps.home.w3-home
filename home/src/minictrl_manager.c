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
#include <aul.h>
#include <efl_assist.h>
#include <minicontrol-viewer.h>
#include <minicontrol-monitor.h>
#include <dlog.h>

#include "conf.h"
#include "layout.h"
#include "page_info.h"
#include "page.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "minictrl.h"

static struct _info {
	Eina_List *item_list;
} s_info = {
	.item_list = NULL,
};

/*!
 * functions to handle linked-list for MC object
 */
static Minictrl_Entry *_minictrl_entry_new(const char *name, const char *service_name, int category, Evas_Object *view)
{
	Minictrl_Entry *entry = NULL;
	retv_if(view == NULL, NULL);
	retv_if(name == NULL, NULL);
	retv_if(service_name == NULL, NULL);

	entry = (Minictrl_Entry *)calloc(1, sizeof(Minictrl_Entry));
	if (entry != NULL) {
		entry->category = category;
		entry->name = strdup(name);
		entry->service_name = strdup(service_name);
		entry->view = view;
	} else {
		_E("failed to allocate memory");
	}

	return entry;
}

static void _minictrl_entry_del(Minictrl_Entry *item)
{
	free(item->name);
	free(item->service_name);
	free(item);
}

static Minictrl_Entry *_minictrl_entry_get_by_category(int category)
{
	Minictrl_Entry *entry = NULL;

	if (category != MC_CATEGORY_CLOCK &&
		category != MC_CATEGORY_NOTIFICATION &&
		category != MC_CATEGORY_DASHBOARD) {
		return NULL;
	}

	Eina_List *l = NULL;
	EINA_LIST_FOREACH(s_info.item_list, l, entry) {
		if (entry != NULL) {
			if (entry->category == category) {
				return entry;
			}
		}
	}

	_E("Failed to find entry with category:%d", category);

	return NULL;
}

static Minictrl_Entry *_minictrl_entry_get_by_name(const char *name)
{
	Minictrl_Entry *entry = NULL;
	retv_if(name == NULL, NULL);

	Eina_List *l = NULL;
	EINA_LIST_FOREACH(s_info.item_list, l, entry) {
		if (entry != NULL && entry->name != NULL) {
			if (strcmp(entry->name, name) == 0) {
				return entry;
			}
		}
	}

	_E("Failed to find entry with name:%s", name);

	return NULL;
}

static int _minictrl_entry_del_by_name(const char *name)
{
	int ret = 0;

	Minictrl_Entry *entry = _minictrl_entry_get_by_name(name);
	if (entry != NULL) {
		s_info.item_list = eina_list_remove(s_info.item_list, entry);
		_minictrl_entry_del(entry);
		ret = 1;
	}

	return ret;
}

static int _minictrl_entry_del_by_category(int category)
{
	int ret = 0;

	Minictrl_Entry *entry = _minictrl_entry_get_by_category(category);
	if (entry != NULL) {
		s_info.item_list = eina_list_remove(s_info.item_list, entry);
		_minictrl_entry_del(entry);
		ret = 1;
	}

	return ret;
}

HAPI void minictrl_manager_entry_add_with_data(const char *name, const char *service_name, int category, Evas_Object *view)
{
	/*!
	 * below category can't have multiple instance
	 */
	switch (category) {
		case MC_CATEGORY_CLOCK:
		case MC_CATEGORY_DASHBOARD:
		if (_minictrl_entry_del_by_category(category) == 1) {
			_E("Exceptional, Why multiple instance exist?");
		}
		break;
	}

	Minictrl_Entry *entry = _minictrl_entry_new(name, service_name, category, view);
	if (entry != NULL) {
		s_info.item_list = eina_list_append(s_info.item_list, entry);
	}
}

HAPI Evas_Object *minictrl_manager_view_get_by_name(const char *name)
{
	retv_if(name == NULL, NULL);

	Minictrl_Entry *entry = _minictrl_entry_get_by_name(name);
	if (entry != NULL) {
		return entry->view;
	}

	return NULL;
}

HAPI Evas_Object *minictrl_manager_view_get_by_category(int category)
{
	Minictrl_Entry *entry = _minictrl_entry_get_by_category(category);
	if (entry != NULL) {
		return entry->view;
	}

	return NULL;
}

HAPI Minictrl_Entry *minictrl_manager_entry_get_by_category(int category)
{
	return _minictrl_entry_get_by_category(category);
}

HAPI Minictrl_Entry *minictrl_manager_entry_get_by_name(const char *name)
{
	return _minictrl_entry_get_by_name(name);
}

HAPI int minictrl_manager_entry_del_by_category(int category)
{
	return _minictrl_entry_del_by_category(category);
}

HAPI int minictrl_manager_entry_del_by_name(const char *name)
{
	return _minictrl_entry_del_by_name(name);
}

HAPI void minictrl_manager_entry_foreach(Minictrl_Entry_Foreach_Cb cb, void *user_data)
{
	Minictrl_Entry *entry = NULL;
	ret_if(cb == NULL);

	Eina_List *l = NULL;
	EINA_LIST_FOREACH(s_info.item_list, l, entry) {
		if (entry != NULL) {
			cb(entry, user_data);
		}
	}
}
