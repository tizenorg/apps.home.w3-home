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

#ifndef __APP_TRAY_MAPBUF_H__
#define __APP_TRAY_MAPBUF_H__

extern void mapbuf_set_color(Evas_Object *obj, int a);
extern w_home_error_e mapbuf_enable(Evas_Object *obj, int force);
extern int mapbuf_disable(Evas_Object *obj, int force);

extern Evas_Object *mapbuf_bind(Evas_Object *box, Evas_Object *page);
extern Evas_Object *mapbuf_unbind(Evas_Object *obj);

extern Evas_Object *mapbuf_get_mapbuf(Evas_Object *obj);
extern Evas_Object *mapbuf_get_page(Evas_Object *obj);

extern int mapbuf_is_enabled(Evas_Object *obj);
extern int mapbuf_can_be_made(Evas_Object *obj);
extern int mapbuf_can_be_on(Evas_Object *obj);


#endif //__APP_TRAY_MAPBUF_H__

// End of a file
