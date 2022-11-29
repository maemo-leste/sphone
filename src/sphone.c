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

#include "sphone-log.h"
#include "sphone-conf.h"
#include "sphone-modules.h"
#include "datapipes.h"
#include "gui.h"
#include "types.h"
#include "comm.h"
#include "signal.h"

#define SPHONE_SERVICE "xyz.uvos.sphone"
#define SPHONE_PATH "/xyz/uvos/sphone/summon"
#define SPHONE_INTERFACE "xyz.uvos.sphone.summon"

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
"	<interface name='xyz.uvos.sphone.summon'>"
"		<method name='OpenDialer'>"
"			<arg name='line_identifier' type='s' direction='in'/>"
"			<arg name='protocoll' type='s' direction='in'/>"
"		</method>"
"		<method name='OpenSendMessage'>"
"			<arg name='line_identifier' type='s' direction='in'/>"
"			<arg name='text' type='s' direction='in'/>"
"			<arg name='protocoll' type='s' direction='in'/>"
"		</method>"
"		<method name='OpenOptions'>"
"		</method>"
"		<method name='OpenMessageHistory'>"
"		</method>"
"	</interface>"
"</node>"; 

void (*exit_main_loop)(void);
void (*main_loop_init)(int argc, char *argv[]);
void (*main_loop)(int argc, char *argv[]);

static void signal_handler(const int nr)
{
	switch (nr) {
	case SIGTERM:
	case SIGINT:
		exit_main_loop();
		break;

	default:
		break;
	}
}

static void method_call_callback(GDBusConnection* connection,
						const gchar* sender,
						const gchar* object_path,
						const gchar* interface_name,
						const gchar* method_name,
						GVariant* parameters,
						GDBusMethodInvocation* invocation,
						gpointer user_data);
  
static const GDBusInterfaceVTable vtable = {
  .method_call = method_call_callback,
  .get_property = NULL,
  .set_property = NULL
};

struct sphone_options {
	sphone_cmd command;
	gchar *number;
};

static void run_command(const struct sphone_options *options)
{
	sphone_log(LL_INFO, "running command");
	
	CommBackend *backend = sphone_comm_default_backend();
	
	switch(options->command) {
		case SPHONE_CMD_DIALER_OPEN:
		{
			CallProperties call = {
				.line_identifier = options->number,
				.backend = backend ? backend->id : 0
			};
			gui_dialer_show(&call);
			break;
		}
		case SPHONE_CMD_SMS_NEW:
		{
			MessageProperties msg = {
				.line_identifier = options->number,
				.backend = backend ? backend->id : 0
			};
			gui_sms_send_show(&msg);
			break;
		}
		case SPHONE_CMD_HISTORY_SMS:
			gui_history_sms();
			break;
		case SPHONE_CMD_OPTIONS:
			gui_options_open();
			break;
		default:
			break;
	}
}

static void send_command(GDBusConnection *connection, struct sphone_options *options)
{
	GVariant *resp = NULL;
	GVariant *params = NULL;
	GError *error = NULL;

	sphone_log(LL_INFO, "Sending commands to sphone instance");

	CommBackend *backend = sphone_comm_default_backend();

	switch(options->command) {
		case SPHONE_CMD_DIALER_OPEN:
			params = g_variant_new("(ss)", options->number ?: "", backend ? backend->name : "default");
			resp = g_dbus_connection_call_sync(connection,
				SPHONE_SERVICE, SPHONE_PATH,
				SPHONE_INTERFACE, "OpenDialer", params, NULL,
				(GDBusCallFlags)G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);
			break;
		case SPHONE_CMD_SMS_NEW:
			params = g_variant_new("(sss)", options->number ?: "", "", backend ? backend->name : "default");
			resp = g_dbus_connection_call_sync(connection,
				SPHONE_SERVICE, SPHONE_PATH,
				SPHONE_INTERFACE, "OpenSendMessage", params, NULL,
				(GDBusCallFlags)G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);
			break;
		case SPHONE_CMD_HISTORY_SMS:
			resp = g_dbus_connection_call_sync(connection,
				SPHONE_SERVICE, SPHONE_PATH,
				SPHONE_INTERFACE, "OpenMessageHistory", NULL, NULL,
				(GDBusCallFlags)G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);
			break;
		case SPHONE_CMD_OPTIONS:
		{
			resp = g_dbus_connection_call_sync(connection,
				SPHONE_SERVICE, SPHONE_PATH,
				SPHONE_INTERFACE, "OpenOptions", NULL, NULL,
				(GDBusCallFlags)G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);
			break;
		}
		default:
			break;
	}

	if(error) {
		sphone_log(LL_ERR, "failed to send command %s", error->message);
		g_error_free(error);
	}

	if(resp)
		g_variant_unref(resp);
}

