#include <glib.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "sphone-conf.h"
#include "datapipes.h"
#include "datapipe.h"

/** Module name */
#define MODULE_NAME		"contacts-ui-exec"

/** Functionality provided by this module */
static const gchar *const provides[] = { "contacts-ui", NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};

static void contact_show_trigger(const void *data, void *user_data)
{
	(void)data;
	(void)user_data;
	char *command = sphone_conf_get_string("ContactsUiExec", "ContactsExec", NULL, NULL);
	if(command) {
		sphone_module_log(LL_DEBUG, "%s", __func__);
		char *argv[] = {command, NULL};
		g_spawn_async(NULL, argv, NULL, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL);
		g_free(command);
	}
}

G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
	append_trigger_to_datapipe(&contact_show_pipe, contact_show_trigger, NULL);
	return NULL;
}

G_MODULE_EXPORT void g_module_unload(GModule *module);
void g_module_unload(GModule *module)
{
	(void)module;
	remove_trigger_from_datapipe(&contact_show_pipe, contact_show_trigger, NULL);
}
