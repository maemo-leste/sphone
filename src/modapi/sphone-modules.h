/**
 * @file sphone-modules.h
 * Headers for the module handling for SPHONE
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
#ifndef _SPHONE_MODULES_H_
#define _SPHONE_MODULES_H_

#include <glib.h>
#include <gmodule.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Name of Modules configuration group */
#define SPHONE_CONF_MODULES_GROUP		"Modules"

/** Name of configuration key for module path */
#define SPHONE_CONF_MODULES_PATH		"ModulePath"

/** Name of configuration key for general modules to load */
#define SPHONE_CONF_MODULES_MODULES	"Modules"

/** Default value for module path */
#define DEFAULT_SPHONE_MODULE_PATH		"/usr/lib/sphone/modules"

typedef struct {
	const gchar *const name;
	const gchar *const *const provides;
// Module priority:
// higher value == higher priority
	const gint priority;
} module_info_struct;

gboolean sphone_modules_init(void);
void sphone_modules_exit(void);

typedef const char* sphone_module_init_fn(void** data);
typedef void sphone_module_exit_fn(void* data);

#ifdef __cplusplus
}
#endif


#endif /* _SPHONE_MODULES_H_ */
