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
#include <gst/gst.h>
#include <alsa/asoundlib.h>

#include "utils.h"



#define ICONS_PATH "/usr/share/sphone/icons/"

/** MCE D-Bus service */
#define MCE_SERVICE			"com.nokia.mce"
/** MCE D-Bus Request interface */
#define MCE_REQUEST_IF			"com.nokia.mce.request"
/** MCE D-Bus Signal interface */
#define MCE_SIGNAL_IF			"com.nokia.mce.signal"
/** MCE D-Bus Request path */
#define MCE_REQUEST_PATH		"/com/nokia/mce/request"
/** MCE D-Bus Signal path */
#define MCE_SIGNAL_PATH			"/com/nokia/mce/signal"

/** No ongoing call */
#define MCE_CALL_STATE_NONE			"none"
/** Call ringing */
#define MCE_CALL_STATE_RINGING			"ringing"
/** Call on-going */
#define MCE_CALL_STATE_ACTIVE			"active"
/** Normal call */
#define MCE_NORMAL_CALL				"normal"
/** Emergency call  */
#define MCE_EMERGENCY_CALL			"emergency"

int conf_key_set_stickiness(char *key_code, char *cmd, char *arg);
int conf_key_set_code(char *key_code, char *cmd, char *arg);
int conf_key_set_power_key(char *key_code, char *cmd, char *arg);
static int utils_audio_route_set_play();
//static int utils_audio_route_set_incall();
static int utils_audio_route_save();
static int utils_audio_route_restore();

struct{
	GDBusConnection *s_bus_conn;
} mce_if_priv;

GDBusConnection *get_dbus_connection(void)
{
	GError *error = NULL;
	char *addr;
	
	GDBusConnection *s_bus_conn;

	#if !GLIB_CHECK_VERSION(2,35,0)
	g_type_init();
	#endif

	addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (addr == NULL) {
		g_error("fail to get dbus addr: %s\n", error->message);
		g_free(error);
		return NULL;
	}

	s_bus_conn = g_dbus_connection_new_for_address_sync(addr,
			G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
			G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
			NULL, NULL, &error);

	if (s_bus_conn == NULL) {
		g_error("fail to create dbus connection: %s\n", error->message);
		g_free(error);
	}

	return s_bus_conn;
}

void utils_mce_init(void)
{
	mce_if_priv.s_bus_conn = get_dbus_connection();
}

bool utils_set_call_mode(utils_call_mode_t mode)
{
	if(!mce_if_priv.s_bus_conn)
		return false;

	GVariant *val;
	GVariant *result;
	GError *error = NULL;

	if(mode == UTILS_MODE_NO_CALL) {
		val = g_variant_new("(ss)", MCE_CALL_STATE_NONE, MCE_NORMAL_CALL);
		debug("%s: %s\n", __func__, MCE_CALL_STATE_NONE);
	}
	else if(mode == UTILS_MODE_RINGING) {
		val = g_variant_new("(ss)", MCE_CALL_STATE_RINGING, MCE_NORMAL_CALL);
		debug("%s: %s\n", __func__, MCE_CALL_STATE_RINGING);
	}
	else if(mode == UTILS_MODE_INCALL) {
		val = g_variant_new("(ss)", MCE_CALL_STATE_ACTIVE, MCE_NORMAL_CALL);
		debug("%s: %s\n", __func__, MCE_CALL_STATE_ACTIVE);
	}
	else {
		return false;
	}

	result = g_dbus_connection_call_sync(mce_if_priv.s_bus_conn, MCE_SERVICE, MCE_REQUEST_PATH,
		MCE_REQUEST_IF, "req_call_state_change", val, NULL,
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error)
		return false;

	g_variant_unref(result);
	return true;
}

/*
 Start/stop vibration
 */

static void utils_vibrate_message(void) {
	if(!mce_if_priv.s_bus_conn)
		return;

	GVariant *val;
	GVariant *result;
	GError *error = NULL;

	val = g_variant_new("(s)", "PatternIncomingMessage");
	result = g_dbus_connection_call_sync(mce_if_priv.s_bus_conn, MCE_SERVICE, MCE_REQUEST_PATH,
		MCE_REQUEST_IF, "req_vibrator_pattern_activate", val, NULL,
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error)
		return;

	g_variant_unref(result);
}

