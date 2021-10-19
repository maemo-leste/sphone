#pragma once

#include <stdbool.h>

bool rtconf_vibration_enabled(void);
bool rtconf_set_vibration_enabled(bool enabled);
bool rtconf_ringer_enabled(void);
bool rtconf_set_ringer_enabled(bool enabled);
char* rtconf_sms_sound_path(void);
bool rtconf_set_sms_sound_path(const char *path);
char* rtconf_call_sound_path(void);
bool rtconf_set_call_sound_path(const char *path);

int rtconf_save(void);

int rtconf_register_backend(bool (*vibration_enabled)(void),
							bool (*set_vibration_enabled)(bool),
							bool (*ringer_enabled)(void),
							bool (*set_ringer_enabled)(bool),
							char* (*sms_sound_path)(void),
							bool (*set_sms_sound_path)(const char *path),
							char* (*call_sound_path)(void),
							bool (*set_call_sound_path)(const char *path),
							int (*save)(void));

void rtconf_unregister_backend(int id);
