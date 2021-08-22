/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * sphone is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * sphone is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <gtk/gtk.h>
#include <stdbool.h>
#include "log.h"
#include "conf.h"

#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))
#define TEST_BIT(x,addr) (1UL & (addr[x/8] >> (x & 0xff)))

void utils_start_ringing(const gchar *dial);
void utils_stop_ringing(const gchar *dial);
int utils_ringing_status();
void utils_sms_notify();
void utils_connected_notify();

GdkPixbuf *utils_get_photo_default();
GdkPixbuf *utils_get_photo_unknown();
GdkPixbuf *utils_get_photo(const gchar *path);
GdkPixbuf *utils_get_icon(const gchar *name);

enum {
	UTILS_AUDIO_ROUTE_UNKNOWN=-1,
	UTILS_AUDIO_ROUTE_SPEAKER=0,
	UTILS_AUDIO_ROUTE_HANDSET=1,
	UTILS_AUDIO_ROUTE_COUNT=2
};

typedef enum {
	UTILS_MODE_NO_CALL=0,
	UTILS_MODE_RINGING,
	UTILS_MODE_INCALL,
} utils_call_mode_t;

GDBusConnection *get_dbus_connection(void);

void utils_mce_init(void);
bool utils_set_call_mode(utils_call_mode_t mode);

void utils_external_exec(conf_ext_t type, ...);

void utils_gst_init(int *argc, char ***argv);
void utils_media_stop();
int utils_media_play_once(gchar *path);
int utils_media_play_repeat(gchar *path);

int utils_audio_route_set(int route);
int utils_audio_route_get();

#endif
