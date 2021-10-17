/**
 * @file sphone-modules.c
 * Module handling for sphone
 * @author Carl Klemm <carl@uvos.xyz>
 *
 * sphone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * sphone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with sphone.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <glib.h>
#include <gmodule.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "sphone-conf.h"

/** List of all loaded modules */
static GSList *modules = NULL;

static gboolean sphone_modules_check_provides(module_info_struct *new_module_info)
{
	for (GSList *module = modules; module; module = module->next) {
		gpointer mip = NULL;
		module_info_struct *module_info;
		if (g_module_symbol(module->data, "module_info", &mip) == FALSE)
			continue;
		module_info = (module_info_struct*)mip;
		for (int i = 0; new_module_info->provides[i]; ++i) {
			for (int j = 0; module_info->provides[j]; ++j) {
				if (g_strcmp0(new_module_info->provides[i], module_info->provides[j]) == 0) {
					sphone_log(LL_WARN, "Module %s has the same provides as module %s, and will not be loaded.",
							new_module_info->name, module_info->name);
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

static gboolean sphone_modules_check_essential(void)
{
	gboolean foundRtconf = FALSE;
	
	for (GSList *module = modules; module; module = module->next) {
		gpointer mip = NULL;
		module_info_struct *module_info;
		if (g_module_symbol(module->data, "module_info", (void**)&mip) == FALSE)
			continue;
		module_info = (module_info_struct*)mip;
		for (int j = 0; module_info->provides[j]; ++j) {
			if (g_strcmp0("rtconf", module_info->provides[j]) == 0) {
				foundRtconf = TRUE;
				break;
			}
		}
	}
	
	if (!foundRtconf) {
		sphone_log(LL_ERR, "Could not find nessecary rtconf module aborting.");
		return FALSE;
	}
	
	return TRUE;
}

static void sphone_modules_load(gchar **modlist)
{
	gchar *path = NULL;
	int i;

	path = sphone_conf_get_string(MCE_CONF_MODULES_GROUP,
				   MCE_CONF_MODULES_PATH,
				   DEFAULT_MCE_MODULE_PATH,
				   NULL);

	for (i = 0; modlist[i]; i++) {
		GModule *module;
		gchar *tmp = g_module_build_path(path, modlist[i]);

		sphone_log(LL_DEBUG, "Loading module: %s from %s", modlist[i], path);

		if ((module = g_module_open(tmp, 0)) != NULL) {
			gpointer mip = NULL;
			gboolean blockLoad = FALSE;

			if (g_module_symbol(module, "module_info", &mip) == FALSE) {
				sphone_log(LL_ERR, "Failed to retrieve module information for: %s", modlist[i]);
				g_module_close(module);
				blockLoad = TRUE;
			} else if (!sphone_modules_check_provides((module_info_struct*)mip)) {
				g_module_close(module);
				blockLoad = TRUE;
			}

			if (!blockLoad)
				modules = g_slist_append(modules, module);
		} else {
			sphone_log(LL_WARN, "Failed to load module: %s; skipping", modlist[i]);
		}

		g_free(tmp);
	}

	g_free(path);
}

/**
 * Init function for the sphone-modules component
 *
 * @return TRUE on success, FALSE on failure
 */
gboolean sphone_modules_init(void)
{
	gchar **modlist = NULL;
	gchar **modlist_device = NULL;
	gchar **modlist_user = NULL;
	gsize length;

	/* Get the list modules to load */
	modlist = sphone_conf_get_string_list(MCE_CONF_MODULES_GROUP,
					   MCE_CONF_MODULES_MODULES,
					   &length,
					   NULL);

	modlist_device = sphone_conf_get_string_list(MCE_CONF_MODULES_GROUP,
					   MCE_CONF_MODULES_DEVMODULES,
					   &length,
					   NULL);

	modlist_user = sphone_conf_get_string_list(MCE_CONF_MODULES_GROUP,
					   MCE_CONF_MODULES_USRMODULES,
					   &length,
					   NULL);

	if (modlist)
		sphone_modules_load(modlist);
	if (modlist_device)
		sphone_modules_load(modlist_device);
	if (modlist_user)
		sphone_modules_load(modlist_user);

	g_strfreev(modlist);
	g_strfreev(modlist_device);
	g_strfreev(modlist_user);

	return sphone_modules_check_essential();
}

/**
 * Exit function for the sphone-modules component
 */
void sphone_modules_exit(void)
{
	GModule *module;
	gint i;

	if (modules != NULL) {
		for (i = 0; (module = g_slist_nth_data(modules, i)) != NULL; i++) {
			g_module_close(module);
		}

		g_slist_free(modules);
		modules = NULL;
	}

	return;
}
