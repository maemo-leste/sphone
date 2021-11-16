#pragma once

#include <stdbool.h>
#include "types.h"

bool gui_dialer_show(const CallProperties* call);

bool gui_sms_send_show(const MessageProperties* message);

bool gui_options_open(void);

bool gui_history_sms(void);

bool gui_contact_shown(const Contact *contact);

int gui_register(bool (*dialer_show)(const CallProperties* call),
			 bool (*sms_send_show)(const MessageProperties* call),
			 bool (*options_open)(void),
			 bool (*history_sms)(void),
			 bool (*contact_shown)(const Contact *contact));

void gui_remove(int id);
