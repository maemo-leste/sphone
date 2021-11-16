#include <glib.h>
#include <time.h>
#include <rtcom-eventlogger/eventlogger.h>
#include <rtcom-eventlogger/eventlogger-query.h>
#include <stdbool.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "types.h"
#include "datapipes.h"
#include "datapipe.h"
#include "comm.h"
#include "storage.h"

/** Module name */
#define MODULE_NAME		"store-rtcom"

/** Functionality provided by this module */
static const gchar *const provides[] = { "store", NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};

static RTComEl *evlog;
int id = -1;

static void call_properties_changed_trigger(const void *data, void *user_data)
{
	RTComEl *el = user_data;
	const CallProperties *call = data;

	if(call->state != SPHONE_CALL_DISCONNECTED)
		return;

	CommBackend *backend = sphone_comm_get_backend(call->backend);
	if(!backend)
		return;
	
	RTComElEvent *ev = rtcom_el_event_new();

	RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup("RTCOM_EL_SERVICE_CALL"));

	if(!call->awnserd) {
		RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CALL_MISSED"));
	} else {
		RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CALL"));
	}
	RTCOM_EL_EVENT_SET_FIELD(ev, outgoing, call->outbound);
	RTCOM_EL_EVENT_SET_FIELD(ev, start_time, call->start_time);
	RTCOM_EL_EVENT_SET_FIELD(ev, end_time, time(NULL));
	if(call->contact && call->contact->name)
		RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup(call->contact->name));
	RTCOM_EL_EVENT_SET_FIELD(ev, local_uid, g_strconcat("sphone/", backend->name, "/", call->line_identifier, NULL));
	RTCOM_EL_EVENT_SET_FIELD(ev, remote_uid, g_strdup(call->line_identifier));

	if(rtcom_el_add_event(el, ev, NULL) < 0)
		sphone_module_log(LL_ERR, "failed to add event to rtcom");
	
	rtcom_el_event_free(ev);
}

static RTComElEvent *create_mesage_event(const MessageProperties *msg)
{
	RTComElEvent *ev = rtcom_el_event_new();

	CommBackend *backend = sphone_comm_get_backend(msg->backend);

	if(g_strcmp0(backend->name, "ofono") == 0) {
		RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup("RTCOM_EL_SERVICE_SMS"));
		RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_SMS_MESSAGE"));
	} else {
		RTCOM_EL_EVENT_SET_FIELD(ev, service, g_strdup("RTCOM_EL_SERVICE_CHAT"));
		RTCOM_EL_EVENT_SET_FIELD(ev, event_type,  g_strdup("RTCOM_EL_EVENTTYPE_CHAT_MESSAGE"));
	}

	RTCOM_EL_EVENT_SET_FIELD(ev, start_time, msg->time);
	RTCOM_EL_EVENT_SET_FIELD(ev, end_time, 0);
	if(msg->contact && msg->contact->name)
		RTCOM_EL_EVENT_SET_FIELD(ev, local_name, g_strdup(msg->contact->name));
	RTCOM_EL_EVENT_SET_FIELD(ev, local_uid, g_strconcat("sphone/", backend->name, NULL));
	RTCOM_EL_EVENT_SET_FIELD(ev, remote_uid, g_strdup(msg->line_identifier));
	RTCOM_EL_EVENT_SET_FIELD (ev, free_text, g_strdup(msg->text));

	return ev;
}

static void message_recived_trigger(const void *data, void *user_data)
{
	RTComEl *el = user_data;
	const MessageProperties *msg = data;

	CommBackend *backend = sphone_comm_get_backend(msg->backend);
	if(!backend)
		return;

	RTComElEvent *ev = create_mesage_event(msg);
	RTCOM_EL_EVENT_SET_FIELD(ev, outgoing, false);

	if(rtcom_el_add_event(el, ev, NULL) < 0)
		sphone_module_log(LL_ERR, "failed to add event to rtcom");
	
	rtcom_el_event_free(ev);
}

static void message_send_trigger(const void *data, void *user_data)
{
	RTComEl *el = user_data;
	const MessageProperties *msg = data;

	CommBackend *backend = sphone_comm_get_backend(msg->backend);
	if(!backend)
		return;

	RTComElEvent *ev = create_mesage_event(msg);
	RTCOM_EL_EVENT_SET_FIELD(ev, outgoing, true);

	if(rtcom_el_add_event(el, ev, NULL) < 0)
		sphone_module_log(LL_ERR, "failed to add event to rtcom");

	rtcom_el_event_free(ev);
}

