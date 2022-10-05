#include <glib.h>
#include <libnotify/notify.h>
#include "types.h"
#include "sphone-modules.h"
#include "datapipe.h"
#include "datapipes.h"
#include "sphone-log.h"
#include "gui.h"

/** Module name */
#define MODULE_NAME		"notify-libnotify"

/** Functionality provided by this module */
static const gchar *const provides[] = { "notify", NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

static void notificaion_closed_cb(NotifyNotification *notification, gpointer user_data)
{
	(void)user_data;
	int reason = notify_notification_get_closed_reason(notification);
	sphone_module_log(LL_DEBUG, "notificaion closed with reason %i", reason);
}

static void notificaion_reply_cb(NotifyNotification *notification, char *action, gpointer user_data)
{
	(void)user_data;
	if(g_strcmp0(action, "default") != 0) {
		sphone_module_log(LL_WARN, "unkown action in %s", __func__);
		return;
	}

	sphone_module_log(LL_DEBUG, "action %s in %s", action, __func__);

	MessageProperties *msg = g_object_get_data(G_OBJECT(notification), "message-proparties");
	g_free(msg->text);
	msg->text = NULL;
	gui_sms_send_show(msg);
}

static void message_received_trigger(gconstpointer data, gpointer user_data)
{
	const MessageProperties *message = (const MessageProperties*)data;
	(void)user_data;

	Contact *contact;
	if(message->contact) {
		contact = contact_copy(message->contact);
	} else {
		contact = g_malloc0(sizeof(*contact));
		contact->line_identifier = message->line_identifier;
		contact->backend = message->backend;
	}

	if(gui_contact_shown(contact)) {
		contact_free(contact);
		return;
	}

	MessageProperties *message_copy = message_properties_copy(message);
	NotifyNotification *notification =
		notify_notification_new(contact->name ? contact->name : contact->line_identifier, message_copy->text, NULL);
	notify_notification_set_category(notification, "im.received");
	notify_notification_add_action(notification, "default", "Reply", notificaion_reply_cb, NULL, NULL);
	g_object_set_data_full(G_OBJECT(notification), "message-proparties",
						   message_copy, (void (*)(void *))message_properties_free);
	g_signal_connect(G_OBJECT(notification), "closed", G_CALLBACK(notificaion_closed_cb), NULL);

	GError *error = NULL;
	if(!notify_notification_show(notification, &error)) {
		sphone_module_log(LL_WARN, "failed send notificaion to desktop notification server: %s", error->message);
		g_error_free(error);
	}

	contact_free(contact);
}

static void notificaion_call_back_cb(NotifyNotification *notification, char *action, gpointer user_data)
{
	(void)user_data;
	if(g_strcmp0(action, "default") != 0) {
		sphone_module_log(LL_WARN, "unkown action %s in %s", action, __func__);
		return;
	}

	sphone_module_log(LL_DEBUG, "action %s in %s", action, __func__);

	CallProperties *call = g_object_get_data(G_OBJECT(notification), "call-proparties");
	gui_dialer_show(call);
}

static void call_properties_changed_trigger(const void *data, void *user_data)
{
	(void)user_data;
	const CallProperties *call = data;

	sphone_module_log(LL_DEBUG, "%s, %i %i %i",  __func__, call->state != SPHONE_CALL_DISCONNECTED, call->outbound, call->awnserd);

	if(call->state != SPHONE_CALL_DISCONNECTED || call->outbound || call->awnserd)
		return;

	CallProperties *call_copy = call_properties_copy(call);

	NotifyNotification *notification =
		notify_notification_new("Missed call", (call->contact && call->contact->name) ? call->contact->name : call->line_identifier, NULL);
	notify_notification_set_category(notification, "im");
	notify_notification_add_action(notification, "default", "Call Back", notificaion_call_back_cb, NULL, NULL);
	g_object_set_data_full(G_OBJECT(notification), "call-proparties",
						   call_copy, (void (*)(void *))call_properties_free);
	g_signal_connect(G_OBJECT(notification), "closed", G_CALLBACK(notificaion_closed_cb), NULL);

	GError *error = NULL;
	if(!notify_notification_show(notification, &error)) {
		sphone_module_log(LL_WARN, "failed send notificaion to desktop notification server: %s", error->message);
		g_error_free(error);
	}
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	if(!notify_init("sphone"))
		return "Failed to init libnotify";
	append_trigger_to_datapipe(&message_received_pipe, message_received_trigger, NULL);
	append_trigger_to_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger, NULL);

	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	remove_trigger_from_datapipe(&message_received_pipe, message_received_trigger, NULL);
	remove_trigger_from_datapipe(&call_properties_changed_pipe, call_properties_changed_trigger, NULL);
	if(notify_is_initted())
		notify_uninit();
}
