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
#include <string.h>
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
	.name = MODULE_NAME,
	.provides = provides,
	.priority = 250
};

/*
 * Roles a card port can fulfil for our purposes. We classify ports by
 * pa_device_port_type_t (with a name-based fallback) instead of relying on
 * specific port names like "[Out] Speaker", which differ between UCM versions.
 */
typedef enum {
	PORT_ROLE_NONE = 0,
	PORT_ROLE_SPEAKER,
	PORT_ROLE_EARPIECE,
	PORT_ROLE_HEADPHONES,
	PORT_ROLE_MIC,
	PORT_ROLE_HEADSET_MIC,
} port_role_t;

struct sphone_pa_if {
	pa_threaded_mainloop *mainloop;
	pa_mainloop_api *api;
	pa_context *context;
};

/*
 * The desired audio state, derived from the call_mode and audio_route pipes.
 * Threaded through pa_context_get_card_info_list() into the per-card callback.
 */
struct apply_request {
	port_role_t want_output;
	port_role_t want_input;
	gboolean want_voice;
};

static void sphone_pa_destroy_interface(struct sphone_pa_if *iface);

static gboolean name_contains_ci(const char *haystack, const char *needle)
{
	if (!haystack || !needle)
		return FALSE;
	gchar *h = g_ascii_strdown(haystack, -1);
	gchar *n = g_ascii_strdown(needle, -1);
	gboolean r = strstr(h, n) != NULL;
	g_free(h);
	g_free(n);
	return r;
}

static port_role_t port_classify(const pa_card_port_info *p)
{
	switch (p->type) {
		case PA_DEVICE_PORT_TYPE_SPEAKER:    return PORT_ROLE_SPEAKER;
		case PA_DEVICE_PORT_TYPE_EARPIECE:   return PORT_ROLE_EARPIECE;
		case PA_DEVICE_PORT_TYPE_HANDSET:    return PORT_ROLE_EARPIECE;
		case PA_DEVICE_PORT_TYPE_HEADPHONES: return PORT_ROLE_HEADPHONES;
		case PA_DEVICE_PORT_TYPE_MIC:        return PORT_ROLE_MIC;
		case PA_DEVICE_PORT_TYPE_HEADSET:    return PORT_ROLE_HEADSET_MIC;
		default: break;
	}

	const char *n = p->name;
	if (name_contains_ci(n, "speaker"))   return PORT_ROLE_SPEAKER;
	if (name_contains_ci(n, "earpiece"))  return PORT_ROLE_EARPIECE;
	if (name_contains_ci(n, "handset"))   return PORT_ROLE_EARPIECE;
	if (name_contains_ci(n, "headphone")) return PORT_ROLE_HEADPHONES;
	if (name_contains_ci(n, "headset"))
		return (p->direction & PA_DIRECTION_INPUT) ? PORT_ROLE_HEADSET_MIC : PORT_ROLE_HEADPHONES;
	if (name_contains_ci(n, "mic"))       return PORT_ROLE_MIC;
	return PORT_ROLE_NONE;
}

static gboolean port_lists_profile(const pa_card_port_info *port, const char *profile_name)
{
	if (!port || !port->profiles2 || !profile_name)
		return FALSE;
	for (uint32_t j = 0; port->profiles2[j]; j++) {
		if (g_strcmp0(port->profiles2[j]->name, profile_name) == 0)
			return TRUE;
	}
	return FALSE;
}

/*
 * Pick the best profile on `card` that is reachable from a port of the
 * requested role on each side it produces (output and/or input).
 *
 * We don't filter by profile name. Instead we use each port's profiles2
 * cross-reference (the same data shown as "Part of profile(s)" in pactl) to
 * confirm a candidate profile actually routes through ports of the desired
 * role. Voice/HiFi preference is only a tiebreaker: under the new alsa-ucm
 * a profile name like "Voice Call (Earpiece)" still starts with "Voice Call",
 * but that's not load-bearing -- if the prefix scheme changes, we'll still
 * pick a working profile, just possibly the wrong "flavour".
 *
 * Caller frees the returned string.
 */
