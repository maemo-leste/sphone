/*
 * comm-error-gtk.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * comm-error-gtk.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * comm-error-gtk.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <glib.h>
#include <gtk/gtk.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "datapipe.h"
#include "datapipes.h"

/** Module name */
#define MODULE_NAME		"comm-error-gtk"

/** Functionality provided by this module */
static const gchar *const provides[] = { "error", NULL };

/** Module information */
SPHONE_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 150
};

static void backend_error_trigger(gconstpointer data, gpointer user_data)
{
	(void)user_data;
	const char *msg = data;
	GtkWidget *dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", msg);
	g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_widget_show_all(dialog);
}

SPHONE_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	append_trigger_to_datapipe(&call_backend_error_pipe, backend_error_trigger, NULL);
	return NULL;
}

SPHONE_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	remove_trigger_from_datapipe(&call_backend_error_pipe, backend_error_trigger, NULL);
	(void)data;
}
