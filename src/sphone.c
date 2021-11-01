/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * main.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * main.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#define PRG_NAME			"sphone"

#include "store.h"
#include "sphone-log.h"
#include "sphone-conf.h"
#include "sphone-modules.h"
#include "datapipes.h"
#include "gui.h"
#include "gtk-gui.h"
#include "types.h"

typedef enum {
	SPHONE_CMD_DIALER_OPEN=1,
	SPHONE_CMD_SMS_NEW,
	SPHONE_CMD_HISTORY_SMS,
	SPHONE_CMD_OPTIONS,
	SPHONE_CMD_NONE,
} sphone_cmd;

static GDBusNodeInfo *dbus_introspection_data = NULL;

static const gchar dbus_introspection_xml[] =
  "<node>"
  "  <interface name='xyz.uvos.sphone.summon'>"
  "    <method name='OpenDialer'>"
  "    </method>"
  "    <method name='OpenSendMessage'>"
  "    </method>"
  "    <method name='OpenOptions'>"
  "    </method>"
  "    <method name='OpenMessageHistory'>"
  "    </method>"
  "  </interface>"
  "</node>";
  
static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
  guint registration_id;

  registration_id = g_dbus_connection_register_object(connection,
													 "/xyz/uvos/sphone/",
													 introspection_data->interfaces[0],
													 &interface_vtable,
													 NULL,  /* user_data */
													 NULL,  /* user_data_free_func */
													 NULL); /* GError** */
  g_assert(registration_id > 0);
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	struct sphone_options *options = user_data;
	store_init();
	gtk_gui_register();
	sphone_modules_init();
	
}

static void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	if(!connection) {
		sphone_log(LL_CRIT, "Can not connect to dbus");
		exit(-1);
	} else {
		sphone_log(LL_CRIT, "should send commands here");
		exit(0);
	}
}

struct sphone_options {
	sphone_cmd command;
	gchar *number;
}

int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_set_locale();
	gtk_init(&argc, &argv);

	struct sphone_options options;
	int c;
	int verbosity = LL_DEFAULT;
	guint owner_id;
	GMainLoop *loop;
	
	options.command = SPHONE_CMD_NONE;
	options.number = NULL;
	
	while ((c = getopt (argc, argv, ":hn:vc:")) != -1) {
		switch (c) {
			case '?':
				if (optopt == 'n')
					break;
			case 'h':
				printf("SPhone \n%s [hvc] \n"
				      "   -h\tDisplay this help\n"
				      "   -v\tEnable debug\n"
				      "   -n [number]\topen with number\n"
				      "   -c [cmd]\tExecute command. Accepted commands are: dialer-open, sms-new, history-sms, options\n"
				      , argv[0]);
				return 0;
			case 'c':
				if(!g_strcmp0(optarg,"dialer-open"))
					options.command = SPHONE_CMD_DIALER_OPEN;
				else if(!g_strcmp0(optarg,"sms-new"))
					options.command = SPHONE_CMD_SMS_NEW;
				else if(!g_strcmp0(optarg,"history-sms"))
					options.command = SPHONE_CMD_HISTORY_SMS;
				else if(!g_strcmp0(optarg,"options"))
					options.command = SPHONE_CMD_OPTIONS;
				break;
			case 'v':
				verbosity = LL_DEBUG;
				break;
			case 'n':
				printf("num: %s\n", optarg);
				options.number = optarg;
				break;
       }
	}
	
	sphone_log_open(PRG_NAME, LOG_USER, SPHONE_LOG_STDERR);
	sphone_log_set_verbosity(verbosity);
	
	gchar** numbersplit = NULL;
	if(options.number) {
		numbersplit = g_strsplit(options.number, ":", 2);
		if(numbersplit[1] != NULL) {
			options.number = numbersplit[1];
		}
		else {
			g_strfreev(numbersplit);
			numbersplit = NULL;
		}
	}

	g_type_init();

	dbus_introspection_data = g_dbus_node_info_new_for_xml(dbus_introspection_xml, NULL);
	if(!dbus_introspection_data) {
		sphone_log(LL_CRIT, "Creating dbus introspection data failed");
		return -1;
	}

	owner_id = g_bus_own_name (G_BUS_TYPE_SESSION, "xyz.uvos.sphone",
								G_BUS_NAME_OWNER_FLAGS_NONE,
								on_bus_acquired,
								on_name_acquired,
								on_name_lost,
								&options,
								NULL);

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run(loop);

	if(numbersplit)
		 g_strfreev(numbersplit);
	
	return 0;
}
