/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * Copyright (C) Carl Klemm 2021 <carl@uvos.xyz>
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

#include <gtk/gtk.h>
#include <gio/gio.h>
#include "rtconf.h"
#include "datapipes.h"
#include "datapipe.h"
#include "gui.h"
#include "sphone-modules.h"

/** Module name */
#define MODULE_NAME		"ui-options-gtk"

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, NULL };

/** Module information */
SPHONE_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

struct {
	GtkWidget *main_window;
}g_options;

struct s_option{
	char* (*get_fn)(void);
	bool (*set_fn)(const char *path);
};

struct b_option{
	bool (*get_fn)(void);
	bool (*set_fn)(bool);
};

static int gui_options_close(GtkWidget *w);
static GtkWidget *gui_options_build_option_check(const struct b_option option, const gchar *label);
static GtkWidget *gui_options_build_option_file_audio(const struct s_option option, const gchar *label);
static void gui_options_set_bool_callback(GtkToggleButton *button);
static void gui_options_set_file_callback(GtkFileChooser *chooser);
static void gui_options_play_callback(GtkButton *button, GtkFileChooser *chooser);
static void gui_options_stop_callback(GtkButton *button, GtkFileChooser *chooser);

static int is_dirty=0;

static bool gtk_gui_options_open(void)
{
	if(g_options.main_window){
		gtk_window_present(GTK_WINDOW(g_options.main_window));
		return true;
	}

	g_options.main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(g_options.main_window),"Sphone options");
	gtk_window_set_default_size(GTK_WINDOW(g_options.main_window),400,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,0);
	GtkWidget *tabs=gtk_notebook_new();
	GtkWidget *actions=gtk_hbox_new(FALSE,0);
	GtkWidget *close=gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	
	GtkWidget *notifications_v=gtk_vbox_new(FALSE,0);
	
	struct b_option boption;
	boption.get_fn = &rtconf_ringer_enabled;
	boption.set_fn = &rtconf_set_ringer_enabled;
	gtk_box_pack_start(GTK_BOX(notifications_v), gui_options_build_option_check(boption, "Enable sound"), FALSE, FALSE, 0);
	boption.get_fn = &rtconf_vibration_enabled;
	boption.set_fn = &rtconf_set_vibration_enabled;
	gtk_box_pack_start(GTK_BOX(notifications_v), gui_options_build_option_check(boption, "Enable vibration"), FALSE, FALSE, 0);

	struct s_option soption;
	soption.get_fn = &rtconf_call_sound_path;
	soption.set_fn = &rtconf_set_call_sound_path;
	gtk_box_pack_start(GTK_BOX(notifications_v), gui_options_build_option_file_audio(soption, "Ring tone"), FALSE, FALSE, 0);
	soption.get_fn = &rtconf_sms_sound_path;
	soption.set_fn = &rtconf_set_sms_sound_path;
	gtk_box_pack_start(GTK_BOX(notifications_v), gui_options_build_option_file_audio(soption, "SMS tone"), FALSE, FALSE, 0);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(tabs), notifications_v, gtk_label_new ("Notifications"));
	
	g_signal_connect(G_OBJECT(g_options.main_window),"delete-event", G_CALLBACK(gui_options_close),NULL);
	g_signal_connect(G_OBJECT(close),"clicked", G_CALLBACK(gui_options_close), NULL);

	gtk_box_pack_start(GTK_BOX(v1), tabs, TRUE, TRUE, 5);
	gtk_box_pack_end(GTK_BOX(actions), close, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(v1), actions, FALSE, FALSE, 5);
	gtk_container_add(GTK_CONTAINER(g_options.main_window), v1);
	
	gtk_widget_show_all(g_options.main_window);
	return true;
}

static GtkWidget *gui_options_build_option_check(const struct b_option option, const gchar *label)
{
	GtkWidget *check=gtk_check_button_new_with_label (label);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), option.get_fn());

	struct b_option *option_new = g_new(struct b_option, 1);
	*option_new = option;
	g_object_set_data_full(G_OBJECT(check), "option", option_new, g_free);
	g_signal_connect(G_OBJECT(check),"toggled", G_CALLBACK(gui_options_set_bool_callback),NULL);
	
	return check;
}

static GtkWidget *gui_options_build_option_file_audio(const struct s_option option, const gchar *label)
{
	GtkWidget *h=gtk_hbox_new(FALSE,0);
	GtkWidget *chooser=gtk_file_chooser_button_new(label, GTK_FILE_CHOOSER_ACTION_OPEN);
	GtkWidget *play=gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	GtkWidget *stop=gtk_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
	
	gchar *default_path = option.get_fn();
	if(default_path){
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(chooser), default_path);
		g_free(default_path);
	}

	GtkFileFilter *filter=gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Audio");
	gtk_file_filter_add_mime_type(filter, "audio/*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	
	struct s_option *option_new = g_new(struct s_option, 1);
	*option_new = option;
	g_object_set_data_full(G_OBJECT(chooser), "option", option_new, g_free);

	gtk_box_pack_start(GTK_BOX(h), gtk_label_new(label), FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(h), chooser, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(h), play, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(h), stop, FALSE, FALSE, 5);
	
	g_signal_connect(G_OBJECT(chooser),"file-set", G_CALLBACK(gui_options_set_file_callback),NULL);
	g_signal_connect(G_OBJECT(play),"clicked", G_CALLBACK(gui_options_play_callback),chooser);
	g_signal_connect(G_OBJECT(stop),"clicked", G_CALLBACK(gui_options_stop_callback),chooser);

	return h;
}

static int gui_options_close(GtkWidget *w)
{
	(void)w;
	// If the called is g_options.main_window, then it is already in the destroy process
	if(w!=g_options.main_window)
		gtk_widget_destroy(g_options.main_window);
	g_options.main_window=NULL;

	if(is_dirty)
		rtconf_save();
	is_dirty=0;
	
	return FALSE;
}

static void gui_options_set_bool_callback(GtkToggleButton *button)
{
	struct b_option *option=g_object_get_data(G_OBJECT(button), "option");
	option->set_fn(gtk_toggle_button_get_active(button) == TRUE);

	is_dirty=1;
}

static void gui_options_set_file_callback(GtkFileChooser *chooser)
{
	struct s_option *option=g_object_get_data(G_OBJECT(chooser), "option");
	gchar *file=gtk_file_chooser_get_filename(chooser);
	option->set_fn(file);
	g_free(file);

	is_dirty=1;
}

static void gui_options_stop_callback(GtkButton *button, GtkFileChooser *chooser)
{
	(void)button;
	(void)chooser;
	execute_datapipe(&audio_stop_pipe, NULL);;
}

static void gui_options_play_callback(GtkButton *button, GtkFileChooser *chooser)
{
	(void)button;
	gchar *file=gtk_file_chooser_get_filename(chooser);
	execute_datapipe(&audio_play_once_pipe, file);;
	g_free(file);
}

SPHONE_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	struct GuiFunctions func = {};
	func.options_open = gtk_gui_options_open;
	*data = GINT_TO_POINTER(gui_register(func));
	return NULL;
}

SPHONE_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	gui_remove(GPOINTER_TO_INT(data));
}
