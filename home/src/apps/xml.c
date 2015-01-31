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

#include <aul.h>
#include <Eina.h>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "log.h"
#include "util.h"
#include "apps/item_info.h"
#include "apps/xml.h"

const char *APPS_XML_ENCODING = "utf-8";
const char *ITEM_TYPE_IDLE_CLOCK_STR = "idle-clock";
const char *ITEM_TYPE_FAVORITE_STR = "favorite";
const char *ITEM_TYPE_APP_WIDGET_STR = "appwidget";
const char *ITEM_TYPE_MORE_APPS_STR = "apps";
const int APPS_DBOX_PER_PAGE = 2;

static void _get_position_by_order(int ordering, int *row, int *cell)
{
	ret_if(!row);
	ret_if(!cell);

	*row = ordering / 2;

	switch(ordering % 2) {
	case 0:
		*cell = 0;
		break;
	case 1:
		*cell = 1;
		break;
/*
	case 2:
		*cell = 2;
		break;
	case 3:
		*cell = 3;
		break;
*/
	default:
		_APPS_E("Failed to get cellx & celly");
		*cell = -1;
		break;
	}
}



static xmlTextWriterPtr _init_writer(const char *xml_file)
{
	xmlTextWriterPtr writer;
	int ret = 0;

	writer = xmlNewTextWriterFilename(xml_file, 0);
	retv_if(!writer, NULL);

	ret = xmlTextWriterStartDocument(writer, NULL, APPS_XML_ENCODING, NULL);
	if (ret < 0) {
		_APPS_E("Failed to start xmlTextWriterStartDocument");
		xmlFreeTextWriter(writer);
		return NULL;
	}

	ret = xmlTextWriterStartElement(writer, BAD_CAST "data");
	if (ret < 0) {
		_APPS_E("Failed to start xmlTextWriterStartElement");
		xmlFreeTextWriter(writer);
		return NULL;
	}

	return writer;
}



static void _destroy_writer(xmlTextWriterPtr writer)
{
	int ret = 0;

	ret = xmlTextWriterEndElement(writer);
	goto_if(ret < 0, END);

	ret = xmlTextWriterEndDocument(writer);
	if (ret < 0) {
		_APPS_E("Cannot end the document");
	}

END:
	xmlFreeTextWriter(writer);
}



HAPI apps_error_e apps_xml_read_list(const char *xml_file, Eina_List *item_info_list)
{
	xmlTextWriterPtr writer;
	Eina_List *l = NULL;
	item_info_s *info = NULL;
	int ret = 0;
	int row = -1;
	int cell = -1;

	retv_if(!xml_file, APPS_ERROR_INVALID_PARAMETER);
	retv_if(!item_info_list, APPS_ERROR_INVALID_PARAMETER);

	writer = _init_writer(xml_file);
	retv_if(!writer, APPS_ERROR_FAIL);

	EINA_LIST_FOREACH(item_info_list, l, info) {
		continue_if(!info);

		_get_position_by_order(info->ordering, &row, &cell);

		ret = xmlTextWriterStartElement(writer, BAD_CAST ITEM_TYPE_MORE_APPS_STR);
		goto_if(ret < 0, END);

		ret = xmlTextWriterWriteFormatElement(writer, BAD_CAST "packageName", "%s", info->pkgid);
		goto_if(ret < 0, END);

		ret = xmlTextWriterWriteFormatElement(writer, BAD_CAST "className", "%s", info->appid);
		goto_if(ret < 0, END);

		ret = xmlTextWriterWriteFormatElement(writer, BAD_CAST "screen", "%d", row);
		goto_if(ret < 0, END);

		ret = xmlTextWriterWriteFormatElement(writer, BAD_CAST "cell", "%d", cell);
		goto_if(ret < 0, END);

		ret = xmlTextWriterEndElement(writer);
		goto_if(ret < 0, END);
	}

END:
	_destroy_writer(writer);

	return APPS_ERROR_NONE;
}



