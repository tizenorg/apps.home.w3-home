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

#include <bundle.h>
#include <Evas.h>
#include <db-util.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Eina.h>
#include <dlog.h>
#include <vconf.h>
#include <app_preference.h>
#include <widget_viewer_evas.h> // WIDGET_VIEWER_EVAS_DEFAULT_PERIOD

#include "util.h"
#include "db.h"
#include "log.h"
#include "page_info.h"
#define HOME_DB_FILE DATADIR"/.home.db"
#define HOME_DB_TTS_FILE DATADIR"/.home_tts.db"


#define retv_with_dbmsg_if(expr, val) do { \
	if (expr) { \
		_E("%s", sqlite3_errmsg(db_info.db)); \
		return (val); \
	} \
} while (0)



static struct {
	sqlite3 *db;
	char *db_file;
} db_info = {
	.db = NULL,
	.db_file = HOME_DB_FILE,
};

struct stmt {
	sqlite3_stmt *stmt;
};



HAPI w_home_error_e db_init(db_file_e db_file)
{
	retv_if(!db_file, W_HOME_ERROR_FAIL);

	db_close();

	switch (db_file) {
	case DB_FILE_NORMAL:
		db_info.db_file = HOME_DB_FILE;
		break;
	case DB_FILE_TTS:
		db_info.db_file = HOME_DB_TTS_FILE;
		break;
	default:
		_E("Invalid db_file");
	}

	retv_if(W_HOME_ERROR_NONE !=
			db_open(db_info.db_file), -1);

	return W_HOME_ERROR_NONE;
}



HAPI w_home_error_e db_open(const char *db_file)
{
	int ret;

	retv_if(!db_file, W_HOME_ERROR_INVALID_PARAMETER);
	if (db_info.db) {
		return W_HOME_ERROR_NONE;
	}

	ret = db_util_open(db_file, &db_info.db, DB_UTIL_REGISTER_HOOK_METHOD);
	if (ret != SQLITE_OK)
		_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}



HAPI stmt_h *db_prepare(const char *query)
{
	stmt_h *handle;
	int ret;

	retv_if(!query, NULL);

	handle = calloc(1, sizeof(stmt_h));
	retv_if(!handle, NULL);

	ret = sqlite3_prepare_v2(db_info.db, query, strlen(query), &(handle->stmt), NULL);
	if (ret != SQLITE_OK) {
		free(handle);
		_E("%s, %s", query, sqlite3_errmsg(db_info.db));
		return NULL;
	}

	return handle;
}



HAPI w_home_error_e db_bind_bool(stmt_h *handle, int idx, bool value)
{
	int ret;

	retv_if(!handle, W_HOME_ERROR_FAIL);

	ret = sqlite3_bind_int(handle->stmt, idx, (int) value);
	if (ret != SQLITE_OK)
		_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}



HAPI w_home_error_e db_bind_int(stmt_h *handle, int idx, int value)
{
	int ret;

	retv_if(!handle, W_HOME_ERROR_FAIL);

	ret = sqlite3_bind_int(handle->stmt, idx, value);
	if (ret != SQLITE_OK)
		_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}



HAPI w_home_error_e db_bind_str(stmt_h *handle, int idx, const char *str)
{
	int ret;

	retv_if(!handle, W_HOME_ERROR_FAIL);
	retv_if(!str, W_HOME_ERROR_FAIL);

	ret = sqlite3_bind_text(handle->stmt, idx, str, strlen(str), SQLITE_TRANSIENT);
	if (ret != SQLITE_OK)
		_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, W_HOME_ERROR_FAIL);

	return W_HOME_ERROR_NONE;
}



