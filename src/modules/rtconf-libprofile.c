#include <libprofile.h>
#include <glib.h>

#include "rtconf.h"
#include "sphone-modules.h"

/** Module name */
#define MODULE_NAME		"rtconf-libprofile"

/** Functionality provided by this module */
static const gchar *const provides[] = { "rtconf", NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
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

G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
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

G_MODULE_EXPORT void g_module_unload(GModule *module);
void g_module_unload(GModule *module)
{
	(void)module;
	rtconf_unregister_backend(backend_id);
}
