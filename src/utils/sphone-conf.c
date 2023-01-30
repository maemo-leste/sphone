/**
 * @file sphone-conf.c
 * Configuration option handling for SPHONE
 * @author Jonathan Wilson
 * @author Carl Philipp Klemm <carl@uvos.xyz>
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sphone-conf.h"
#include "sphone-log.h"

struct sphone_conf_file
{
	gpointer keyfile;
	gchar *path;
	gchar *filename;
};

/** Pointer to the keyfile structure where config values are read from */
static struct sphone_conf_file *conf_files = NULL;
static size_t sphone_conf_file_count = 0;

static struct sphone_conf_file *sphone_conf_find_key_in_files(const gchar *group, const gchar *key) 
{
	GError *error = NULL;
	
	if (conf_files) {
		for (size_t i = sphone_conf_file_count; i > 0; --i) {
			if (g_key_file_has_key(conf_files[i-1].keyfile, group, key, &error) && 
				error == NULL) {
				g_clear_error(&error);
				return &(conf_files[i-1]);
			}
			else {
				g_clear_error(&error);
				error = NULL;
			}
		}
 	}
	return NULL;
}

static gpointer sphone_conf_decide_keyfile_to_use(const gchar *group, const gchar *key, gpointer keyfile)
{
	if (keyfile == NULL) {
		struct sphone_conf_file *conf_file;
		conf_file = sphone_conf_find_key_in_files(group, key);
		if (conf_file == NULL)
			sphone_log(LL_DEBUG, "sphone-conf: Could not get config key %s/%s", group, key);
		else
			keyfile = conf_file->keyfile;
	}

	return keyfile;
}

/**
 * Get a boolean configuration value
 *
 * @param group The configuration group to get the value from
 * @param key The configuration key to get the value of
 * @param defaultval The default value to use if the key isn't set
 * @param keyfileptr A keyfile pointer, or NULL to use the default keyfile
 * @return The configuration value on success, the default value on failure
 */
gboolean sphone_conf_get_bool(const gchar *group, const gchar *key,
			   const gboolean defaultval, gpointer keyfileptr)
{
	gboolean tmp = FALSE;
	GError *error = NULL;

	keyfileptr = sphone_conf_decide_keyfile_to_use(group, key, keyfileptr);
	if (keyfileptr == NULL) {
		tmp = defaultval;
		goto EXIT;
	}

	tmp = g_key_file_get_boolean(keyfileptr, group, key, &error);

	if (error != NULL) {
		sphone_log(LL_DEBUG, "sphone-conf: "
			"Could not get config key %s/%s; %s; "
			"defaulting to `%d'",
			group, key, error->message, defaultval);
		tmp = defaultval;
	}

	g_clear_error(&error);

EXIT:
	return tmp;
}

/**
 * Get an integer configuration value
 *
 * @param group The configuration group to get the value from
 * @param key The configuration key to get the value of
 * @param defaultval The default value to use if the key isn't set
 * @param keyfileptr A keyfile pointer, or NULL to use the default keyfile
 * @return The configuration value on success, the default value on failure
 */
gint sphone_conf_get_int(const gchar *group, const gchar *key,
		      const gint defaultval, gpointer keyfileptr)
{
	gint tmp = -1;
	GError *error = NULL;

	keyfileptr = sphone_conf_decide_keyfile_to_use(group, key, keyfileptr);
	if (keyfileptr == NULL) {
		tmp = defaultval;
		goto EXIT;
	}

	tmp = g_key_file_get_integer(keyfileptr, group, key, &error);

	if (error != NULL) {
		sphone_log(LL_DEBUG, "sphone-conf: "
			"Could not get config key %s/%s; %s; "
			"defaulting to `%d'",
			group, key, error->message, defaultval);
		tmp = defaultval;
	}

	g_clear_error(&error);

EXIT:
	return tmp;
}

/**
 * Get an integer list configuration value
 *
 * @param group The configuration group to get the value from
 * @param key The configuration key to get the value of
 * @param length The length of the list
 * @param keyfileptr A keyfile pointer, or NULL to use the default keyfile
 * @return The configuration value on success, NULL on failure
 */
gint *sphone_conf_get_int_list(const gchar *group, const gchar *key,
			    gsize *length, gpointer keyfileptr)
{
	gint *tmp = NULL;
	GError *error = NULL;

	keyfileptr = sphone_conf_decide_keyfile_to_use(group, key, keyfileptr);
	if (keyfileptr == NULL) {
		*length = 0;
		goto EXIT;
	}

	tmp = g_key_file_get_integer_list(keyfileptr, group, key,
					  length, &error);

	if (error != NULL) {
		sphone_log(LL_DEBUG, "sphone-conf: "
			"Could not get config key %s/%s; %s",
			group, key, error->message);
		*length = 0;
	}

	g_clear_error(&error);

EXIT:
	return tmp;
}