static xmlTextReaderPtr _init_reader(const char *xml_file)
{
	xmlTextReaderPtr reader = NULL;

	retv_if(!xml_file, NULL);

	reader = xmlReaderForFile(xml_file, NULL, 0);
	retv_if(!reader, NULL);

	return reader;
}



static void _destroy_reader(xmlTextReaderPtr reader)
{
	ret_if(!reader);
	xmlFreeTextReader(reader);
}



static inline void _destroy_list(Eina_List *list)
{
	item_info_s *info = NULL;

	if (!list) return;

	EINA_LIST_FREE(list, info) {
		continue_if(!info);
		apps_item_info_destroy(info);
	}
}



HAPI Eina_List *apps_xml_write_list(const char *xml_file)
{
	xmlTextReaderPtr reader = NULL;
	Eina_List *list = NULL;
	item_info_s *info = NULL;
	const char *name, *value;
	char *element = NULL;
	int ret = -1;

	retv_if(!xml_file, NULL);

	reader = _init_reader(xml_file);
	retv_if(!reader, NULL);

	while ((ret = xmlTextReaderRead(reader)) == 1) {
		int dep = 0;
		int node_type = 0;
		static int cell = 0;
		static int page_no = 0;

		dep = xmlTextReaderDepth(reader);
		node_type = xmlTextReaderNodeType(reader);
		continue_if(-1 == dep || -1 == node_type);

		name = (const char*) xmlTextReaderConstName(reader);
		continue_if(!name);

		value = (const char*) xmlTextReaderConstValue(reader);
		/* value can be NULL in case of 'category' */

		// Category, element is 1
		if ((1 == dep) && (1 == node_type)) {
			if (info) {
				apps_item_info_destroy(info);
				info = NULL;
			}

			if (strcasecmp(ITEM_TYPE_MORE_APPS_STR, name)) {
				_APPS_E("Type(%s) is wrong. But, go through with it.", name);
			}
			continue;
		}

		// Element name
		if ((2 == dep) && (1 == node_type)) {
			if (element) {
				_APPS_E("The element name is NOT NULL");
				free(element);
				element = NULL;
			}

			element = strdup(name);
			goto_if(!element, CRITICAL_ERROR);
			continue;
		}

		// Element value, textfield is 3
		if ((3 == dep) && (3 == node_type)) {
			continue_if(!element);

			if (!strcasecmp("packageName", element)) {
				/* Do nothing */
			} else if (!strcasecmp("className", element)) {
				info = apps_item_info_create(value);
				if(!info) _E("info of [%s] is NULL", value);
			} else if(!strcasecmp("screen", element)) {
				page_no = atoi(value);
			} else if(!strcasecmp("cell", element)) {
				cell = atoi(value);
			}
			continue;
		}

		// End of the Element
		if ((2 == dep) && (15 == node_type)) {
			if (!element) {
				apps_item_info_destroy(info);
				info = NULL;
				continue;
			}

			free(element);
			element = NULL;
			continue;
		}

		// End of the category
		if ((1 == dep) && (15 == node_type)) { // End of the category
			if(info) {
				info->ordering = page_no * APPS_DBOX_PER_PAGE + cell;
				list = eina_list_append(list, info);
				_APPS_D("Append a package into the list : (%s:%s:%d) (%d:%d)"
					, info->pkgid, info->appid, info->ordering
					, page_no, cell);
			}

			info = NULL;
			page_no = 0;
			cell = 0;
		}
	}

	if(element) {
		_APPS_E("An element is not appended properly");
		free(element);
	}

	if(info) {
		_APPS_E("A node(%s:%s) is not appended into the list", info->pkgid, info->appid);
		apps_item_info_destroy(info);
	}
	goto_if(ret, CRITICAL_ERROR);

	_destroy_reader(reader);
	return list;

CRITICAL_ERROR:
	apps_item_info_destroy(info);
	_destroy_list(list);
	_destroy_reader(reader);
	return NULL;
}



// End of a file
