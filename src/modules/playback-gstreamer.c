#include <glib.h>
#include <gst/gst.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "datapipes.h"

/** Module name */
#define MODULE_NAME		"playback-gstreamer"

/** Functionality provided by this module */
static const gchar *const provides[] = { "playback", NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};

static GstElement *utils_gst_play;
static int utils_gst_repeat;

static void audio_stop_trigger(gconstpointer data);

static int utils_gst_rewind(void)
{
	return gst_element_seek_simple(utils_gst_play, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 0);
}

static gboolean utils_gst_bus_callback (GstBus *bus,GstMessage *message, gpointer data)
{
	GMainLoop *loop=(GMainLoop *)data;

	switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_ERROR: {
			GError *err;
			gchar *debug;

			gst_message_parse_error (message, &err, &debug);
		sphone_module_log(LL_ERR, "Error: %s", err->message);
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
				audio_stop_trigger(NULL);
			}
			break;
		default:
			/* unhandled message */
			break;
	}

	return TRUE;
}

static int utils_gst_start(const gchar *path)
{
	if(utils_gst_play)
		return 0;

	GstBus *bus;
	gchar *uri = g_filename_to_uri(path, NULL, NULL);

	if(!uri) {
		sphone_module_log(LL_ERR, "%s: unable to get uri for %s", __func__, path);
		return 1;
	}

	utils_gst_play = gst_element_factory_make ("playbin", "play");
	g_object_set (G_OBJECT (utils_gst_play), "uri", uri, NULL);

	bus = gst_pipeline_get_bus (GST_PIPELINE (utils_gst_play));
	gst_bus_add_watch (bus, utils_gst_bus_callback, NULL);
	gst_object_unref (bus);

	gst_element_set_state (utils_gst_play, GST_STATE_PLAYING);
	g_free(uri);

	return 0;
}

static void audio_stop_trigger(gconstpointer data)
{
	(void)data;

	if(!utils_gst_play)
		return;

	gst_element_set_state (utils_gst_play, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (utils_gst_play));
	utils_gst_play=NULL;
	utils_gst_repeat=0;
}

static void audio_play_looping_trigger(gconstpointer data)
{
	const char *filename = (const char*)data;
	
	if(!filename) {
		sphone_module_log(LL_WARN, "%s recived invalid file name", __func__);
		return;
	}
	
	utils_gst_repeat=1;
	if(utils_gst_start(filename) != 0)
		sphone_module_log(LL_ERR, "failed to play %s", filename);
}

static void audio_play_once_trigger(gconstpointer data)
{
	const char *filename = (const char*)data;
	
	if(!filename) {
		sphone_module_log(LL_WARN, "%s recived invalid file name", __func__);
		return;
	}
	
	utils_gst_repeat=0;
	if(utils_gst_start(filename) != 0)
		sphone_module_log(LL_ERR, "failed to play %s", filename);
}


G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
	append_trigger_to_datapipe(&audio_stop_pipe, audio_stop_trigger);
	append_trigger_to_datapipe(&audio_play_once_pipe, audio_play_once_trigger);
	append_trigger_to_datapipe(&audio_play_looping_pipe, audio_play_looping_trigger);
	
	gst_init(NULL, NULL);
	
	return NULL;
}

G_MODULE_EXPORT void g_module_unload(GModule *module);
void g_module_unload(GModule *module)
{
	(void)module;
	
	append_trigger_to_datapipe(&audio_stop_pipe, audio_stop_trigger);
	append_trigger_to_datapipe(&audio_play_once_pipe, audio_play_once_trigger);
	append_trigger_to_datapipe(&audio_play_looping_pipe, audio_play_looping_trigger);
}
