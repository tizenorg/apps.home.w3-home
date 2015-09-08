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

extern int add_viewer_util_add_to_home(struct add_viewer_package *package, int size, int use_noti);
extern int add_viewer_util_init(void);
extern int add_viewer_util_fini(void);
extern int add_viewer_util_is_lcd_off(void);
extern char *add_viewer_util_highlight_keyword(const char *name, const char *filter);
extern int add_viewer_util_get_utf8_len(char ch);
extern void add_viewer_util_update_matched_color(void);

/* End of a file */
