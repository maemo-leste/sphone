#pragma once

#include <gtk/gtk.h>

typedef enum {
	SPHONE_AUDIO_ROUTE_UNKNOWN=-1,
	SPHONE_AUDIO_ROUTE_SPEAKER=0,
	SPHONE_AUDIO_ROUTE_HANDSET,
	SPHONE_AUDIO_ROUTE_HEADSET,
	SPHONE_AUDIO_ROUTE_BT,
	SPHONE_AUDIO_ROUTE_COUNT
} sphone_audio_route_t;

typedef enum {
	SPHONE_MODE_NO_CALL=0,
	SPHONE_MODE_RINGING,
	SPHONE_MODE_INCALL,
} sphone_call_mode_t;

typedef enum {
	SPHONE_VIBRATE_CALL=0,
	SPHONE_VIBRATE_MESSAGE,
	SPHONE_VIBRATE_STOP,
} sphone_vibrate_type_t;

typedef struct _CallProperties{
	gchar *name;
	gchar *path;
	gchar *line_identifier;
	gchar *state;
	gchar *technology;
	gint backend;
	gchar *start_time;
	gboolean emergency;
} CallProperties;

typedef struct _MessageProperties{
	gchar *name;
	gchar *line_identifier;
	gchar *technology;
	gint backend;
	gchar *time;
} MessageProperties;

typedef struct _Notification{
	gchar *title;
	gchar *text;
} Notification;

typedef struct _Contact {
	gchar *name;
	GdkPixbuf *photo;
	gchar *line_identifier;
	gint backend;
} Contact;
