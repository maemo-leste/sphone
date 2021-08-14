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


#include <stdbool.h>
#include <gio/gio.h>
#include "ofono-dbus-names.h"
#include "ofono.h"
#include "dbus-marshalers.h"
#include "utils.h"

enum {
	NEW_CALL_HANDEL_ID = 0,
	END_CALL_HANDEL_ID,
	NETWORK_HANDEL_ID,
	HANDEL_ID_COUNT
};

struct{
	GDBusConnection *s_bus_conn;
	gchar *modem;
	int callback_ids[HANDEL_ID_COUNT];
	call_handler_t call_handler;
	void *call_handler_user_data;
} ofono_if_priv;

struct str_list {
  char **data;
  int count;
};

static void call_added_cb(GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		void *data);

static bool ofono_init_valid()
{
	return ofono_if_priv.s_bus_conn && ofono_if_priv.modem;
}

static GDBusConnection *get_dbus_connection()
{
	GError *error = NULL;
	char *addr;
	
	GDBusConnection *s_bus_conn;

	#if !GLIB_CHECK_VERSION(2,35,0)
	g_type_init();
	#endif

	addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (addr == NULL) {
		g_error("fail to get dbus addr: %s\n", error->message);
		g_free(error);
		return NULL;
	}

	s_bus_conn = g_dbus_connection_new_for_address_sync(addr,
			G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
			G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
			NULL, NULL, &error);

	if (s_bus_conn == NULL) {
		g_error("fail to create dbus connection: %s\n", error->message);
		g_free(error);
	}

	return s_bus_conn;
}


// Print the dbus error message and free the error object
static void error_dbus(GError *error)
{
	if(!error)
		return;
	
	g_printerr ("Error: %s\n", error->message);
	g_error_free (error);
}

static struct str_list *ofono_get_modems(GDBusConnection *s_bus_conn)
{
	struct str_list *modems;
	GError *error = NULL;
	GVariant *var_resp, *var_val;
	GVariantIter *iter;
	char *path;
	int i = 0;

	modems = g_malloc(sizeof(struct str_list));
	modems->count = 0;
	modems->data = NULL;

