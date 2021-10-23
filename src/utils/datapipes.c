#include "datapipes.h"

datapipe_struct audio_play_once_pipe;
datapipe_struct audio_play_looping_pipe;
datapipe_struct audio_stop_pipe;
datapipe_struct audio_route_pipe;

datapipe_struct call_mode_pipe;
datapipe_struct call_new_pipe;
datapipe_struct call_hangup_pipe;
datapipe_struct call_hold_pipe;
datapipe_struct call_accept_pipe;
datapipe_struct call_dial_pipe;
datapipe_struct call_backend_error_pipe;
datapipe_struct call_properties_changed_pipe;

datapipe_struct vibrate_pipe;

datapipe_struct message_recived_pipe;
datapipe_struct message_send_pipe;

datapipe_struct notification_raise_pipe;
datapipe_struct contact_show_pipe;

void datapipes_init(void)
{
	setup_datapipe(&audio_play_once_pipe);
	setup_datapipe(&audio_play_looping_pipe);
	setup_datapipe(&audio_stop_pipe);
	setup_datapipe(&audio_route_pipe);
	setup_datapipe(&call_mode_pipe);
	setup_datapipe(&call_new_pipe);
	setup_datapipe(&call_hangup_pipe);
	setup_datapipe(&call_hold_pipe);
	setup_datapipe(&call_dial_pipe);
	setup_datapipe(&call_backend_error_pipe);
	setup_datapipe(&call_properties_changed_pipe);
	setup_datapipe(&vibrate_pipe);
	setup_datapipe(&message_send_pipe);
	setup_datapipe(&notification_raise_pipe);
	setup_datapipe(&call_accept_pipe);
	setup_datapipe(&contact_show_pipe);
}

void datapipes_exit(void)
{
	free_datapipe(&audio_play_once_pipe);
	free_datapipe(&audio_play_looping_pipe);
	free_datapipe(&audio_stop_pipe);
	free_datapipe(&audio_route_pipe);
	free_datapipe(&call_mode_pipe);
	free_datapipe(&call_new_pipe);
	free_datapipe(&call_hangup_pipe);
	free_datapipe(&call_hold_pipe);
	free_datapipe(&call_dial_pipe);
	free_datapipe(&call_backend_error_pipe);
	free_datapipe(&call_properties_changed_pipe);
	free_datapipe(&vibrate_pipe);
	free_datapipe(&message_send_pipe);
	free_datapipe(&notification_raise_pipe);
	free_datapipe(&call_accept_pipe);
	free_datapipe(&contact_show_pipe);
}