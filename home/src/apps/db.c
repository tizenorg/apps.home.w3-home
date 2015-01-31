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

#include <db-util.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "util.h"
#include "apps/db.h"
#include "apps/item_info.h"
#include "apps/list.h"




#define retv_with_dbmsg_if(expr, val) do { \
	if (expr) { \
		_APPS_E("%s", sqlite3_errmsg(db_info.db)); \
		return (val); \
	} \
} while (0)

static struct {
	sqlite3 *db;
} db_info = {
	.db = NULL,
};

struct stmt {
	sqlite3_stmt *stmt;
};



HAPI apps_error_e apps_db_open(const char *db_file)
{
	int ret;

	retv_if(NULL == db_file, APPS_ERROR_INVALID_PARAMETER);
	if (db_info.db) {
		return APPS_ERROR_NONE;
	}

	ret = db_util_open(db_file, &db_info.db, DB_UTIL_REGISTER_HOOK_METHOD);
	if (ret != SQLITE_OK) _APPS_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, APPS_ERROR_FAIL);

	return APPS_ERROR_NONE;
}



HAPI stmt_h *apps_db_prepare(const char *query)
{
	int ret;
	stmt_h *handle;

	retv_if(NULL == query, NULL);

	handle = calloc(1, sizeof(stmt_h));
	retv_if(NULL == handle, NULL);

	ret = sqlite3_prepare_v2(db_info.db, query, strlen(query), &(handle->stmt), NULL);
	if (ret != SQLITE_OK) {
		free(handle);
		_APPS_E("%s, %s", query, sqlite3_errmsg(db_info.db));
		return NULL;
	}

	return handle;
}



HAPI apps_error_e apps_db_bind_bool(stmt_h *handle, int idx, bool value)
{
	int ret;

	retv_if(NULL == handle, APPS_ERROR_FAIL);

	ret = sqlite3_bind_int(handle->stmt, idx, (int) value);
	if (ret != SQLITE_OK) _APPS_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, APPS_ERROR_FAIL);

	return APPS_ERROR_NONE;
}



HAPI apps_error_e apps_db_bind_int(stmt_h *handle, int idx, int value)
{
	int ret;

	retv_if(NULL == handle, APPS_ERROR_FAIL);

	ret = sqlite3_bind_int(handle->stmt, idx, value);
	if (ret != SQLITE_OK) _APPS_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, APPS_ERROR_FAIL);

	return APPS_ERROR_NONE;
}



HAPI apps_error_e apps_db_bind_str(stmt_h *handle, int idx, const char *str)
{
	int ret;

	retv_if(NULL == handle, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == str, APPS_ERROR_INVALID_PARAMETER);

	ret = sqlite3_bind_text(handle->stmt, idx, str, strlen(str), SQLITE_TRANSIENT);
	if (ret != SQLITE_OK) _APPS_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, APPS_ERROR_FAIL);

	return APPS_ERROR_NONE;
}



HAPI apps_error_e apps_db_next(stmt_h *handle)
{
	int ret;

	retv_if(NULL == handle, APPS_ERROR_FAIL);

	ret = sqlite3_step(handle->stmt);
	switch (ret) {
		case SQLITE_ROW:
			return APPS_ERROR_NONE;
		case SQLITE_DONE:
			return APPS_ERROR_NO_DATA;
		default:
			_APPS_E("%s", sqlite3_errmsg(db_info.db));
			retv_with_dbmsg_if(1, APPS_ERROR_FAIL);
	}
}



HAPI bool apps_db_get_bool(stmt_h *handle, int index)
{
	retv_if(NULL == handle, false);
	return (bool) sqlite3_column_int(handle->stmt, index);
}



HAPI int apps_db_get_int(stmt_h *handle, int index)
{
	retv_if(NULL == handle, 0);
	return sqlite3_column_int(handle->stmt, index);
}


HAPI const char *apps_db_get_str(stmt_h *handle, int index)
{
	retv_if(NULL == handle, NULL);
	return (const char *) sqlite3_column_text(handle->stmt, index);
}



HAPI apps_error_e apps_db_reset(stmt_h *handle)
{
	int ret;

	retv_if(NULL == handle, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == handle->stmt, APPS_ERROR_INVALID_PARAMETER);

	ret = sqlite3_reset(handle->stmt);
	if (ret != SQLITE_OK) _APPS_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, APPS_ERROR_FAIL);

	sqlite3_clear_bindings(handle->stmt);

	return APPS_ERROR_NONE;
}



