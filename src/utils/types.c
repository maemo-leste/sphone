#include "types.h"

const char *sphone_get_state_string(sphone_call_state_t state)
{
	switch(state) {
		case SPHONE_CALL_ACTIVE:
			return "Active";
		case SPHONE_CALL_HELD:
			return "Held";
		case SPHONE_CALL_DIALING:
			return "Dialing";
		case SPHONE_CALL_ALERTING:
			return "Alerting";
		case SPHONE_CALL_INCOMING:
			return "Incoming";
		case SPHONE_CALL_WATING:
			return "Wating";
		case SPHONE_CALL_DISCONNECTED:
			return "Disconnected";
		default:
			return "Unkown";
	}
}

void contact_free(Contact *contact)
{
	if(!contact)
		return;
	g_free(contact->name);
	if(contact->photo)
		g_object_unref(G_OBJECT(contact->photo));
	g_free(contact);
}

Contact *contact_copy(const Contact *contact)
{
	if(!contact)
		return NULL;
	Contact *new_contact = g_malloc0(sizeof(*new_contact));
	new_contact->name = g_strdup(contact->name);
	if(contact->photo) {
		new_contact->photo = contact->photo;
		g_object_ref(G_OBJECT(contact->photo));
	}
	return new_contact;
}

void call_properties_free(CallProperties *properties)
{
	if(!properties)
		return;
	contact_free(properties->contact);
	g_free(properties->line_identifier);
	g_free(properties->technology);
	g_free(properties->backend_data);
	g_free(properties);
}
bool call_properties_comp(const CallProperties *a, const CallProperties *b)
{
	return a->backend == b->backend && g_strcmp0(a->line_identifier, b->line_identifier) == 0;
}
CallProperties *call_properties_copy(const CallProperties *properties)
{
	if(!properties)
		return NULL;
	CallProperties *new_props = g_malloc0(sizeof(*new_props));
	new_props->contact = contact_copy(properties->contact);
	new_props->line_identifier = g_strdup(properties->line_identifier);
	new_props->technology = g_strdup(properties->technology);
	new_props->backend_data = g_strdup(properties->backend_data);
	new_props->start_time = properties->start_time;
	new_props->awnserd = properties->awnserd;
	new_props->backend = properties->backend;
	new_props->state = properties->state;
	new_props->needs_route = properties->needs_route;
	return new_props;
}


MessageProperties *message_properties_copy(const MessageProperties *properties)
{
	if(!properties)
		return NULL;
	MessageProperties *new_props = g_malloc0(sizeof(*new_props));
	new_props->contact = contact_copy(properties->contact);
	new_props->line_identifier = g_strdup(properties->line_identifier);
	new_props->technology = g_strdup(properties->technology);
	new_props->backend_data = g_strdup(properties->backend_data);
	new_props->time = properties->time;
	new_props->backend = properties->backend;
	return new_props;
}
void message_properties_free(MessageProperties *properties)
{
	contact_free(properties->contact);
	g_free(properties->line_identifier);
	g_free(properties->technology);
	g_free(properties->text);
	g_free(properties->backend_data);
	g_free(properties);
}

void notification_free(Notification *notification)
{
	g_free(notification->title);
	g_free(notification->text);
	g_free(notification);
}
