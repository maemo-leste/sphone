#pragma once
#include <pulse/pulseaudio.h>
#include <stdbool.h>

struct sphone_pa_if {
	pa_threaded_mainloop *mainloop;
	pa_mainloop_api *api;
	pa_context *context;
};

int sphone_pa_create_interface(struct sphone_pa_if* iface);

void sphone_pa_distroy_interface(struct sphone_pa_if* iface);

int sphone_pa_audio_route_set_in_call(struct sphone_pa_if *pa_if);

int sphone_pa_audio_route_set_playback(struct sphone_pa_if *pa_if);
