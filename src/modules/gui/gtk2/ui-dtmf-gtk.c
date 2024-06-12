#include <gtk/gtk.h>
#include "types.h"
#include "sphone-modules.h"
#include "datapipe.h"
#include "datapipes.h"
#include "gui.h"
#include "keypad.h"

/** Module name */
#define MODULE_NAME		"ui-dtmf-gtk"

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

static bool dtmf_show(const CallProperties *call)
{
	(void)call;
	GtkWidget *window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(window),"DTMF Keypad");
	gtk_window_set_default_size(GTK_WINDOW(window),400,220);
	GtkWidget *keypad = gui_keypad_setup(NULL);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->vbox), keypad);
	gtk_widget_show_all(window);


	return true;
}

SPHONE_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	*data = GINT_TO_POINTER(gui_register(NULL, &dtmf_show, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
	return NULL;
}

SPHONE_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	int id = GPOINTER_TO_INT(data);
	gui_remove(id);
}