static GList *get_messages_for_contact(const Contact *contact)
{
	if(!evlog)
		return NULL;

	RTComElQuery* querySms = rtcom_el_query_new(evlog);
	RTComElQuery* queryChat = rtcom_el_query_new(evlog);

	if(!contact) {
		if(!rtcom_el_query_prepare(querySms, "service-id", rtcom_el_get_service_id(evlog, "RTCOM_EL_SERVICE_SMS"), RTCOM_EL_OP_EQUAL,
			                       "event-type-id", rtcom_el_get_eventtype_id(evlog, "RTCOM_EL_EVENTTYPE_SMS_MESSAGE"), RTCOM_EL_OP_EQUAL,
			                       NULL))
			return NULL;
		if(!rtcom_el_query_prepare(queryChat, "service-id", rtcom_el_get_service_id(evlog, "RTCOM_EL_SERVICE_CHAT"), RTCOM_EL_OP_EQUAL,
			                       "event-type-id", rtcom_el_get_eventtype_id(evlog, "RTCOM_EL_EVENTTYPE_CHAT_MESSAGE"), RTCOM_EL_OP_EQUAL,
			                       NULL))
			return NULL;
	} else {
		CommBackend *backend = sphone_comm_get_backend(contact->backend);

		if(!contact->line_identifier)
			return NULL;
		if(!backend)
			return NULL;

		char *localid = g_strconcat("sphone/", backend->name, NULL);
		if(!rtcom_el_query_prepare(querySms, "service-id", rtcom_el_get_service_id(evlog, "RTCOM_EL_SERVICE_SMS"), RTCOM_EL_OP_EQUAL,
			                       "event-type-id", rtcom_el_get_eventtype_id(evlog, "RTCOM_EL_EVENTTYPE_SMS_MESSAGE"), RTCOM_EL_OP_EQUAL,
			                       "local-uid", localid, RTCOM_EL_OP_EQUAL, "remote-uid", contact->line_identifier, RTCOM_EL_OP_EQUAL, 
			                       NULL))
			return NULL;
		if(!rtcom_el_query_prepare(queryChat, "service-id", rtcom_el_get_service_id(evlog, "RTCOM_EL_SERVICE_CHAT"), RTCOM_EL_OP_EQUAL,
			                       "event-type-id", rtcom_el_get_eventtype_id(evlog, "RTCOM_EL_EVENTTYPE_CHAT_MESSAGE"), RTCOM_EL_OP_EQUAL,
			                       "local-uid", localid, RTCOM_EL_OP_EQUAL, "remote-uid", contact->line_identifier, RTCOM_EL_OP_EQUAL, 
			                       NULL))
			return NULL;
	}

	RTComElIter *iterSms = rtcom_el_get_events(evlog, querySms);
	RTComElIter *iterChat = rtcom_el_get_events(evlog, queryChat);
	if(!iterSms && !iterChat)
		return NULL;

	GList *messages = NULL;

	if(iterSms) {
		do {
			char *line_identifier;
			char *local_uid;
			char *text;
			gboolean outbound;
			MessageProperties *msg = g_malloc0(sizeof(*msg));
			if(!rtcom_el_iter_get_values(iterSms, "local-uid", &local_uid,
											"remote-uid", &line_identifier,
											"outgoing", &outbound,
											"start-time", &msg->time,
											"free-text", &text, NULL)) {
				sphone_module_log(LL_ERR, "Failed to access event by iterator");
				g_free(msg);
				continue;
			}

			msg->line_identifier = g_strdup(line_identifier);
			msg->text = g_strdup(text);
			msg->outbound = outbound;

			char **tokens = g_strsplit_set(local_uid, "/", 0);

			if(tokens && tokens[1] && g_strcmp0(tokens[0], "sphone") == 0) {
				msg->backend = sphone_comm_find_backend_id(tokens[1]);
			} else {
				g_strfreev(tokens);
				message_properties_free(msg);
				continue;
			}
			g_strfreev(tokens);
			execute_datapipe_filters(&message_recived_pipe, msg);
			messages = g_list_append(messages, msg);
		} while(rtcom_el_iter_next(iterSms));
	}

	if(iterChat) {
		do {
			char *line_identifier;
			char *local_uid;
			char *text;
			gboolean outbound;
			MessageProperties *msg = g_malloc0(sizeof(*msg));
			if(!rtcom_el_iter_get_values(iterChat, "local-uid", &local_uid,
											"remote-uid", &line_identifier,
											"outgoing", &outbound,
											"start-time", &msg->time,
											"free-text", &text, NULL)) {
				sphone_module_log(LL_ERR, "Failed to access event by iterator");
				g_free(msg);
				continue;
			}

			msg->line_identifier = g_strdup(line_identifier);
			msg->text = g_strdup(text);
			msg->outbound = outbound;

			char **tokens = g_strsplit_set(local_uid, "/", 0);

			if(tokens[0] && tokens[1] && g_strcmp0(tokens[0], "sphone") == 0) {
				msg->backend = sphone_comm_find_backend_id(tokens[1]);
			} else {
				g_strfreev(tokens);
				message_properties_free(msg);
				continue;
			}
			g_strfreev(tokens);
			execute_datapipe_filters(&message_recived_pipe, msg);
			messages = g_list_append(messages, msg);
		} while(rtcom_el_iter_next(iterChat));
	}

	g_object_unref(querySms);
	g_object_unref(queryChat);

	return messages;
}

