/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * 
 * sphone is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * sphone is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <glib.h>
#include <glib-object.h>
#include <sqlite3.h>
#include <string.h>
#include <sys/stat.h>
#include "store.h"
#include "sphone-log.h"

#define STRINGIFY(s) #s

static sqlite3 *sqlitedb = NULL;
static void store_sqlite_error(const char *module)
{
	sphone_log(LL_ERR, "%s error: %s", module, sqlite3_errmsg(sqlitedb));
}

/*
 	return -1: failed, 0: success, no rows, 1: success with row(s)
 */
int store_sql_exec(const gchar * sql, sqlite3_stmt ** ret_stmt, GType t, ...)
{
	int ret = 0;
	sqlite3_stmt *stmt = NULL;
	while (*sql) {
		sphone_log(LL_DEBUG, "store_sql_exec: exec %s", sql);
		if (stmt)
			sqlite3_finalize(stmt);
		stmt = NULL;
		if (sqlite3_prepare_v2(sqlitedb, sql, -1, &stmt, &sql)) {
			store_sqlite_error("store_sql_exec prepare statement");
			goto error;
		}
		while (*sql == ';' || g_ascii_isspace(*sql))
			sql++;

		// fill-in the parameters
		va_list ap;
		va_start(ap, t);
		int i = 1;
		while (t != G_TYPE_INVALID) {
			if (t == G_TYPE_STRING) {
				gchar *value = va_arg(ap, gchar *);
				sphone_log(LL_DEBUG, "store_sql_exec bind string %s", value);
				if (sqlite3_bind_text(stmt, i++, value, -1, SQLITE_STATIC)) {
					store_sqlite_error("store_sql_exec sqlite3_bind_text");
					goto error_va;
				}
			} else if (t == G_TYPE_INT) {
				int value = va_arg(ap, int);
				sphone_log(LL_DEBUG, "store_sql_exec bind int %d", value);
				if (sqlite3_bind_int(stmt, i++, value)) {
					store_sqlite_error("store_sql_exec sqlite3_bind_int");
					goto error_va;
				}
			} else
				sphone_log(LL_ERR, "store_sql_exec sqlite3_bind_text: Invalid parameter type");
			t = va_arg(ap, GType);
		}
 error_va:
		va_end(ap);

		ret = sqlite3_step(stmt);
		if (ret == SQLITE_ROW)
			sphone_log(LL_DEBUG, "store_sql_exec  row returned");
		if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
			store_sqlite_error("store_sql_exec step");
			goto error;
		}
	}

	if (ret_stmt)
		*ret_stmt = stmt;
	else if (stmt)
		sqlite3_finalize(stmt);

	if (ret == SQLITE_ROW)
		sphone_log(LL_DEBUG, "store_sql_exec  row returned2");
	return (ret == SQLITE_ROW ? 1 : 0);
 error:
	if (stmt)
		sqlite3_finalize(stmt);
	return -1;
}

static int store_finalize(void)
{
	return sqlite3_close(sqlitedb);
}

