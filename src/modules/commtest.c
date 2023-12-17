/*
 * commtest.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * commtest.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * commtest.c is distributed in the hope that it will be useful, but
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
#include "comm.h"
#include "types.h"
#include "datapipes.h"
#include "datapipe.h"

/** Module name */
#define MODULE_NAME		"commtest"

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, NULL };

static GSList *calls;

static int id;

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

static const Scheme test_scheme =
{
	.scheme = (char*)"test",
	.flags = BACKEND_FLAG_MESSAGE | BACKEND_FLAG_CALL
};

static gboolean call_remote_accept(void *data)
{
	CallProperties *call = data;
	if(call->state == SPHONE_CALL_INVALID) {
		call->state = SPHONE_CALL_DIALING;
		execute_datapipe(&call_new_pipe, call);
		sphone_module_log(LL_DEBUG, "set state diling on %s", call->line_identifier);
		return true;
	} else if(call->state == SPHONE_CALL_DIALING) {
		call->state = SPHONE_CALL_ALERTING;
		execute_datapipe(&call_properties_changed_pipe, call);
		sphone_module_log(LL_DEBUG, "set state alerting on %s", call->line_identifier);
		return true;
	}  else if(call->state == SPHONE_CALL_ALERTING) {
		call->state = SPHONE_CALL_ACTIVE;
		call->awnserd = true;
		call->start_time = time(NULL);
		execute_datapipe(&call_properties_changed_pipe, call);
		sphone_module_log(LL_DEBUG, "set state active on %s", call->line_identifier);
		return false;
	}
	return false;
}

static gboolean mock_incomeing_call(void *data)
{
	CallProperties *call = g_malloc0(sizeof(*call));
	
	call->line_identifier = data;
	call->backend = id;
	call->needs_route = true;
	call->state = SPHONE_CALL_INCOMING;
	call->start_time = time(NULL);
	calls = g_slist_prepend(calls ,call);
	execute_datapipe(&call_new_pipe, call);
	return false;
}

static CallProperties *find_call(const CallProperties *icall)
{
	GSList *element;
	for(element = calls; element; element = element->next) {
		CallProperties *call = element->data;
		if(call_properties_comp(icall, call))
			return call;
	}
	sphone_module_log(LL_WARN, "unable to find call %s", icall->line_identifier);
	return NULL;
}

static void call_dial_trigger(const void *data, void *user_data)
{
	(void)user_data;
	CallProperties *call = call_properties_copy(data);
	
	if(call->backend == id) {
		call->state = SPHONE_CALL_INVALID;
		g_timeout_add_seconds(3, call_remote_accept, call);
		call->needs_route = true;
		call->outbound = true;
		calls = g_slist_prepend(calls ,call);
	}
}

static void call_accept_trigger(const void *data, void *user_data)
{
	(void)user_data;
	const CallProperties *icall = data;
	
	if(icall->backend == id && icall->state == SPHONE_CALL_INCOMING) {
		CallProperties *call = find_call(icall);
		if(call) {
			call->awnserd = true;
			call->state = SPHONE_CALL_ACTIVE;
			call->start_time = time(NULL);
			execute_datapipe(&call_properties_changed_pipe, call);
		}
	}
}

static void call_hold_trigger(const void *data, void *user_data)
{
	(void)user_data;
	(void)data;
	sphone_module_log(LL_WARN, "%s unhandled", __func__);
}

static void call_hangup_trigger(const void *data, void *user_data)
{
	(void)user_data;
	(void)data;
	const CallProperties *icall = data;
	
	if(icall->backend == id) {
		CallProperties *call = find_call(icall);
		if(call) {
			call->state = SPHONE_CALL_DISCONNECTED;
			call->end_time = time(NULL);
			execute_datapipe(&call_properties_changed_pipe, call);
			
			if(call->awnserd) {
				char *line_id = g_strdup(call->line_identifier);
				sphone_module_log(LL_DEBUG, "%s will call back in 10s", line_id);
				g_timeout_add_seconds(10, mock_incomeing_call, line_id);
			}
			calls = g_slist_remove(calls, call);
			call_properties_free(call);
		}
	}
}

static gboolean mock_incomeing_message(void *data)
{
	MessageProperties *msg = data;
	
	msg->time = time(NULL);
	msg->outbound = false;
	sphone_module_log(LL_DEBUG, "Mock received message %s with text \"%s\"", msg->line_identifier, msg->text);
	execute_datapipe(&message_received_pipe, msg);
	message_properties_free(msg);
	return false;
}

static void message_send_trigger(const void *data, void *user_data)
{
	(void)user_data;
	(void)data;
	const MessageProperties *msg = data;
	
	if(msg->backend == id) {
		sphone_module_log(LL_DEBUG, "Mock sending message to %s with text \"%s\"", msg->line_identifier, msg->text);
		g_timeout_add_seconds(2, mock_incomeing_message, message_properties_copy(msg));
	}
	
}

static bool is_not_special(uint32_t codepoint)
{
	if(codepoint >= 0x40)
		return true;
	else
		return false;
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	sphone_module_log(LL_DEBUG, "enabled");


	const Scheme* commtest_schemes[2] =
	{
		&test_scheme,
		NULL
	};
	
	id = sphone_comm_add_backend(MODULE_NAME, commtest_schemes, BACKEND_FLAG_MESSAGE | BACKEND_FLAG_CALL, &is_not_special);

	append_trigger_to_datapipe(&call_dial_pipe, call_dial_trigger, NULL);
	append_trigger_to_datapipe(&call_accept_pipe, call_accept_trigger, NULL);
	append_trigger_to_datapipe(&call_hold_pipe, call_hold_trigger, NULL);
	append_trigger_to_datapipe(&call_hangup_pipe, call_hangup_trigger, NULL);
	append_trigger_to_datapipe(&message_send_pipe, message_send_trigger, NULL);
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	sphone_comm_remove_backend(id);
	remove_trigger_from_datapipe(&call_dial_pipe, call_dial_trigger, NULL);
	remove_trigger_from_datapipe(&call_accept_pipe, call_accept_trigger, NULL);
	remove_trigger_from_datapipe(&call_hold_pipe, call_hold_trigger, NULL);
	remove_trigger_from_datapipe(&call_hangup_pipe, call_hangup_trigger, NULL);
	remove_trigger_from_datapipe(&message_send_pipe, message_send_trigger, NULL);
}
