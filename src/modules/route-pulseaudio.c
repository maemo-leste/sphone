#include <glib.h>
#include <pulse/pulseaudio.h>

#include "sphone-modules.h"
#include "sphone-log.h"
#include "datapipes.h"
#include "datapipe.h"
#include "types.h"

/** Module name */
#define MODULE_NAME		"route-pulseaudio"

/** Functionality provided by this module */
static const gchar *const provides[] = { "route", NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

struct sphone_pa_if {
	pa_threaded_mainloop *mainloop;
	pa_mainloop_api *api;
	pa_context *context;
};

static struct sphone_pa_if pa_if;

static void sphone_pa_distroy_interface(struct sphone_pa_if* iface);

static void sphone_pa_state_callback(pa_context *c, void *userdata)
{
	struct sphone_pa_if *iface = userdata;

	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
			break;
		case PA_CONTEXT_READY:
			sphone_module_log(LL_DEBUG, "Pulse audio context is ready");
			break;
		case PA_CONTEXT_TERMINATED:
			sphone_module_log(LL_DEBUG, "Context terminated: %s", pa_strerror(pa_context_errno(c)));
			sphone_pa_distroy_interface(iface);
			break;
		case PA_CONTEXT_FAILED:
		default:
		sphone_module_log(LL_ERR, "Connection failure: %s", pa_strerror(pa_context_errno(c)));
			sphone_pa_distroy_interface(iface);
	}
}

static void sphone_pa_callback(pa_context *c, int success, void *userdata)
{
	if (!success)
		sphone_module_log(LL_WARN, "failure: %s %s", (const char*)userdata, pa_strerror(pa_context_errno(c)));
	else
		sphone_module_log(LL_DEBUG, "sucess: %s", (const char*)userdata);
}

static int sphone_pa_create_interface(struct sphone_pa_if* iface)
{
	iface->mainloop = pa_threaded_mainloop_new();
	if (!iface->mainloop) {
		sphone_module_log(LL_DEBUG, "pa_threaded_mainloop_new() failed.");
		return -1;
	}
	iface->api = pa_threaded_mainloop_get_api(iface->mainloop);
	iface->context = pa_context_new(iface->api, NULL);
	if (!iface->context) {
		pa_threaded_mainloop_free(iface->mainloop);
		sphone_module_log(LL_DEBUG, "pa_context_new failed.");
		return -1;
	}
	pa_context_set_state_callback(iface->context, sphone_pa_state_callback, iface);
	if (pa_context_connect(iface->context, NULL, 0, NULL) < 0) {
		sphone_module_log(LL_DEBUG, "pa_context_connect() failed: %s", pa_strerror(pa_context_errno(iface->context)));
    }
    pa_threaded_mainloop_start(iface->mainloop);
	return 0;
}

static void sphone_pa_distroy_interface(struct sphone_pa_if* iface)
{
	pa_context_unref(iface->context);
	pa_threaded_mainloop_stop(iface->mainloop);
	pa_threaded_mainloop_free(iface->mainloop);
	iface->mainloop = NULL;
	iface->context = NULL;
	iface->api = NULL;
}

static int sphone_pa_audio_route_set_in_call(struct sphone_pa_if *pa_if_l)
{
	if(!pa_if_l->context)
		sphone_module_log(LL_DEBUG, "pulse context not present.");
	pa_operation *operation = 
		pa_context_set_card_profile_by_index(pa_if_l->context, 0,
											"Voice Call", sphone_pa_callback,
											(void*)"set ucm profile to Voice Call");

	if (!operation) {
		sphone_module_log(LL_DEBUG, "pulse create operation failed %s.",
			  pa_strerror(pa_context_errno(pa_if_l->context)));
		return -1;
	}

	pa_signal_done();
	pa_operation_unref(operation);
	return 0;
}

static int sphone_pa_audio_route_set_playback(struct sphone_pa_if *pa_if_l)
{
	if(!pa_if_l->context)
		sphone_module_log(LL_DEBUG, "pulse context not present.");
	pa_operation *operation = 
		pa_context_set_card_profile_by_index(pa_if_l->context, 0,
											"HiFi", sphone_pa_callback,
											(void*)"set ucm profile to HiFi");

	if (!operation) {
		sphone_module_log(LL_DEBUG, "pulse create operation failed %s.",
			  pa_strerror(pa_context_errno(pa_if_l->context)));
		return -1;
	}

	pa_signal_done();
	pa_operation_unref(operation);
	return 0;
}

static void audio_route_trigger(gconstpointer data, gpointer user_data)
{
	(void)data;
	(void)user_data;
	//TODO
}

static void call_mode_trigger(gconstpointer data, gpointer user_data)
{
	(void)user_data;
	sphone_call_mode_t mode = (sphone_vibrate_type_t)data;

	if(mode == SPHONE_MODE_NO_CALL) {
		sphone_pa_audio_route_set_playback(&pa_if);
	}
	else if(mode == SPHONE_MODE_RINGING) {
		sphone_pa_audio_route_set_playback(&pa_if);
	}
	else if(mode == SPHONE_MODE_INCALL) {
		sphone_pa_audio_route_set_in_call(&pa_if);
	}
}

G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
	if(sphone_pa_create_interface(&pa_if) != 0)
		return "Failed to create pulseaudio context";
	
	append_trigger_to_datapipe(&call_mode_pipe, call_mode_trigger, NULL);
	append_trigger_to_datapipe(&audio_route_pipe, audio_route_trigger, NULL);
	return NULL;
}

G_MODULE_EXPORT void g_module_unload(GModule *module);
void g_module_unload(GModule *module)
{
	(void)module;
	sphone_pa_distroy_interface(&pa_if);
	remove_trigger_from_datapipe(&call_mode_pipe, call_mode_trigger);
	remove_trigger_from_datapipe(&audio_route_pipe, audio_route_trigger);
}
