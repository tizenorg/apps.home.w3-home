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

#ifndef __W_HOME_WIDGET_H__
#define __W_HOME_WIDGET_H__


extern Evas_Object *widget_create(Evas_Object *parent, const char *id, const char *subid, double period);
extern void widget_destroy(Evas_Object *widget);

extern void widget_add_callback(Evas_Object *widget, Evas_Object *page);
extern void widget_del_callback(Evas_Object *widget);

extern void widget_set_update_callback(Evas_Object *obj, int (*updated)(Evas_Object *obj));
extern void widget_set_scroll_callback(Evas_Object *obj, int (*scroll)(Evas_Object *obj, int hold));

extern void widget_init(Evas_Object *win);
extern void widget_fini(void);

#endif /* __W_HOME_WIDGET_H__ */