static char *choose_profile_for_card(const pa_card_info *card, const struct apply_request *req)
{
	if (!card || !card->profiles2)
		return NULL;

	GPtrArray *out_ports = g_ptr_array_new();
	GPtrArray *in_ports  = g_ptr_array_new();

	for (uint32_t i = 0; i < card->n_ports; i++) {
		pa_card_port_info *p = card->ports[i];
		if (!p || p->available == PA_PORT_AVAILABLE_NO)
			continue;
		port_role_t role = port_classify(p);
		if ((p->direction & PA_DIRECTION_OUTPUT) && role == req->want_output)
			g_ptr_array_add(out_ports, p);
		if ((p->direction & PA_DIRECTION_INPUT) && role == req->want_input)
			g_ptr_array_add(in_ports, p);
	}

	char *best_name = NULL;
	int best_voice_score = -1;
	uint32_t best_priority = 0;

	for (uint32_t i = 0; card->profiles2[i]; i++) {
		pa_card_profile_info2 *prof = card->profiles2[i];
		if (!prof->available)
			continue;
		if (g_strcmp0(prof->name, "off") == 0)
			continue;

		if (prof->n_sinks > 0) {
			gboolean found = FALSE;
			for (guint k = 0; k < out_ports->len && !found; k++)
				found = port_lists_profile(out_ports->pdata[k], prof->name);
			if (!found)
				continue;
		}
		if (prof->n_sources > 0) {
			gboolean found = FALSE;
			for (guint k = 0; k < in_ports->len && !found; k++)
				found = port_lists_profile(in_ports->pdata[k], prof->name);
			if (!found)
				continue;
		}

		gboolean is_voice = g_str_has_prefix(prof->name, "Voice Call");
		gboolean is_hifi  = g_str_has_prefix(prof->name, "HiFi");
		int voice_score = (req->want_voice ? is_voice : is_hifi) ? 1 : 0;

		if (voice_score > best_voice_score ||
		    (voice_score == best_voice_score && prof->priority > best_priority)) {
			best_voice_score = voice_score;
			best_priority = prof->priority;
			g_free(best_name);
			best_name = g_strdup(prof->name);
		}
	}

	g_ptr_array_free(out_ports, TRUE);
	g_ptr_array_free(in_ports, TRUE);
	return best_name;
}

static void sphone_pa_set_profile_done(pa_context *c, int success, void *userdata)
{
	gchar *log_msg = userdata;
	if (!success)
		sphone_module_log(LL_WARN, "%s failed: %s", log_msg, pa_strerror(pa_context_errno(c)));
	else
		sphone_module_log(LL_DEBUG, "%s succeeded", log_msg);
	g_free(log_msg);
}

static void apply_card_cb(pa_context *c, const pa_card_info *card, int eol, void *userdata)
{
	struct apply_request *req = userdata;

	if (eol < 0) {
		sphone_module_log(LL_WARN, "card enumeration failed: %s", pa_strerror(pa_context_errno(c)));
		g_free(req);
		return;
	}
	if (eol) {
		g_free(req);
		return;
	}

	gchar *name = choose_profile_for_card(card, req);
	if (!name) {
		sphone_module_log(LL_DEBUG, "no matching profile on card %s (voice=%d out=%d in=%d)",
			card->name, req->want_voice, req->want_output, req->want_input);
		return;
	}

	const char *active = card->active_profile2 ? card->active_profile2->name : NULL;
	if (g_strcmp0(name, active) == 0) {
		sphone_module_log(LL_DEBUG, "card %s already on profile %s", card->name, name);
		g_free(name);
		return;
	}

	gchar *log_msg = g_strdup_printf("set %s profile %s -> %s",
		card->name, active ? active : "(none)", name);
	sphone_module_log(LL_INFO, "%s", log_msg);

	pa_operation *op = pa_context_set_card_profile_by_index(
		c, card->index, name, sphone_pa_set_profile_done, log_msg);
	if (!op) {
		sphone_module_log(LL_ERR, "set_card_profile_by_index failed: %s",
			pa_strerror(pa_context_errno(c)));
		g_free(log_msg);
	} else {
		pa_operation_unref(op);
	}
	g_free(name);
}

static void resolve_target(struct apply_request *req)
{
	sphone_call_mode_t mode = datapipe_get_last_data_int(&call_mode_pipe);
	sphone_audio_route_t route = datapipe_get_last_data_int(&audio_route_pipe);

	switch (route) {
		case SPHONE_AUDIO_ROUTE_HANDSET:
			req->want_output = PORT_ROLE_EARPIECE;
			break;
		case SPHONE_AUDIO_ROUTE_HEADSET:
			req->want_output = PORT_ROLE_HEADPHONES;
			break;
		case SPHONE_AUDIO_ROUTE_BT:
			/* Bluetooth routing handled by a separate card (HFP/HSP) and
			 * not yet wired up here; fall back to speaker so we still pick a
			 * sensible profile on the built-in card. */
			req->want_output = PORT_ROLE_SPEAKER;
			break;
		case SPHONE_AUDIO_ROUTE_SPEAKER:
		case SPHONE_AUDIO_ROUTE_UNKNOWN:
		case SPHONE_AUDIO_ROUTE_COUNT:
		default:
			req->want_output = PORT_ROLE_SPEAKER;
			break;
	}

	req->want_input = (route == SPHONE_AUDIO_ROUTE_HEADSET)
		? PORT_ROLE_HEADSET_MIC : PORT_ROLE_MIC;

	/* SPHONE_MODE_INCALL_NO_ROUTE is for calls we know are active but where
	 * routing is owned by another component (e.g. external dialler); leave the
	 * profile alone by treating it as non-voice and matching whatever HiFi
	 * profile fits the current ports. */
	req->want_voice = (mode == SPHONE_MODE_INCALL);
}

