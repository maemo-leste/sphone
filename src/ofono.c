/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
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

#define _XOPEN_SOURCE

#include <stdbool.h>
#include <gio/gio.h>
#include <time.h>
#include "ofono-dbus-names.h"
#include "ofono.h"
#include "dbus-marshalers.h"
#include "utils.h"


enum {
	NEW_CALL_HANDLE_ID = 0,
	END_CALL_HANDLE_ID,
	NEW_SMS_HANDLE_ID,
	NETWORK_HANDLE_ID,
	HANDLE_ID_COUNT
};

struct{
	GDBusConnection *s_bus_conn;
	gchar *modem;
	int callback_ids[HANDLE_ID_COUNT];
	call_handler_t new_call_handler;
	void *new_call_handler_user_data;
	sms_handler_t new_sms_handler;
	void *new_sms_handler_user_data;
	GList *call_handler_list;
} ofono_if_priv;

struct ofono_call_handler_endpoint {
	char *path;
	call_property_handler_t callback;
	void *user_data;
	int id;
};

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
		
static void new_sms_cb(GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		void *data);

static bool ofono_init_valid(void)
{
	return ofono_if_priv.s_bus_conn && ofono_if_priv.modem;
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

int ofono_init(void)
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
	
	ofono_if_priv.callback_ids[NEW_CALL_HANDLE_ID] = g_dbus_connection_signal_subscribe(
		ofono_if_priv.s_bus_conn,
		OFONO_SERVICE,
		OFONO_VOICECALL_MANAGER_IFACE,
		"CallAdded",
		ofono_if_priv.modem,
		NULL,
		G_DBUS_SIGNAL_FLAGS_NONE,
		call_added_cb,
		NULL,
		NULL);
		
	ofono_if_priv.callback_ids[NEW_SMS_HANDLE_ID] = g_dbus_connection_signal_subscribe(
		ofono_if_priv.s_bus_conn,
		OFONO_SERVICE,
		OFONO_MESSAGE_MANAGER_IFACE,
		"IncomingMessage",
		ofono_if_priv.modem,
		NULL,
		G_DBUS_SIGNAL_FLAGS_NONE,
		new_sms_cb,
		NULL,
		NULL);

	return 0;
}

