/*
 * gui.h
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * gui.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gui.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool gui_dialer_show(const CallProperties* call);

bool gui_dtmf_show(const CallProperties* call);

bool gui_sms_send_show(const MessageProperties* message);

bool gui_options_open(void);

bool gui_history_sms(void);

bool gui_contact_thread_shown(const Contact *contact);

void gui_show_thread_for_contact(const Contact *contact);

void gui_history_calls(void);

void gui_contact_show(const Contact *contact, void (*callback)(Contact*, void*), void *user_data);

void gui_close_contact_diag(void);

int gui_register(bool (*dialer_show)(const CallProperties* call),
                 bool (*dtmf_show)(const CallProperties* call),
                 bool (*sms_send_show)(const MessageProperties* call),
                 bool (*options_open)(void),
                 bool (*history_sms)(void),
                 bool (*contact_thread_shown)(const Contact *contact),
                 void (*show_thread_for_contact)(const Contact *contact),
                 void (*history_calls)(void),
                 void (*contact_show)(const Contact *contact, void (*callback)(Contact*, void*), void *user_data),
                 void (*close_contact_diag)(void));

void gui_remove(int id);

#ifdef __cplusplus
}
#endif
