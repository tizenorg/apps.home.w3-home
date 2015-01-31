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
#include <Evas.h>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <dlog.h>

#include "dynamicbox_evas.h" // DYNAMICBOX_EVAS_DEFAULT_PERIOD

#include "log.h"
#include "page_info.h"
#include "util.h"
#include "xml.h"

const char *XML_ENCODING = "utf-8";
const char *PAGE_CATEGORY_IDLE_CLOCK_STR = "idle-clock";
const char *PAGE_CATEGORY_FAVORITE_STR = "favorite";
const char *PAGE_CATEGORY_APP_WIDGET_STR = "appwidget";
const char *PAGE_CATEGORY_MORE_APPS_STR = "more-apps";
const int DBOX_PER_PAGE = 4;

static void _get_position_by_order(int ordering, int *row, int *cellx, int *celly)
{
	ret_if(!row);
	ret_if(!cellx);
	ret_if(!celly);

	*row = ordering / 4;

	switch(ordering % 4) {
	case 0:
		*cellx = 0;
		*celly = 0;
		break;
	case 1:
		*cellx = 1;
		*celly = 0;
		break;
	case 2:
		*cellx = 0;
		*celly = 1;
		break;
	case 3:
		*cellx = 1;
		*celly = 1;
		break;
	default:
		_E("Failed to get cellx & celly");
		*cellx = -1;
		*celly = -1;
		break;
	}
}



static xmlTextWriterPtr _init_writer(const char *xml_file)
{
	xmlTextWriterPtr writer;
	int ret = 0;

	writer = xmlNewTextWriterFilename(xml_file, 0);
	retv_if(!writer, NULL);

	ret = xmlTextWriterStartDocument(writer, NULL, XML_ENCODING, NULL);
	if (ret < 0) {
		_E("Failed to start xmlTextWriterStartDocument");
		xmlFreeTextWriter(writer);
		return NULL;
	}

	ret = xmlTextWriterStartElement(writer, BAD_CAST "data");
	if (ret < 0) {
		_E("Failed to start xmlTextWriterStartElement");
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
		_E("Cannot end the document");
	}

END:
	xmlFreeTextWriter(writer);
}



