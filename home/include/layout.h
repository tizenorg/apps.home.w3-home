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

#ifndef __W_HOME_LAYOUT_H__
#define __W_HOME_LAYOUT_H__

#define LAYOUT_SMART_SIGNAL_EDIT_ON "edit,on"
#define LAYOUT_SMART_SIGNAL_EDIT_OFF "edit,off"
#define LAYOUT_SMART_SIGNAL_FLICK_UP "flick,up"

extern Evas_Object *layout_create(Evas_Object *win);
extern void layout_destroy(Evas_Object *win);

extern void layout_add_mouse_cb(Evas_Object *layout);
extern void layout_del_mouse_cb(Evas_Object *layout);

extern void layout_show_left_index(Evas_Object *layout);
extern void layout_show_right_index(Evas_Object *layout);
extern void layout_hide_index(Evas_Object *layout);

extern int layout_is_edit_mode(Evas_Object *layout);

extern void layout_set_idle(Evas_Object *layout);
extern int layout_is_idle(Evas_Object *layout);

#endif /* __W_HOME_LAYOUT_H__ */
