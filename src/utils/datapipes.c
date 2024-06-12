/*
 * datapipes.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * datapipes.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * datapipes.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "datapipes.h"
#include "sphone-conf.h"

datapipe_struct audio_play_once_pipe;
datapipe_struct audio_play_looping_pipe;
datapipe_struct audio_stop_pipe;
datapipe_struct audio_route_pipe;
datapipe_struct audio_playing_pipe;

datapipe_struct call_mode_pipe;
datapipe_struct call_new_pipe;
datapipe_struct call_hangup_pipe;
datapipe_struct call_hold_pipe;
datapipe_struct call_accept_pipe;
datapipe_struct call_dial_pipe;
datapipe_struct gui_error_pipe;
datapipe_struct call_properties_changed_pipe;

datapipe_struct vibrate_pipe;

datapipe_struct message_received_pipe;
datapipe_struct message_send_pipe;

datapipe_struct contact_fill_pipe;

datapipe_struct notification_raise_pipe;
datapipe_struct contact_show_pipe;

datapipe_struct comm_backend_added_pipe;
datapipe_struct comm_backend_removed_pipe;

void *drop(void *data, void *user_data)
{
	(void)data;
	(void)user_data;
	return NULL;
}

void datapipes_init(void)
{
	setup_datapipe(&audio_play_once_pipe);
	setup_datapipe(&audio_play_looping_pipe);
	setup_datapipe(&audio_stop_pipe);
	setup_datapipe(&audio_playing_pipe);
	setup_datapipe(&audio_route_pipe);
	setup_datapipe(&call_mode_pipe);
	setup_datapipe(&gui_error_pipe);
	setup_datapipe(&call_new_pipe);
	setup_datapipe(&call_hangup_pipe);
	setup_datapipe(&call_hold_pipe);
	setup_datapipe(&call_dial_pipe);
	setup_datapipe(&call_properties_changed_pipe);
	setup_datapipe(&vibrate_pipe);
	setup_datapipe(&message_send_pipe);
	setup_datapipe(&message_received_pipe);
	setup_datapipe(&notification_raise_pipe);
	setup_datapipe(&call_accept_pipe);
	setup_datapipe(&contact_fill_pipe);
	setup_datapipe(&comm_backend_added_pipe);
	setup_datapipe(&comm_backend_removed_pipe);

	if(!(sphone_conf_get_features() & SPHONE_FEATURE_CALLS)) {
		append_filter_to_datapipe(&call_new_pipe, drop, NULL);
		append_filter_to_datapipe(&call_hangup_pipe, drop, NULL);
		append_filter_to_datapipe(&call_hold_pipe, drop, NULL);
		append_filter_to_datapipe(&call_dial_pipe, drop, NULL);
		append_filter_to_datapipe(&call_properties_changed_pipe, drop, NULL);
	}

	if(!(sphone_conf_get_features() & SPHONE_FEATURE_MESSAGES)) {
		append_filter_to_datapipe(&message_send_pipe, drop, NULL);
		append_filter_to_datapipe(&message_received_pipe, drop, NULL);
	}
}

void datapipes_exit(void)
{
	if(!(sphone_conf_get_features() & SPHONE_FEATURE_CALLS)) {
		remove_filter_from_datapipe(&call_new_pipe, drop, NULL);
		remove_filter_from_datapipe(&call_hangup_pipe, drop, NULL);
		remove_filter_from_datapipe(&call_hold_pipe, drop, NULL);
		remove_filter_from_datapipe(&call_dial_pipe, drop, NULL);
		remove_filter_from_datapipe(&call_properties_changed_pipe, drop, NULL);
	}

	if(!(sphone_conf_get_features() & SPHONE_FEATURE_MESSAGES)) {
		remove_filter_from_datapipe(&message_send_pipe, drop, NULL);
		remove_filter_from_datapipe(&message_received_pipe, drop, NULL);
	}

	free_datapipe(&audio_play_once_pipe);
	free_datapipe(&audio_play_looping_pipe);
	free_datapipe(&audio_stop_pipe);
	free_datapipe(&audio_playing_pipe);
	free_datapipe(&audio_route_pipe);
	free_datapipe(&call_mode_pipe);
	free_datapipe(&call_new_pipe);
	free_datapipe(&call_hangup_pipe);
	free_datapipe(&call_hold_pipe);
	free_datapipe(&call_dial_pipe);
	free_datapipe(&gui_error_pipe);
	free_datapipe(&call_properties_changed_pipe);
	free_datapipe(&vibrate_pipe);
	free_datapipe(&message_send_pipe);
	free_datapipe(&message_received_pipe);
	free_datapipe(&notification_raise_pipe);
	free_datapipe(&call_accept_pipe);
	free_datapipe(&contact_fill_pipe);
	free_datapipe(&comm_backend_added_pipe);
	free_datapipe(&comm_backend_removed_pipe);
}
