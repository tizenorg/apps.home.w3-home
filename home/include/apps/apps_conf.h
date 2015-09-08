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

#ifndef _APPS_CONF_H_
#define _APPS_CONF_H_

/* Layout */
#define BASE_WIDTH (360.0)
#if CIRCLE_TYPE
#define BASE_HEIGHT (360.0)
#else
#define BASE_HEIGHT (480.0)
#endif

#define LAYOUT_PAGE_INDICATOR_HEIGHT 40

#define LAYOUT_TITLE_BG_HEIGHT 50
#define LAYOUT_TITLE_BG_MIN 0 LAYOUT_TITLE_BG_HEIGHT

#define LAYOUT_TITLE_HEIGHT 50
#define LAYOUT_TITLE_MIN 0 LAYOUT_TITLE_HEIGHT

#define BUTTON_HEIGHT (69)

#define GRID_EDIT_HEIGHT (BASE_HEIGHT-LAYOUT_PAGE_INDICATOR_HEIGHT-LAYOUT_TITLE_HEIGHT-LAYOUT_PAD_AFTER_TITLE_HEIGHT-BUTTON_HEIGHT)
#define GRID_NORMAL_HEIGHT (BASE_HEIGHT-LAYOUT_PAGE_INDICATOR_HEIGHT-LAYOUT_TITLE_HEIGHT-LAYOUT_PAD_AFTER_TITLE_HEIGHT)

#if CIRCLE_TYPE
#define CIRCLE 1
#define ELM_SCROLLER_REGION_GET(scroller, x) elm_scroller_region_get(scroller, x, NULL, NULL, NULL)
#define ELM_SCROLLER_REGION_BRING_IN(scroller, x) elm_scroller_region_bring_in(scroller, x, 0, BASE_WIDTH, BASE_HEIGHT-1)
#define BOX_TOP_HEIGHT 10
#define BOX_TOP_MENU_WIDTH 360
#define BOX_TOP_MENU_HEIGHT 90
#define BOX_EDIT_TOP_HEIGHT 12
#define BOX_BOTTOM_HEIGHT 10
#define BOX_BOTTOM_MENU_HEIGHT 61

#define APPS_PAD_W (40+16)
#define APPS_PAGE_PAD_W (16)
#define APPS_PAGE_PAD_H (16)
#define APPS_PAGE_WIDTH (360)
#define APPS_PAGE_HEIGHT (360)

#define ITEM_WIDTH (216)
#define ITEM_HEIGHT (300)
#define ITEM_EDIT_WIDTH (3+23+95+23+3)
#define ITEM_EDIT_HEIGHT (95+58+7)

#define ITEM_ICON_WIDTH (176)
#define ITEM_ICON_HEIGHT (176)

#define SCROLLER_POLICY_VERTICAL ELM_SCROLLER_POLICY_OFF
#define SCROLLER_HORIZONTAL EINA_TRUE
#define SCROLL_DISTANCE BASE_WIDTH
#define SCROLL_PAD 0
#define PAGE_IN_VIEW 1
#define SCROLLER_PAGE_LIMIT_HORIZONTAL 1
#define SCROLLER_PAGE_LIMIT_VERTICAL 0
#else
#define CIRCLE 0
#define ELM_SCROLLER_REGION_GET(scroller, y) elm_scroller_region_get(scroller, NULL, y, NULL, NULL)
#define ELM_SCROLLER_REGION_BRING_IN(scroller, y) elm_scroller_region_bring_in(scroller, 0, y, BASE_WIDTH, BASE_HEIGHT-1)
#define BOX_TOP_HEIGHT 5
#define BOX_EDIT_TOP_HEIGHT 12
#define BOX_BOTTOM_HEIGHT 18
#define BOX_BOTTOM_MENU_HEIGHT 86

#define APPS_PAGE_WIDTH (300)
#define APPS_PAGE_HEIGHT (134)

#define ITEM_WIDTH (29+85+29)
#define ITEM_HEIGHT (85+30+9)
#define ITEM_EDIT_WIDTH (3+23+95+23+3)
#define ITEM_EDIT_HEIGHT (95+58+7)

#define ITEM_ICON_WIDTH (85)
#define ITEM_ICON_HEIGHT (85)

#define SCROLLER_POLICY_VERTICAL ELM_SCROLLER_POLICY_AUTO
#define SCROLLER_HORIZONTAL EINA_FALSE
#define SCROLL_DISTANCE (174)
#define SCROLL_PAD 150
#define SCROLLER_PAGE_LIMIT_HORIZONTAL 0
#define SCROLLER_PAGE_LIMIT_VERTICAL 3
#define PAGE_IN_VIEW 3
#endif

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
#if CIRCLE_TYPE
#define APPS_PER_PAGE 1
#else
#define APPS_PER_PAGE 2
#endif
#define LINE_SIZE 10
#define LONGPRESS_TIME 0.5f

#endif // _APPS_CONF_H_
// End of file