HAPI apps_error_e apps_db_finalize(stmt_h *handle)
{
	int ret;

	retv_if(NULL == handle, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == handle->stmt, APPS_ERROR_INVALID_PARAMETER);

	ret = sqlite3_finalize(handle->stmt);
	if (ret != SQLITE_OK) _APPS_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, APPS_ERROR_FAIL);
	free(handle);

	return APPS_ERROR_NONE;
}



HAPI apps_error_e apps_db_exec(const char *query)
{
	retv_if(NULL == query, APPS_ERROR_INVALID_PARAMETER);
	retv_if(NULL == db_info.db, APPS_ERROR_FAIL);

	stmt_h *handle = apps_db_prepare(query);
	retv_if(NULL == handle, APPS_ERROR_FAIL);

	goto_if(APPS_ERROR_FAIL == apps_db_next(handle), ERROR);
	if (APPS_ERROR_NONE != apps_db_finalize(handle)) return APPS_ERROR_FAIL;

	return APPS_ERROR_NONE;

ERROR:
	if (handle) apps_db_finalize(handle);
	return APPS_ERROR_FAIL;
}



HAPI void apps_db_close(void)
{
	ret_if(!db_info.db);
	sqlite3_close(db_info.db);
	db_info.db = NULL;
}



HAPI apps_error_e apps_db_begin_transaction(void)
{
	int ret = -1;

	ret = sqlite3_exec(db_info.db, "BEGIN IMMEDIATE TRANSACTION", NULL, NULL, NULL);

	while (SQLITE_BUSY == ret) {
		sleep(1);
		ret = sqlite3_exec(db_info.db, "BEGIN IMMEDIATE TRANSACTION", NULL, NULL, NULL);
	}

	if (SQLITE_OK != ret) {
		_APPS_E("sqlite3_exec() Failed(%d)", ret);
		return APPS_ERROR_FAIL;
	}

	return APPS_ERROR_NONE;
}



#define COMMIT_TRY_MAX 3
HAPI apps_error_e apps_db_end_transaction(bool success)
{
	int ret = -1;
	int i = 0;
	char *errmsg = NULL;

	if (success) {
		ret = sqlite3_exec(db_info.db, "COMMIT TRANSACTION", NULL, NULL, &errmsg);
		if (SQLITE_OK != ret) {
			_APPS_E("sqlite3_exec(COMMIT) Failed(%d, %s)", ret, errmsg);
			sqlite3_free(errmsg);

			while (SQLITE_BUSY == ret && i < COMMIT_TRY_MAX) {
				i++;
				sleep(1);
				ret = sqlite3_exec(db_info.db, "COMMIT TRANSACTION", NULL, NULL, NULL);
			}

			if (SQLITE_OK != ret) {
				_APPS_E("sqlite3_exec() Failed(%d)", ret);
				ret = sqlite3_exec(db_info.db, "ROLLBACK TRANSACTION", NULL, NULL, NULL);
				if (SQLITE_OK != ret) {
					_APPS_E("sqlite3_exec() Failed(%d)", ret);
				}

				return APPS_ERROR_FAIL;
			}
		}
	} else {
		ret = sqlite3_exec(db_info.db, "ROLLBACK TRANSACTION", NULL, NULL, NULL);
		if (SQLITE_OK != ret) {
			_APPS_E("sqlite3_exec() Failed(%d)", ret);
			return APPS_ERROR_FAIL;
		}
	}

	return APPS_ERROR_NONE;
}



#define APPS_DB_FILE DATADIR"/.apps.db"
#define APPS_TABLE "apps"
#define QUERY_CREATE_TABLE "CREATE TABLE IF NOT EXISTS "APPS_TABLE" ("\
	"id TEXT NOT NULL PRIMARY KEY, "\
	"ordering INTEGER "\
	");"

#define QUERY_INSERT_ITEM "INSERT INTO "APPS_TABLE" "\
	"(id, ordering) "\
	"VALUES "\
	"('%s', %d);"

HAPI apps_error_e apps_db_init(void)
{
	char *query = NULL;

	retv_if(APPS_ERROR_NONE !=
			apps_db_open(APPS_DB_FILE), -1);

	query = sqlite3_mprintf(QUERY_CREATE_TABLE);
	retv_if(query == NULL, -1);

	if (apps_db_exec(query) != APPS_ERROR_NONE) {
		_APPS_E("Cannot execute query.[%s]", query);
		sqlite3_free(query);
		return APPS_ERROR_DB_FAILED;
	}

	return APPS_ERROR_NONE;
}

