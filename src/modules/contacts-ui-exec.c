/*
 * contacts-ui-exec.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * contacts-ui-exec.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * contacts-ui-exec.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "sphone-conf.h"
#include "datapipes.h"
#include "datapipe.h"
#include "gui.h"
#include "types.h"

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

int gui_id;

static void contact_show_trigger(const Contact *data, void (*callback)(Contact*, void*), void *user_data)
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

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	gui_id = gui_register(NULL, NULL, NULL, NULL, NULL, contact_show_trigger, NULL);
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	gui_remove(gui_id);
}