static void utils_stop_ringing_vibrate()
{
	if(!mce_if_priv.s_bus_conn)
		return;

	GVariant *val;
	GVariant *result;
	GError *error = NULL;

	val = g_variant_new("(s)", "PatternIncomingCall");
	result = g_dbus_connection_call_sync(mce_if_priv.s_bus_conn, MCE_SERVICE, MCE_REQUEST_PATH,
		MCE_REQUEST_IF, "req_vibrator_pattern_deactivate", val, NULL,
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error)
		return;

	g_variant_unref(result);
}

static void utils_start_ringing_vibrate()
{
	if(!mce_if_priv.s_bus_conn)
		return;

	GVariant *val;
	GVariant *result;
	GError *error = NULL;

	val = g_variant_new("(s)", "PatternIncomingCall");
	result = g_dbus_connection_call_sync(mce_if_priv.s_bus_conn, MCE_SERVICE, MCE_REQUEST_PATH,
		MCE_REQUEST_IF, "req_vibrator_pattern_activate", val, NULL,
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if(error)
		return;

	g_variant_unref(result);
}

static int utils_ringing_state=0;
/*
 Ringing starting:
 - Start vibration (if enabled)
 - Start ringtone playing (if enabled)
 - Execute external application handler
*/
void utils_start_ringing(const gchar *dial)
{
	if(utils_ringing_state)
		return;
	utils_ringing_state=1;

	if(conf_vibration_enabled())
		utils_start_ringing_vibrate();

	if(conf_ringer_enabled()){
		char *path=conf_call_sound_path();
		if(path)
			utils_media_play_repeat(path);
	}
	
	utils_external_exec(CONF_ATTR_EXTERNAL_RINGING_ON, dial, NULL);
}

/*
 Ringing stop:
 - Stop vibration
 - Stop ringtone playing
 - Execute external application handler
*/
void utils_stop_ringing(const gchar *dial)
{
	if(!utils_ringing_state)
		return;
	utils_ringing_state=0;
	
	utils_stop_ringing_vibrate();
	utils_media_stop();
	utils_external_exec(CONF_ATTR_EXTERNAL_RINGING_OFF,dial,NULL);
}

/*
 Get ringing status, 1: ringing is active
 */
int utils_ringing_status()
{
	return utils_ringing_state && (conf_ringer_enabled() || conf_vibration_enabled());
}

/*
 Notify the user of incoming sms:
 - Play sms notification
 - Short vibration
*/
void utils_sms_notify()
{
	if(conf_ringer_enabled()) {
		char *path = conf_sms_sound_path();

		if(path) {
			utils_media_play_once(path);
			g_free(path);
		}
	}
	
	if(conf_vibration_enabled())
		utils_vibrate_message();
}

void utils_connected_notify()
{
	if(conf_vibration_enabled())
		utils_vibrate_message();
}

static GdkPixbuf *photo_default=NULL;
static GdkPixbuf *photo_unknown=NULL;

GdkPixbuf *utils_get_photo_default()
{
	if(!photo_default){
		debug("utils_get_photo_default load %s\n",ICONS_PATH "contact-person.png");
		photo_default=gdk_pixbuf_new_from_file(ICONS_PATH "contact-person.png", NULL);
	}
	g_object_ref(G_OBJECT(photo_default));
	return photo_default;
}

GdkPixbuf *utils_get_photo_unknown()
{
	if(!photo_unknown){
		debug("utils_get_photo_unknown load %s\n",ICONS_PATH "contact-person-unknown.png");
		photo_unknown=gdk_pixbuf_new_from_file(ICONS_PATH "contact-person-unknown.png", NULL);
	}
	g_object_ref(G_OBJECT(photo_unknown));
	return photo_unknown;
}

GdkPixbuf *utils_get_photo(const gchar *path)
{
	return NULL;
}

GdkPixbuf *utils_get_icon(const gchar *name)
{
	gchar *path=g_build_filename(ICONS_PATH,name,NULL);
	debug("utils_get_icon load %s\n",path);
	GdkPixbuf *icon=gdk_pixbuf_new_from_file(path, NULL);
	g_free(path);
	return icon;
}

/****************************************************
 	External applications execution
 ****************************************************/

void utils_external_exec(conf_ext_t type, ...)
{
	debug("utils_external_exec %i\n", type);
	gchar *path=conf_get_external_handler(type);
	if(!path){
		debug("utils_external_exec: No external handler for %i\n", type);
		return;
	}
	signal(SIGCHLD, SIG_IGN);	// Prevent zombie processes

	int pid=fork();
	if(pid==-1){
		error("Fork failed\n");
		g_free(path);
		return;
	}

	if(pid){
		debug("Fork succeeded\n");
		g_free(path);
		
		return;
	}

	// we are now the child process
	gchar *args[10];
	int args_count=1;

	args[0]=path;

	va_list va;
	va_start(va, type);

	while(args_count<9){
		gchar *a=va_arg(va, gchar *);
		if(!a)
			break;
		args[args_count++]=a;
	}
	args[args_count]=NULL;

	va_end(va);
	
	char* typestr = g_strdup_printf("%i", type);
	setenv("SPHONE_ACTION", typestr, 1);
	g_free(typestr);
	execv(path,args);
	error("utils_external_exec: execv failed %s\n",path);
	exit(0);
}

/****************************************************
 	Audio playback routines using gstreamer
 ****************************************************/

static GstElement *utils_gst_play;
static int utils_gst_repeat;

static int utils_gst_rewind()
{
	return gst_element_seek_simple(utils_gst_play, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 0);
}

static gboolean utils_gst_bus_callback (GstBus *bus,GstMessage *message, gpointer    data)
{
	GMainLoop *loop=(GMainLoop *)data;

	switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_ERROR: {
			GError *err;
			gchar *debug;

			gst_message_parse_error (message, &err, &debug);
			error("Error: %s\n", err->message);
			g_error_free (err);
			g_free (debug);

			g_main_loop_quit (loop);
			break;
		}
		case GST_MESSAGE_EOS:
			/* end-of-stream */
			if(utils_gst_repeat)
				utils_gst_rewind();
			else{
				gst_element_set_state (utils_gst_play, GST_STATE_NULL);
				gst_bus_set_flushing(bus, TRUE);
				utils_media_stop();
			}
			break;
		default:
			/* unhandled message */
			break;
	}

	return TRUE;
}

