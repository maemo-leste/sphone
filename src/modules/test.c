#include <glib.h>
#include "sphone-modules.h"
#include "sphone-log.h"

/** Module name */
#define MODULE_NAME		"test"

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

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	sphone_module_log(LL_DEBUG, "works");
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
}
