/*
 *  W-Home
 *
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jin Yoon <jinny.yoon@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __W_HOME_WMS_H__
#define __W_HOME_WMS_H__

// 0 : init, 1 : backup request, 2 : restore request, 3: write done
enum {
	W_HOME_WMS_INIT = 0,
	W_HOME_WMS_BACKUP,
	W_HOME_WMS_RESOTRE,
	W_HOME_WMS_DONE,
};

extern void wms_change_apps_order(int value);
extern void wms_change_favorite_order(int value);

extern void wms_register_setup_wizard_vconf();
extern void wms_unregister_setup_wizard_vconf();

extern void wms_launch_gear_manager(Evas_Object *parent, char *link);

#endif /* __W_HOME_MAIN_H__ */
