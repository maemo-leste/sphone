#include <log.h>
#include "pulse.h"

static void sphone_pa_state_callback(pa_context *c, void *userdata)
{
	struct sphone_pa_if *iface = userdata;

	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
			break;
		case PA_CONTEXT_READY:
			debug("PA_CONTEXT_READY\n");
			break;
		case PA_CONTEXT_TERMINATED:
			debug("Context terminated: %s", pa_strerror(pa_context_errno(c)));
			sphone_pa_distroy_interface(iface);
			break;
		case PA_CONTEXT_FAILED:
		default:
			error("Connection failure: %s", pa_strerror(pa_context_errno(c)));
			sphone_pa_distroy_interface(iface);
	}
}

static void sphone_pa_callback(pa_context *c, int success, void *userdata)
{
	if (!success)
		debug("Pulse audio failure: %s\n", pa_strerror(pa_context_errno(c)));
	else
		debug("Pulse audio sucess\n");
}

int sphone_pa_create_interface(struct sphone_pa_if* iface)
{
	iface->mainloop = pa_threaded_mainloop_new();
	if (!iface->mainloop) {
		debug("pa_threaded_mainloop_new() failed.\n");
		return -1;
	}
	iface->api = pa_threaded_mainloop_get_api(iface->mainloop);
	iface->context = pa_context_new(iface->api, NULL);
	if (!iface->context) {
		pa_threaded_mainloop_free(iface->mainloop);
		debug("pa_context_new failed.\n");
		return -1;
	}
	pa_context_set_state_callback(iface->context, sphone_pa_state_callback, iface);
	if (pa_context_connect(iface->context, NULL, 0, NULL) < 0) {
		debug("pa_context_connect() failed: %s", pa_strerror(pa_context_errno(iface->context)));
    }
    pa_threaded_mainloop_start(iface->mainloop);
	return 0;
}

void sphone_pa_distroy_interface(struct sphone_pa_if* iface)
{
	pa_context_unref(iface->context);
	pa_threaded_mainloop_stop(iface->mainloop);
	pa_threaded_mainloop_free(iface->mainloop);
	iface->mainloop = NULL;
	iface->context = NULL;
	iface->api = NULL;
}

int sphone_pa_audio_route_set_in_call(struct sphone_pa_if *pa_if)
{
	if(!pa_if->context)
		debug("pulse context not present.\n");
	pa_operation *operation = 
		pa_context_set_card_profile_by_index(pa_if->context, 0,
											"Voice Call", sphone_pa_callback,
											NULL);

	if (!operation) {
		debug("pulse create operation failed %s.\n",
			  pa_strerror(pa_context_errno(pa_if->context)));
		return -1;
	}

	pa_signal_done();
	pa_operation_unref(operation);
	return 0;
}

int sphone_pa_audio_route_set_playback(struct sphone_pa_if *pa_if)
{
	if(!pa_if->context)
		debug("pulse context not present.\n");
	pa_operation *operation = 
		pa_context_set_card_profile_by_index(pa_if->context, 0,
											"HiFi", sphone_pa_callback,
											NULL);

	if (!operation) {
		debug("pulse create operation failed %s.\n",
			  pa_strerror(pa_context_errno(pa_if->context)));
		return -1;
	}

	pa_signal_done();
	pa_operation_unref(operation);
	return 0;
}
