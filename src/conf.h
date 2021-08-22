#pragma once

#include <stdbool.h>

typedef enum {
	CONF_ATTR_EXTERNAL_RINGING_ON,
	CONF_ATTR_EXTERNAL_RINGING_OFF,
	CONF_ATTR_EXTERNAL_CALL_INCOMING,
	CONF_ATTR_EXTERNAL_CALL_INCOMING_MISSED,
	CONF_ATTR_EXTERNAL_CALL_INCOMING_ANSWERED,
	CONF_ATTR_EXTERNAL_CALL_OUTGOING,
	CONF_ATTR_EXTERNAL_CALL_OUTGOING_MISSED,
	CONF_ATTR_EXTERNAL_CALL_OUTGOING_ANSWERED,
	CONF_ATTR_EXTERNAL_SMS_OUTGOING,
	CONF_ATTR_EXTERNAL_SMS_INCOMING,
	CONF_ATTR_EXTERNAL_INCALL_START,
	CONF_ATTR_EXTERNAL_INCALL_STOP,
} conf_ext_t;

bool conf_vibration_enabled(void);
bool conf_set_vibration_enabled(bool enabled);
bool conf_ringer_enabled(void);
bool conf_set_ringer_enabled(bool enabled);
char* conf_sms_sound_path(void);
bool conf_set_sms_sound_path(const char *path);
char* conf_call_sound_path(void);
bool conf_set_call_sound_path(const char *path);

char* conf_get_external_handler(conf_ext_t type);

int conf_save(void);
