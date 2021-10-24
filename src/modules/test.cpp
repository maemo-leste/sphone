#include <QWidget>
#include <QApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <glib.h>

extern "C" 
{
	
#include "sphone-modules.h"
#include "sphone-log.h"
#include "datapipe.h"
#include "datapipes.h"

}

/** Module name */
#define MODULE_NAME		"test"

QWidget* window;
QVBoxLayout* layout;
QPushButton* button;

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

extern "C" 
{

void contact_show_trigger(const void* data, void* user_data)
{
	(void)data;
	QWidget *window = (QWidget *)user_data;
	sphone_module_log(LL_DEBUG, "%s", __func__);
	window->show();
}

G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
	sphone_module_log(LL_DEBUG, "%s", __func__);
	window = new QWidget();
	button = new QPushButton("SPHONE!!", window);
	layout = new QVBoxLayout(window);
	layout->addWidget(button);
	window->setLayout(layout);
	
	append_trigger_to_datapipe(&contact_show_pipe, contact_show_trigger, window);
	
	return NULL;
}

G_MODULE_EXPORT void g_module_unload(GModule *mod);
void g_module_unload(GModule *mod)
{
	delete window;
	(void)mod;
}

}
