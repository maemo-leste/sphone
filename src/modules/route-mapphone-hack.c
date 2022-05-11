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

#include <fcntl.h>
#include <unistd.h>
#include <glib.h>

#include "sphone-modules.h"
#include "sphone-log.h"
#include "datapipes.h"
#include "datapipe.h"
#include "types.h"

/** Module name */
#define MODULE_NAME		"route-mapphone-hack"

/** Functionality provided by this module */
static const gchar *const provides[] = { "route-mapphone-hack", NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

struct sphone_mph {
	sphone_call_mode_t current_call_mode;
};

#define SYSFS_REGISTER_HACK_PATH "/sys/kernel/debug/regmap/spi0.0/registers"

static void audio_route_trigger(gconstpointer data, gpointer user_data)
{
	struct sphone_mph *dat = user_data;
	sphone_audio_route_t mode = GPOINTER_TO_INT(data);

	GError *err = NULL;

	if (dat->current_call_mode == SPHONE_MODE_INCALL_NO_ROUTE) {
		sphone_module_log(LL_DEBUG,
				  "audio_route_trigger: current call mode is SPHONE_MODE_INCALL_NOROUTE");
		return;
	}

	int fd = open(SYSFS_REGISTER_HACK_PATH, O_WRONLY);

	if (mode == SPHONE_AUDIO_ROUTE_SPEAKER) {
		const char* msg = "081c 0002\n";
		write(fd, msg, strlen(msg));
	} else if (mode == SPHONE_AUDIO_ROUTE_HANDSET) {
		const char* msg = "081c 0001\n";
		write(fd, msg, strlen(msg));
	} else if (mode == SPHONE_AUDIO_ROUTE_HEADSET) {
		const char* msg = "081c 0260\n";
		write(fd, msg, strlen(msg));
	} else if (mode == SPHONE_AUDIO_ROUTE_BT) {
		/* Nothing for now */
	}

	close(fd);

	if (err != NULL) {
		sphone_module_log(LL_WARN,
				  "Failed to write to registers path: %s",
				  err->message);
	}

	g_clear_error(&err);

	return;
}

static void call_mode_trigger(gconstpointer data, gpointer user_data)
{
	struct sphone_mph *dat = user_data;
	sphone_call_mode_t mode = GPOINTER_TO_INT(data);
	dat->current_call_mode = mode;

#if 0
	if (mode == SPHONE_MODE_NO_CALL) {
	} else if (mode == SPHONE_MODE_RINGING) {
	} else if (mode == SPHONE_MODE_INCALL) {
	} else if (mode == SPHONE_MODE_INCALL_NOROUTE) {
	}
#endif
}

G_MODULE_EXPORT const gchar *sphone_module_init(void **data);
const gchar *sphone_module_init(void **data)
{
	struct sphone_mph *dat = g_malloc0(sizeof(*dat));
	append_trigger_to_datapipe(&call_mode_pipe, call_mode_trigger, dat);
	append_trigger_to_datapipe(&audio_route_pipe, audio_route_trigger, dat);
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void *data);
void sphone_module_exit(void *data)
{
	struct sphone_mph *dat = data;
	remove_trigger_from_datapipe(&call_mode_pipe, call_mode_trigger, dat);
	remove_trigger_from_datapipe(&audio_route_pipe, audio_route_trigger,
				     dat);
	g_free(dat);
}
