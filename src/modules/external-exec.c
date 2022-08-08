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
#include "sphone-log.h"
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
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};

static void new_call_trigger(gconstpointer data, gpointer user_data)
{
	(void)user_data;
	const CallProperties *call = data;
	if(call->state == SPHONE_CALL_INCOMING) {
		char *command = sphone_conf_get_string("ExternalExec", "IncomeingCall", NULL, NULL);
		if(command) {
			char *argv[] = {command, call->line_identifier, sphone_comm_get_backend(call->backend)->name, NULL};
			g_spawn_async(NULL, argv, NULL, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL);
			g_free(command);
		}
	}
}

static void call_properties_changed_trigger(gconstpointer data, gpointer user_data)
{
	(void)user_data;
	const CallProperties *call = data;
	char *command = NULL;
	if(call->state == SPHONE_CALL_DIALING)
		command = sphone_conf_get_string("ExternalExec", "OutgoingCall", NULL, NULL);
	else if(call->state == SPHONE_CALL_ACTIVE)
		command = sphone_conf_get_string("ExternalExec", "CallAwnserd", NULL, NULL);
	else if(call->state == SPHONE_CALL_DISCONNECTED && !call->awnserd)
		command = sphone_conf_get_string("ExternalExec", "CallMissed", NULL, NULL);

	if(command) {
		char *argv[] = {command, call->line_identifier, sphone_comm_get_backend(call->backend)->name, NULL};
		g_spawn_async(NULL, argv, NULL, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL);
		g_free(command);
	}
}

static void message_send_trigger(gconstpointer data, gpointer user_data)
{
	(void)user_data;
	const MessageProperties *msg = data;
	char *command = sphone_conf_get_string("ExternalExec", "MessageSent", NULL, NULL);
	if(command) {
		char *argv[] = {command, msg->line_identifier, msg->text, sphone_comm_get_backend(msg->backend)->name, NULL};
		g_spawn_async(NULL, argv, NULL, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL);
		g_free(command);
	}
}

static void message_received_trigger(gconstpointer data, gpointer user_data)
{
	(void)user_data;
	const MessageProperties *msg = data;
	char *command = sphone_conf_get_string("ExternalExec", "MessageRecived", NULL, NULL);
	if(command) {
		char *argv[] = {command, msg->line_identifier, msg->text, sphone_comm_get_backend(msg->backend)->name, NULL};
		g_spawn_async(NULL, argv, NULL, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL);
		g_free(command);
	}
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	append_trigger_to_datapipe(&call_new_pipe, new_call_trigger, NULL);
	append_trigger_to_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger, NULL);
	
	append_trigger_to_datapipe(&message_received_pipe, message_received_trigger, NULL);
	append_trigger_to_datapipe(&message_send_pipe, message_send_trigger, NULL);

	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;

	remove_trigger_from_datapipe(&call_new_pipe, new_call_trigger, NULL);
	remove_trigger_from_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger, NULL);

	remove_trigger_from_datapipe(&message_received_pipe, message_received_trigger, NULL);
	remove_trigger_from_datapipe(&message_send_pipe, message_send_trigger, NULL);
}
