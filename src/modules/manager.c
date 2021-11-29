#include <glib.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "types.h"
#include "datapipe.h"
#include "datapipes.h"
#include "rtconf.h"
#include "gui.h"

/** Module name */
#define MODULE_NAME		"manager"

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

static GSList *calls;

inline static bool call_state_wants_route(const sphone_call_state_t state)
{
	return state == SPHONE_CALL_ACTIVE || state == SPHONE_CALL_DIALING || state == SPHONE_CALL_ALERTING;
}

static void check_needed_state(void)
{
	bool incall = false;
	bool incall_no_route = false;
	bool incomeing_call = false;

	bool playing = false;
	sphone_audio_route_t route = datapipe_get_last_data_int(&audio_route_pipe);
	sphone_call_mode_t mode = datapipe_get_last_data_int(&call_mode_pipe);

	execute_datapipe(&audio_playing_pipe, &playing);

	for(GSList *element = calls; element; element = element->next) {
		CallProperties *call = element->data;
		sphone_module_log(LL_DEBUG, "%s: call %s state %s", __func__, call->line_identifier, sphone_get_state_string(call->state));
		if(call->state == SPHONE_CALL_INCOMING)
			incomeing_call = true;
		else  if(call_state_wants_route(call->state) && call->needs_route)
			incall = true;
		else if(call_state_wants_route(call->state))
			incall_no_route = true;
	}
	
	sphone_module_log(LL_DEBUG, "%s: incall %s, incall_no_route %s", __func__, incall ? "true" : "false", incall_no_route ? "true" : "false");
	
	if(incall || incall_no_route) {
		if(mode != SPHONE_MODE_INCALL) {
			execute_datapipe(&call_mode_pipe, GINT_TO_POINTER(incall ? SPHONE_MODE_INCALL : SPHONE_MODE_INCALL_NO_ROUTE));
			if(route != SPHONE_AUDIO_ROUTE_HEADSET)
				execute_datapipe(&audio_route_pipe, GINT_TO_POINTER(SPHONE_AUDIO_ROUTE_HANDSET));
		}
		if(playing)
			execute_datapipe(&audio_stop_pipe, NULL);
		execute_datapipe(&vibrate_pipe, GINT_TO_POINTER(SPHONE_VIBRATE_STOP));
	} else if(incomeing_call) {
		execute_datapipe(&call_mode_pipe, GINT_TO_POINTER(SPHONE_MODE_RINGING));
		if(rtconf_vibration_enabled())
			execute_datapipe(&vibrate_pipe, GINT_TO_POINTER(SPHONE_VIBRATE_CALL));
		if(rtconf_ringer_enabled()) {
			if(route != SPHONE_AUDIO_ROUTE_HEADSET)
				execute_datapipe(&audio_route_pipe, GINT_TO_POINTER(SPHONE_AUDIO_ROUTE_SPEAKER));
			char *path=rtconf_call_sound_path();
			if(path) {
				execute_datapipe(&audio_play_looping_pipe, path);
				g_free(path);
			}
		}
	} else {
		execute_datapipe(&call_mode_pipe, GINT_TO_POINTER(SPHONE_MODE_NO_CALL));
		if(route == SPHONE_AUDIO_ROUTE_HANDSET)
				execute_datapipe(&audio_route_pipe, GINT_TO_POINTER(SPHONE_AUDIO_ROUTE_SPEAKER));
		execute_datapipe(&vibrate_pipe, GINT_TO_POINTER(SPHONE_VIBRATE_STOP));
		if(playing)
			execute_datapipe(&audio_stop_pipe, NULL);
	}
}

static void call_new_trigger(const void *data, void *user_data)
{
	const CallProperties *call = (const CallProperties*)data;
	(void)user_data;
	
	calls = g_slist_prepend(calls, call_properties_copy(call));
	check_needed_state();
}

static void call_changed_trigger(const void *data, void *user_data)
{
	(void)user_data;
	const CallProperties *icall = (const CallProperties*)data;
	GSList *element;
	for(element = calls; element; element = element->next) {
		CallProperties *call = element->data;
		if(call_properties_comp(icall, call)) {
			call->state = icall->state;
			if(call->state == SPHONE_CALL_DISCONNECTED) {
				call_properties_free(call);
				calls = g_slist_remove(calls, call);
			}
			break;
		}
	}
	check_needed_state();
}

static void message_recived_trigger(const void *data, void *user_data)
{
	(void)user_data;
	const MessageProperties *msg = data;
	
	Contact contact = {0};
	contact.line_identifier = msg->line_identifier;
	contact.backend = msg->backend;
	if(gui_contact_shown(&contact))
		return;

	bool playing = false;
	execute_datapipe(&audio_playing_pipe, &playing);
	
	if(!playing && rtconf_ringer_enabled()) {
		char *path = rtconf_sms_sound_path();
		if(path) {
			execute_datapipe(&audio_play_once_pipe, path);
			g_free(path);
		}
	}

	if(rtconf_vibration_enabled())
		execute_datapipe(&vibrate_pipe, GINT_TO_POINTER(SPHONE_VIBRATE_MESSAGE));
}

G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
	append_trigger_to_datapipe(&call_new_pipe, call_new_trigger, NULL);
	append_trigger_to_datapipe(&call_properties_changed_pipe, call_changed_trigger, NULL);
	append_trigger_to_datapipe(&message_recived_pipe, message_recived_trigger, NULL);
	return NULL;
}

G_MODULE_EXPORT void g_module_unload(GModule *module);
void g_module_unload(GModule *module)
{
	(void)module;
	remove_trigger_from_datapipe(&call_new_pipe, call_new_trigger, NULL);
	remove_trigger_from_datapipe(&call_properties_changed_pipe, call_changed_trigger, NULL);
}
