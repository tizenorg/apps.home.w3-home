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

#ifndef __W_HOME_NOTIFICATION_TIME_H__
#define __W_HOME_NOTIFICATION_TIME_H__

extern w_home_error_e notification_time_to_string(time_t *time, char *string, int len);
extern void notification_time_init(void);
extern void notification_time_fini(void);

#endif /* __W_HOME_NOTIFICATION_TIME_H__ */
