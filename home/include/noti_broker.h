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

#ifndef __W_HOME_NOTI_BROKER_H__
#define __W_HOME_NOTI_BROKER_H__

extern const int EVENT_SOURCE_SCROLLER;
extern const int EVENT_SOURCE_VIEW;
extern const int EVENT_SOURCE_EDITING;

extern const int EVENT_TYPE_ANIM_START;
extern const int EVENT_TYPE_ANIM_STOP;
extern const int EVENT_TYPE_DRAG_START;
extern const int EVENT_TYPE_DRAG_STOP;
extern const int EVENT_TYPE_EDGE_LEFT;
extern const int EVENT_TYPE_EDGE_RIGHT;
extern const int EVENT_TYPE_EDIT_START;
extern const int EVENT_TYPE_EDIT_STOP;
extern const int EVENT_TYPE_NOTI_DELETE;
extern const int EVENT_TYPE_NOTI_DELETE_ALL;
extern const int EVENT_TYPE_MOUSE_DOWN;
extern const int EVENT_TYPE_MOUSE_UP;
extern const int EVENT_TYPE_SCROLLING;
extern const int EVENT_TYPE_APPS_SHOW;
extern const int EVENT_TYPE_APPS_HIDE;

extern void noti_broker_load(void);
extern void noti_broker_init(void);
extern void noti_broker_fini(void);
extern int noti_broker_event_fire_to_plugin(int source, int event, void *data);

#endif /* __W_HOME_NOTI_BROKER_H__ */
