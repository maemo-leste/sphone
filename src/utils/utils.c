/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * sphone is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * sphone is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "utils.h"
#include "sphone-log.h"
#include "datapipes.h"
#include "rtconf.h"
#include "types.h"

static int utils_ringing_state=0;
/*
 Ringing starting:
 - Start vibration (if enabled)
 - Start ringtone playing (if enabled)
 - Execute external application handler
*/
void utils_start_ringing(void)
{
	if(utils_ringing_state)
		return;
	utils_ringing_state=1;

	if(rtconf_vibration_enabled())
		execute_datapipe(&vibrate_pipe, GINT_TO_POINTER(SPHONE_VIBRATE_CALL));

	if(rtconf_ringer_enabled()){
		char *path=rtconf_call_sound_path();
		if(path) {
			execute_datapipe(&audio_play_looping_pipe, path);
			g_free(path);
		}
	}
}

/*
 Ringing stop:
 - Stop vibration
 - Stop ringtone playing
 - Execute external application handler
*/
void utils_stop_ringing(void)
{
	if(!utils_ringing_state)
		return;
	utils_ringing_state=0;
	
	execute_datapipe(&vibrate_pipe, GINT_TO_POINTER(SPHONE_VIBRATE_STOP));
	execute_datapipe(&audio_stop_pipe, NULL);
}

/*
 Get ringing status, 1: ringing is active
 */
int utils_ringing_status(void)
{
	return utils_ringing_state && (rtconf_ringer_enabled() || rtconf_vibration_enabled());
}

/*
 Notify the user of incoming sms:
 - Play sms notification
 - Short vibration
*/
void utils_sms_notify(void)
{
	if(rtconf_ringer_enabled()) {
		char *path = rtconf_sms_sound_path();

		if(path) {
			execute_datapipe(&audio_play_once_pipe, path);
			g_free(path);
		}
	}
	
	if(rtconf_vibration_enabled())
		execute_datapipe(&vibrate_pipe, GINT_TO_POINTER(SPHONE_VIBRATE_MESSAGE));
}

char *sphone_time_to_new_string(time_t time)
{
	char *str = g_malloc(256);
	if(strftime(str, sizeof(256), "%m-%d-%Y %H-%M (mon=%b)", localtime(&time)) == 0)
		str[0] = '\0';
	return str;
}