static void apply_audio_state(struct sphone_pa_if *pa_if)
{
	if (!pa_if->context || pa_context_get_state(pa_if->context) != PA_CONTEXT_READY) {
		sphone_module_log(LL_DEBUG, "skipping apply: pulse context not ready");
		return;
	}

	struct apply_request *req = g_new0(struct apply_request, 1);
	resolve_target(req);

	sphone_module_log(LL_DEBUG, "apply: voice=%d out=%d in=%d",
		req->want_voice, req->want_output, req->want_input);

	pa_operation *op = pa_context_get_card_info_list(pa_if->context, apply_card_cb, req);
	if (!op) {
		sphone_module_log(LL_ERR, "get_card_info_list failed: %s",
			pa_strerror(pa_context_errno(pa_if->context)));
		g_free(req);
		return;
	}
	pa_operation_unref(op);
}

static void sphone_pa_state_callback(pa_context *c, void *userdata)
{
	struct sphone_pa_if *iface = userdata;

	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		case PA_CONTEXT_UNCONNECTED:
			break;
		case PA_CONTEXT_READY:
			sphone_module_log(LL_DEBUG, "Pulse audio context is ready");
			/* Sync to whatever state the pipes already hold. Safe to call
			 * directly: we're on the PA mainloop thread here. */
			apply_audio_state(iface);
			break;
		case PA_CONTEXT_TERMINATED:
			sphone_module_log(LL_DEBUG, "Context terminated: %s", pa_strerror(pa_context_errno(c)));
			sphone_module_failed(&module_info, SPHONE_MODULE_RELOAD);
			break;
		case PA_CONTEXT_FAILED:
		default:
			sphone_module_log(LL_ERR, "Connection failure: %s", pa_strerror(pa_context_errno(c)));
			sphone_module_failed(&module_info, SPHONE_MODULE_FATAL);
	}
}

static int sphone_pa_create_interface(struct sphone_pa_if *iface)
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
	if (pa_context_connect(iface->context, NULL, 0, NULL) < 0)
		sphone_module_log(LL_DEBUG, "pa_context_connect() failed: %s",
			pa_strerror(pa_context_errno(iface->context)));
	pa_threaded_mainloop_start(iface->mainloop);
	return 0;
}

static void sphone_pa_destroy_interface(struct sphone_pa_if *iface)
{
	if (iface->mainloop)
		pa_threaded_mainloop_stop(iface->mainloop);
	if (iface->context) {
		pa_context_disconnect(iface->context);
		pa_context_unref(iface->context);
	}
	if (iface->mainloop)
		pa_threaded_mainloop_free(iface->mainloop);
	iface->mainloop = NULL;
	iface->context = NULL;
	iface->api = NULL;
}

/*
 * Single trigger for both call_mode_pipe and audio_route_pipe. apply_audio_state
 * reads the latest values from both pipes and picks one profile per card; this
 * avoids the previous race where switching profile and switching port were
 * separate ops and could end up briefly mismatched.
 */
static void route_trigger(gconstpointer data, gpointer user_data)
{
	(void)data;
	struct sphone_pa_if *pa_if = user_data;
	if (!pa_if->mainloop)
		return;
	pa_threaded_mainloop_lock(pa_if->mainloop);
	apply_audio_state(pa_if);
	pa_threaded_mainloop_unlock(pa_if->mainloop);
}

SPHONE_MODULE_EXPORT const gchar *sphone_module_init(void **data);
const gchar *sphone_module_init(void **data)
{
	struct sphone_pa_if *pa_if = g_malloc0(sizeof(*pa_if));
	*data = pa_if;
	if (sphone_pa_create_interface(pa_if) != 0)
		return "Failed to create pulseaudio context";

	append_trigger_to_datapipe(&call_mode_pipe, route_trigger, pa_if);
	append_trigger_to_datapipe(&audio_route_pipe, route_trigger, pa_if);
	return NULL;
}

SPHONE_MODULE_EXPORT void sphone_module_exit(void *data);
void sphone_module_exit(void *data)
{
	struct sphone_pa_if *pa_if = data;
	remove_trigger_from_datapipe(&call_mode_pipe, route_trigger, pa_if);
	remove_trigger_from_datapipe(&audio_route_pipe, route_trigger, pa_if);
	sphone_pa_destroy_interface(pa_if);
	g_free(pa_if);
}
