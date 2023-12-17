/*
 * comm.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * comm.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * comm.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "comm.h"
#include "datapipes.h"
#include "sphone-log.h"
#include <sys/types.h>

static GSList *backends;

static CommBackend *default_backend;

static Scheme *sphone_comm_copy_scheme(const Scheme* scheme)
{
	Scheme* copy = g_malloc0(sizeof(*copy));
	copy->scheme = g_strdup(scheme->scheme);
	copy->flags = scheme->flags;
	return copy;
}

static void sphone_comm_free_scheme(Scheme* scheme)
{
	g_free(scheme->scheme);
	g_free(scheme);
}

static Scheme **sphone_comm_copy_scheme_array(const Scheme** schemes)
{
	size_t len = 0;
	while(schemes[len])
		++len;
	++len;
	Scheme** copy = g_malloc0(sizeof(*copy)*len);

	for(len = 0; schemes[len]; ++len)
		copy[len] = sphone_comm_copy_scheme(schemes[len]);

	return copy;
}

static bool sphone_comm_any_ch(uint32_t codepoint)
{
	(void)codepoint;
	return true;
}

static void sphone_comm_free_scheme_array(Scheme** schemes)
{
	for(size_t i = 0; schemes[i]; ++i)
		sphone_comm_free_scheme(schemes[i]);
	g_free(schemes);
}

int sphone_comm_add_backend(const char* name, const Scheme** schemes, BackendFlag flags, bool (*is_valid_ch)(uint32_t codepoint))
{
	CommBackend *backend = g_malloc0(sizeof(*backend));
	int id = 0;
	if(backends != NULL) {
		GSList *element = g_slist_last(backends);
		CommBackend *last_backend = (CommBackend*)element->data;
		id = last_backend->id+1;
	}
	backend->id = id;
	backend->name = g_strdup(name);
	backend->flags = flags;
	backend->schemes = sphone_comm_copy_scheme_array(schemes);
	backend->is_valid_ch = is_valid_ch ?: &sphone_comm_any_ch;
	
	sphone_log(LL_INFO, "Comm backend added: %s", backend->name);

	backends = g_slist_append(backends, backend);
	default_backend = backend;

	execute_datapipe(&comm_backend_added_pipe, backend);

	return id;
}

void sphone_comm_remove_backend(int id)
{
	GSList *element;
	for(element = backends; element; element = element->next) {
		CommBackend *last_backend = (CommBackend*)element->data;
		if(last_backend->id == id)
			break;
	}
	if (!element)
		sphone_log(LL_WARN, "Trying to remove non-existing comm backend with id %d", id);

	if(default_backend && default_backend->id == id)
		default_backend = NULL;

	CommBackend* backend = (CommBackend*)(element->data);

	execute_datapipe(&comm_backend_removed_pipe, backend);

	g_free(backend->name);
	sphone_comm_free_scheme_array(backend->schemes);
	g_free(element->data);

	backends = g_slist_remove(backends, element->data);
}

CommBackend *sphone_comm_get_backend_for_scheme(const char* scheme, BackendFlag requiredFlags)
{
	GSList *element;
	for(element = backends; element; element = element->next) {
		CommBackend *backend = (CommBackend*)element->data;
		for(size_t i = 0; backend->schemes[i]; ++i) {
			if((requiredFlags & ~backend->schemes[i]->flags) == 0 && g_str_equal(backend->schemes[i]->scheme, scheme))
				return backend;
		}
	}
	return NULL;
}

GSList *sphone_comm_get_backends(void)
{
	return backends;
}

CommBackend *sphone_comm_default_backend(void)
{
	return default_backend;
}

CommBackend *sphone_comm_get_backend(int id)
{
	GSList *element;
	for(element = backends; element; element = element->next) {
		CommBackend *last_backend = (CommBackend*)element->data;
		if(last_backend->id == id)
			return element->data;
	}
	return NULL;
}

bool sphone_comm_set_default_backend(int id)
{
	GSList *element;
	for(element = backends; element; element = element->next) {
		CommBackend *last_backend = (CommBackend*)element->data;
		if(last_backend->id == id) {
			default_backend = last_backend;
			break;
		}
	}
	return (bool)element;
}

int sphone_comm_find_backend_id(const char* name)
{
	GSList *element;
	for(element = backends; element; element = element->next) {
		CommBackend *last_backend = (CommBackend*)element->data;
		if(g_strcmp0(last_backend->name, name) == 0)
			return last_backend->id;
	}
	return -1;
}

bool sphone_comm_valid_string(int id, const char* str)
{
	GError *err = NULL;
	glong cpoints;
	CommBackend *backend = sphone_comm_get_backend(id);
	uint32_t *unistring = g_utf8_to_ucs4(str, -1, NULL, &cpoints, &err);

	if(err) {
		sphone_log(LL_DEBUG, "user input contains an invalid unicode codepoint");
		g_clear_error(&err);
		return false;
	}

	bool ret = true;
	for(ssize_t i = 0; i < cpoints && ret; ++i) {
		if(!backend->is_valid_ch(unistring[i]))
			ret = false;
	}

	g_free(unistring);

	return ret;
}

char *sphone_comm_create_cleaned_string(int id, const char* str)
{
	GError *err = NULL;
	glong cpoints;
	CommBackend *backend = sphone_comm_get_backend(id);
	uint32_t *unistring = g_utf8_to_ucs4(str, -1, NULL, &cpoints, &err);
	uint32_t *resultunistring = g_malloc0(sizeof(*resultunistring)*cpoints+1);

	if(err) {
		sphone_log(LL_DEBUG, "user input contains an invalid unicode codepoint");
		g_clear_error(&err);
		return NULL;
	}

	ssize_t j = 0;
	for(ssize_t i = 0; i < cpoints; ++i) {
		if(backend->is_valid_ch(unistring[i])) {
			resultunistring[j] = unistring[i];
			++j;
		}
	}

	g_free(unistring);

	char *result = g_ucs4_to_utf8(resultunistring, -1, NULL, NULL, NULL);

	g_free(resultunistring);

	return result;
}