static char* get_scheme_from_uri(const char* uri)
{
	GUri* guri = g_uri_parse(uri, G_URI_FLAGS_SCHEME_NORMALIZE, NULL);

	if(!guri)
		return NULL;

	char* scheme = g_strdup(g_uri_get_scheme(guri));
	g_uri_unref(guri);
	return scheme;
}

static int get_backend_id(const char* uri, const char* backend_name, BackendFlag flags)
{
	int ret = -1;
	if(g_str_equal(backend_name, "default")) {
		char* scheme = get_scheme_from_uri(uri);
		if(scheme) {
			sphone_log(LL_DEBUG, "got %s scheme for uri %s", scheme, uri);
			CommBackend* backend = sphone_comm_get_backend_for_scheme(scheme, flags);
			if(backend) {
				sphone_log(LL_DEBUG, "using backend %s for scheme %s", backend->name, scheme);
				ret = backend->id;
			}
			else {
				sphone_log(LL_WARN, "No backend to handle scheme %s avaialble", scheme);
			}
			g_free(scheme);
		}
		else {
			sphone_log(LL_DEBUG, "%s is not a uri", uri);
		}
	}
	else {
		ret = sphone_comm_find_backend_id(backend_name);
	}

	return ret;
}

static char* remove_scheme(const char* uri)
{
	gchar** split = g_strsplit(uri, ":", 2);
	char* ret = NULL;
	if(split[1] != NULL) {
		if(split[1][0] == '/' && split[1][1] == '/') {
			ret = g_strdup((split[1])+2);
		}
		else {
			ret = g_strdup(split[1]);
		}
	}
	else {
		ret = g_strdup(uri);
	}
	g_strfreev(split);
	return ret;
}

static void method_call_callback(GDBusConnection* connection,
						  const gchar* sender,
						  const gchar* object_path,
						  const gchar* interface_name,
						  const gchar* method_name,
						  GVariant* parameters,
						  GDBusMethodInvocation* invocation,
						  gpointer user_data)
{
	(void)connection;
	(void)sender;
	(void)object_path;
	(void)interface_name;
	(void)parameters;
	(void)invocation;
	(void)user_data;

	sphone_log(LL_DEBUG, "got dbus call to %s with %s parameters",
			   method_name, g_variant_get_type_string(parameters));

	if(g_strcmp0(method_name, "OpenDialer") == 0) {
		CallProperties *call = NULL;
		if(g_strcmp0(g_variant_get_type_string(parameters), "(ss)") == 0) {
			call = g_malloc0(sizeof(*call));
			char *backend_name;
			char *line_identifier;
			g_variant_get(parameters, "(ss)", &line_identifier, &backend_name);
			call->backend = get_backend_id(line_identifier, backend_name, BACKEND_FLAG_CALL);
			call->line_identifier = remove_scheme(line_identifier);
		}
		gui_dialer_show(call);
		g_free(call);
	}
	else if(g_strcmp0(method_name, "OpenSendMessage") == 0) {
		MessageProperties *message = NULL;
		if(g_strcmp0(g_variant_get_type_string(parameters), "(sss)") == 0) {
			message = g_malloc0(sizeof(*message));
			char *backend_name;
			char *line_identifier;
			g_variant_get(parameters, "(sss)", &line_identifier, &message->text, &backend_name);
			message->backend = get_backend_id(line_identifier, backend_name, BACKEND_FLAG_MESSAGE);
			message->line_identifier = remove_scheme(line_identifier);
		}
		gui_sms_send_show(message);
		g_free(message);
	}
	else if(g_strcmp0(method_name, "OpenOptions") == 0)
		gui_options_open();
	else if(g_strcmp0(method_name, "OpenMessageHistory") == 0)
		gui_history_sms();
	else
		sphone_log(LL_WARN, "Unkown dbus method %s called", method_name);
	g_dbus_method_invocation_return_value(invocation, NULL);
}
  
