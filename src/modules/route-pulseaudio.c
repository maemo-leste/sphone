/*
 * route-pulseaudio.c
 * Copyright (C) Carl Philipp Klemm 2022 <carl@uvos.xyz>
 * 
 * route-pulseaudio.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * route-pulseaudio.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
SPHONE_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};


#define VOICE_CALL_NAME "Voice Call"
#define HIFI_NAME "HiFi"

struct sphone_pa_if {
	pa_threaded_mainloop *mainloop;
	pa_mainloop_api *api;
	pa_context *context;
	GSList *cards;
};

static void sphone_pa_distroy_interface(struct sphone_pa_if* iface);

static void sphone_pa_card_info_cb(pa_context *c, const pa_card_info *i, int eol, void *userdata)
{
	(void)c;

	struct sphone_pa_if *iface = userdata;

	if(eol < 0) {
		sphone_module_log(LL_WARN, "Pulse audio callback faliure in %s", __func__);
		return;
	}

	if(eol) {
		if(!iface->cards)
			sphone_module_log(LL_WARN, "No pulse card availabe with "VOICE_CALL_NAME" profile");
		return;
	}

	for (uint32_t k = 0; k < i->n_profiles; ++k) {
		if (g_strcmp0(i->profiles[k].name, VOICE_CALL_NAME) == 0) {
			iface->cards = g_slist_prepend(iface->cards, GUINT_TO_POINTER(i->index));
			break;
		}
	}
}

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
			pa_operation *operation =
				pa_context_get_card_info_list(iface->context, sphone_pa_card_info_cb, iface);
			if (!operation)
				sphone_module_log(LL_ERR, "pulse create operation failed %s.",
								  pa_strerror(pa_context_errno(iface->context)));
			else
				pa_operation_unref(operation);
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

static int sphone_pa_audio_route_set_profile(struct sphone_pa_if *pa_if_l, const char *name)
{
	if(!pa_if_l->context) {
		sphone_module_log(LL_DEBUG, "pulse context not present.");
		return 0;
	}

	for(GSList *element = pa_if_l->cards; element; element = element->next) {
		if(!pa_if_l->context)
			sphone_module_log(LL_DEBUG, "pulse context not present.");
		pa_operation *operation = 
			pa_context_set_card_profile_by_index(pa_if_l->context, GPOINTER_TO_UINT(element->data),
				name, sphone_pa_callback,
				(void*)"set ucm profile");

		if(!operation) {
			sphone_module_log(LL_ERR, "pulse create operation failed %s.",
				pa_strerror(pa_context_errno(pa_if_l->context)));
			return -1;
		}

		pa_signal_done();
		pa_operation_unref(operation);
	}
	return 0;
}

static void audio_route_set_cb(pa_context *c, const pa_server_info *i, void *userdata)
{
	if(!i) {
		sphone_module_log(LL_ERR, "failed to get default sink name from pulse %s.", pa_strerror(pa_context_errno(c)));
		return;
	}

	const sphone_audio_route_t mode = GPOINTER_TO_INT(userdata);
	pa_operation* operation = NULL;

	sphone_module_log(LL_DEBUG, "Seting route on %s", i->default_sink_name);

	switch(mode) {
		case SPHONE_AUDIO_ROUTE_SPEAKER:
			operation = pa_context_set_sink_port_by_name(c, i->default_sink_name, "[Out] Speaker",
														 sphone_pa_callback, (void*)"Set sink to Speaker");
			break;
		case SPHONE_AUDIO_ROUTE_HANDSET:
			operation = pa_context_set_sink_port_by_name(c, i->default_sink_name, "[Out] Earpiece",
														 sphone_pa_callback, (void*)"Set sink to Earpiece");
			break;
		case SPHONE_AUDIO_ROUTE_HEADSET:
			operation = pa_context_set_sink_port_by_name(c, i->default_sink_name, "[Out] Headphones",
														 sphone_pa_callback, (void*)"Set sink to Headphones");
			break;
		case SPHONE_AUDIO_ROUTE_BT:
			sphone_module_log(LL_WARN, "Currently audio routing via bluetooth is not supported");
		default:
			sphone_module_log(LL_WARN, "Unsupported routing mode selected");
			return;
	}

	if(!operation) {
		sphone_module_log(LL_ERR, "pulse create operation failed %s.",
			pa_strerror(pa_context_errno(c)));
		return;
	}
	pa_operation_unref(operation);
}

static void audio_route_trigger(gconstpointer data, gpointer user_data)
{
	struct sphone_pa_if *pa_if = user_data;

	pa_operation *operation = pa_context_get_server_info(pa_if->context, audio_route_set_cb, (void*)data);

	if(!operation) {
		sphone_module_log(LL_ERR, "pulse create operation failed %s.",
			pa_strerror(pa_context_errno(pa_if->context)));
		return;
	}
	pa_operation_unref(operation);
}

static void call_mode_trigger(gconstpointer data, gpointer user_data)
{
	sphone_call_mode_t mode = GPOINTER_TO_INT(data);
	struct sphone_pa_if *pa_if = user_data;

	if(mode == SPHONE_MODE_NO_CALL) {
		sphone_pa_audio_route_set_profile(pa_if, HIFI_NAME);
	} else if(mode == SPHONE_MODE_RINGING) {
		sphone_pa_audio_route_set_profile(pa_if, HIFI_NAME);
	} else if(mode == SPHONE_MODE_INCALL) {
		sphone_pa_audio_route_set_profile(pa_if, VOICE_CALL_NAME);
	}
}

SPHONE_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	struct sphone_pa_if *pa_if = g_malloc0(sizeof(*pa_if));
	*data = pa_if;
	if(sphone_pa_create_interface(pa_if) != 0)
		return "Failed to create pulseaudio context";
	
	append_trigger_to_datapipe(&call_mode_pipe, call_mode_trigger, pa_if);
	append_trigger_to_datapipe(&audio_route_pipe, audio_route_trigger, pa_if);
	return NULL;
}

SPHONE_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	struct sphone_pa_if *pa_if = data;
	sphone_pa_distroy_interface(pa_if);
	remove_trigger_from_datapipe(&call_mode_pipe, call_mode_trigger, pa_if);
	remove_trigger_from_datapipe(&audio_route_pipe, audio_route_trigger, pa_if);
	g_free(pa_if);
}