void ofono_clear(void)
{
	if(!ofono_init_valid())
		return;

	g_dbus_connection_signal_unsubscribe(ofono_if_priv.s_bus_conn, ofono_if_priv.callback_ids[NEW_CALL_HANDLE_ID]);
	g_dbus_connection_signal_unsubscribe(ofono_if_priv.s_bus_conn, ofono_if_priv.callback_ids[NEW_SMS_HANDLE_ID]);
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
	ofono_if_priv.new_call_handler_user_data = data;
	ofono_if_priv.new_call_handler = handler; 
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
	if(ofono_if_priv.new_call_handler) {
		(void)data;
		GVariantIter *info_iter;
		char *path;
		OfonoCallProperties *call_info = g_malloc0(sizeof(*call_info));

		g_variant_get(parameters, "(oa{sv})", &path, &info_iter);
		g_debug("%s: %s", __func__, path);
		ofono_voice_call_decode_properties(call_info, info_iter, path);

		g_variant_iter_free(info_iter);
		g_free(path);
		ofono_if_priv.new_call_handler(call_info, 1, ofono_if_priv.new_call_handler_user_data);
		ofono_call_properties_free(call_info);
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

static void call_properties_cb(GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		void *data)
{
	g_debug("%s: %s", __func__, object_path);
	GList *element = ofono_if_priv.call_handler_list;
	do {
		struct ofono_call_handler_endpoint *endpoint = 
			(struct ofono_call_handler_endpoint*)element->data;
		if(g_strcmp0(object_path, endpoint->path) == 0) {
			g_debug("%s: %s matched %p %p", __func__, object_path, endpoint->callback, endpoint->user_data);
  			gchar *key;
  			GVariant *value;
			
  			g_variant_get(parameters, "(sv)", &key, &value);
  			if(g_strcmp0(key, "State") == 0)
  				endpoint->callback(CALL_PROPERTY_STATE, g_variant_get_string(value, NULL), endpoint->user_data);
			
  			g_variant_unref(value);
  			g_free(key);
		}
	} while((element = element->next));
}

int ofono_voice_call_properties_add_handler(gchar *path, call_property_handler_t handler, void *data)
{
	if(!ofono_init_valid())
		return -1;
	g_debug("%s: %s %p %p", __func__, path, handler, data);
	struct ofono_call_handler_endpoint *endpoint = g_malloc(sizeof(*endpoint));
	endpoint->user_data = data;
	endpoint->callback = handler;
	endpoint->path = g_strdup(path);
	
	int ret = g_dbus_connection_signal_subscribe(
		ofono_if_priv.s_bus_conn,
		OFONO_SERVICE,
		OFONO_VOICECALL_IFACE,
		"PropertyChanged",
		endpoint->path,
		NULL,
		G_DBUS_SIGNAL_FLAGS_NONE,
		call_properties_cb,
		NULL,
		NULL);
	
	if(ret >= 0)
	{
		endpoint->id=ret;
		ofono_if_priv.call_handler_list = 
			g_list_append(ofono_if_priv.call_handler_list, endpoint);
	}
	return ret;
}

int ofono_voice_call_properties_remove_handler(int id)
{
	if(!ofono_init_valid())
		return -1;
	
	GList *element = ofono_if_priv.call_handler_list;
	do {
		struct ofono_call_handler_endpoint *endpoint = 
			(struct ofono_call_handler_endpoint*)element->data;
		if(endpoint->id == id) {
			g_debug("%s: %u", __func__, g_list_length(ofono_if_priv.call_handler_list));
			g_free(endpoint->path);
			ofono_if_priv.call_handler_list = g_list_remove(ofono_if_priv.call_handler_list, element);
			g_debug("%s: %i", __func__, g_list_length(ofono_if_priv.call_handler_list));
			break;
		}
	} while((element = element->next));
	
	g_dbus_connection_signal_unsubscribe(ofono_if_priv.s_bus_conn, id);
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

int ofono_call_hold_and_answer(void)
{
	if(!ofono_init_valid())
		return -1;
	g_debug("%s", __func__);
	return 0;
}

int ofono_call_swap(void)
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

static void new_sms_cb(GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		void *data)
{
	GVariantIter *iter;
	GVariant *var;
	char *key;
	char *message = NULL;
	
	const char *from = NULL;
	struct tm tm;

	g_variant_get(parameters, "(sa{sv})", &message, &iter);
	
	while (g_variant_iter_next(iter, "{sv}", &key, &var)) {
		if (g_strcmp0(key, "Sender") == 0) {
			from = g_variant_get_string(var, NULL);
			if (!from) {
				g_variant_unref(var);
				g_free(key);
				goto error;
			}
		}
		else if (g_strcmp0(key, "LocalSentTime") == 0) {
			const char *time = g_variant_get_string(var, NULL);
			if (!time) {
				g_variant_unref(var);
				g_free(key);
				goto error;
			}
			strptime(time,"%Y-%m-%dT%H:%M:%S%z", &tm);
			g_debug("%s: sms time: %s", __func__, time);
		}
		g_variant_unref(var);
		g_free(key);
	}
	
	if(ofono_if_priv.new_sms_handler && from && message) {
		
		time_t time = mktime(&tm);
		ofono_if_priv.new_sms_handler(from, message, time, ofono_if_priv.new_sms_handler_user_data);
	}

	error:
	g_free(message);
	g_variant_iter_free(iter);
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

int ofono_sms_incoming_add_handler(sms_handler_t handler, void *data)
{
	ofono_if_priv.new_sms_handler = handler;
	ofono_if_priv.new_sms_handler_user_data = data;
	return 0;
}

