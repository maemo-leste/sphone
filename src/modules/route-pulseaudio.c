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

static port_role_t port_classify_by_name(const char *n, int direction)
{
	if (name_contains_ci(n, "speaker"))   return PORT_ROLE_SPEAKER;
	if (name_contains_ci(n, "earpiece"))  return PORT_ROLE_EARPIECE;
	if (name_contains_ci(n, "handset"))   return PORT_ROLE_EARPIECE;
	if (name_contains_ci(n, "receiver"))  return PORT_ROLE_EARPIECE;
	if (name_contains_ci(n, "headphone")) return PORT_ROLE_HEADPHONES;
	if (name_contains_ci(n, "headset"))
		return (direction & PA_DIRECTION_INPUT) ? PORT_ROLE_HEADSET_MIC : PORT_ROLE_HEADPHONES;
	if (name_contains_ci(n, "mic"))       return PORT_ROLE_MIC;
	return PORT_ROLE_NONE;
}

static port_role_t port_classify(const pa_card_port_info *p)
{
	/* For specific port types, trust pa_device_port_type_t. For generic /
	 * unknown types, fall back to the name -- some UCM templates leave
	 * port.type unset and only the name distinguishes (see N900: InternalMic
	 * and ExternalMic are both type=Unknown but named clearly). */
	switch (p->type) {
		case PA_DEVICE_PORT_TYPE_SPEAKER:    return PORT_ROLE_SPEAKER;
		case PA_DEVICE_PORT_TYPE_EARPIECE:   return PORT_ROLE_EARPIECE;
		case PA_DEVICE_PORT_TYPE_HANDSET:    return PORT_ROLE_EARPIECE;
		case PA_DEVICE_PORT_TYPE_HEADPHONES: return PORT_ROLE_HEADPHONES;
		case PA_DEVICE_PORT_TYPE_MIC:        return PORT_ROLE_MIC;
		case PA_DEVICE_PORT_TYPE_HEADSET:
			return (p->direction & PA_DIRECTION_INPUT)
				? PORT_ROLE_HEADSET_MIC : PORT_ROLE_HEADPHONES;
		case PA_DEVICE_PORT_TYPE_UNKNOWN:
		case PA_DEVICE_PORT_TYPE_AUX:
		case PA_DEVICE_PORT_TYPE_LINE:
		default:
			return port_classify_by_name(p->name, p->direction);
	}
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
 * Pick the best profile on `card`, preferring (in this order):
 *   tier 3: matches voice/hifi flavour AND output-port role AND input-port role
 *   tier 2: matches voice/hifi flavour AND output-port role (input relaxed)
 *   tier 1: matches voice/hifi flavour only (both ports relaxed)
 *
 * Within a tier, higher pa_card_profile_info2::priority wins. Profiles of the
 * wrong flavour (Voice Call when we want HiFi, or vice versa) are excluded
 * outright -- without that floor we could end up flipping to Voice Call when
 * the user wants music playback.
 *
 * The relaxed tiers exist because some devices (e.g. Nokia N900) only carry
 * Voice Call profiles for the earpiece port and have no HiFi-with-earpiece
 * combo at all. After a call ends with route=HANDSET, the strict lookup for
 * "HiFi with earpiece" returns nothing; tier 1 then picks the highest-priority
 * available HiFi profile so we recover instead of staying on Voice Call.
 *
 * Port membership is derived from each port's profiles2 cross-reference (the
 * same data shown as "Part of profile(s)" in pactl), not from profile names.
 *
 * Caller frees the returned string. Sets *out_tier to which tier matched (1-3)
 * or 0 if nothing matched (best_name == NULL).
 */