/**
 * Get a string configuration value
 *
 * @param group The configuration group to get the value from
 * @param key The configuration key to get the value of
 * @param defaultval The default value to use if the key isn't set
 * @param keyfileptr A keyfile pointer, or NULL to use the default keyfile
 * @return The configuration value on success, the default value on failure
 */
gchar *sphone_conf_get_string(const gchar *group, const gchar *key,
			   const gchar *defaultval, gpointer keyfileptr)
{
	gchar *tmp = NULL;
	GError *error = NULL;
	
	keyfileptr = sphone_conf_decide_keyfile_to_use(group, key, keyfileptr);
	if (keyfileptr == NULL) {
		if (defaultval != NULL)
			tmp = g_strdup(defaultval);
		goto EXIT;
	}

	tmp = g_key_file_get_string(keyfileptr, group, key, &error);

	if (error != NULL) {
		sphone_log(LL_DEBUG, "sphone-conf: "
			"Could not get config key %s/%s; %s; %s%s%s",
			group, key, error->message,
			defaultval ? "defaulting to `" : "no default set",
			defaultval ? defaultval : "",
			defaultval ? "'" : "");

		if (defaultval != NULL)
			tmp = g_strdup(defaultval);
	}

	g_clear_error(&error);

EXIT:
	return tmp;
}

/**
 * Get a string list configuration value
 *
 * @param group The configuration group to get the value from
 * @param key The configuration key to get the value of
 * @param length The length of the list
 * @param keyfileptr A keyfile pointer, or NULL to use the default keyfile
 * @return The configuration value on success, NULL on failure
 */
gchar **sphone_conf_get_string_list(const gchar *group, const gchar *key,
				 gsize *length, gpointer keyfileptr)
{
	gchar **tmp = NULL;
	GError *error = NULL;

	keyfileptr = sphone_conf_decide_keyfile_to_use(group, key, keyfileptr);
	if (keyfileptr == NULL) {
		*length = 0;
		goto EXIT;
	}

	tmp = g_key_file_get_string_list(keyfileptr, group, key,
					 length, &error);

	if (error != NULL) {
		sphone_log(LL_DEBUG, "sphone-conf: "
			"Could not get config key %s/%s; %s",
			group, key, error->message);
		*length = 0;
	}

	g_clear_error(&error);

EXIT:
	return tmp;
}

sphone_feature_t sphone_conf_get_features(void)
{
	static sphone_feature_t features = SPHONE_FEATURE_NONE;

	if(features != SPHONE_FEATURE_NONE)
		return features;

	gsize size;
	gchar** featureList = sphone_conf_get_string_list("Sphone", "Features", &size, NULL);
	if(!featureList) {
		sphone_log(LL_ERR, "sphone-conf: No sphone feature list found");
		return SPHONE_FEATURE_NONE;
	}

	for(gsize i = 0; i < size; ++i) {
		if(g_str_equal(featureList[i], "calls"))
			features |= SPHONE_FEATURE_CALLS;
		else if(g_str_equal(featureList[i], "messages"))
			features |= SPHONE_FEATURE_MESSAGES;
	}
	g_strfreev(featureList);
	return features;
}

/**
 * Free configuration file
 *
 * @param keyfileptr A pointer to the keyfile to free
 */
void sphone_conf_free_conf_file(gpointer keyfileptr)
{
	if (keyfileptr != NULL) {
		g_key_file_free(keyfileptr);
	}
}


static int sphone_conf_compare_file_prio(const void *a, const void *b)
{
	const struct sphone_conf_file *conf_a = (const struct sphone_conf_file *) a;
	const struct sphone_conf_file *conf_b = (const struct sphone_conf_file *) b;
	
	if (conf_a->filename == NULL && conf_b->filename == NULL)
		return 0;
	if (conf_a->filename == NULL && conf_b->filename != NULL)
		return -1;
	if (conf_a->filename != NULL && conf_b->filename == NULL)
		return 1;
	if (strcmp(conf_a->filename, G_STRINGIFY(SPHONE_SYSCONF_INI)) == 0)
		return -1;
	if (strcmp(conf_b->filename, G_STRINGIFY(SPHONE_SYSCONF_INI)) == 0)
		return 1;

	return strverscmp(conf_a->filename, conf_b->filename);
}

