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

#ifndef __W_HOME_DB_H__
#define __W_HOME_DB_H__

typedef struct stmt stmt_h;
typedef enum {
	DB_FILE_INVALID = 0,
	DB_FILE_NORMAL,
	DB_FILE_TTS,
	DB_FILE_MAX
} db_file_e;

extern w_home_error_e db_init(db_file_e db_file);
extern w_home_error_e db_open(const char *db_file);
extern stmt_h *db_prepare(const char *query);
extern w_home_error_e db_bind_bool(stmt_h *handle, int idx, bool value);
extern w_home_error_e db_bind_int(stmt_h *handle, int idx, int value);
extern w_home_error_e db_bind_str(stmt_h *handle, int idx, const char *str);
extern w_home_error_e db_next(stmt_h *handle);
extern bool db_get_bool(stmt_h *handle, int index);
extern int db_get_int(stmt_h *handle, int index);
extern const char *db_get_str(stmt_h *handle, int index);
extern w_home_error_e db_reset(stmt_h *handle);
extern w_home_error_e db_finalize(stmt_h *handle);
extern w_home_error_e db_exec(const char *query);
extern void db_close(void);

extern w_home_error_e db_begin_transaction(void);
extern w_home_error_e db_end_transaction(bool success);

extern int db_insert_item(const char *id, const char *subid, int ordering);
extern int db_update_item(const char *id, const char *subid, int ordering);
extern int db_update_item_by_ordering(const char *id, const char *subid, int ordering);
extern int db_remove_item(const char *id, const char *subid);
extern int db_remove_item_after_max(int max);
extern int db_remove_all_item(void);

extern int db_count_item(const char *id, const char *subid);
extern int db_count_ordering(int ordering);

extern w_home_error_e db_read_list(Eina_List *page_info_list);
extern Eina_List *db_write_list(void);

#endif // __W_HOME_DB_H__
