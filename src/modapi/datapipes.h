#pragma once

#include "datapipe.h"

//input: string with filename to play
extern datapipe_struct audio_play_once_pipe;

//input: string with filename to play
extern datapipe_struct audio_play_looping_pipe;

//input: ignored
extern datapipe_struct audio_stop_pipe;

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

//input: NotificationProperties
extern datapipe_struct notification_raise_pipe;

//input: Contact or NULL
extern datapipe_struct contact_show_pipe;

void datapipes_init(void);
void datapipes_exit(void);
