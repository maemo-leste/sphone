/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
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

#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))
#define TEST_BIT(x,addr) (1UL & (addr[x/8] >> (x & 0xff)))

void set_debug(int level);
void debug(const char *s,...); 
void syserror(const char *s,...);
void error(const char *s,...);

void utils_vibrate(int val);
void utils_audio_set(int val);
void utils_start_ringing(const gchar *dial);
void utils_stop_ringing(const gchar *dial);
void utils_notify();

GdkPixbuf *utils_get_photo_default();
GdkPixbuf *utils_get_photo_unknown();
GdkPixbuf *utils_get_photo(const gchar *path);
GdkPixbuf *utils_get_icon(const gchar *name);

#define UTILS_CONF_GROUP_EXTERNAL "external.handlers"
#define UTILS_CONF_ATTR_EXTERNAL_RINGING_ON "ringing.on"
#define UTILS_CONF_ATTR_EXTERNAL_RINGING_OFF "ringing.off"
#define UTILS_CONF_ATTR_EXTERNAL_CALL_INCOMING "call.incoming"
#define UTILS_CONF_ATTR_EXTERNAL_CALL_INCOMING_MISSED "call.incoming.missed"
#define UTILS_CONF_ATTR_EXTERNAL_CALL_INCOMING_ANSWERED "call.incoming.answered"
#define UTILS_CONF_ATTR_EXTERNAL_CALL_OUTGOING "call.outgoing"
#define UTILS_CONF_ATTR_EXTERNAL_CALL_OUTGOING_MISSED "call.outgoing.missed"
#define UTILS_CONF_ATTR_EXTERNAL_CALL_OUTGOING_ANSWERED "call.outgoing.answered"
#define UTILS_CONF_ATTR_EXTERNAL_SMS_OUTGOING "sms.outgoing"
#define UTILS_CONF_ATTR_EXTERNAL_SMS_INCOMING "sms.incoming"

#define UTILS_CONF_GROUP_ACTION_VIBRATE "action.vibrate"
#define UTILS_CONF_ATTR_ACTION_VIBRATE_PATH "path"
#define UTILS_CONF_ATTR_ACTION_VIBRATE_VALUE_ON "on.value"
#define UTILS_CONF_ATTR_ACTION_VIBRATE_VALUE_OFF "off.value"

#define UTILS_CONF_GROUP_ACTION_AUDIO "action.audio"
#define UTILS_CONF_ATTR_ACTION_AUDIO_PATH "path"
#define UTILS_CONF_ATTR_ACTION_AUDIO_VALUE_ON "on.value"
#define UTILS_CONF_ATTR_ACTION_AUDIO_VALUE_OFF "off.value"


gchar *utils_conf_get_string(const gchar *group, const gchar *name);
gint utils_conf_get_int(const gchar *group, const gchar *name);
gboolean utils_conf_has_key(const gchar *group, const gchar *name);
void utils_external_exec(const gchar *name, ...);

#endif
