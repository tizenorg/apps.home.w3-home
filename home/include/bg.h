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

#ifndef __W_HOME_BG_H__
#define __W_HOME_BG_H__

extern Evas_Object *bg_create(Evas_Object *win);
extern void bg_destroy(Evas_Object *win);

extern void bg_get_rgb(int *r, int *g, int *b);
extern void bg_set_rgb(Evas_Object *bg, const char *buf);

extern void bg_register_object(Evas_Object *obj);

#endif /* __W_HOME_BG_H__ */
