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

#include <Elementary.h>
#include <stdbool.h>
#include <efl_assist.h>
#include <dlog.h>
#include <bundle.h>

#include "conf.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "mapbuf.h"

#define PRIVATE_DATA_KEY_MAPBUF_ENABLED "p_mb_en"


HAPI Evas_Object *mapbuf_get_mapbuf(Evas_Object *obj)
{
	Evas_Object *mapbuf;

	if (obj == NULL) return NULL;

	mapbuf = evas_object_data_get(obj, DATA_KEY_MAPBUF);
	if (!mapbuf && evas_object_data_get(obj, DATA_KEY_PAGE)) {
		mapbuf = obj;
	}

	return mapbuf;
}



HAPI Evas_Object *mapbuf_get_page(Evas_Object *obj)
{
	Evas_Object *page;

	retv_if(NULL == obj, NULL);

	page = evas_object_data_get(obj, DATA_KEY_PAGE);
	if (!page && evas_object_data_get(obj, DATA_KEY_MAPBUF)) {
		page = obj;
	}

	return page;
}



HAPI void mapbuf_set_color(Evas_Object *obj, int a)
{
	Evas_Object *mapbuf = mapbuf_get_mapbuf(obj);
	ret_if(NULL == mapbuf);

	int i;
	for (i = 0; i < 4; i ++) {
		elm_mapbuf_point_color_set(mapbuf, i, a, a, a, a);
	}
}



static void _move_pages(Evas_Object *page)
{
	ret_if(NULL == page);

	Evas_Object *scroller = evas_object_data_get(page, DATA_KEY_SCROLLER);
	if (NULL == scroller) return;

	Evas_Coord x, y;
	evas_object_geometry_get(scroller, &x, &y, NULL, NULL);
	evas_object_move(page, x, y);
}



HAPI w_home_error_e mapbuf_enable(Evas_Object *obj, int force)
{
	Evas_Object *mapbuf = mapbuf_get_mapbuf(obj);
	retv_if(NULL == mapbuf, W_HOME_ERROR_FAIL);

	Evas_Object *page = mapbuf_get_page(obj);
	retv_if(NULL == page, W_HOME_ERROR_FAIL);

	if (force) {
		evas_object_data_set(mapbuf, PRIVATE_DATA_KEY_MAPBUF_ENABLED, (void*)0);
		_move_pages(page);
		elm_mapbuf_enabled_set(mapbuf, 1);
		return W_HOME_ERROR_NONE;
	}

	int cnt = (int)evas_object_data_get(mapbuf, PRIVATE_DATA_KEY_MAPBUF_ENABLED);
	cnt ++;
	evas_object_data_set(mapbuf, PRIVATE_DATA_KEY_MAPBUF_ENABLED, (void*)cnt);

	if (cnt == 0) {
		if (!elm_mapbuf_enabled_get(mapbuf)) {
			_move_pages(page);
			elm_mapbuf_enabled_set(mapbuf, 1);
		}
	}

	return W_HOME_ERROR_NONE;
}



HAPI int mapbuf_is_enabled(Evas_Object *obj)
{
	Evas_Object *mapbuf;
	mapbuf = mapbuf_get_mapbuf(obj);
	if (!mapbuf) {
		return 0;
	}

	return elm_mapbuf_enabled_get(mapbuf);
}



HAPI int mapbuf_disable(Evas_Object *obj, int force)
{
	Evas_Object *mapbuf;
	int cnt;

	mapbuf = mapbuf_get_mapbuf(obj);
	if (!mapbuf) {
		_D("Failed to get the mapbuf object");
		return W_HOME_ERROR_FAIL;
	}

	if (force) {
		evas_object_data_set(mapbuf, PRIVATE_DATA_KEY_MAPBUF_ENABLED, (void*)-1);
		elm_mapbuf_enabled_set(mapbuf, 0);
		return W_HOME_ERROR_NONE;
	}

	cnt = (int)evas_object_data_get(mapbuf, PRIVATE_DATA_KEY_MAPBUF_ENABLED);
	if (cnt == 0) {
		if (elm_mapbuf_enabled_get(mapbuf)) {
			elm_mapbuf_enabled_set(mapbuf, 0);
		}
	}

	cnt --;
	evas_object_data_set(mapbuf, PRIVATE_DATA_KEY_MAPBUF_ENABLED, (void*)cnt);

	return W_HOME_ERROR_NONE;
}



HAPI Evas_Object *mapbuf_bind(Evas_Object *box, Evas_Object *page)
{
	Evas_Object *mapbuf;

	mapbuf = elm_mapbuf_add(box);
	if (!mapbuf) {
		_E("Failed to create a new mapbuf");
		return NULL;
	}

	elm_mapbuf_smooth_set(mapbuf, EINA_TRUE);
	elm_mapbuf_alpha_set(mapbuf, EINA_TRUE);
	elm_object_content_set(mapbuf, page);

	evas_object_data_set(page, DATA_KEY_MAPBUF, mapbuf);
	evas_object_data_set(mapbuf, DATA_KEY_PAGE, page);

	evas_object_show(mapbuf);

	return mapbuf;
}



HAPI Evas_Object *mapbuf_unbind(Evas_Object *obj)
{
	Evas_Object *page;
	Evas_Object *mapbuf;

	page = evas_object_data_get(obj, DATA_KEY_PAGE);
	if (page) {
		mapbuf = obj;
	} else {
		page = obj;
		mapbuf = evas_object_data_get(obj, DATA_KEY_MAPBUF);
	}

	if (mapbuf) {
		elm_mapbuf_enabled_set(mapbuf, 0);
		evas_object_data_del(page, DATA_KEY_MAPBUF);
		evas_object_data_del(mapbuf, DATA_KEY_PAGE);
		evas_object_data_del(mapbuf, PRIVATE_DATA_KEY_MAPBUF_ENABLED);
		page = elm_object_content_unset(mapbuf);
		evas_object_del(mapbuf);
	}
	return page;
}



HAPI int mapbuf_can_be_made(Evas_Object *obj)
{
	if (!main_get_info()->is_mapbuf) return 0;

	Evas_Object *page = evas_object_data_get(obj, DATA_KEY_PAGE);
	if (!page) page = obj;
	retv_if(NULL == page, 0);

	bool mapbuf_disabled_page = (bool) evas_object_data_get(page, DATA_KEY_MAPBUF_DISABLED_PAGE);
	if (mapbuf_disabled_page) return 0;

	return 1;
}



HAPI int mapbuf_can_be_on(Evas_Object *obj)
{
	if (!main_get_info()->is_mapbuf) return 0;

	Evas_Object *page = evas_object_data_get(obj, DATA_KEY_PAGE);
	if (!page) page = obj;
	retv_if(NULL == page, 0);

	bool mapbuf_disabled_page = (bool) evas_object_data_get(page, DATA_KEY_MAPBUF_DISABLED_PAGE);
	if (mapbuf_disabled_page) return 0;

	return 1;
}



// End of a file
