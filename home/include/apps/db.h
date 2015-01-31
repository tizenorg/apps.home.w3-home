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

#ifndef __APPS_DB_H__
#define __APPS_DB_H__

#include <stdbool.h>

#include "util.h"

typedef struct stmt stmt_h;

apps_error_e apps_db_open(const char *db_file);
stmt_h *apps_db_prepare(const char *query);
apps_error_e apps_db_bind_bool(stmt_h *handle, int idx, bool value);
apps_error_e apps_db_bind_int(stmt_h *handle, int idx, int value);
apps_error_e apps_db_bind_str(stmt_h *handle, int idx, const char *str);
apps_error_e apps_db_next(stmt_h *handle);
bool apps_db_get_bool(stmt_h *handle, int index);
int apps_db_get_int(stmt_h *handle, int index);
const char *apps_db_get_str(stmt_h *handle, int index);
apps_error_e apps_db_reset(stmt_h *handle);
apps_error_e apps_db_finalize(stmt_h *handle);
apps_error_e apps_db_exec(const char *query);
void apps_db_close(void);

apps_error_e apps_db_begin_transaction(void);
apps_error_e apps_db_end_transaction(bool success);

HAPI apps_error_e apps_db_init(void);
HAPI int apps_db_insert_item(const char *id, int ordering);
HAPI int apps_db_update_item(const char *id, int ordering);
HAPI int apps_db_remove_item(const char *id);
HAPI int apps_db_count_item(const char *id);
HAPI int apps_db_count_item_in(void);

HAPI apps_error_e apps_db_read_list(Eina_List *item_info_list);
HAPI Eina_List *apps_db_write_list(void);
HAPI Eina_List *apps_db_write_list_by_name(void);

HAPI apps_error_e apps_db_find_empty_position(int *pos);
HAPI apps_error_e apps_db_trim(void);
HAPI apps_error_e apps_db_sync(void);

#endif // __APPS_DB_H__
