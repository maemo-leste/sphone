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
G_MODULE_EXPORT module_info_struct module_info = {
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
	GtkWidget *dialog = gtk_dialog_new_with_buttons ("Error", NULL,
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "Ok",
		GTK_RESPONSE_NONE, NULL);
	g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
	GtkWidget *label = gtk_label_new(msg);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);
	gtk_widget_show_all(dialog);
}

G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
	append_trigger_to_datapipe(&call_backend_error_pipe, backend_error_trigger, NULL);
	return NULL;
}

G_MODULE_EXPORT void g_module_unload(GModule *module);
void g_module_unload(GModule *module)
{
	remove_trigger_from_datapipe(&call_backend_error_pipe, backend_error_trigger);
	(void)module;
}
