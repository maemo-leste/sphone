/**
 * @file sphone-conf.h
 * Headers for the configuration option handling for SPHONE
 * @author Jonathan Wilson <carl@uvos.xyz>
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
#ifndef _SPHONE_CONF_H_
#define _SPHONE_CONF_H_

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SPHONE_FEATURE_NONE = 0,
	SPHONE_FEATURE_CALLS = 1,
	SPHONE_FEATURE_MESSAGES = 1<<1,
} sphone_feature_t;

gboolean sphone_conf_get_bool(const gchar *group, const gchar *key,
			   const gboolean defaultval, gpointer keyfileptr);
gint sphone_conf_get_int(const gchar *group, const gchar *key,
		      const gint defaultval, gpointer keyfileptr);
gint *sphone_conf_get_int_list(const gchar *group, const gchar *key,
			    gsize *length, gpointer keyfileptr);
gchar *sphone_conf_get_string(const gchar *group, const gchar *key,
			   const gchar *defaultval, gpointer keyfileptr);
gchar **sphone_conf_get_string_list(const gchar *group, const gchar *key,
				 gsize *length, gpointer keyfileptr);
sphone_feature_t sphone_conf_get_features(void);

gpointer sphone_conf_read_conf_file(const gchar *const conffile);
void sphone_conf_free_conf_file(gpointer keyfileptr);

gboolean sphone_conf_init(void);
void sphone_conf_exit(void);

#ifdef __cplusplus
}
#endif
#endif /* _SPHONE_CONF_H_ */