/**
 * Read configuration file
 *
 * @param conffile The full path to the configuration file to read
 * @return A keyfile pointer on success, NULL on failure
 */
gpointer sphone_conf_read_conf_file(const gchar *const conffile)
{
	GError *error = NULL;
	GKeyFile *keyfileptr;

	if ((keyfileptr = g_key_file_new()) == NULL)
		goto EXIT;

	if (g_key_file_load_from_file(keyfileptr, conffile,
				      G_KEY_FILE_NONE, &error) == FALSE) {
		sphone_conf_free_conf_file(keyfileptr);
		keyfileptr = NULL;
		sphone_log(LL_DEBUG, "sphone-conf: Could not load %s; %s",
			conffile, error->message);
		goto EXIT;
	}

EXIT:
	g_clear_error(&error);

	return keyfileptr;
}

static gboolean sphone_conf_is_ini_file(const char *filename)
{
	char *point_location = strrchr(filename, '.');
	if(point_location == NULL)
		return FALSE;
	else if(strcmp(point_location, ".ini") == 0)
		return TRUE;
	else
		return FALSE;
}

/**
 * Init function for the sphone-conf component
 *
 * @return TRUE on success, FALSE on failure
 */
gboolean sphone_conf_init(void)
{
	DIR *dir = NULL;
	sphone_conf_file_count = 1;
	struct dirent *direntry;

	const char *home = getenv("HOME");
	if(!home)
		home = "/home/user";

	gchar *override_dir_path = g_strconcat(home, "/", G_STRINGIFY(SPHONE_SYSCONF_OVR_DIR), NULL);
	dir = opendir(override_dir_path);
	if (dir) {
		while ((direntry = readdir(dir)) != NULL && telldir(dir)) {
			if ((direntry->d_type == DT_REG || direntry->d_type == DT_LNK) && 
				sphone_conf_is_ini_file(direntry->d_name))
				++sphone_conf_file_count;
		}
		rewinddir(dir);
	} else {
		sphone_log(LL_DEBUG, "sphone-conf: Could not open config overide dir %s",
				   override_dir_path);
	}
	g_free(override_dir_path);

	conf_files = calloc(sphone_conf_file_count, sizeof(*conf_files));
	
	conf_files[0].filename = g_strdup(G_STRINGIFY(SPHONE_SYSCONF_INI));
	conf_files[0].path     = g_strconcat(G_STRINGIFY(SPHONE_SYSCONF_DIR), "/", 
										 G_STRINGIFY(SPHONE_SYSCONF_INI), NULL);
	gpointer main_conf_file = sphone_conf_read_conf_file(conf_files[0].path);
	if (main_conf_file == NULL) {
		sphone_log(LL_ERR, "sphone-conf: failed to open main config file %s %s", 
				conf_files[0].path, g_strerror(errno));
		g_free(conf_files[0].filename);
		g_free(conf_files[0].path);
		free(conf_files);
		conf_files = NULL;
		return FALSE;
	}
	conf_files[0].keyfile = main_conf_file;

	if (dir) {
		size_t i = 1;
		direntry = readdir(dir);
		while (direntry != NULL && i < sphone_conf_file_count && telldir(dir)) {
			if ((direntry->d_type == DT_REG || direntry->d_type == DT_LNK) && 
				sphone_conf_is_ini_file(direntry->d_name)) {
				conf_files[i].filename = g_strdup(direntry->d_name);
				conf_files[i].path     = g_strconcat(home, "/", G_STRINGIFY(SPHONE_SYSCONF_OVR_DIR),
													 "/", conf_files[i].filename, NULL);
				conf_files[i].keyfile  = sphone_conf_read_conf_file(conf_files[i].path);
				 ++i;
			}
			direntry = readdir(dir);
		}
		closedir(dir);
		
		qsort(conf_files, sphone_conf_file_count, sizeof(*conf_files), &sphone_conf_compare_file_prio);
	}
	
	for (size_t i = 0; i < sphone_conf_file_count; ++i)
		sphone_log(LL_DEBUG, "sphone-conf: found conf file %lu: %s", (unsigned long)i, conf_files[i].filename);

	return TRUE;
}

/**
 * Exit function for the sphone-conf component
 */
void sphone_conf_exit(void)
{
	for (size_t i = 0; i < sphone_conf_file_count; ++i) {
		if (conf_files[i].filename)
			g_free(conf_files[i].filename);
		if (conf_files[i].path)
			g_free(conf_files[i].path);
		
		sphone_conf_free_conf_file(conf_files[i].keyfile);
	}

	return;
}
