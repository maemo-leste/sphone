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
#include "sphone-log.h"

static GSList *backends;

static CommBackend *default_backend;

int sphone_comm_add_backend(const char* name)
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
	
	sphone_log(LL_INFO, "Comm backend added: %s", backend->name);

	backends = g_slist_append(backends, backend);
	default_backend = backend;
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

	g_free(((CommBackend*)(element->data))->name);
	g_free(element->data);

	backends = g_slist_remove(backends, element->data);
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