	var_resp = g_dbus_connection_call_sync(s_bus_conn,
			OFONO_SERVICE, OFONO_MANAGER_PATH,
			OFONO_MANAGER_IFACE, "GetModems", NULL, NULL,
			G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

	if (var_resp == NULL) {
		g_error("dbus call failed (%s)", error->message);
		g_error_free(error);
		return modems;
	}

	g_variant_get(var_resp, "(a(oa{sv}))", &iter);
	modems->count = g_variant_iter_n_children(iter);
	modems->data = g_malloc(sizeof(char *) * modems->count);
	while (g_variant_iter_next(iter, "(o@a{sv})", &path, &var_val)) {
		modems->data[i++] = path;
		g_variant_unref(var_val);
	}

	g_variant_iter_free(iter);
	g_variant_unref(var_resp);

	return modems;
}

int ofono_init()
{	
	ofono_if_priv.s_bus_conn = get_dbus_connection();
	
	if(!ofono_if_priv.s_bus_conn)
		return -1;
	
	struct str_list *modems = ofono_get_modems(ofono_if_priv.s_bus_conn);

	if (modems->count <= 0) {
		g_warning("There is no modem");
		g_free(modems);
		return -1;
	}

	if (ofono_if_priv.s_bus_conn == NULL) {
		g_error("Fail to get dbus connection");
		return -1;
	}
	
	ofono_if_priv.modem = g_strdup(modems->data[0]);
	
	g_debug("Using modem: %s", ofono_if_priv.modem);
	
	g_free(modems);
	
	g_dbus_connection_signal_subscribe(ofono_if_priv.s_bus_conn,
		OFONO_SERVICE,
		OFONO_VOICECALL_MANAGER_IFACE,
		"CallAdded",
		ofono_if_priv.modem,
		NULL,
		G_DBUS_SIGNAL_FLAGS_NONE,
		call_added_cb,
		NULL,
		NULL);

	return 0;
}

void ofono_clear()
{
	if(!ofono_init_valid())
		return;

	g_free(ofono_if_priv.modem);
	g_dbus_connection_close_sync(ofono_if_priv.s_bus_conn, NULL, NULL);
}

int ofono_read_network_properties(OfonoNetworkProperties *properties)
{
	g_debug("%s", __func__);
	return 0;
}

void ofono_network_properties_free(OfonoNetworkProperties *properties)
{
	g_debug("%s", __func__);
}

int ofono_network_properties_add_handler(gpointer handler, gpointer data)
{
	g_debug("%s", __func__);
	return 0;
}

void ofono_network_properties_remove_handler(void)
{

}

int ofono_voice_call_add_new_call_handler(call_handler_t handler, gpointer data)
{
	ofono_if_priv.call_handler_user_data = data;
	ofono_if_priv.call_handler = handler; 
	return 0;
}

static void ofono_voice_call_decode_properties(OfonoCallProperties *call, GVariantIter *iter_val, char *path)
{
	char *key;
	GVariant *val;
	
	call->path = g_strdup(path);
	while (g_variant_iter_loop(iter_val, "{sv}", &key, &val)) {
		if (g_strcmp0(key, "LineIdentification") == 0)
			call->line_identifier = g_variant_dup_string(val, NULL);
		else if (g_strcmp0(key, "State") == 0)
			call->state = g_variant_dup_string(val, NULL);
		else if (g_strcmp0(key, "Emergency") == 0)
			g_variant_get(val, "b", call->emergency);
	}

	g_debug("call: %s, state: %s, line identification %s, emergency: %i", 
		call->path, call->state, call->line_identifier, call->emergency);
}

static void call_added_cb(GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		void *data)
{
	g_debug("%s", __func__);
	if(ofono_if_priv.call_handler) {
		(void)data;
		GVariantIter *info_iter;
		char *path;
		OfonoCallProperties *call_info;
		call_info = g_malloc0(sizeof(*call_info));

		g_variant_get(parameters, "(oa{sv})", &path, &info_iter);
		g_debug("%s: %s", __func__, path);
		ofono_voice_call_decode_properties(call_info, info_iter, path);

		g_variant_iter_free(info_iter);
		g_free(path);
		ofono_if_priv.call_handler(call_info, 1, ofono_if_priv.call_handler_user_data);
	}
}

int ofono_voice_call_get_calls(OfonoCallProperties **calls, size_t *count)
{
	if(!ofono_init_valid())
		return -1;
	
	g_debug("%s", __func__);
	GError *error = NULL;
	GVariant *result;
	char *path;

	result = g_dbus_connection_call_sync(ofono_if_priv.s_bus_conn, OFONO_SERVICE,
		ofono_if_priv.modem, OFONO_VOICECALL_MANAGER_IFACE, "GetCalls",
		NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error) {
		error_dbus(error);
		return -1;
	}

	GVariantIter *iter;
	g_variant_get(result, "(a(oa{sv}))", &iter);

	*count = g_variant_iter_n_children(iter);
	if (*count == 0) {
		*calls = NULL;
		g_debug("No call");
		g_variant_iter_free(iter);
		g_variant_unref(result);
		return 0;
	}

	*calls = g_malloc0(sizeof(OfonoCallProperties)*(*count));

	int i = 0;
	GVariantIter *iter_val;
	while (g_variant_iter_loop(iter, "(oa{sv})", &path, &iter_val)) {
		ofono_voice_call_decode_properties(&((*calls)[i]), iter_val, path);
		++i;
	}
	g_variant_iter_free(iter);
	g_variant_unref(result);
	
	return 0;
}

OfonoCallProperties *ofono_call_properties_read(gchar *path)
{
	if(!ofono_init_valid())
		return NULL;

	OfonoCallProperties *properties = g_malloc0(sizeof(*properties));
	g_debug("%s: %s", __func__, path);
	
	GError *error = NULL;
	GVariant *var_properties;
	var_properties = g_dbus_connection_call_sync(ofono_if_priv.s_bus_conn,
		OFONO_SERVICE, path,
		OFONO_VOICECALL_IFACE,
		"GetProperties", NULL, NULL,
		G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);

	if (error) {
		error_dbus(error);
		return NULL;
	}

	GVariantIter *iter;
	g_variant_get(var_properties, "(a{sv})", &iter);
	ofono_voice_call_decode_properties(properties, iter, path);

	return properties;
}

void ofono_call_properties_free(OfonoCallProperties *properties)
{
	g_free(properties->path);
	g_free(properties->line_identifier);
	g_free(properties->state);
	g_free(properties->start_time);
	g_free(properties);
}

int ofono_voice_call_properties_add_handler(gchar *path, gpointer handler, gpointer data)
{
	if(!ofono_init_valid())
		return -1;
	g_debug("%s: %s", __func__, path);
	return 0;
}

int ofono_voice_call_properties_remove_handler(int id)
{
	if(!ofono_init_valid())
		return -1;
	g_debug("%s", __func__);
	return 0;
}

int ofono_call_answer(gchar *path)
{
	if(!ofono_init_valid())
		return -1;

	GVariant *result;
	GError *error = NULL;
	
	result = g_dbus_connection_call_sync(ofono_if_priv.s_bus_conn, OFONO_SERVICE, path,
		OFONO_VOICECALL_IFACE, "Answer", NULL, NULL,
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error) {
		error_dbus(error);
		return -1;
	}

	g_variant_unref(result);
	return 0;
}

int ofono_call_hangup(gchar *path)
{
	if(!ofono_init_valid())
		return -1;

	GVariant *result;
	GError *error = NULL;
	
	result = g_dbus_connection_call_sync(ofono_if_priv.s_bus_conn, OFONO_SERVICE, path,
		OFONO_VOICECALL_IFACE, "Hangup", NULL, NULL,
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error) {
		error_dbus(error);
		return -1;
	}

	g_variant_unref(result);
	return 0;
}

int ofono_call_hold_and_answer()
{
	if(!ofono_init_valid())
		return -1;
	g_debug("%s", __func__);
	return 0;
}

int ofono_call_swap()
{
	if(!ofono_init_valid())
		return -1;
	g_debug("%s", __func__);
	return 0;
}

int ofono_dial(const gchar *dial)
{
	if(!ofono_init_valid())
		return -1;

	GVariant *val;
	GVariant *result;
	GError *error = NULL;

	g_debug("Dialing number: %s", dial);

	val = g_variant_new("(ss)", dial, "enabled");
	result = g_dbus_connection_call_sync(ofono_if_priv.s_bus_conn, OFONO_SERVICE, ofono_if_priv.modem,
		OFONO_VOICECALL_MANAGER_IFACE, "Dial", val, NULL,
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error) {
		error_dbus(error);
		return -1;
	}

	g_variant_unref(result);

	return 0;
}

int ofono_sms_send(const gchar *to, const gchar *text)
{
	if(!ofono_init_valid())
		return -1;

	GVariant *val;
	GVariant *result;
	GError *error = NULL;

	g_debug("Sending sms: %s %s", to, text);

	val = g_variant_new("(ss)", to, text);
	result = g_dbus_connection_call_sync(ofono_if_priv.s_bus_conn, OFONO_SERVICE, ofono_if_priv.modem,
		OFONO_MESSAGE_MANAGER_IFACE, "SendMessage", val, NULL,
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error) {
		error_dbus(error);
		return -1;
	}

	g_variant_unref(result);
	return 0;
}

int ofono_sms_incoming_add_handler(gpointer handler, gpointer data)
{
	return 0;
}

