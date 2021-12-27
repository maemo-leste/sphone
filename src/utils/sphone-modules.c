/**
 * @file sphone-modules.c
 * module handling for sphone
 * @author David Weinehall <david.weinehall@nokia.com>
 * @author Jonathan Wilson <jfwfreo@tpgi.com.au>
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
#include "sphone-modules.h"
#include "sphone-log.h"
#include "sphone-conf.h"

struct sphone_module {
	GModule *module;
	void *data;
};

/** List of all loaded modules */
static GSList *modules = NULL;

static module_info_struct *sphone_modules_get_info(struct sphone_module *module)
{
	gpointer mip = NULL;
	if(g_module_symbol(module->module, "module_info", (void**)&mip) == FALSE)
		return NULL;
	return (module_info_struct*)mip;
}

static gboolean sphone_modules_check_provides(module_info_struct *new_module_info)
{
	for (GSList *module = modules; module; module = module->next) {
		module_info_struct *module_info = sphone_modules_get_info(module->data);
		for (int i = 0; new_module_info->provides[i]; ++i) {
			for (int j = 0; module_info->provides[j]; ++j) {
				if (g_strcmp0(new_module_info->provides[i], module_info->provides[j]) == 0) {
					sphone_log(LL_WARN,
							   "Module %s has the same provides as module %s, and will not be loaded.",
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
	gboolean foundRtconf = TRUE;
	
	for (GSList *module = modules; module; module = module->next) {
		module_info_struct *module_info = sphone_modules_get_info(module->data);
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

static gboolean sphone_modules_init_modules(void)
{
	for (GSList *element = modules; element; element = element->next) {
		gpointer fnp = NULL;
		sphone_module_init_fn *init_fn;
		struct sphone_module *module = element->data;
		if (g_module_symbol(module->module, "sphone_module_init", (void**)&fnp) == FALSE) {
			sphone_log(LL_ERR, "faled to load module %s: missing symbol sphone_module_init",
					   sphone_modules_get_info(module)->name);
			return FALSE;
		}
		init_fn = (sphone_module_init_fn*)fnp;
		const char* result = init_fn(&module->data);
		if(result) {
			sphone_log(LL_ERR, "faled to load module %s: %s",
					   sphone_modules_get_info(module)->name, result);
			return FALSE;
		}
	}

	return TRUE;
}

static void sphone_modules_load(gchar **modlist)
{
	gchar *path = NULL;
	int i;

	path = sphone_conf_get_string(SPHONE_CONF_MODULES_GROUP,
				   SPHONE_CONF_MODULES_PATH,
				   DEFAULT_SPHONE_MODULE_PATH,
				   NULL);

	for (i = 0; modlist[i]; i++) {
		struct sphone_module *module = g_malloc(sizeof(*module));
		gchar *tmp = g_module_build_path(path, modlist[i]);

		sphone_log(LL_DEBUG, "Loading module: %s from %s", modlist[i], path);

		if ((module->module = g_module_open(tmp, 0)) != NULL) {
			module_info_struct *info = sphone_modules_get_info(module);
			gboolean blockLoad = FALSE;

			if (!info) {
				sphone_log(LL_ERR, "Failed to retrieve module information for: %s", modlist[i]);
				g_module_close(module->module);
				g_free(module);
				blockLoad = TRUE;
			} else if (!sphone_modules_check_provides(info)) {
				g_module_close(module->module);
				g_free(module);
				blockLoad = TRUE;
			}

			if (!blockLoad)
				modules = g_slist_append(modules, module);
		} else {
			sphone_log(LL_WARN, "Failed to load module %s: %s; skipping", modlist[i], g_module_error());
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
	gsize length;

	/* Get the list modules to load */
	modlist = sphone_conf_get_string_list(SPHONE_CONF_MODULES_GROUP,
					   SPHONE_CONF_MODULES_MODULES,
					   &length,
					   NULL);

	if (modlist) {
		sphone_modules_load(modlist);
		g_strfreev(modlist);

		if(!sphone_modules_check_essential())
			return FALSE;

		return sphone_modules_init_modules();
	}

	return TRUE;
}

/**
 * Exit function for the sphone-modules component
 */
void sphone_modules_exit(void)
{
	struct sphone_module *module;
	gint i;

	if (modules != NULL) {
		for (i = 0; (module = g_slist_nth_data(modules, i)) != NULL; i++) {
			gpointer fnp = NULL;

			if (g_module_symbol(module->module, "sphone_module_exit", (void**)&fnp) == TRUE) {
				sphone_module_exit_fn *exit_fn = (sphone_module_exit_fn*)fnp;
				exit_fn(module->data);
			} else {
				sphone_log(LL_DEBUG, "module %s: has no sphone_module_exit", sphone_modules_get_info(module)->name);
			}
			
			g_module_close(module->module);
			g_free(module);
		}

		g_slist_free(modules);
		modules = NULL;
	}

	return;
}
