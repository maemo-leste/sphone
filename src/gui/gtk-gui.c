#include "gtk-gui.h"

#include "gui.h"
#include "gui-dialer.h"
#include "gui-options.h"
#include "gui-sms.h"

static int gtk_gui_id;

void gtk_gui_register(void)
{
	gtk_gui_calls_manager_init();
	gtk_gui_dialer_init();
	gtk_gui_sms_init();
	gtk_gui_id = gui_register(gtk_gui_dialer_show, gtk_gui_sms_send_show, gtk_gui_options_open, gtk_gui_history_sms);
}

void gtk_gui_unregister(void)
{
	gui_remove(gtk_gui_id);
	gtk_gui_calls_manager_exit();
	gtk_gui_dialer_exit();
	gtk_gui_sms_exit();
}
