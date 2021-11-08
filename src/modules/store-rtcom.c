#include <glib.h>
#include <time.h>
#include <rtcom-eventlogger/eventlogger.h>
#include <stdbool.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "types.h"
#include "datapipes.h"
#include "datapipe.h"
#include "comm.h"

/** Module name */
#define MODULE_NAME		"store-rtcom"

/** Functionality provided by this module */
static const gchar *const provides[] = { "store", NULL };

static RTComEl *evlog;

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};

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
	RTCOM_EL_EVENT_SET_FIELD(ev, local_uid, g_strconcat("/sphone/", backend->name, "/", call->line_identifier, NULL));
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
	RTCOM_EL_EVENT_SET_FIELD(ev, local_uid, g_strconcat("/sphone/", backend->name, "/", msg->line_identifier, NULL));
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

G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
	evlog = rtcom_el_new();
	if(!evlog)
		return "Unable to connect to rtcom-eventlogger";

	append_trigger_to_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger, evlog);
	append_trigger_to_datapipe(&message_recived_pipe, message_recived_trigger, evlog);
	append_trigger_to_datapipe(&message_send_pipe, message_send_trigger, evlog);

	return NULL;
}

G_MODULE_EXPORT void g_module_unload(GModule *module);
void g_module_unload(GModule *module)
{
	(void)module;
	remove_trigger_from_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger);
	remove_trigger_from_datapipe(&message_recived_pipe, message_recived_trigger);
	remove_trigger_from_datapipe(&message_send_pipe, message_send_trigger);
	g_object_unref(evlog);
}