int store_init(void)
{
	int ret = 0;
	gboolean newdb = FALSE;
	sqlite3_stmt *stmt = NULL;
	gchar *dbpath;
	sphone_log(LL_DEBUG, "store_init start");
	dbpath = g_build_filename(g_get_user_config_dir(), "sphone", NULL);
	mkdir(dbpath, S_IREAD | S_IWRITE | S_IEXEC);
	g_free(dbpath);
	dbpath = g_build_filename(g_get_user_config_dir(), "sphone", "store.sqlite", NULL);
	sphone_log(LL_DEBUG, "store_init open DB %s", dbpath);
	ret = sqlite3_open_v2(dbpath, &sqlitedb, SQLITE_OPEN_READWRITE, NULL);
	if (ret == SQLITE_CANTOPEN) {
		if (sqlitedb)
			sqlite3_close(sqlitedb);
		ret = sqlite3_open_v2(dbpath, &sqlitedb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		newdb = TRUE;
		sphone_log(LL_DEBUG, "store_init: create new store %s", dbpath);
	}
	g_free(dbpath);

	if (ret) {
		sphone_log(LL_ERR, "store_init error: %s", sqlite3_errmsg(sqlitedb));
		goto error;
	} else
		sphone_log(LL_DEBUG, "success");

	// Create the DB structure for the newly created DB
	if (newdb) {
		if (store_sql_exec(STRINGIFY(
						 CREATE TABLE "interaction"("id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
									"type" INTEGER NOT NULL,
									"direction" INTEGER NOT NULL,
									"time" TEXT NOT NULL,
									"line_identifier" TEXT NOT NULL,
									"backend" TEXT NOT NULL);
					     CREATE TABLE "interaction_call"("interaction_id" INTEGER NOT NULL,
									     "duration" INTEGER, "status" INTEGER);
					     CREATE TABLE "interaction_sms"("interaction_id" INTEGER NOT NULL,
									    "content" TEXT);)
				   , NULL, G_TYPE_INVALID))
			goto error;
	}

	if (stmt)
		sqlite3_finalize(stmt);

	return 0;
 error:
	if (sqlitedb)
		sqlite3_close(sqlitedb);
	sqlitedb = NULL;
	if (stmt)
		sqlite3_finalize(stmt);
	return 1;
}

// Filter all invalid characters
static char *store_dial_clean(const gchar *dial)
{
	static char store_dial_clean_buf[64];
	int pos = 0;
	for(; *dial; dial++)
		if(((*dial >= '0' && *dial <= '9') || *dial == '*' || *dial == '#') && pos < 19)
			store_dial_clean_buf[pos++] = *dial;

	store_dial_clean_buf[pos] = 0;

	return store_dial_clean_buf;
}

/* Get the id corresponding to a dial, or generate a new
 The matching logic is as following:
 		- Both dials will be cleared from the following characters: <whitespace>, -, (, ), +
		- exact match if length of any is less than 7
		- If shortest number of digits will be matched starting from the right
 */
int store_dial_get_id(const gchar *line_identifier)
{
	int ret = 0;
	char *cleandial = store_dial_clean(line_identifier);

	sphone_log(LL_DEBUG, "store_dial_get_id %s\n", cleandial);

	if(!*cleandial)
		return -1;

	sqlite3_stmt *stmt = NULL;

	// If less than 7 digits: exact match
	if(strlen(cleandial) < 7) {
		ret = store_sql_exec(STRINGIFY(select id from interaction where cleandial = ?)
				     , &stmt, G_TYPE_STRING, cleandial, G_TYPE_INVALID);
		if (ret < 0)
			goto error;
		else if (ret > 0)
			ret = sqlite3_column_int(stmt, 0);
		else
			ret = 0;
		goto done;
	}

	ret = store_sql_exec(STRINGIFY(select id from interaction where
				       length(cleandial) > 6 and
				       substr(cleandial, -min(length(cleandial), length(? 1))) =
				       substr(? 1, -min(length(cleandial), length(? 1)))
			     )
			     , &stmt, G_TYPE_STRING, cleandial, G_TYPE_INVALID);
	if (ret < 0)
		goto error;
	if (ret > 0)
		ret = sqlite3_column_int(stmt, 0);

 done:
	if (stmt)
		sqlite3_finalize(stmt);
	return ret;

 error:
	if (stmt)
		sqlite3_finalize(stmt);
	return -1;
}

int store_add(const store_interaction_struct *inter)
{
	int ret = 0;
	char date_buf[25];
	struct tm *ts;
	
	const char *line_id;
	if(inter->type == SPHONE_VIBRATE_CALL)
		line_id = inter->call->line_identifier;
	else
		line_id = inter->message->line_identifier;

	ts = localtime(&date);
	strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S %Z", ts);
	
	CommBackend* backend =
		sphone_comm_get_backend(inter->type == SPHONE_VIBRATE_CALL ? inter->call->backend : inter->message->backend);
	if(!backend)
		return -1;

	ret = store_sql_exec(STRINGIFY(insert into interaction(type, direction, time, line_identifier, backend)
		values(? 1, ? 2, ? 3, ? 4, ? 5)), NULL, 
		G_TYPE_INT, inter->type,
		G_TYPE_INT, inter->direction,
		G_TYPE_STRING, date_buf,
		G_TYPE_STRING, line_id,
		G_TYPE_STRING, backend->name,
		G_TYPE_INVALID);
	if(ret == 0) {
		if(inter->type == SPHONE_VIBRATE_CALL) {
			ret = store_sql_exec(STRINGIFY(insert into interaction_call(interaction_id, duration, status)
				values(last_insert_rowid(), ? 1, ? 2)), NULL,
				G_TYPE_INT, duration,
				G_TYPE_INT, inter->call->missed,
				G_TYPE_INVALID);
		} else {
			ret = store_sql_exec(STRINGIFY(insert into interaction_sms(interaction_id, content)
				values(last_insert_rowid(), ? 1)), NULL,
				G_TYPE_STRING, inter->message->text,
				G_TYPE_INVALID);
		}
	} 
	return ret;
}

int store_contact_add_dial(gint contactid, const gchar * dial)
{
	int ret = 0;
	int dialid = store_dial_get_id(dial);
	if (dialid < 1 || contactid < 1)
		return -1;

	store_contact_dial_delete(dialid);

	ret = store_sql_exec(STRINGIFY(insert into contact_dial(contact_id, dial_id)
				       values(? 1, ? 2)
			     )
			     , NULL, G_TYPE_INT, contactid, G_TYPE_INT, dialid, G_TYPE_INVALID);

	return ret <= 0 ? ret : sqlite3_last_insert_rowid(sqlitedb);
}

gchar *store_interaction_sms_get_by_id(gint id)
{
	gchar *ret = NULL;

	sqlite3_stmt *stmt = NULL;

	if (store_sql_exec(STRINGIFY(select content from interaction_sms where interaction_id = ?)
			   , &stmt, G_TYPE_INT, id, G_TYPE_INVALID) > 0)
		ret = g_strdup((const gchar *)sqlite3_column_text(stmt, 0));

	if (stmt)
		sqlite3_finalize(stmt);
	return ret;
}

int store_bulk_transaction_start(void)
{
	return store_sql_exec(STRINGIFY(PRAGMA journal_mode = MEMORY; BEGIN TRANSACTION)
			      , NULL, G_TYPE_INVALID);
}

int store_transaction_commit(void)
{
	return store_sql_exec("COMMIT", NULL, G_TYPE_INVALID);
}

int store_transaction_rollback(void)
{
	return store_sql_exec("ROLLBACK", NULL, G_TYPE_INVALID);
}
