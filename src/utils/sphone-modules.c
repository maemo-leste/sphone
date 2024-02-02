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
#include <gmodule.h>
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

static bool sphone_modules_check_provides(module_info_struct *new_module_info)
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

static bool sphone_modules_check_essential(void)
{
	bool foundRtconf = TRUE;
	
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


static bool sphone_module_exit(struct sphone_module* module)
{
	gpointer fnp = NULL;

	if (g_module_symbol(module->module, "sphone_module_exit", (void**)&fnp) == TRUE) {
		sphone_module_exit_fn *exit_fn = (sphone_module_exit_fn*)fnp;
		exit_fn(module->data);
		return true;
	} else {
		sphone_log(LL_DEBUG, "module %s: has no sphone_module_exit", sphone_modules_get_info(module)->name);
		return false;
	}
}

static bool sphone_module_init(struct sphone_module* module)
{
	gpointer fnp = NULL;
	sphone_module_init_fn *init_fn;
	if (g_module_symbol(module->module, "sphone_module_init", (void**)&fnp) == FALSE) {
		sphone_log(LL_ERR, "faled to load module %s: missing symbol sphone_module_init",
					sphone_modules_get_info(module)->name);
		return false;
	}
	init_fn = (sphone_module_init_fn*)fnp;
	const char* result = init_fn(&module->data);
	if(result) {
		sphone_log(LL_ERR, "faled to load module %s: %s",
					sphone_modules_get_info(module)->name, result);
		return false;
	}

	return true;
}

static bool sphone_modules_init_modules(void)
{
	for (GSList *element = modules; element; element = element->next) {
		bool ret = sphone_module_init(element->data);
		if(!ret)
			return false;
	}

	return true;
}

static struct sphone_module *sphone_module_load(const char *path)
{
	struct sphone_module *module = g_malloc(sizeof(*module));
	sphone_log(LL_DEBUG, "Loading module: %s", path);

	if ((module->module = g_module_open(path, 0)) != NULL) {
		module_info_struct *info = sphone_modules_get_info(module);
		bool blockLoad = FALSE;

		if (!info) {
			sphone_log(LL_ERR, "Failed to retrieve module information for: %s", path);
			g_module_close(module->module);
			g_free(module);
			blockLoad = TRUE;
		} else
		{
			if (!sphone_modules_check_provides(info)) {
				g_module_close(module->module);
				g_free(module);
				blockLoad = TRUE;
			}
			else {
				sphone_log(LL_INFO, "Loaded module %s", info->name);
			}
		}

		if (!blockLoad) {
			modules = g_slist_append(modules, module);
			return module;
		}
	} else {
		sphone_log(LL_WARN, "Failed to load module %s: %s; skipping", path, g_module_error());
	}
	return NULL;
}

static void sphone_modules_load(char **modlist)
{
	char *dir = NULL;
	int i;

	dir = sphone_conf_get_string(SPHONE_CONF_MODULES_GROUP,
				   SPHONE_CONF_MODULES_PATH,
				   DEFAULT_SPHONE_MODULE_PATH,
				   NULL);

	for (i = 0; modlist[i]; i++) {
		char *path = g_module_build_path(dir, modlist[i]);

		sphone_module_load(path);

		g_free(path);
	}

	g_free(dir);
}

bool sphone_module_insmod(const char *path)
{
	struct sphone_module *module = sphone_module_load(path);

	if(!module)
		return false;

	bool ret = sphone_module_init(module);

	if (!ret) {
		module_info_struct *info = sphone_modules_get_info(module);
		sphone_module_unload(info->name);
		return false;
	}

	return true;
}

bool sphone_module_unload(const char *name)
{
	for (GSList *module = modules; module; module = module->next) {
		module_info_struct *info = sphone_modules_get_info(module->data);
		if(g_strcmp0(info->name, name) == 0) {
			struct sphone_module *smodule = module->data;
			sphone_module_exit(smodule);
			g_module_close(smodule->module);
			g_free(module);
			sphone_log(LL_INFO, "Unloaded module: %s", name);
			return true;
		}
	}

	sphone_log(LL_INFO, "Could not unload: %s; Module not loaded", name);
	return false;
}

/**
 * Init function for the sphone-modules component
 *
 * @return TRUE on success, FALSE on failure
 */
bool sphone_modules_init(void)
{
	char **modlist = NULL;
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
			sphone_module_exit(module);
			g_module_close(module->module);
			g_free(module);
		}

		g_slist_free(modules);
		modules = NULL;
	}

	return;
}
