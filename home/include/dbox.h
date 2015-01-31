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

#ifndef __W_HOME_DYNAMICBOX_H__
#define __W_HOME_DYNAMICBOX_H__


extern Evas_Object *dbox_create(Evas_Object *parent, const char *id, const char *subid, double period);
extern void dbox_destroy(Evas_Object *dynamicbox);

extern void dbox_add_callback(Evas_Object *dynamicbox, Evas_Object *page);
extern void dbox_del_callback(Evas_Object *dynamicbox);

extern void dbox_set_update_callback(Evas_Object *obj, int (*updated)(Evas_Object *obj));
extern void dbox_set_scroll_callback(Evas_Object *obj, int (*scroll)(Evas_Object *obj, int hold));

extern void dbox_init(Evas_Object *win);
extern void dbox_fini(void);

#endif /* __W_HOME_DYNAMICBOX_H__ */
