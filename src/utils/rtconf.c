#include <stddef.h>

#include "rtconf.h"
#include "sphone-log.h"

static bool (*vibration_enabled_be)(void);
static bool (*set_vibration_enabled_be)(bool);
static bool (*ringer_enabled_be)(void);
static bool (*set_ringer_enabled_be)(bool);
static char* (*sms_sound_path_be)(void);
static bool (*set_sms_sound_path_be)(const char *path);
static char* (*call_sound_path_be)(void);
static bool (*set_call_sound_path_be)(const char *path);
static int (*save_be)(void);

bool rtconf_vibration_enabled(void)
{
	if(!vibration_enabled_be) {
		sphone_log(LL_WARN, "rtconf: %s used without backend", __func__);
		return false;
	}
	
	return vibration_enabled_be();
}

bool rtconf_set_vibration_enabled(bool enabled)
{
	if(!set_vibration_enabled_be) {
		sphone_log(LL_WARN, "rtconf: %s used without backend", __func__);
		return false;
	}
	
	return set_vibration_enabled_be(enabled);
}

bool rtconf_ringer_enabled(void)
{
	if(!ringer_enabled_be) {
		sphone_log(LL_WARN, "rtconf: %s used without backend", __func__);
		return false;
	}
	
	return ringer_enabled_be();
}

bool rtconf_set_ringer_enabled(bool enabled)
{
	if(!set_ringer_enabled_be) {
		sphone_log(LL_WARN, "rtconf: %s used without backend", __func__);
		return false;
	}
	
	return set_ringer_enabled_be(enabled);
}

char* rtconf_sms_sound_path(void)
{
	if(!sms_sound_path_be) {
		sphone_log(LL_WARN, "rtconf: %s used without backend", __func__);
		return NULL;
	}
	
	return sms_sound_path_be();
}

bool rtconf_set_sms_sound_path(const char *path)
{
	if(!set_sms_sound_path_be) {
		sphone_log(LL_WARN, "rtconf: %s used without backend", __func__);
		return false;
	}
	
	return set_sms_sound_path_be(path);
}

char* rtconf_call_sound_path(void)
{
	if(!call_sound_path_be) {
		sphone_log(LL_WARN, "rtconf: %s used without backend", __func__);
		return NULL;
	}
	
	return call_sound_path_be();
}

bool rtconf_set_call_sound_path(const char *path)
{
	if(!set_call_sound_path_be) {
		sphone_log(LL_WARN, "rtconf: %s used without backend", __func__);
		return NULL;
	}
	
	return set_call_sound_path_be(path);
}

int rtconf_save(void)
{
	if(!save_be) {
		sphone_log(LL_WARN, "rtconf: %s used without backend", __func__);
		return -1;
	}
	
	return save_be();
}

int rtconf_register_backend(bool (*vibration_enabled)(void),
							bool (*set_vibration_enabled)(bool),
							bool (*ringer_enabled)(void),
							bool (*set_ringer_enabled)(bool),
							char* (*sms_sound_path)(void),
							bool (*set_sms_sound_path)(const char *path),
							char* (*call_sound_path)(void),
							bool (*set_call_sound_path)(const char *path),
							int (*save)(void))
{
	vibration_enabled_be = vibration_enabled;
	set_vibration_enabled_be = set_vibration_enabled;
	ringer_enabled_be = ringer_enabled;
	set_ringer_enabled_be = set_ringer_enabled;
	sms_sound_path_be = sms_sound_path;
	set_sms_sound_path_be = set_sms_sound_path;
	call_sound_path_be = call_sound_path;
	set_call_sound_path_be = set_call_sound_path;
	save_be = save;
	return 1;
}

void rtconf_unregister_backend(int id)
{
	(void)id;
	vibration_enabled_be = NULL;
	set_vibration_enabled_be = NULL;
	ringer_enabled_be = NULL;
	set_ringer_enabled_be = NULL;
	sms_sound_path_be = NULL;
	set_sms_sound_path_be = NULL;
	call_sound_path_be = NULL;
	set_call_sound_path_be = NULL;
	save_be = NULL;
}