HAPI int apps_db_insert_item(const char *id, int ordering)
{
	char *query = NULL;

	retv_if(!id, -1);
	_APPS_D("Insert the item[%s:%d]", id, ordering);

	retv_if(APPS_ERROR_NONE !=
				apps_db_open(APPS_DB_FILE), -1);

	query = sqlite3_mprintf(QUERY_INSERT_ITEM, id, ordering);
	retv_if(query == NULL, -1);

	if (apps_db_exec(query) != APPS_ERROR_NONE) {
		_APPS_E("Cannot execute query.[%s]", query);
		sqlite3_free(query);
		return -1;
	}

	sqlite3_free(query);

	/* keep the home DB opened */

	return 0;
}



#define QUERY_UPDATE_ITEM "UPDATE "APPS_TABLE" SET ordering=%d WHERE id='%s'"
HAPI int apps_db_update_item(const char *id, int ordering)
{
	char *query = NULL;

	retv_if(!id, -1);
	_APPS_D("Update the item[%s:%d]", id, ordering);

	retv_if(APPS_ERROR_NONE !=
				apps_db_open(APPS_DB_FILE), -1);

	query = sqlite3_mprintf(QUERY_UPDATE_ITEM, ordering, id);
	retv_if(query == NULL, -1);

	if (apps_db_exec(query) != APPS_ERROR_NONE) {
		_APPS_E("Cannot execute query.[%s]", query);
		sqlite3_free(query);
		return -1;
	}

	sqlite3_free(query);

	/* keep the home DB opened */

	return 0;
}



#define QUERY_REMOVE_ITEM "DELETE FROM "APPS_TABLE" WHERE id='%s'"
HAPI int apps_db_remove_item(const char *id)
{
	char *query = NULL;

	retv_if(!id, -1);

	_APPS_D("Remove the item[%s]", id);

	retv_if(APPS_ERROR_NONE !=
				apps_db_open(APPS_DB_FILE), -1);

	query = sqlite3_mprintf(QUERY_REMOVE_ITEM, id);
	retv_if(query == NULL, -1);

	if (apps_db_exec(query) != APPS_ERROR_NONE) {
		_APPS_E("Cannot execute query.[%s]", query);
		sqlite3_free(query);
		return -1;
	}

	sqlite3_free(query);

	/* keep the home DB opened */

	return 0;
}



#define QUERY_COUNT_ITEM "SELECT COUNT(*) FROM "APPS_TABLE" WHERE id=?"
HAPI int apps_db_count_item(const char *id)
{
	stmt_h *st;
	int count;

	retv_if(id == NULL, -1);

	retv_if(APPS_ERROR_NONE !=
				apps_db_open(APPS_DB_FILE), -1);

	st = apps_db_prepare(QUERY_COUNT_ITEM);
	retv_if(st == NULL, -1);

	if (apps_db_bind_str(st, 1, id) != APPS_ERROR_NONE) {
		_APPS_E("db_bind_str error");
		apps_db_finalize(st);
		return -1;
	}

	if (apps_db_next(st) == APPS_ERROR_FAIL) {
		_APPS_E("db_next error");
		apps_db_finalize(st);
		return -1;
	}

	count = apps_db_get_int(st, 0);
	apps_db_finalize(st);

	/* keep the home DB opened */

	return count;
}



HAPI apps_error_e apps_db_read_list(Eina_List *item_info_list)
{
	const Eina_List *l, *ln;
	item_info_s *item_info = NULL;

	retv_if(!item_info_list, APPS_ERROR_INVALID_PARAMETER);

	EINA_LIST_FOREACH_SAFE(item_info_list, l, ln, item_info) {
		int count;
		continue_if(!item_info);
		continue_if(!item_info->appid);
		count = apps_db_count_item(item_info->appid);
		if (count > 0) {
			apps_db_update_item(item_info->appid, item_info->ordering);
		} else {
			apps_db_init();
			apps_db_insert_item(item_info->appid, item_info->ordering);
		}
	}

	return APPS_ERROR_NONE;
}



#define QUERY_SELECT_ORDERING_ITEM "SELECT id, ordering FROM "APPS_TABLE" ORDER BY ordering ASC"
HAPI Eina_List *apps_db_write_list(void)
{
	stmt_h *st = NULL;
	Eina_List *list = NULL;

	retv_if(APPS_ERROR_NONE !=
				apps_db_open(APPS_DB_FILE), NULL);

	st = apps_db_prepare(QUERY_SELECT_ORDERING_ITEM);
	if (!st) {
		_APPS_E("Cannot prepare the DB");
		apps_db_finalize(st);
		return NULL;
	}

	while (APPS_ERROR_NONE == apps_db_next(st)) {
		item_info_s *item_info = NULL;
		char *appid = (char *) apps_db_get_str(st, 0);
		continue_if(!appid);

		item_info = apps_item_info_create(appid);
		continue_if(!item_info);
		item_info->ordering = apps_db_get_int(st, 1);

		list = eina_list_append(list, item_info);
	}

	apps_db_finalize(st);

	return list;
}



