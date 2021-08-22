#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_LIBPROFILE
#include <libprofile.h>
#include <glib.h>

#include "conf.h"

#define SOUNDS_DIR "/usr/share/sounds/"

bool conf_vibration_enabled(void)
{
	char* profile = profile_get_profile();
	bool ret = profile_get_value_as_bool(profile, "vibrating.alert.enabled");
	g_free(profile);
	return ret;
}

bool conf_set_vibration_enabled(bool enabled)
{
	char* profile = profile_get_profile();
	int ret = profile_set_value_as_bool(profile, "vibrating.alert.enabled", enabled);
	g_free(profile);
	return !ret;
}

bool conf_ringer_enabled(void)
{
	char* profile = profile_get_profile();
	char* value = profile_get_value(profile, "ringing.alert.type");
	g_free(profile);
	bool ret = g_strcmp0(value, "silent") == 0;
	g_free(value);
	return !ret;
}

bool conf_set_ringer_enabled(bool enabled)
{
	char* profile = profile_get_profile();
	int ret = profile_set_value(profile, "ringing.alert.type", enabled ? "ringing" : "silent");
	g_free(profile);
	return !ret;
}

char* conf_sms_sound_path(void)
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

bool conf_set_sms_sound_path(const char *path)
{
	char* profile = profile_get_profile();
	int ret = profile_set_value(profile, "sms.alert.tone", path);
	g_free(profile);
	return !ret;
}

char* conf_call_sound_path(void)
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

bool conf_set_call_sound_path(const char *path)
{
	char* profile = profile_get_profile();
	int ret = profile_set_value(profile, "ringing.alert.tone", path);
	g_free(profile);
	return !ret;
}

int conf_save(void)
{
	return 0;
}

char* conf_get_external_handler(conf_ext_t type)
{
	return NULL;
}

#endif