HAPI w_home_error_e xml_read_list(const char *xml_file, Eina_List *page_info_list)
{
	xmlTextWriterPtr writer;
	Eina_List *l = NULL;
	page_info_s *page_info = NULL;
	char *category = NULL;
	int ret = 0;
	int row = -1;
	int cellx = -1;
	int celly = -1;

	retv_if(!xml_file, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(!page_info_list, W_HOME_ERROR_INVALID_PARAMETER);

	writer = _init_writer(xml_file);
	retv_if(!writer, W_HOME_ERROR_FAIL);

	EINA_LIST_FOREACH(page_info_list, l, page_info) {
		continue_if(!page_info);

		_get_position_by_order(page_info->ordering, &row, &cellx, &celly);

		switch(page_info->category) {
		case PAGE_CATEGORY_IDLE_CLOCK:
			category = (char *) PAGE_CATEGORY_IDLE_CLOCK_STR;
			break;
		case PAGE_CATEGORY_APP:
			category = (char *) PAGE_CATEGORY_FAVORITE_STR;
			break;
		case PAGE_CATEGORY_WIDGET:
			category = (char *) PAGE_CATEGORY_APP_WIDGET_STR;
			break;
		case PAGE_CATEGORY_MORE_APPS:
			category = (char *) PAGE_CATEGORY_MORE_APPS_STR;
			break;
		default:
			_E("page_info->category error");
			break;
		}

		ret = xmlTextWriterStartElement(writer, BAD_CAST category);
		goto_if(ret < 0, END);

		ret = xmlTextWriterWriteFormatElement(writer, BAD_CAST "packageName", "%s", page_info->id);
		goto_if(ret < 0, END);

		if (page_info->subid) {
			ret = xmlTextWriterWriteFormatElement(writer, BAD_CAST "className", "%s", page_info->subid);
		} else {
			ret = xmlTextWriterWriteFormatElement(writer, BAD_CAST "className", "%s", page_info->id);
		}
		goto_if(ret < 0, END);

		ret = xmlTextWriterWriteFormatElement(writer, BAD_CAST "screen", "%d", row);
		goto_if(ret < 0, END);

		ret = xmlTextWriterWriteFormatElement(writer, BAD_CAST "cellX", "%d", cellx);
		goto_if(ret < 0, END);

		ret = xmlTextWriterWriteFormatElement(writer, BAD_CAST "cellY", "%d", celly);
		goto_if(ret < 0, END);

		ret = xmlTextWriterEndElement(writer);
		goto_if(ret < 0, END);
	}

END:
	_destroy_writer(writer);

	return W_HOME_ERROR_NONE;
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



HAPI Eina_List *xml_write_list(const char *xml_file)
{
	xmlTextReaderPtr reader = NULL;
	Eina_List *page_info_list = NULL;
	page_info_s *page_info = NULL;
	const char *name, *value;
	char *element = NULL;
	int ret = -1;

	retv_if(!xml_file, NULL);

	reader = _init_reader(xml_file);
	retv_if(!reader, NULL);

	while ((ret = xmlTextReaderRead(reader)) == 1) {
		int dep = 0;
		int node_type = 0;
		static int cellx = 0;
		static int celly = 0;
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
			if (page_info) {
				page_info_destroy(page_info);
				page_info = NULL;
			}

			page_info = page_info_create(NULL, NULL, DYNAMICBOX_EVAS_DEFAULT_PERIOD);
			goto_if(!page_info, CRITICAL_ERROR);

			if(!strcasecmp(PAGE_CATEGORY_IDLE_CLOCK_STR, name)) {
				page_info->category = PAGE_CATEGORY_IDLE_CLOCK;
			} else if(!strcasecmp(PAGE_CATEGORY_FAVORITE_STR, name)) {
				page_info->category = PAGE_CATEGORY_APP;
			} else if(!strcasecmp(PAGE_CATEGORY_APP_WIDGET_STR, name)) {
				page_info->category = PAGE_CATEGORY_WIDGET;
			} else if(!strcasecmp(PAGE_CATEGORY_MORE_APPS_STR, name)) {
				page_info->category = PAGE_CATEGORY_MORE_APPS;
			} else {
				_E("Type is wrong.");
				page_info_destroy(page_info);
				page_info = NULL;
			}
			continue;
		}

		// Category has to be at first.
		if (!page_info) continue;

		// Element name
		if ((2 == dep) && (1 == node_type)) {
			if (element) {
				_E("The element name is NOT NULL");
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
				page_info->id = strdup(value);
				goto_if(!page_info->id, CRITICAL_ERROR);
			} else if (!strcasecmp("className", element)) {
				if (page_info->id && !strcmp(value, page_info->id)) {
					_D("%s subid is NULL", page_info->id);
					page_info->subid = NULL;
				} else {
					page_info->subid = strdup(value);
					goto_if(!page_info->subid, CRITICAL_ERROR);
				}
			} else if(!strcasecmp("screen", element)) {
				page_no = atoi(value);
			} else if(!strcasecmp("cellx", element)) {
				cellx = atoi(value);
			} else if(!strcasecmp("celly", element)) {
				celly = atoi(value);
			}
			continue;
		}

		// End of the Element
		if ((2 == dep) && (15 == node_type)) {
			if (!element) {
				if (page_info) {
					page_info_destroy(page_info);
					page_info = NULL;
					continue;
				}
			}

			free(element);
			element = NULL;
			continue;
		}

		// End of the category
		if ((1 == dep) && (15 == node_type)) { // End of the category
			page_info->direction = PAGE_DIRECTION_RIGHT;
			page_info->ordering = page_no * DBOX_PER_PAGE + celly * 2 + cellx;
			page_info->removable = page_info_is_removable(page_info->id);
			page_info_list = eina_list_append(page_info_list, page_info);
			_D("Append a package into the page_info_list : (%s:%s:%d:%d) (%d:%d:%d)"
				, page_info->id, page_info->subid, page_info->category, page_info->ordering
				, page_no, celly, cellx);

			page_info = NULL;
			page_no = 0;
			cellx = 0;
			celly = 0;
		}
	}

	if(element) {
		_E("An element is not appended properly");
		free(element);
		element = NULL;
	}

	if(page_info) {
		_E("A node(%s) is not appended into the page_info_list", page_info->subid);
		page_info_destroy(page_info);
		page_info = NULL;
	}
	goto_if(ret, CRITICAL_ERROR);

	_destroy_reader(reader);
	return page_info_list;

CRITICAL_ERROR:
	if (page_info) page_info_destroy(page_info);
	page_info_list_destroy(page_info_list);
	_destroy_reader(reader);
	free(element);
	return NULL;
}



// End of a file
