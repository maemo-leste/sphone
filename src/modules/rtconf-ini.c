/*
 * rtconf-ini.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * rtconf-ini.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * rtconf-ini.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <glib.h>
#include "rtconf.h"
#include "sphone-modules.h"
#include "sphone-log.h"

/** Module name */
#define MODULE_NAME		"rtconf-ini"

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

#define CONF_GROUP_EXTERNAL "external.handlers"
#define CONF_ATTR_EXTERNAL_RINGING_ON "ringing.on"
#define CONF_ATTR_EXTERNAL_RINGING_OFF "ringing.off"
#define CONF_ATTR_EXTERNAL_CALL_INCOMING "call.incoming"
#define CONF_ATTR_EXTERNAL_CALL_INCOMING_MISSED "call.incoming.missed"
#define CONF_ATTR_EXTERNAL_CALL_INCOMING_ANSWERED "call.incoming.answered"
#define CONF_ATTR_EXTERNAL_CALL_OUTGOING "call.outgoing"
#define CONF_ATTR_EXTERNAL_CALL_OUTGOING_MISSED "call.outgoing.missed"
#define CONF_ATTR_EXTERNAL_CALL_OUTGOING_ANSWERED "call.outgoing.answered"
#define CONF_ATTR_EXTERNAL_SMS_OUTGOING "sms.outgoing"
#define CONF_ATTR_EXTERNAL_SMS_INCOMING "sms.incoming"
#define CONF_ATTR_EXTERNAL_INCALL_START "incall.start"
#define CONF_ATTR_EXTERNAL_INCALL_STOP "incall.stop"

#define CONF_GROUP_NOTIFICATIONS "notifications"
#define CONF_ATTR_NOTIFICATIONS_SOUND_ENABLE "sound.enable"
#define CONF_ATTR_NOTIFICATIONS_VIBRATION_ENABLE "vibration.enable"
#define CONF_ATTR_NOTIFICATIONS_SOUND_VOICE_INCOMING_PATH "sound.voice.incoming.path"
#define CONF_ATTR_NOTIFICATIONS_SOUND_SMS_INCOMING_PATH "sound.sms.incoming.path"

static GKeyFile *conf_global=NULL;
static GKeyFile *conf_user=NULL;

static int backend_id;

static void conf_ini_load(void)
{
	if(conf_global)
		return;

	conf_global=g_key_file_new();
	conf_user=g_key_file_new();

	// Load system wide configuration
	gchar **confdirs = g_strdupv((gchar**)g_get_system_config_dirs());
	g_key_file_load_from_dirs(conf_global,"sphone/sphone.conf", (const gchar**)confdirs,NULL,G_KEY_FILE_NONE,NULL);
	g_strfreev(confdirs);
	
	// load local configuration
	gchar *localpath=g_build_filename(g_get_user_config_dir(),"sphone","sphone.conf",NULL);
	g_key_file_load_from_file(conf_user, localpath, G_KEY_FILE_NONE,NULL);
	g_free(localpath);
}

static bool conf_vibration_enabled(void)
{
	conf_ini_load();
	if(g_key_file_has_key(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_VIBRATION_ENABLE, NULL))
		return g_key_file_get_boolean(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_VIBRATION_ENABLE, NULL);
	return g_key_file_get_boolean(conf_global, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_VIBRATION_ENABLE, NULL);
}

static bool conf_set_vibration_enabled(bool enabled)
{
	conf_ini_load();
	g_key_file_set_boolean(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_VIBRATION_ENABLE, enabled);
	return true;
}

static bool conf_ringer_enabled(void)
{
	conf_ini_load();
	if(g_key_file_has_key(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_ENABLE, NULL))
		return g_key_file_get_boolean(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_ENABLE, NULL);
	return g_key_file_get_boolean(conf_global, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_ENABLE, NULL);
}

static bool conf_set_ringer_enabled(bool enabled)
{
	conf_ini_load();
	g_key_file_set_boolean(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_ENABLE, enabled);
	return true;
}

static char* conf_sms_sound_path(void)
{
	conf_ini_load();
	if(g_key_file_has_key(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_SMS_INCOMING_PATH, NULL))
		return g_key_file_get_string(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_SMS_INCOMING_PATH, NULL);
	return g_key_file_get_string(conf_global, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_SMS_INCOMING_PATH, NULL);
}

static bool conf_set_sms_sound_path(const char *path)
{
	conf_ini_load();
	g_key_file_set_string(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_SMS_INCOMING_PATH, path);
	return true;
}

static char* conf_call_sound_path(void)
{
	conf_ini_load();
	if(g_key_file_has_key(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_VOICE_INCOMING_PATH, NULL))
		return g_key_file_get_string(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_VOICE_INCOMING_PATH, NULL);
	return g_key_file_get_string(conf_global, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_VOICE_INCOMING_PATH, NULL);
}

static bool conf_set_call_sound_path(const char *path)
{
	conf_ini_load();
	g_key_file_set_string(conf_user, CONF_GROUP_NOTIFICATIONS, CONF_ATTR_NOTIFICATIONS_SOUND_VOICE_INCOMING_PATH, path);
	return true;
}

static int conf_save(void)
{
	int ret=1;
	
	gchar *localpath=g_build_filename(g_get_user_config_dir(),"sphone","sphone.conf",NULL);
	gchar *contents=g_key_file_to_data(conf_user, NULL, NULL);
	sphone_module_log(LL_DEBUG, "utils_conf_save_local: contents: %s", contents);
	if(contents){
		GError *err=NULL;
		ret=g_file_set_contents(localpath, contents, -1, &err)?0:1;
		if(ret){
			sphone_module_log(LL_ERR, "utils_conf_save_local: configuration file save failed: %s", err->message);
			g_error_free(err);
		}
	}
	g_free(localpath);

	return ret;
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
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

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	rtconf_unregister_backend(backend_id);
}