HAPI w_home_error_e db_next(stmt_h *handle)
{
	int ret;

	retv_if(!handle, W_HOME_ERROR_FAIL);

	ret = sqlite3_step(handle->stmt);
	switch (ret) {
	case SQLITE_ROW:
		return W_HOME_ERROR_NONE;
	case SQLITE_DONE:
		return W_HOME_ERROR_NO_DATA;
	default:
		_E("%s", sqlite3_errmsg(db_info.db));
		retv_with_dbmsg_if(1, W_HOME_ERROR_FAIL);
	}
}



HAPI bool db_get_bool(stmt_h *handle, int index)
{
	retv_if(!handle, false);
	return (bool) sqlite3_column_int(handle->stmt, index);
}



HAPI int db_get_int(stmt_h *handle, int index)
{
	retv_if(!handle, 0);
	return sqlite3_column_int(handle->stmt, index);
}


HAPI const char *db_get_str(stmt_h *handle, int index)
{
	retv_if(!handle, NULL);
	return (const char *) sqlite3_column_text(handle->stmt, index);
}



HAPI w_home_error_e db_reset(stmt_h *handle)
{
	int ret;

	retv_if(!handle, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(!handle->stmt, W_HOME_ERROR_INVALID_PARAMETER);

	ret = sqlite3_reset(handle->stmt);
	if (ret != SQLITE_OK)
		_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, W_HOME_ERROR_FAIL);

	sqlite3_clear_bindings(handle->stmt);

	return W_HOME_ERROR_NONE;
}



