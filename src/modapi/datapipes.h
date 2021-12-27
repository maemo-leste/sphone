/*
 * datapipe.h
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * datapipe.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * datapipe.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "datapipe.h"

#ifdef __cplusplus
extern "C" {
#endif

//input: string with filename to play
extern datapipe_struct audio_play_once_pipe;

//input: string with filename to play
extern datapipe_struct audio_play_looping_pipe;

//input: ignored
extern datapipe_struct audio_stop_pipe;

//input: bool
extern datapipe_struct audio_playing_pipe;

//input: sphone_audio_route_t
extern datapipe_struct audio_route_pipe;

//input: sphone_call_mode_t
extern datapipe_struct call_mode_pipe;

//input: CallProperties
extern datapipe_struct call_new_pipe;

//input: CallProperties
extern datapipe_struct call_properties_changed_pipe;

//input: CallProperties
extern datapipe_struct call_hangup_pipe;

//input: CallProperties
extern datapipe_struct call_accept_pipe;

//input: CallProperties
extern datapipe_struct call_dial_pipe;

//input: CallProperties
extern datapipe_struct call_hold_pipe;

//input: string with error message
extern datapipe_struct call_backend_error_pipe;

//input: sphone_vibrate_type_t
extern datapipe_struct vibrate_pipe;

//input: MessageProperties
extern datapipe_struct message_recived_pipe;

//input: MessageProperties
extern datapipe_struct message_send_pipe;

//input: Contact
extern datapipe_struct contact_fill_pipe;

//input: NotificationProperties
extern datapipe_struct notification_raise_pipe;

//input: Contact or NULL
extern datapipe_struct contact_show_pipe;

void datapipes_init(void);
void datapipes_exit(void);

#ifdef __cplusplus
}
#endif

