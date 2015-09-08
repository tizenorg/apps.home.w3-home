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

#ifndef __W_HOME_DBUS_H__
#define __W_HOME_DBUS_H__

typedef enum {
	DBUS_EVENT_COOLDOWN_STATE_CHANGED = 0,
	DBUS_EVENT_ALPM_MANAGER_STATE_CHANGED = 1,
	DBUS_EVENT_LCD_ON = 2,
	DBUS_EVENT_LCD_OFF = 3,
	DBUS_EVENT_MAX,
} dbus_event_type_e;

/*!
 * DBUS interfaces and signals
 */
#define DBUS_ALPM_MANAGER_PATH "/Org/Tizen/System/AlpmMgr"
#define DBUS_ALPM_MANAGER_INTERFACE "org.tizen.system.alpmmgr"
#define DBUS_ALPM_MANAGER_MEMBER_STATUS "ALPMStatus"

#define DBUS_WAKEUP_GESTURE_PATH "/org/tizen/sensor/context/gesture"
#define DBUS_WAKEUP_GESTURE_INTERFACE "org.tizen.sensor.context.gesture"
#define DBUS_WAKEUP_GESTURE_MEMBER_WAKEUP "wakeup"

#define DBUS_LOW_BATTERY_PATH "/Org/Tizen/System/Popup/Lowbat"
#define DBUS_LOW_BATTERY_INTERFACE "org.tizen.system.popup.Lowbat"
#define DBUS_LOW_BATTERY_MEMBER_EXTREME_LEVEL "Extreme"

#define DBUS_HOME_BUS_NAME "org.tizen.coreapps.home"
#define DBUS_HOME_RAISE_PATH "/Org/Tizen/Coreapps/home/raise"
#define DBUS_HOME_RAISE_INTERFACE DBUS_HOME_BUS_NAME".raise"
#define DBUS_HOME_RAISE_MEMBER "homeraise"

#define DBUS_PROCSWEEP_PATH "/Org/Tizen/ResourceD/Process"
#define DBUS_PROCSWEEP_INTERFACE "org.tizen.resourced.process"
#define DBUS_PROCSWEEP_METHOD "ProcSweep"

#define DBUS_DEVICED_BUS_NAME "org.tizen.system.deviced"
#define DBUS_DEVICED_PATH "/Org/Tizen/System/DeviceD"
#define DBUS_DEVICED_INTERFACE DBUS_DEVICED_BUS_NAME
// deviced::display
#define DBUS_DEVICED_DISPLAY_PATH DBUS_DEVICED_PATH"/Display"
#define DBUS_DEVICED_DISPLAY_INTERFACE DBUS_DEVICED_INTERFACE".display"
#define DBUS_DEVICED_DISPLAY_MEMBER_LCD_ON "LCDOn"
#define DBUS_DEVICED_DISPLAY_MEMBER_LCD_OFF "LCDOff"
#define DBUS_DEVICED_DISPLAY_MEMBER_LCD_ON_BY_POWERKEY "LCDOnByPowerkey"
#define DBUS_DEVICED_DISPLAY_METHOD_LCD_OFF "PowerKeyLCDOff"
#define DBUS_DEVICED_DISPLAY_METHOD_CHANGE_STATE "changestate"
#define DBUS_DEVICED_DISPLAY_METHOD_CUSTOM_LCD_ON "CustomLCDOn"
#define DBUS_DEVICED_DISPLAY_COMMAND_LCD_ON "lcdon"
// deviced::cooldown mode
#define DBUS_DEVICED_SYSNOTI_PATH DBUS_DEVICED_PATH"/SysNoti"
#define DBUS_DEVICED_SYSNOTI_INTERFACE DBUS_DEVICED_INTERFACE".SysNoti"
#define DBUS_DEVICED_SYSNOTI_MEMBER_COOLDOWN_CHANGED "CoolDownChanged"
#define DBUS_DEVICED_SYSNOTI_METHOD_COOLDOWN_STATUS "GetCoolDownStatus"
// deviced::cpu booster
#define DBUS_DEVICED_CPU_BOOSTER_PATH DBUS_DEVICED_PATH"/PmQos"
#define DBUS_DEVICED_CPU_BOOSTER_INTERFACE DBUS_DEVICED_INTERFACE".PmQos"
#define DBUS_DEVICED_CPU_BOOSTER_METHOD_HOME "HomeScreen"
#define DBUS_DEVICED_CPU_BOOSTER_METHOD_HOME_LAUNCH "AppLaunchHome"

/*!
 * DBUS main functions
 */
extern void *home_dbus_connection_get(void);
extern void home_dbus_init(void *data);
extern void home_dbus_fini(void *data);
extern int home_dbus_register_cb(int type, void (*result_cb)(void *, void *), void *result_data);
extern void home_dbus_unregister_cb(int type, void (*result_cb)(void *, void *));
/*!
 * DBUS utility functions
 */
extern void home_dbus_lcd_on_signal_send(Eina_Bool lcd_on);
extern void home_dbus_lcd_off_signal_send(void);
extern void home_dbus_procsweep_signal_send(void);
extern void home_dbus_home_raise_signal_send(void);
extern void home_dbus_cpu_booster_signal_send(void);
extern void home_dbus_scroll_booster_signal_send(int sec);
extern char *home_dbus_cooldown_status_get(void);

#endif