HAPI w_home_error_e db_finalize(stmt_h *handle)
{
	int ret;

	retv_if(!handle, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(!handle->stmt, W_HOME_ERROR_INVALID_PARAMETER);

	ret = sqlite3_finalize(handle->stmt);
	if (ret != SQLITE_OK)
		_E("%s", sqlite3_errmsg(db_info.db));
	retv_with_dbmsg_if(ret != SQLITE_OK, W_HOME_ERROR_FAIL);
	free(handle);

	return W_HOME_ERROR_NONE;
}



HAPI w_home_error_e db_exec(const char *query)
{
	retv_if(!query, W_HOME_ERROR_INVALID_PARAMETER);
	retv_if(!db_info.db, W_HOME_ERROR_FAIL);

	stmt_h *handle = db_prepare(query);
	retv_if(!handle, W_HOME_ERROR_FAIL);

	goto_if(W_HOME_ERROR_FAIL == db_next(handle), ERROR);
	if (W_HOME_ERROR_NONE != db_finalize(handle))
		return W_HOME_ERROR_FAIL;

	return W_HOME_ERROR_NONE;

ERROR:
	if (handle) db_finalize(handle);
	return W_HOME_ERROR_FAIL;
}



HAPI void db_close(void)
{
	if (!db_info.db) {
		_D("DB is already NULL");
		return;
	}
	sqlite3_close(db_info.db);
	db_info.db = NULL;
}



HAPI w_home_error_e db_begin_transaction(void)
{
	int ret = -1;

	ret = sqlite3_exec(db_info.db, "BEGIN IMMEDIATE TRANSACTION", NULL, NULL, NULL);

	while (SQLITE_BUSY == ret) {
		sleep(1);
		ret = sqlite3_exec(db_info.db, "BEGIN IMMEDIATE TRANSACTION", NULL, NULL, NULL);
	}

	if (SQLITE_OK != ret) {
		_E("sqlite3_exec() Failed(%d)", ret);
		return W_HOME_ERROR_FAIL;
	}

	return W_HOME_ERROR_NONE;
}



#define COMMIT_TRY_MAX 3
HAPI w_home_error_e db_end_transaction(bool success)
{
	int ret = -1;
	int i = 0;
	char *errmsg = NULL;

	if (success) {
		ret = sqlite3_exec(db_info.db, "COMMIT TRANSACTION", NULL, NULL, &errmsg);
		if (ret != SQLITE_OK) {
			_E("sqlite3_exec(COMMIT) Failed(%d, %s)", ret, errmsg);
			sqlite3_free(errmsg);

			while (SQLITE_BUSY == ret && i < COMMIT_TRY_MAX) {
				i++;
				sleep(1);
				ret = sqlite3_exec(db_info.db, "COMMIT TRANSACTION", NULL, NULL, NULL);
			}

			if (ret != SQLITE_OK) {
				_E("sqlite3_exec() Failed(%d)", ret);
				ret = sqlite3_exec(db_info.db, "ROLLBACK TRANSACTION", NULL, NULL, NULL);
				if (ret != SQLITE_OK) {
					_E("sqlite3_exec() Failed(%d)", ret);
				}

				return W_HOME_ERROR_FAIL;
			}
		}
	} else {
		ret = sqlite3_exec(db_info.db, "ROLLBACK TRANSACTION", NULL, NULL, NULL);
		if (ret != SQLITE_OK) {
			_E("sqlite3_exec() Failed(%d)", ret);
			return W_HOME_ERROR_FAIL;
		}
	}

	return W_HOME_ERROR_NONE;
}



#define HOME_DB_FILE DATADIR"/.home.db"
#define HOME_TABLE "home"
#define QUERY_INSERT_ITEM "INSERT INTO "HOME_TABLE" "\
		"(id, subid, ordering) "\
		"VALUES "\
		"('%s', '%s', %d);"
HAPI int db_insert_item(const char *id, const char *subid, int ordering)
{
	char *query;

	retv_if(!id, -1);
	retv_if(W_HOME_ERROR_NONE !=
			db_open(db_info.db_file), -1);

	if (subid) {
		_D("Insert the item[%s:%s:%d]", id, subid, ordering);
		query = sqlite3_mprintf(QUERY_INSERT_ITEM, id, subid, ordering);
	} else {
		_D("Insert the item[%s:%d]", id, ordering);
		query = sqlite3_mprintf(QUERY_INSERT_ITEM, id, "", ordering);
	}
	retv_if(query == NULL, -1);

	if (db_exec(query) != W_HOME_ERROR_NONE) {
		_E("Cannot execute query.[%s]", query);
		sqlite3_free(query);
		return -1;
	}

	sqlite3_free(query);

	/* keep the home DB opened */

	return 0;
}



#define QUERY_UPDATE_ITEM "UPDATE "HOME_TABLE" SET ordering=%d WHERE id='%s' and subid='%s'"
HAPI int db_update_item(const char *id, const char *subid, int ordering)
{
	char *query;

	retv_if(!id, -1);
	retv_if(W_HOME_ERROR_NONE !=
				db_open(db_info.db_file), -1);

	if (subid) {
		_D("Update the item[%s:%s:%d]", id, subid, ordering);
		query = sqlite3_mprintf(QUERY_UPDATE_ITEM, ordering, id, subid);
	} else {
		_D("Update the item[%s:%d]", id, ordering);
		query = sqlite3_mprintf(QUERY_UPDATE_ITEM, ordering, id, "");
	}
	retv_if(query == NULL, -1);

	if (db_exec(query) != W_HOME_ERROR_NONE) {
		_E("Cannot execute query.[%s]", query);
		sqlite3_free(query);
		return -1;
	}

	sqlite3_free(query);

	/* keep the home DB opened */

	return 0;
}



#define QUERY_UPDATE_ITEM_BY_ORDERING "UPDATE "HOME_TABLE" SET id='%s', subid='%s' WHERE ordering=%d"
HAPI int db_update_item_by_ordering(const char *id, const char *subid, int ordering)
{
	char *query = NULL;

	retv_if(!id, -1);
	retv_if(W_HOME_ERROR_NONE !=
				db_open(db_info.db_file), -1);

	if (subid) {
		_SD("Update the item[%s:%s:%d]", id, subid, ordering);
		query = sqlite3_mprintf(QUERY_UPDATE_ITEM_BY_ORDERING, id, subid, ordering);
	} else {
		_SD("Update the item[%s:%d]", id, ordering);
		query = sqlite3_mprintf(QUERY_UPDATE_ITEM_BY_ORDERING, id, "", ordering);
	}
	retv_if(query == NULL, -1);

	if (db_exec(query) != W_HOME_ERROR_NONE) {
		_E("Cannot execute query.[%s]", query);
		sqlite3_free(query);
		return -1;
	}

	sqlite3_free(query);

	/* keep the home DB opened */

	return 0;
}



#define QUERY_REMOVE_ITEM "DELETE FROM "HOME_TABLE" WHERE id='%s' and subid='%s'"
HAPI int db_remove_item(const char *id, const char *subid)
{
	char *query;

	retv_if(!id, -1);
	retv_if(W_HOME_ERROR_NONE !=
				db_open(db_info.db_file), -1);

	if (subid) {
		_D("Remove the item[%s:%s]", id, subid);
		query = sqlite3_mprintf(QUERY_REMOVE_ITEM, id, subid);
	} else {
		_D("Remove the item[%s]", id);
		query = sqlite3_mprintf(QUERY_REMOVE_ITEM, id, "");
	}
	retv_if(query == NULL, -1);

	if (db_exec(query) != W_HOME_ERROR_NONE) {
		_E("Cannot execute query.[%s]", query);
		sqlite3_free(query);
		return -1;
	}

	sqlite3_free(query);

	/* keep the home DB opened */

	return 0;
}



#define QUERY_REMOVE_ITEM_AFTER_MAX "DELETE FROM "HOME_TABLE" WHERE ordering > %d"
HAPI int db_remove_item_after_max(int max)
{
	char *query;

	_D("Remove the item after max[%d]", max);

	retv_if(W_HOME_ERROR_NONE !=
				db_open(db_info.db_file), -1);

	query = sqlite3_mprintf(QUERY_REMOVE_ITEM_AFTER_MAX, max);
	retv_if(query == NULL, -1);

	if (db_exec(query) != W_HOME_ERROR_NONE) {
		_E("Cannot execute query.[%s]", query);
		sqlite3_free(query);
		return -1;
	}

	sqlite3_free(query);

	/* keep the home DB opened */

	return 0;
}



#define QUERY_REMOVE_ALL_ITEM "DELETE FROM "HOME_TABLE
HAPI int db_remove_all_item(void)
{
	char *query;

	_D("Remove all the item");

	retv_if(W_HOME_ERROR_NONE !=
				db_open(db_info.db_file), -1);

	query = sqlite3_mprintf(QUERY_REMOVE_ALL_ITEM);
	retv_if(query == NULL, -1);

	if (db_exec(query) != W_HOME_ERROR_NONE) {
		_E("Cannot execute query.[%s]", query);
		sqlite3_free(query);
		return -1;
	}

	sqlite3_free(query);

	if (preference_set_string(VCONF_KEY_HOME_LOGGING, ";") != 0) {
		_SE("Failed to set %s as NULL", VCONF_KEY_HOME_LOGGING);
	} else {
		_SD("Set %s as ';'", VCONF_KEY_HOME_LOGGING);
	}

	/* keep the home DB opened */

	return 0;
}



#define QUERY_COUNT_ITEM "SELECT COUNT(*) FROM "HOME_TABLE" WHERE id=? and subid=?"
HAPI int db_count_item(const char *id, const char *subid)
{
	stmt_h *st;
	int count;

	retv_if(id == NULL, -1);
	retv_if(W_HOME_ERROR_NONE !=
				db_open(db_info.db_file), -1);

	st = db_prepare(QUERY_COUNT_ITEM);
	retv_if(st == NULL, -1);

	if (db_bind_str(st, 1, id) != W_HOME_ERROR_NONE) {
		_E("db_bind_str error");
		db_finalize(st);
		return -1;
	}

	if (subid) {
		if (db_bind_str(st, 2, subid) != W_HOME_ERROR_NONE) {
			_E("db_bind_str error");
			db_finalize(st);
			return -1;
		}
	} else {
		if (db_bind_str(st, 2, "") != W_HOME_ERROR_NONE) {
			_E("db_bind_str error");
			db_finalize(st);
			return -1;
		}
	}

	if (db_next(st) == W_HOME_ERROR_FAIL) {
		_E("db_next error");
		db_finalize(st);
		return -1;
	}

	count = db_get_int(st, 0);
	db_finalize(st);

	/* keep the home DB opened */

	return count;
}



#define QUERY_COUNT_ORDERING "SELECT COUNT(*) FROM "HOME_TABLE" WHERE ordering=?"
HAPI int db_count_ordering(int ordering)
{
	stmt_h *st;
	int count;

	retv_if(W_HOME_ERROR_NONE !=
				db_open(db_info.db_file), -1);

	st = db_prepare(QUERY_COUNT_ORDERING);
	retv_if(st == NULL, -1);

	if (db_bind_int(st, 1, ordering) != W_HOME_ERROR_NONE) {
		_E("db_bind_str error");
		db_finalize(st);
		return -1;
	}

	if (db_next(st) == W_HOME_ERROR_FAIL) {
		_E("db_next error");
		db_finalize(st);
		return -1;
	}

	count = db_get_int(st, 0);
	db_finalize(st);

	/* keep the home DB opened */

	return count;
}



#define QUERY_SELECT_PAGE "SELECT id, subid, ordering FROM "HOME_TABLE" ORDER BY ordering ASC"
HAPI Eina_List *db_write_list(void)
{
	stmt_h *st = NULL;
	Eina_List *page_info_list = NULL;
	page_info_s *page_info = NULL;
	const char *id;
	const char *subid;

	retv_if(W_HOME_ERROR_NONE !=
				db_open(db_info.db_file), NULL);

	st = db_prepare(QUERY_SELECT_PAGE);
	goto_if(NULL == st, ERROR);

	while (W_HOME_ERROR_NONE == db_next(st)) {
		id = db_get_str(st, 0);
		subid = db_get_str(st, 1);

		page_info = page_info_create(id, subid, WIDGET_VIEWER_EVAS_DEFAULT_PERIOD);
		goto_if(!page_info, ERROR);

		page_info->ordering = db_get_int(st, 2);
		page_info->removable = page_info_is_removable(id);
		page_info_list = eina_list_append(page_info_list, page_info);
	}

	db_finalize(st);

	return page_info_list;

ERROR:
	page_info_list_destroy(page_info_list);
	db_finalize(st);

	return NULL;
}



HAPI w_home_error_e db_read_list(Eina_List *page_info_list)
{
	const char *logging = NULL;
	const Eina_List *l, *ln;
	Eina_Strbuf *strbuf = NULL;
	page_info_s *page_info = NULL;
	int ordering = 0;

	retv_if(!page_info_list, W_HOME_ERROR_INVALID_PARAMETER);

	_W("push all the pages into the DB");

	db_remove_all_item();
	strbuf = eina_strbuf_new();
	retv_if(!strbuf, W_HOME_ERROR_FAIL);

	EINA_LIST_FOREACH_SAFE(page_info_list, l, ln, page_info) {
		continue_if(!page_info);
		if (!page_info->id) continue;

		db_insert_item(page_info->id, page_info->subid, ordering);
		if (ordering) {
			eina_strbuf_append_char(strbuf, ';');
		}
		eina_strbuf_append(strbuf, page_info->id);
		ordering++;
	}

	logging = eina_strbuf_string_get(strbuf);
	if (logging) {
		_SD("Logging : [%s]", logging);
		if (preference_set_string(VCONF_KEY_HOME_LOGGING, logging) != 0) {
			_E("Failed to set %s as %s", VCONF_KEY_HOME_LOGGING, logging);
		}
	}
	eina_strbuf_free(strbuf);

	return W_HOME_ERROR_NONE;
}



// End of file.