#define QUERY_SELECT_ORDERING_ITEM "SELECT id, ordering FROM "APPS_TABLE" ORDER BY ordering ASC"
static Eina_List *_db_write_list_direct(void)
{
	stmt_h *st = NULL;
	Eina_List *list = NULL;

	retv_if(APPS_ERROR_NONE !=
				apps_db_open(APPS_DB_FILE), NULL);

	st = apps_db_prepare(QUERY_SELECT_ORDERING_ITEM);
	if (!st) {
		_APPS_E("Cannot prepare the DB");
		apps_db_finalize(st);
		return NULL;
	}

	while (APPS_ERROR_NONE == apps_db_next(st)) {
		item_info_s *item_info = NULL;
		char *appid = (char *) apps_db_get_str(st, 0);
		continue_if(!appid);

		item_info = calloc(1, sizeof(item_info_s));
		continue_if(!item_info);

		item_info->appid = strdup(appid);
		if (!item_info->appid) {
			_E("Cannot copy appid");
			free(item_info);
			continue;
		}
		item_info->ordering = apps_db_get_int(st, 1);

		list = eina_list_append(list, item_info);
	}

	apps_db_finalize(st);

	return list;
}



static int _sort_by_name_cb(const void *d1, const void *d2)
{
	const item_info_s *info1 = d1;
	const item_info_s *info2 = d2;

	if (NULL == info1 || NULL == info1->name) return 1;
	else if (NULL == info2 || NULL == info2->name) return -1;

	return strcmp(info1->name, info2->name);
}



#define QUERY_SELECT_ITEM "SELECT id, ordering FROM "APPS_TABLE""
HAPI Eina_List *apps_db_write_list_by_name(void)
{
	stmt_h *st = NULL;
	Eina_List *list = NULL;

	retv_if(APPS_ERROR_NONE !=
				apps_db_open(APPS_DB_FILE), NULL);

	st = apps_db_prepare(QUERY_SELECT_ITEM);
	if (!st) {
		_APPS_E("Cannot prepare the DB");
		apps_db_finalize(st);
		return NULL;
	}

	while (APPS_ERROR_NONE == apps_db_next(st)) {
		item_info_s *item_info = NULL;
		char *appid = (char *) apps_db_get_str(st, 0);
		continue_if(!appid);

		item_info = apps_item_info_create(appid);
		continue_if(!item_info);
		item_info->ordering = apps_db_get_int(st, 1);

		list = eina_list_append(list, item_info);
	}

	apps_db_finalize(st);

	list = eina_list_sort(list, eina_list_count(list), _sort_by_name_cb);

	return list;
}



#define QUERY_SELECT_ITEM_ORDER_BY_ASC "SELECT id, ordering FROM "APPS_TABLE" ORDER BY id ASC"
HAPI Eina_List *_db_write_list_direct_by_name(void)
{
	stmt_h *st = NULL;
	Eina_List *list = NULL;

	retv_if(APPS_ERROR_NONE !=
				apps_db_open(APPS_DB_FILE), NULL);

	st = apps_db_prepare(QUERY_SELECT_ITEM_ORDER_BY_ASC);
	if (!st) {
		_APPS_E("Cannot prepare the DB");
		apps_db_finalize(st);
		return NULL;
	}

	while (APPS_ERROR_NONE == apps_db_next(st)) {
		item_info_s *item_info = NULL;
		char *appid = (char *) apps_db_get_str(st, 0);
		continue_if(!appid);

		item_info = calloc(1, sizeof(item_info_s));
		continue_if(!item_info);

		item_info->appid = strdup(appid);
		if (!item_info->appid) {
			_E("Cannot copy appid");
			free(item_info);
			continue;
		}
		item_info->ordering = apps_db_get_int(st, 1);

		list = eina_list_append(list, item_info);
	}

	apps_db_finalize(st);

	return list;
}



#define QUERY_ITEM_COUNT_ITEM "SELECT COUNT(*) FROM "APPS_TABLE
HAPI int apps_db_count_item_in(void)
{
	stmt_h *st = NULL;
	int count = 0;

	retv_if(APPS_ERROR_NONE != apps_db_open(APPS_DB_FILE), -1);
	st = apps_db_prepare(QUERY_ITEM_COUNT_ITEM);
	retv_if(!st, -1);

	if (APPS_ERROR_FAIL == apps_db_next(st)) {
		_APPS_E("db_next error");
		apps_db_finalize(st);
		return -1;
	}
	count = apps_db_get_int(st, 0);
	apps_db_finalize(st);

	return count;
}



