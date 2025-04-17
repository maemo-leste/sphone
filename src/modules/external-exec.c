/*
 * external-exec.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * external-exec.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * external-exec.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include "sphone-modules.h"
#include "sphone-conf.h"
#include "datapipes.h"
#include "datapipe.h"
#include "types.h"
#include "comm.h"

/** Module name */
#define MODULE_NAME		"external-exec"

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, NULL };

/** Module information */
SPHONE_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};


struct exec_priv {
	char *incoming_call_cmd;
	char *outgoing_call_cmd;
	char *call_answered_cmd;
	char *call_missed_cmd;
	char *message_sent_cmd;
	char *message_received_cmd;
};

static void new_call_trigger(gconstpointer data, gpointer user_data)
{
	struct exec_priv *priv = user_data;
	const CallProperties *call = data;
	if(call->state == SPHONE_CALL_INCOMING) {
		if(priv->incoming_call_cmd) {
			char *argv[] = {priv->incoming_call_cmd, call->line_identifier, sphone_comm_get_backend(call->backend)->name, NULL};
			g_spawn_async(NULL, argv, NULL, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL);
		}
	}
}

static void call_properties_changed_trigger(gconstpointer data, gpointer user_data)
{
	struct exec_priv *priv = user_data;
	const CallProperties *call = data;
	char *command = NULL;
	if(call->state == SPHONE_CALL_DIALING)
		command = priv->outgoing_call_cmd;
	else if(call->state == SPHONE_CALL_ACTIVE)
		command = priv->call_answered_cmd;
	else if(call->state == SPHONE_CALL_DISCONNECTED && !call->answered)
		command = priv->call_missed_cmd;

	if(command) {
		char *argv[] = {command, call->line_identifier, sphone_comm_get_backend(call->backend)->name, NULL};
		g_spawn_async(NULL, argv, NULL, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL);
	}
}

static void message_send_trigger(gconstpointer data, gpointer user_data)
{
	struct exec_priv *priv = user_data;
	const MessageProperties *msg = data;
	if(priv->message_sent_cmd) {
		char *argv[] = {priv->message_sent_cmd, msg->line_identifier, msg->text, sphone_comm_get_backend(msg->backend)->name, NULL};
		g_spawn_async(NULL, argv, NULL, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL);
	}
}

static void message_received_trigger(gconstpointer data, gpointer user_data)
{
	struct exec_priv *priv = user_data;
	const MessageProperties *msg = data;
	if(priv->message_received_cmd) {
		char *argv[] = {priv->message_received_cmd, msg->line_identifier, msg->text, sphone_comm_get_backend(msg->backend)->name, NULL};
		g_spawn_async(NULL, argv, NULL, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL);
	}
}

SPHONE_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	struct exec_priv *priv = calloc(1, sizeof(*priv));
	*data = priv;
	priv->incoming_call_cmd = sphone_conf_get_string("ExternalExec", "IncomingCall", NULL, NULL);
	priv->outgoing_call_cmd = sphone_conf_get_string("ExternalExec", "OutgoingCall", NULL, NULL);
	priv->call_answered_cmd = sphone_conf_get_string("ExternalExec", "CallAnswered", NULL, NULL);
	priv->call_missed_cmd = sphone_conf_get_string("ExternalExec", "CallMissed", NULL, NULL);
	priv->message_sent_cmd = sphone_conf_get_string("ExternalExec", "MessageSent", NULL, NULL);
	priv->message_received_cmd = sphone_conf_get_string("ExternalExec", "MessageReceived", NULL, NULL);

	append_trigger_to_datapipe(&call_new_pipe, new_call_trigger, priv);
	append_trigger_to_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger, priv);
	
	append_trigger_to_datapipe(&message_received_pipe, message_received_trigger, priv);
	append_trigger_to_datapipe(&message_send_pipe, message_send_trigger, priv);

	return NULL;
}

SPHONE_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	struct exec_priv *priv = data;

	remove_trigger_from_datapipe(&call_new_pipe, new_call_trigger, priv);
	remove_trigger_from_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger, priv);

	remove_trigger_from_datapipe(&message_received_pipe, message_received_trigger, priv);
	remove_trigger_from_datapipe(&message_send_pipe, message_send_trigger, priv);

	g_free(priv->incoming_call_cmd);
	g_free(priv->outgoing_call_cmd);
	g_free(priv->call_answered_cmd);
	g_free(priv->call_missed_cmd);
	g_free(priv->message_sent_cmd);
	g_free(priv->message_received_cmd);
	g_free(priv);
}
