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

static void message_recived_trigger(gconstpointer data, gpointer user_data)
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

G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
	append_trigger_to_datapipe(&call_new_pipe, new_call_trigger, NULL);
	append_trigger_to_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger, NULL);
	
	append_trigger_to_datapipe(&message_recived_pipe, message_recived_trigger, NULL);
	append_trigger_to_datapipe(&message_send_pipe, message_send_trigger, NULL);

	return NULL;
}

G_MODULE_EXPORT void g_module_unload(GModule *module);
void g_module_unload(GModule *module)
{
	(void)module;

	remove_trigger_from_datapipe(&call_new_pipe, new_call_trigger);
	remove_trigger_from_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger);

	remove_trigger_from_datapipe(&message_recived_pipe, message_recived_trigger);
	remove_trigger_from_datapipe(&message_send_pipe, message_send_trigger);
}