#define QUERY_ITEM_GET_MAX_POSITION "SELECT MAX(ordering) FROM "APPS_TABLE
HAPI apps_error_e apps_db_find_empty_position(int *pos)
{
	stmt_h *st = NULL;
	*pos = 0;

	if (0 == apps_db_count_item_in()) {
		return APPS_ERROR_NONE;
	}

	retv_if(APPS_ERROR_NONE != apps_db_open(APPS_DB_FILE), APPS_ERROR_FAIL);

	st = apps_db_prepare(QUERY_ITEM_GET_MAX_POSITION);
	retv_if(!st, APPS_ERROR_FAIL);

	if (APPS_ERROR_FAIL == apps_db_next(st)) {
		_APPS_E("db_next error");
		apps_db_finalize(st);
		return -1;
	}

	*pos = apps_db_get_int(st, 0) +1;

	apps_db_finalize(st);
	return APPS_ERROR_NONE;
}



static void _free_list(Eina_List *list)
{
	item_info_s *data;

	if (NULL == list) return;
	EINA_LIST_FREE(list, data) {
		if (data) apps_item_info_destroy(data);
	}
}



HAPI apps_error_e apps_db_trim(void)
{
	Eina_List *list = NULL;
	item_info_s *item = NULL;
	register int i;
	int position = 0;

	list = _db_write_list_direct();
	retv_if (!list, APPS_ERROR_FAIL);

	for (i = 0; true; i++) {
		item = eina_list_nth(list, i);
		if (!item) break;
		if (item->ordering == position) {
			position ++;
			continue;
		}

		if (APPS_ERROR_NONE != apps_db_update_item(item->appid, position)) {
			_APPS_E("cannot set an item(%s)'s position(%d)", item->appid, position);
			_free_list(list);
			return APPS_ERROR_FAIL;
		}
		_APPS_D("Trim the DB : %s (%d->%d)", item->appid, item->ordering, position);
		position ++;
	}
	_free_list(list);

	return APPS_ERROR_NONE;
}



HAPI apps_error_e apps_db_sync(void)
{
	int pos = 0;
	app_list *pkg_list = NULL;
	Eina_List *db_list = NULL;
	item_info_s *db_item = NULL;
	item_info_s *pkg_item = NULL;

	_APPS_D("apps db sync");

	pkg_list = list_create_by_appid();
	retv_if(!pkg_list, APPS_ERROR_FAIL);

	db_list = _db_write_list_direct_by_name();
	goto_if(!db_list, ERROR);

	pkg_item = eina_list_nth(pkg_list->list, 0);
	db_item = eina_list_nth(db_list, 0);

	while (1) {
		int serial = 0;

		if (pkg_item && !db_item) {
			goto_if(!pkg_item->appid, ERROR);
			serial = -1;
		} else if (!pkg_item && db_item) {
			goto_if(!db_item->appid, ERROR);
			serial = 1;
		} else if (!pkg_item && !db_item) {
			break;
		} else {
			goto_if(!pkg_item->appid, ERROR);
			goto_if(!db_item->appid, ERROR);
			serial = strcmp(pkg_item->appid, db_item->appid);
		}

		if (serial < 0) {
			if (APPS_ERROR_NONE != apps_db_find_empty_position(&pos)) {
				_APPS_E("Cannot find an empty position.");
				break;
			}

			if (APPS_ERROR_NONE != apps_db_insert_item(pkg_item->appid, pos)) {
				_APPS_E("Cannot add a package.");
				break;
			}

			pkg_list->list = eina_list_next(pkg_list->list);
			pkg_item = eina_list_data_get(pkg_list->list);
		} else if (serial > 0) {
			if (APPS_ERROR_NONE != apps_db_remove_item(db_item->appid)) {
				_APPS_E("Cannot remove a package.");
				break;
			}

			db_list = eina_list_next(db_list);
			db_item = eina_list_data_get(db_list);
		} else {
			pkg_list->list = eina_list_next(pkg_list->list);
			pkg_item = eina_list_data_get(pkg_list->list);

			db_list = eina_list_next(db_list);
			db_item = eina_list_data_get(db_list);
		}
	}

	list_destroy(pkg_list);
	_free_list(db_list);

	retv_if(APPS_ERROR_NONE != apps_db_trim(), APPS_ERROR_FAIL);

	return APPS_ERROR_NONE;

ERROR:
	if (pkg_list) list_destroy(pkg_list);
	if (db_list) _free_list(db_list);

	return APPS_ERROR_FAIL;
}



// End of file.
