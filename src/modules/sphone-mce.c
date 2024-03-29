/*
 * sphone-mce.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * sphone-mce.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * sphone-mce.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <gio/gio.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "datapipes.h"
#include "datapipe.h"
#include "types.h"

/** Module name */
#define MODULE_NAME		"sphone-mce"

/** Functionality provided by this module */
static const gchar *const provides[] = { "vibration", "call-mode",NULL };

/** Module information */
SPHONE_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};

/** MCE D-Bus service */
#define MCE_SERVICE			"com.nokia.mce"
/** MCE D-Bus Request interface */
#define MCE_REQUEST_IF			"com.nokia.mce.request"
/** MCE D-Bus Signal interface */
#define MCE_SIGNAL_IF			"com.nokia.mce.signal"
/** MCE D-Bus Request path */
#define MCE_REQUEST_PATH		"/com/nokia/mce/request"
/** MCE D-Bus Signal path */
#define MCE_SIGNAL_PATH			"/com/nokia/mce/signal"

/** No ongoing call */
#define MCE_CALL_STATE_NONE			"none"
/** Call ringing */
#define MCE_CALL_STATE_RINGING			"ringing"
/** Call on-going */
#define MCE_CALL_STATE_ACTIVE			"active"
/** Normal call */
#define MCE_NORMAL_CALL				"normal"
/** Emergency call  */
#define MCE_EMERGENCY_CALL			"emergency"

static GDBusConnection *s_bus_conn;

static GDBusConnection *get_dbus_connection(void)
{
	GError *error = NULL;
	char *addr;
	
	GDBusConnection *s_bus_conn_l;

	#if !GLIB_CHECK_VERSION(2,35,0)
	g_type_init();
	#endif

	addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (addr == NULL) {
		sphone_module_log(LL_ERR, "fail to get dbus addr: %s", error->message);
		g_free(error);
		return NULL;
	}

	s_bus_conn_l = g_dbus_connection_new_for_address_sync(addr,
			G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
			G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
			NULL, NULL, &error);

	if (s_bus_conn_l == NULL) {
		sphone_module_log(LL_ERR, "fail to create dbus connection: %s", error->message);
		g_free(error);
	}

	return s_bus_conn_l;
}

static void call_mode_trigger(gconstpointer data, gpointer user_data)
{
	(void)user_data;
	sphone_call_mode_t mode = GPOINTER_TO_INT(data);
	GVariant *val = NULL;
	GVariant *result;
	GError *error = NULL;

	if(mode == SPHONE_MODE_NO_CALL) {
		val = g_variant_new("(ss)", MCE_CALL_STATE_NONE, MCE_NORMAL_CALL);
		sphone_module_log(LL_DEBUG, "%s: %s", __func__, MCE_CALL_STATE_NONE);
	}
	else if(mode == SPHONE_MODE_RINGING) {
		val = g_variant_new("(ss)", MCE_CALL_STATE_RINGING, MCE_NORMAL_CALL);
		sphone_module_log(LL_DEBUG, "%s: %s", __func__, MCE_CALL_STATE_RINGING);
	}
	else if(mode == SPHONE_MODE_INCALL || mode == SPHONE_MODE_INCALL_NO_ROUTE) {
		val = g_variant_new("(ss)", MCE_CALL_STATE_ACTIVE, MCE_NORMAL_CALL);
		sphone_module_log(LL_DEBUG, "%s: %s", __func__, MCE_CALL_STATE_ACTIVE);
	}

	if(val) {
		result = g_dbus_connection_call_sync(s_bus_conn, MCE_SERVICE, MCE_REQUEST_PATH,
			MCE_REQUEST_IF, "req_call_state_change", val, NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
		if(result)
			g_variant_unref(result);
	}

	if(error) {
		sphone_module_log(LL_ERR, "g_dbus_connection_call_sync faied with %s", error->message);
		g_error_free(error);
	}
}

static void vibration_trigger(gconstpointer data, gpointer user_data)
{
	(void)user_data;
	sphone_vibrate_type_t type = (sphone_vibrate_type_t)data;
	GVariant *val = NULL;
	GVariant *result = NULL;
	GError *error = NULL;
	
	if(type != SPHONE_VIBRATE_STOP) {
		if(type == SPHONE_VIBRATE_CALL) {
			val = g_variant_new("(s)", "PatternIncomingCall");
		} else if(type == SPHONE_VIBRATE_MESSAGE) {
			val = g_variant_new("(s)", "PatternIncomingMessage");
		}
		
		if(val) {
			result = g_dbus_connection_call_sync(s_bus_conn, MCE_SERVICE, MCE_REQUEST_PATH,
			MCE_REQUEST_IF, "req_vibrator_pattern_activate", val, NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
		}
	} else {
		val = g_variant_new("(s)", "PatternIncomingCall");
		result = g_dbus_connection_call_sync(s_bus_conn, MCE_SERVICE, MCE_REQUEST_PATH,
			MCE_REQUEST_IF, "req_vibrator_pattern_deactivate", val, NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	}

	if(result)
		g_variant_unref(result);

	if(error) {
		sphone_module_log(LL_ERR, "dbus call to mce faied with %s", error->message);
		g_error_free(error);
	}
}

SPHONE_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	s_bus_conn = get_dbus_connection();
	if(!s_bus_conn)
		return "Failed to connect dbus system bus";
	
	append_trigger_to_datapipe(&vibrate_pipe, vibration_trigger, NULL);
	append_trigger_to_datapipe(&call_mode_pipe, call_mode_trigger, NULL);
	
	return NULL;
}

SPHONE_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;

	remove_trigger_from_datapipe(&vibrate_pipe, vibration_trigger, NULL);
	remove_trigger_from_datapipe(&call_mode_pipe, call_mode_trigger, NULL);
}
