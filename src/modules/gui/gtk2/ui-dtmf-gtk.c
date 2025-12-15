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

static void dtmf_dialer_keypad_callback(const char *value, void *data)
{
	const CallProperties *call = data;

	sphone_dtmf_t dtmf;
	if(strcmp(value, "0") == 0)
		dtmf = SPHONE_DTMF_0;
	else if(strcmp(value, "1") == 0)
		dtmf = SPHONE_DTMF_1;
	else if(strcmp(value, "2") == 0)
		dtmf = SPHONE_DTMF_2;
	else if(strcmp(value, "3") == 0)
		dtmf = SPHONE_DTMF_3;
	else if(strcmp(value, "4") == 0)
		dtmf = SPHONE_DTMF_4;
	else if(strcmp(value, "5") == 0)
		dtmf = SPHONE_DTMF_5;
	else if(strcmp(value, "6") == 0)
		dtmf = SPHONE_DTMF_6;
	else if(strcmp(value, "7") == 0)
		dtmf = SPHONE_DTMF_7;
	else if(strcmp(value, "8") == 0)
		dtmf = SPHONE_DTMF_8;
	else if(strcmp(value, "9") == 0)
		dtmf = SPHONE_DTMF_9;
	else if(strcmp(value, "#") == 0)
		dtmf = SPHONE_DTMF_HASH;
	else if(strcmp(value, "*") == 0)
		dtmf = SPHONE_DTMF_STAR;
	else if(strcmp(value, "A") == 0)
		dtmf = SPHONE_DTMF_A;
	else if(strcmp(value, "B") == 0)
		dtmf = SPHONE_DTMF_B;
	else if(strcmp(value, "C") == 0)
		dtmf = SPHONE_DTMF_C;
	else if(strcmp(value, "D") == 0)
		dtmf = SPHONE_DTMF_D;
	else
		dtmf = SPHONE_DTMF_STOP;

	DtmfRequest request = {.dtmf = dtmf, .call = call};
	execute_datapipe(&call_dtmf_pipe, &request);
}

static bool dtmf_show(const CallProperties *call)
{
	CallProperties *call_cpy = call_properties_copy(call);
	GtkWidget *window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(window),"DTMF Keypad");
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 220);
	g_object_set_data_full(G_OBJECT(window), "call", call_cpy, (GDestroyNotify)call_properties_free);
	GtkWidget *keypad = gui_keypad_setup(dtmf_dialer_keypad_callback, (void*)call_cpy);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->vbox), keypad);
	gtk_widget_show_all(window);

	return true;
}

SPHONE_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	struct GuiFunctions func = {};
	func.dtmf_show = dtmf_show;
	*data = GINT_TO_POINTER(gui_register(func));
	return NULL;
}

SPHONE_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	int id = GPOINTER_TO_INT(data);
	gui_remove(id);
}
