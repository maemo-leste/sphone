#pragma once
#include <glib.h>
#include <stdbool.h>

typedef struct  _CommBackend {
	char* name;
	int id;
} CommBackend;

int sphone_comm_add_backend(const char* name);

void sphone_comm_remove_backend(int id);

GSList *sphone_comm_get_backends(void);

CommBackend *sphone_comm_default_backend(void);

CommBackend *sphone_comm_get_backend(int id);

bool sphone_comm_set_default_backend(int id);