static GList *get_calls_for_contact(const Contact *contact)
{
	if(!evlog)
		return 0;

	RTComElQuery* query = rtcom_el_query_new(evlog);

	if(!contact) {
		if(!rtcom_el_query_prepare(query, "service-id", rtcom_el_get_service_id(evlog, "RTCOM_EL_SERVICE_CALL"), RTCOM_EL_OP_EQUAL, NULL))
			return NULL;
	} else {
		CommBackend *backend = sphone_comm_get_backend(contact->backend);

		if(!contact->line_identifier)
			return NULL;
		if(!backend)
			return NULL;

		char *localid = g_strconcat("sphone/", backend->name, NULL);
		if(!rtcom_el_query_prepare(query, "service-id", rtcom_el_get_service_id(evlog, "RTCOM_EL_SERVICE_CALL"), RTCOM_EL_OP_EQUAL,
			                              "local-uid", localid, RTCOM_EL_OP_EQUAL,
			                              "remote-uid", contact->line_identifier, RTCOM_EL_OP_EQUAL, NULL))
			return NULL;
	}

	RTComElIter *iter = rtcom_el_get_events(evlog, query);
	if(!iter)
		return NULL;

	GList *calls = NULL;

	do {
		char *line_identifier;
		char *local_uid;
		int type;
		gboolean outbound;
		CallProperties *call = g_malloc0(sizeof(*call));

		if(!rtcom_el_iter_get_values(iter, "local-uid", &local_uid,
			                               "remote-uid", &line_identifier,
			                               "outgoing", &outbound,
			                               "start-time", &call->start_time,
			                               "end-time", &call->end_time,
		                                   "event-type-id", &type, NULL)) {
			sphone_module_log(LL_ERR, "Failed to access event by iterator");
			g_free(call);
			continue;
		}
		sphone_module_log(LL_DEBUG, "got line: %s", line_identifier ?: "NULL");
		call->line_identifier = g_strdup(line_identifier);
		call->awnserd = type != rtcom_el_get_eventtype_id(evlog, "RTCOM_EL_EVENTTYPE_CALL_MISSED");
		call->outbound = outbound;
		call->state = SPHONE_CALL_DISCONNECTED;

		char **tokens = g_strsplit_set(local_uid, "/", 0);

		if(tokens[0] && tokens[1] && g_strcmp0(tokens[0], "sphone") == 0)
			call->backend = sphone_comm_find_backend_id(tokens[1]);
		else
			call->backend = -1;
		g_strfreev(tokens);
		execute_datapipe_filters(&call_new_pipe, call);
		calls = g_list_append(calls, call);
	} while(rtcom_el_iter_next(iter));
	
	g_object_unref(query);

	return calls;
}

G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
	evlog = rtcom_el_new();
	if(!evlog)
		return "Unable to connect to rtcom-eventlogger database";

	sphone_module_log(LL_INFO, "Successfully opened rtcom-eventlogger database");

	append_trigger_to_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger, evlog);
	append_trigger_to_datapipe(&message_recived_pipe, message_recived_trigger, evlog);
	append_trigger_to_datapipe(&message_send_pipe, message_send_trigger, evlog);

	id = store_register_backend(get_messages_for_contact, get_calls_for_contact);

	return NULL;
}

G_MODULE_EXPORT void g_module_unload(GModule *module);
void g_module_unload(GModule *module)
{
	(void)module;
	remove_trigger_from_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger, evlog);
	remove_trigger_from_datapipe(&message_recived_pipe, message_recived_trigger, evlog);
	remove_trigger_from_datapipe(&message_send_pipe, message_send_trigger, evlog);
	store_unregister_backend(id);
	g_object_unref(evlog);
}
