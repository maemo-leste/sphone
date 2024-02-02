/*
 * rtconf-libprofile.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * rtconf-libprofile.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * rtconf-libprofile.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libprofile.h>
#include <glib.h>

#include "rtconf.h"
#include "sphone-modules.h"

/** Module name */
#define MODULE_NAME		"rtconf-libprofile"

/** Functionality provided by this module */
static const gchar *const provides[] = { "rtconf", NULL };

/** Module information */
SPHONE_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

#define SOUNDS_DIR "/usr/share/sounds/"

static int backend_id;

static bool conf_vibration_enabled(void)
{
	char* profile = profile_get_profile();
	bool ret = profile_get_value_as_bool(profile, "vibrating.alert.enabled");
	g_free(profile);
	return ret;
}

static bool conf_set_vibration_enabled(bool enabled)
{
	char* profile = profile_get_profile();
	int ret = profile_set_value_as_bool(profile, "vibrating.alert.enabled", enabled);
	g_free(profile);
	return !ret;
}

static bool conf_ringer_enabled(void)
{
	char* profile = profile_get_profile();
	char* value = profile_get_value(profile, "ringing.alert.type");
	g_free(profile);
	bool ret = g_strcmp0(value, "silent") == 0;
	g_free(value);
	return !ret;
}

static bool conf_set_ringer_enabled(bool enabled)
{
	char* profile = profile_get_profile();
	int ret = profile_set_value(profile, "ringing.alert.type", enabled ? "ringing" : "silent");
	g_free(profile);
	return !ret;
}

static char* conf_sms_sound_path(void)
{
	char* profile = profile_get_profile();
	char* value = profile_get_value(profile, "sms.alert.tone");
	if(value && value[0] != '/')
	{
		char* tmp = g_strconcat(SOUNDS_DIR, value, NULL);
		g_free(value);
		value = tmp;
	}

	g_free(profile);
	return value;
}

static bool conf_set_sms_sound_path(const char *path)
{
	char* profile = profile_get_profile();
	int ret = profile_set_value(profile, "sms.alert.tone", path);
	g_free(profile);
	return !ret;
}

static char* conf_call_sound_path(void)
{
	char* profile = profile_get_profile();
	char* value = profile_get_value(profile, "ringing.alert.tone");

	if(value && value[0] != '/')
	{
		char* tmp = g_strconcat(SOUNDS_DIR, value, NULL);
		g_free(value);
		value = tmp;
	}

	g_free(profile);
	return value;
}

static bool conf_set_call_sound_path(const char *path)
{
	char* profile = profile_get_profile();
	int ret = profile_set_value(profile, "ringing.alert.tone", path);
	g_free(profile);
	return !ret;
}

static int conf_save(void)
{
	return 0;
}

SPHONE_MODULE_EXPORT const gchar *sphone_module_init(void** dat);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	backend_id = rtconf_register_backend(conf_vibration_enabled,
							conf_set_vibration_enabled,
							conf_ringer_enabled,
							conf_set_ringer_enabled,
							conf_sms_sound_path,
							conf_set_sms_sound_path,
							conf_call_sound_path,
							conf_set_call_sound_path,
							conf_save);
	
	return NULL;
}

SPHONE_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	rtconf_unregister_backend(backend_id);
}