static char *choose_profile_for_card(const pa_card_info *card,
                                     const struct apply_request *req,
                                     int *out_tier)
{
	if (out_tier)
		*out_tier = 0;
	if (!card || !card->profiles2)
		return NULL;

	GPtrArray *out_ports = g_ptr_array_new();
	GPtrArray *in_ports  = g_ptr_array_new();

	for (uint32_t i = 0; i < card->n_ports; i++) {
		pa_card_port_info *p = card->ports[i];
		if (!p)
			continue;
		port_role_t role = port_classify(p);
		sphone_module_log(LL_DEBUG,
			"  card %s port %s type=%u dir=%d avail=%d -> role=%d",
			card->name, p->name, p->type, p->direction, p->available, role);
		if (p->available == PA_PORT_AVAILABLE_NO)
			continue;
		if ((p->direction & PA_DIRECTION_OUTPUT) && role == req->want_output)
			g_ptr_array_add(out_ports, p);
		if ((p->direction & PA_DIRECTION_INPUT) && role == req->want_input)
			g_ptr_array_add(in_ports, p);
	}

	char *best_name = NULL;
	int best_tier = 0;
	uint32_t best_priority = 0;

	for (uint32_t i = 0; card->profiles2[i]; i++) {
		pa_card_profile_info2 *prof = card->profiles2[i];
		if (!prof->available)
			continue;
		if (g_strcmp0(prof->name, "off") == 0)
			continue;

		gboolean is_voice = g_str_has_prefix(prof->name, "Voice Call");
		gboolean is_hifi  = g_str_has_prefix(prof->name, "HiFi");
		gboolean flavour_ok = req->want_voice ? is_voice : is_hifi;
		if (!flavour_ok)
			continue;

		gboolean out_ok = TRUE;
		if (prof->n_sinks > 0) {
			out_ok = FALSE;
			for (guint k = 0; k < out_ports->len && !out_ok; k++)
				out_ok = port_lists_profile(out_ports->pdata[k], prof->name);
		}
		gboolean in_ok = TRUE;
		if (prof->n_sources > 0) {
			in_ok = FALSE;
			for (guint k = 0; k < in_ports->len && !in_ok; k++)
				in_ok = port_lists_profile(in_ports->pdata[k], prof->name);
		}

		int tier;
		if (out_ok && in_ok)
			tier = 3;
		else if (out_ok)
			tier = 2;
		else
			tier = 1;

		if (tier > best_tier ||
		    (tier == best_tier && prof->priority > best_priority)) {
			best_tier = tier;
			best_priority = prof->priority;
			g_free(best_name);
			best_name = g_strdup(prof->name);
		}
	}

	if (out_tier)
		*out_tier = best_tier;
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

	int tier = 0;
	gchar *name = choose_profile_for_card(card, req, &tier);
	if (!name) {
		sphone_module_log(LL_WARN,
			"no matching profile on card %s for voice=%d out=%d in=%d",
			card->name, req->want_voice, req->want_output, req->want_input);
		return;
	}

	const char *active = card->active_profile2 ? card->active_profile2->name : NULL;
	if (g_strcmp0(name, active) == 0) {
		sphone_module_log(LL_DEBUG, "card %s already on profile %s (tier=%d)",
			card->name, name, tier);
		g_free(name);
		return;
	}

	if (tier < 3)
		sphone_module_log(LL_WARN,
			"card %s: no exact profile for voice=%d out=%d in=%d, falling back (tier=%d)",
			card->name, req->want_voice, req->want_output, req->want_input, tier);

	gchar *log_msg = g_strdup_printf("set %s profile %s -> %s (tier=%d)",
		card->name, active ? active : "(none)", name, tier);
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

static void resolve_target(struct apply_request *req,
                           sphone_call_mode_t mode,
                           sphone_audio_route_t route)
{
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

	req->want_voice = (mode == SPHONE_MODE_INCALL);
}

static void apply_audio_state(struct sphone_pa_if *pa_if)
{
	if (!pa_if->context || pa_context_get_state(pa_if->context) != PA_CONTEXT_READY) {
		sphone_module_log(LL_DEBUG, "skipping apply: pulse context not ready");
		return;
	}

	sphone_call_mode_t mode = datapipe_get_last_data_int(&call_mode_pipe);
	sphone_audio_route_t route = datapipe_get_last_data_int(&audio_route_pipe);

	/* INCALL_NO_ROUTE means a call is active but another component owns audio
	 * routing (e.g. an external dialler that programs PA itself). Touching the
	 * profile from here would fight that component, so leave it alone. */
	if (mode == SPHONE_MODE_INCALL_NO_ROUTE) {
		sphone_module_log(LL_DEBUG,
			"skipping apply: INCALL_NO_ROUTE, another component owns routing");
		return;
	}

	struct apply_request *req = g_new0(struct apply_request, 1);
	resolve_target(req, mode, route);

	sphone_module_log(LL_INFO,
		"apply: mode=%d route=%d -> voice=%d out=%d in=%d",
		mode, route, req->want_voice, req->want_output, req->want_input);

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
