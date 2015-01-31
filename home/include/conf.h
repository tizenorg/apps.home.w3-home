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

#ifndef _W_HOME_CONF_H_
#define _W_HOME_CONF_H_

/* Layout */
#define BASE_WIDTH (360.0)
#define BASE_HEIGHT (480.0)

#define SCROLLER_NORMAL_HEIGHT BASE_HEIGHT
#define SCROLLER_EDIT_HEIGHT BASE_HEIGHT

#define INDICATOR_START_Y (7.0)
#define INDICATOR_HEIGHT (20.0)

#define INDEX_THUMBNAIL_HOME_BG_SIZE 20 20
#define INDEX_THUMBNAIL_HOME_IND_SIZE 20 20

#define PAGE_EDIT_PAD_WIDTH (16.0)
#define PAGE_EDIT_SIDE_PAD_WIDTH (40+16)

#define PAGE_EDIT_WIDTH (PAGE_EDIT_PAD_WIDTH+216.0+PAGE_EDIT_PAD_WIDTH)
#define PAGE_EDIT_HEIGHT (25.0+29.0+265.0)

#define ITEM_EDIT_WIDTH (216.0)
#define ITEM_EDIT_HEIGHT (288.0)

#define ITEM_EDIT_EF_WIDTH 242
#define ITEM_EDIT_EF_HEIGHT 314

#define ITEM_EDIT_LINE_WIDTH (ITEM_EDIT_WIDTH+4)
#define ITEM_EDIT_LINE_HEIGHT (ITEM_EDIT_HEIGHT+4)

#define ADD_WIDGET_BUTTON_HEIGHT (24+37+24)
#define BUTTON_HEIGHT_REL (0.15)
#define ADD_WIDGET_BUTTON_TEXT_MIN 328 86
#define ADD_VIEWER_PREVIEW_WIDTH 216
#define ADD_VIEWER_PREVIEW_HEIGHT 288
#define ADD_VIEWER_PREVIEW_LINE_WIDTH (ADD_VIEWER_PREVIEW_WIDTH+4)
#define ADD_VIEWER_PREVIEW_LINE_HEIGHT (ADD_VIEWER_PREVIEW_HEIGHT+4)
#define ADD_VIEWER_TEXT_HEIGHT 74
#define ADD_VIEWER_PAGE_WIDTH 360
#define ADD_VIEWER_PAGE_HEIGHT 377
#define ADD_VIEWER_PREVIEW_PAD_LEFT 72
#define ADD_VIEWER_PREVIEW_PAD_TOP 7
#define ADD_VIEWER_PAD 8

#define CLOCK_SHORTCUT_AREA_W 90
#define CLOCK_SHORTCUT_AREA_H 90

#define BEZEL_MOVE_THRESHOLD 80

#define BUTTON_LOCATION 411.5

#define CLOSE_BUTTON_X 316
#define CLOSE_BUTTON_Y 41

#define IMAGE_Y 112

/* Configuration */
#define BOOTING_STATE_DONE 1
#define MAX_DBOX 5
#define LONGPRESS_THRESHOLD 15
#define LONGPRESS_TIME 0.5f
#define INDEX_UPDATE_TIME 0.1f

#define DEFAULT_XML_FILE RESDIR"/default_items.xml"
#define TTS_XML_FILE RESDIR"/default_items_tts.xml"
#define APP_CONTROL_OPERATION_MAIN "http://tizen.org/appcontrol/operation/main"

#endif // _W_HOME_CONF_H_
// End of file