static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	(void)user_data;
	guint registration_id;
	(void)name;

	registration_id = g_dbus_connection_register_object(connection,
														SPHONE_PATH,
														dbus_introspection_data->interfaces[0],
														&vtable,
														NULL,  
														NULL,  
														NULL);
	if(registration_id <= 0) {
		sphone_log(LL_CRIT, "Can not register dbus object");
		exit(-1);
	}
	
	sphone_log(LL_DEBUG, "Registered dbus object");
}

static void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	struct sphone_options *options = user_data;
	(void)name;

	if(!connection) {
		sphone_log(LL_CRIT, "Can not connect to dbus");
		exit(-1);
	} else if(options->command != SPHONE_CMD_NONE) {
		send_command(connection, options);
	} else {
		sphone_log(LL_WARN, "Sphone is allready running.");
	}

	exit(0);
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	(void)connection;
	(void)name;
	struct sphone_options *options = user_data;
	sphone_log(LL_INFO, "Starting new instance");
	sphone_modules_init();
	sphone_log(LL_DEBUG, "Modules initalized");
	run_command(options);
	sphone_log(LL_DEBUG, "Instance setup finished");
}

static void load_loop_module(GModule **loop_module)
{
	char *module_dir_path = sphone_conf_get_string(SPHONE_CONF_MODULES_GROUP,
				SPHONE_CONF_MODULES_PATH,
				DEFAULT_SPHONE_MODULE_PATH,
				NULL);
	char *module_name = sphone_conf_get_string(SPHONE_CONF_MODULES_GROUP, "LoopModule", NULL, NULL);
	char *module_path;
	if(!module_name) {
		sphone_log(LL_CRIT, "A valid LoopModule must be specified!");
		exit(-1);
	}
	module_path = g_module_build_path(module_dir_path, module_name);
	sphone_log(LL_DEBUG, "Loading %s as loop module", module_path);
	*loop_module = g_module_open(module_path, 0);
	if(!*loop_module) {
		sphone_log(LL_CRIT, "Failed to load module %s no usable LoopModule available, abort\n%s", module_name, g_module_error());
		exit(-1);
	}

	gpointer fnp = NULL;
	if (g_module_symbol(*loop_module, "sphone_loop_setup", (void**)&fnp) == FALSE) {
		sphone_log(LL_CRIT, "Loop Module dosent contain sphone_loop_setup");
		exit(-1);
	}
	main_loop_init = fnp;

	if (g_module_symbol(*loop_module, "sphone_loop_run", (void**)&fnp) == FALSE) {
		sphone_log(LL_CRIT, "Loop Module dosent contain sphone_loop_run");
		exit(-1);
	}
	main_loop = fnp;
	
	if (g_module_symbol(*loop_module, "sphone_loop_exit", (void**)&fnp) == FALSE) {
		sphone_log(LL_CRIT, "Loop Module dosent contain sphone_loop_exit");
		exit(-1);
	}
	exit_main_loop = fnp;
}

int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	struct sphone_options options;
	int c;
	int verbosity = LL_DEFAULT;
	guint owner_id;
	GModule *loop_module = NULL;
	
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

	if(!sphone_conf_init()) {
		sphone_log(LL_CRIT, "sphone_conf_init failed");
		return -1;
	}

	load_loop_module(&loop_module);
	main_loop_init(argc, argv);

	dbus_introspection_data = g_dbus_node_info_new_for_xml(dbus_introspection_xml, NULL);
	if(!dbus_introspection_data) {
		sphone_log(LL_CRIT, "Creating dbus introspection data failed");
		return -1;
	}

	owner_id = g_bus_own_name (G_BUS_TYPE_SESSION, SPHONE_SERVICE,
								G_BUS_NAME_OWNER_FLAGS_NONE,
								on_bus_acquired,
								on_name_acquired,
								on_name_lost,
								&options,
								NULL);

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	main_loop(argc, argv);
	
	sphone_log(LL_INFO, "shuting down");

	sphone_modules_exit();
	datapipes_exit();
	
	if(owner_id > 0)
		g_bus_unown_name(owner_id);
	
	return 0;
}
