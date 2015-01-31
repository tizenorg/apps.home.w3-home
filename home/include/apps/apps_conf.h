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

#ifndef _APPS_CONF_H_
#define _APPS_CONF_H_

/* Layout */
#define BASE_WIDTH (360.0)
#define BASE_HEIGHT (480.0)

#define LAYOUT_PAGE_INDICATOR_HEIGHT 40

#define LAYOUT_TITLE_BG_HEIGHT 50
#define LAYOUT_TITLE_BG_MIN 0 LAYOUT_TITLE_BG_HEIGHT

#define LAYOUT_TITLE_HEIGHT 50
#define LAYOUT_TITLE_MIN 0 LAYOUT_TITLE_HEIGHT

#define BUTTON_HEIGHT (69)

#define GRID_EDIT_HEIGHT (BASE_HEIGHT-LAYOUT_PAGE_INDICATOR_HEIGHT-LAYOUT_TITLE_HEIGHT-LAYOUT_PAD_AFTER_TITLE_HEIGHT-BUTTON_HEIGHT)
#define GRID_NORMAL_HEIGHT (BASE_HEIGHT-LAYOUT_PAGE_INDICATOR_HEIGHT-LAYOUT_TITLE_HEIGHT-LAYOUT_PAD_AFTER_TITLE_HEIGHT)

#define BOX_TOP_HEIGHT 10
#define BOX_TOP_MENU_WIDTH 360
#define BOX_TOP_MENU_HEIGHT 87
#define BOX_BOTTOM_HEIGHT 18
#define BOX_BOTTOM_MENU_HEIGHT 61

#define ITEM_WIDTH (33+98+33)
#define ITEM_HEIGHT (98+64+16)
#define ITEM_EDIT_WIDTH (3+23+95+23+3)
#define ITEM_EDIT_HEIGHT (95+58+7)

#define ITEM_ICON_WIDTH (105)
#define ITEM_ICON_HEIGHT (108)

#define ITEM_SMALL_ICON_WIDTH (95)
#define ITEM_SMALL_ICON_HEIGHT (95)

#define ITEM_ICON_Y (45)
#define ITEM_TEXT_Y (90)

#define ITEM_BADGE_X (29+87)
#define ITEM_BADGE_Y (14)

#define ITEM_BADGE_W 54
#define ITEM_BADGE_H 59
#define ITEM_BADGE_GAP 17
#define ITEM_BADGE_2W (ITEM_BADGE_W+ITEM_BADGE_GAP)
#define ITEM_BADGE_3W (ITEM_BADGE_W+(ITEM_BADGE_GAP*2))

#define EDIT_BUTTON_SIZE_W (50)
#define EDIT_BUTTON_SIZE_H (50)

#define CTXPOPUP_ICON_SIZE 49

/* Configuration */
#define APPS_PER_PAGE 2
#define LINE_SIZE 10
#define LONGPRESS_TIME 0.5f

#endif // _APPS_CONF_H_
// End of file
