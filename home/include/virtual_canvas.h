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

#ifndef __APP_TRAY_VIRTUAL_CANVAS_H__
#define __APP_TRAY_VIRTUAL_CANVAS_H__

extern Evas *virtual_canvas_create(int w, int h);
extern bool virtual_canvas_flush_to_file(Evas *e, const char *filename, int w, int h);
extern void *virtual_canvas_load_file_to_data(Evas *e, const char *file, int w, int h);
extern bool virtual_canvas_destroy(Evas *e);

#endif //__APP_TRAY_VIRTUAL_CANVAS_H__

// End of a file
