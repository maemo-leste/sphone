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
#include "sphone-modules.h"
#include "sphone-log.h"
#include "comm.h"
#include "datapipe.h"
#include "datapipes.h"
#include "types.h"

/** Module name */
#define MODULE_NAME		"comm-ofono"

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};

static const Scheme call_scheme =
{
	.scheme = (char*)"tel",
	.flags = BACKEND_FLAG_CALL | BACKEND_FLAG_CELLULAR
};

static const Scheme sms_scheme =
{
	.scheme = (char*)"sms",
	.flags = BACKEND_FLAG_MESSAGE | BACKEND_FLAG_CELLULAR
};

enum {
	NEW_CALL_HANDLE_ID = 0,
	END_CALL_HANDLE_ID,
	NEW_SMS_HANDLE_ID,
	NETWORK_HANDLE_ID,
	HANDLE_ID_COUNT
};

struct ofono_if_priv_s {
	GDBusConnection *s_bus_conn;
	gchar *modem;
	unsigned int ofono_service_watcher;
	int backend_id;
	int callback_ids[HANDLE_ID_COUNT];
	GSList *call_prop_sig_ids;
	GSList *calls;
};

struct call_watcher {
	char *path;
	gint id;
};

static GDBusConnection *get_dbus_connection(void)
{
	GError *error = NULL;
	char *addr;
	
	GDBusConnection *s_bus_conn;

	#if !GLIB_CHECK_VERSION(2,35,0)
	g_type_init();
	#endif

	addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (addr == NULL) {
		sphone_module_log(LL_ERR, "failed to get dbus addr: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	s_bus_conn = g_dbus_connection_new_for_address_sync(addr,
			G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
			G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
			NULL, NULL, &error);

	if (s_bus_conn == NULL) {
		sphone_module_log(LL_ERR, "failed to create dbus connection: %s", error->message);
		g_error_free(error);
	}

	return s_bus_conn;
}

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

static bool ofono_init_valid(struct ofono_if_priv_s *priv)
{
	return priv->s_bus_conn && priv->modem;
}

// Print the dbus error message and free the error object
static void error_dbus(GError *gerror)
{
	if(!gerror)
		return;
	
	sphone_module_log(LL_ERR, "Error: %s", gerror->message);
	g_error_free(gerror);
}

static struct str_list *ofono_get_modems(GDBusConnection *s_bus_conn)
{
	struct str_list *modems;
	GError *gerror = NULL;
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
			(GDBusCallFlags)G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &gerror);

	if (var_resp == NULL) {
		sphone_module_log(LL_ERR, "dbus call failed (%s)", gerror->message);
		g_error_free(gerror);
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

static void ofono_service_appeard(GDBusConnection *connection, const gchar *name,
									const gchar *name_owner, gpointer user_data)
{
	(void)connection;
	(void)name;
	(void)name_owner;
	(void)user_data;

	struct ofono_if_priv_s *private = (struct ofono_if_priv_s*)user_data;
	struct str_list *modems = ofono_get_modems(private->s_bus_conn);
	
	sphone_module_log(LL_DEBUG, "Ofono has appeard.");
		
	if (modems->count <= 0) {
		sphone_module_log(LL_DEBUG, "There is no modem.");
		g_free(modems);
		return;
	}
	private->modem = g_strdup(modems->data[0]);
	sphone_module_log(LL_DEBUG, "Using modem: %s", private->modem);
	g_free(modems);
	
	private->callback_ids[NEW_CALL_HANDLE_ID] = g_dbus_connection_signal_subscribe(
		private->s_bus_conn,
		OFONO_SERVICE,
		OFONO_VOICECALL_MANAGER_IFACE,
		"CallAdded",
		private->modem,
		NULL,
		G_DBUS_SIGNAL_FLAGS_NONE,
		call_added_cb,
		private,
		NULL);
		
	private->callback_ids[NEW_SMS_HANDLE_ID] = g_dbus_connection_signal_subscribe(
		private->s_bus_conn,
		OFONO_SERVICE,
		OFONO_MESSAGE_MANAGER_IFACE,
		"IncomingMessage",
		private->modem,
		NULL,
		G_DBUS_SIGNAL_FLAGS_NONE,
		new_sms_cb,
		private,
		NULL);
}

static void ofono_service_vanished(GDBusConnection *connection, const gchar *name, 
								   gpointer user_data)
{
	(void)connection;
	(void)name;

	struct ofono_if_priv_s *private = (struct ofono_if_priv_s*)user_data;

	sphone_module_log(LL_DEBUG, "Ofono has vanished.");

	g_dbus_connection_signal_unsubscribe(private->s_bus_conn, private->callback_ids[NEW_CALL_HANDLE_ID]);
	g_dbus_connection_signal_unsubscribe(private->s_bus_conn, private->callback_ids[NEW_SMS_HANDLE_ID]);
	g_free(private->modem);
	private->modem = NULL;
}

static sphone_call_state_t ofono_string_to_call_state(const gchar *state)
{
	if(g_strcmp0(state, "active") == 0)
		return SPHONE_CALL_ACTIVE;
	else if(g_strcmp0(state, "held") == 0)
		return SPHONE_CALL_HELD;
	else if(g_strcmp0(state, "dialing") == 0)
		return SPHONE_CALL_DIALING;
	else if(g_strcmp0(state, "alerting") == 0)
		return SPHONE_CALL_ALERTING;
	else if(g_strcmp0(state, "incoming") == 0)
		return SPHONE_CALL_INCOMING;
	else if(g_strcmp0(state, "waiting") == 0)
		return SPHONE_CALL_WATING;
	else if(g_strcmp0(state, "disconnected") == 0)
		return SPHONE_CALL_DISCONNECTED;
	sphone_module_log(LL_WARN, "Got invalid call state %s", state);
	return SPHONE_CALL_INVALID;
}

static void ofono_voice_call_decode_properties(CallProperties *call, GVariantIter *iter_val, const char *path)
{
	char *key;
	GVariant *val;
	
	call->backend_data = g_strdup(path);
	while (g_variant_iter_loop(iter_val, "{sv}", &key, &val)) {
		if (g_strcmp0(key, "LineIdentification") == 0)
			call->line_identifier = g_variant_dup_string(val, NULL);
		else if (g_strcmp0(key, "State") == 0)
			call->state = ofono_string_to_call_state(g_variant_get_string(val, NULL));
		else if (g_strcmp0(key, "Emergency") == 0)
			g_variant_get(val, "b", call->emergency);
	}

	sphone_module_log(LL_DEBUG, "call: %s, state: %s, line identification %s, emergency: %i", 
		call->backend_data, sphone_get_state_string(call->state), call->line_identifier, call->emergency);
}

static void ofono_voice_call_properties_remove_handler(struct ofono_if_priv_s *priv, const gchar *path)
{
	if(!ofono_init_valid(priv))
		return;
	sphone_module_log(LL_DEBUG, "%s: %s\n", __func__, path);

	GSList *element;
	struct call_watcher *watcher;
	
	for(element = priv->call_prop_sig_ids; element; element = element->next) {
		watcher = element->data;
		if(g_strcmp0(watcher->path, path) == 0) {
			g_dbus_connection_signal_unsubscribe(priv->s_bus_conn, watcher->id);
			priv->call_prop_sig_ids = g_slist_remove(priv->call_prop_sig_ids, element->data);
			g_free(watcher->path);
			g_free(watcher);
			break;
		}
	}
}

static CallProperties *ofono_find_call(struct ofono_if_priv_s *priv, const gchar *object_path)
{
	GSList *element;
	for(element = priv->calls; element; element = element->next) {
		CallProperties *call = element->data;
		if(g_strcmp0(call->backend_data, object_path) == 0)
			return call;
	}
	sphone_module_log(LL_WARN, "%s unable to find call %s", __func__, object_path);
	return NULL;
}

static void call_properties_cb(GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		void *data)
{
	(void)connection;
	(void)sender_name;
	(void)interface_name;
	(void)signal_name;
	
	struct ofono_if_priv_s *priv = data;
	
	sphone_module_log(LL_DEBUG, "%s: %s", __func__, object_path);
	CallProperties *call = ofono_find_call(priv, object_path);

	if(call) {
		gchar *key;
		GVariant *value;
		
		g_variant_get(parameters, "(sv)", &key, &value);
		
		if(g_strcmp0(key, "State") == 0) {
			call->state = ofono_string_to_call_state(g_variant_get_string(value, NULL));
			
			if(call->state == SPHONE_CALL_ACTIVE)
				call->start_time = time(NULL);
			else if(call->state == SPHONE_CALL_DISCONNECTED)
				call->end_time = time(NULL);
			execute_datapipe(&call_properties_changed_pipe, call);

			if(call->state == SPHONE_CALL_DISCONNECTED) {
				priv->calls = g_slist_remove(priv->calls, call);
				ofono_voice_call_properties_remove_handler(priv, object_path);
				call_properties_free(call);
			} 
		}
	}
}

static void ofono_voice_call_properties_add_handler(struct ofono_if_priv_s *priv, const gchar *path)
{
	if(!ofono_init_valid(priv))
		return;
	sphone_module_log(LL_DEBUG, "%s: %s\n", __func__, path);
	
	int ret = g_dbus_connection_signal_subscribe(
		priv->s_bus_conn,
		OFONO_SERVICE,
		OFONO_VOICECALL_IFACE,
		"PropertyChanged",
		path,
		NULL,
		G_DBUS_SIGNAL_FLAGS_NONE,
		call_properties_cb,
		priv,
		NULL);
	
	if(ret >= 0) {
		struct call_watcher *watcher = g_malloc0(sizeof(*watcher));
		watcher->path = g_strdup(path);
		watcher->id = ret;
		priv->call_prop_sig_ids = g_slist_prepend(priv->call_prop_sig_ids, watcher);
	}
}

static void call_added_cb(GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		void *data)
{
	(void)connection;
	(void)sender_name;
	(void)object_path;
	(void)interface_name;
	(void)signal_name;
	
	struct ofono_if_priv_s *priv = data;
	
	CallProperties *call = g_malloc0(sizeof(*call));
	call->backend = priv->backend_id;
	call->needs_route = true;
	call->start_time = time(NULL);
	GVariantIter *info_iter;
	char *path;

	g_variant_get(parameters, "(oa{sv})", &path, &info_iter);
	sphone_module_log(LL_DEBUG, "%s: %s", __func__, path);
	ofono_voice_call_decode_properties(call, info_iter, path);
	
	call->outbound = call->state != SPHONE_CALL_INCOMING;
	
	ofono_voice_call_properties_add_handler(priv, path);

	g_variant_iter_free(info_iter);
	g_free(path);
	execute_datapipe(&call_new_pipe, call);
	priv->calls = g_slist_prepend(priv->calls, call);
}

static void call_hold_trigger(gconstpointer data, gpointer user_data)
{
	const CallProperties *call = (const CallProperties*)data;
	struct ofono_if_priv_s *priv = (struct ofono_if_priv_s*)user_data;
	sphone_module_log(LL_WARN, "TODO: implement %s", __func__);
	(void)call;
	(void)priv;
}

static void call_accept_trigger(gconstpointer data, gpointer user_data)
{
	const CallProperties *icall = (const CallProperties*)data;
	struct ofono_if_priv_s *priv = (struct ofono_if_priv_s*)user_data;

	if(icall->backend == priv->backend_id && ofono_init_valid(priv)) {
		CallProperties *call = ofono_find_call(priv, icall->backend_data);

		if(!call) 
			return;

		GVariant *result;
		GError *gerror = NULL;
		
		result = g_dbus_connection_call_sync(priv->s_bus_conn, OFONO_SERVICE, call->backend_data,
			OFONO_VOICECALL_IFACE, "Answer", NULL, NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &gerror);

		call->answered = true;

		g_variant_unref(result);

		if(gerror) {
			error_dbus(gerror);
			gchar message[] = "Unable to awnser call via ofono";
			execute_datapipe(&call_backend_error_pipe, message);
			return;
		}
	} else if(!ofono_init_valid(priv) && icall->backend == priv->backend_id) {
		gchar message[] = "Ofono is not ready";
		execute_datapipe(&call_backend_error_pipe, message);
	}
}

static void call_hangup_trigger(gconstpointer data, gpointer user_data)
{
	const CallProperties *call = (const CallProperties*)data;
	struct ofono_if_priv_s *priv = (struct ofono_if_priv_s*)user_data;

	if(call->backend == priv->backend_id && ofono_init_valid(priv)) {
		GVariant *result;
		GError *gerror = NULL;
		
		result = g_dbus_connection_call_sync(priv->s_bus_conn, OFONO_SERVICE, call->backend_data,
			OFONO_VOICECALL_IFACE, "Hangup", NULL, NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &gerror);

		if(gerror) {
			error_dbus(gerror);
			gchar message[] = "Unable to hangup via ofono";
			execute_datapipe(&call_backend_error_pipe, message);
			return;
		}

		g_variant_unref(result);
	} else if(!ofono_init_valid(priv) && call->backend == priv->backend_id) {
		gchar message[] = "Ofono is not ready";
		execute_datapipe(&call_backend_error_pipe, message);
	}
}

static void new_sms_cb(GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		void *data)
{
	(void)connection;
	(void)sender_name;
	(void)object_path;
	(void)interface_name;
	(void)signal_name;

	struct ofono_if_priv_s *priv = data;

	GVariantIter *iter;
	GVariant *var;
	char *key;
	
	MessageProperties *message = g_malloc0(sizeof(*message));
	message->backend = priv->backend_id;

	struct tm tm = {0};

	g_variant_get(parameters, "(sa{sv})", &message->text, &iter);
	
	while (g_variant_iter_next(iter, "{sv}", &key, &var)) {
		if (g_strcmp0(key, "Sender") == 0) {
			message->line_identifier = g_strdup(g_variant_get_string(var, NULL));
			if (!message->line_identifier) {
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
			sphone_module_log(LL_DEBUG, "time: %s", time);
			strptime(time, "%Y-%m-%dT%H:%M:%S%z", &tm);
			sphone_module_log(LL_DEBUG, "sms time: %s", time);
		}
		g_variant_unref(var);
		g_free(key);
	}
	
	message->time = mktime(&tm);
	
	if(message->line_identifier && message->text)
		execute_datapipe(&message_received_pipe, message);

	error:
	message_properties_free(message);
	g_variant_iter_free(iter);
}

static void call_dial_trigger(gconstpointer data, gpointer user_data)
{
	const CallProperties *call = (const CallProperties*)data;
	struct ofono_if_priv_s *priv = (struct ofono_if_priv_s*)user_data;
	
	if(call->backend == priv->backend_id && ofono_init_valid(priv)) {
		sphone_module_log(LL_DEBUG, "Dialing number: %s", call->line_identifier);
		GVariant *val;
		GVariant *result;
		GError *gerror = NULL;
		
		val = g_variant_new("(ss)", call->line_identifier, "enabled");
		result = g_dbus_connection_call_sync(priv->s_bus_conn, OFONO_SERVICE, priv->modem,
			OFONO_VOICECALL_MANAGER_IFACE, "Dial", val, NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &gerror);

		if(gerror) {
			error_dbus(gerror);
			gchar message[] = "Unable to transmit or dial number via ofono";
			execute_datapipe(&call_backend_error_pipe, message);
			return;
		}
	
		g_variant_unref(result);
	} else if(!ofono_init_valid(priv) && call->backend == priv->backend_id) {
		gchar message[] = "Ofono is not ready";
		execute_datapipe(&call_backend_error_pipe, message);
	}
}

static void message_send_trigger(gconstpointer data, gpointer user_data)
{
	const MessageProperties *message = (const MessageProperties*)data;
	struct ofono_if_priv_s *priv = (struct ofono_if_priv_s*)user_data;
	
	if(message->backend == priv->backend_id && ofono_init_valid(priv)) {
		sphone_module_log(LL_DEBUG, "Sending sms: %s %s", message->line_identifier, message->text);
		GVariant *val;
		GVariant *result;
		GError *gerror = NULL;
		val = g_variant_new("(ss)", message->line_identifier, message->text);
		result = g_dbus_connection_call_sync(priv->s_bus_conn, OFONO_SERVICE, priv->modem,
			OFONO_MESSAGE_MANAGER_IFACE, "SendMessage", val, NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &gerror);

		if(gerror) {
			error_dbus(gerror);
			gchar err_message[] = "Unable to transmit message via ofono";
			execute_datapipe(&call_backend_error_pipe, err_message);
			return;
		}

		g_variant_unref(result);
	} else if(!ofono_init_valid(priv) && message->backend == priv->backend_id) {
		gchar message_text[] = "Ofono is not ready";
		execute_datapipe(&call_backend_error_pipe, message_text);
	}
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{	
	struct ofono_if_priv_s *priv = g_malloc0(sizeof(*priv));
	*data = priv;
	priv->s_bus_conn = get_dbus_connection();
	
	if(!priv->s_bus_conn)
		return "Unable to connect to dbus!";

	const Scheme* schemes[3] =
	{
		&call_scheme,
		&sms_scheme,
		NULL
	};
	
	priv->backend_id = sphone_comm_add_backend("ofono", schemes, BACKEND_FLAG_MESSAGE | BACKEND_FLAG_CALL | BACKEND_FLAG_CELLULAR);

	priv->ofono_service_watcher =
						g_bus_watch_name_on_connection(priv->s_bus_conn, OFONO_SERVICE, 
												G_BUS_NAME_WATCHER_FLAGS_NONE, ofono_service_appeard, 
												ofono_service_vanished, priv, NULL);
						
	append_trigger_to_datapipe(&call_dial_pipe,   call_dial_trigger, priv);
	append_trigger_to_datapipe(&call_accept_pipe, call_accept_trigger, priv);
	append_trigger_to_datapipe(&call_hold_pipe,   call_hold_trigger, priv);
	append_trigger_to_datapipe(&call_hangup_pipe, call_hangup_trigger, priv);
	
	append_trigger_to_datapipe(&message_send_pipe, message_send_trigger, priv);
	
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	struct ofono_if_priv_s *priv = data;
	
	sphone_comm_remove_backend(priv->backend_id);
	
	remove_trigger_from_datapipe(&call_dial_pipe,   call_dial_trigger, priv);
	remove_trigger_from_datapipe(&call_accept_pipe, call_accept_trigger, priv);
	remove_trigger_from_datapipe(&call_hold_pipe,   call_hold_trigger, priv);
	remove_trigger_from_datapipe(&call_hangup_pipe, call_hangup_trigger,priv);
	
	remove_trigger_from_datapipe(&message_send_pipe, message_send_trigger, priv);

	if(!ofono_init_valid(priv))
		return;

	g_dbus_connection_signal_unsubscribe(priv->s_bus_conn, priv->callback_ids[NEW_CALL_HANDLE_ID]);
	g_dbus_connection_signal_unsubscribe(priv->s_bus_conn, priv->callback_ids[NEW_SMS_HANDLE_ID]);
	g_free(priv->modem);
	g_bus_unwatch_name(priv->ofono_service_watcher);
	g_dbus_connection_close_sync(priv->s_bus_conn, NULL, NULL);
	g_free(priv);
}