void utils_gst_init(int *argc, char ***argv)
{
	gst_init(argc, argv);
}

static int utils_gst_start(gchar *path)
{
	if(utils_gst_play)
		return 0;

	GstBus *bus;
	gchar *uri=g_filename_to_uri(path,NULL,NULL);

	if(!uri) {
		error("%s: unable to get uri for %s", path);
		return 1;
	}

	utils_gst_play = gst_element_factory_make ("playbin", "play");
	g_object_set (G_OBJECT (utils_gst_play), "uri", uri, NULL);

	bus = gst_pipeline_get_bus (GST_PIPELINE (utils_gst_play));
	gst_bus_add_watch (bus, utils_gst_bus_callback, NULL);
	gst_object_unref (bus);

	// Set audio routing
	utils_audio_route_save();
	utils_audio_route_set_play();

	gst_element_set_state (utils_gst_play, GST_STATE_PLAYING);
	g_free(uri);

	return 0;
}

void utils_media_stop()
{
	if(!utils_gst_play)
		return ;

	gst_element_set_state (utils_gst_play, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (utils_gst_play));
	utils_gst_play=NULL;
	utils_gst_repeat=0;
	
	utils_audio_route_restore();
}

int utils_media_play_once(gchar *path)
{
	utils_gst_repeat=0;
	return utils_gst_start(path);
}

int utils_media_play_repeat(gchar *path)
{
	utils_gst_repeat=1;
	return utils_gst_start(path);
}

/****************************************************
 	Audio playback routines using gstreamer
 ****************************************************/

static int utils_audio_route_set_play()
{
	return 0;
}


static int utils_audio_route_save()
{
	return 0;
}

static int utils_audio_route_restore()
{
	return 0;
}

/*
 Change sound routing to incall value
 */
/*
static int utils_audio_route_set_incall()
{
	return 0;
}
*/
/*
 Change sound routing
 */
int utils_audio_route_set(int route)
{	
	//TODO
	return 0;
}

/*
 Get sound routing
 */
int utils_audio_route_get()
{
	//TODO
	return UTILS_AUDIO_ROUTE_UNKNOWN;
}
