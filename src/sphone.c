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
#include <gio/gio.h>
#include <sys/wait.h>
#include <time.h>

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
	SPHONE_CMD_HISTORY_CALLS,
	SPHONE_CMD_OPTIONS,
	SPHONE_CMD_INSMOD,
	SPHONE_CMD_RMMOD,
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
"		<method name='Insmod'>"
"			<arg name='path' type='s' direction='in'/>"
"		</method>"
"		<method name='Rmmod'>"
"			<arg name='name' type='s' direction='in'/>"
"		</method>"
"		<method name='OpenOptions'>"
"		</method>"
"		<method name='OpenMessageHistory'>"
"		</method>"
"		<method name='OpenCallHistory'>"
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
	bool unsupervised;
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
		case SPHONE_CMD_HISTORY_CALLS:
			gui_history_calls();
			break;
		case SPHONE_CMD_OPTIONS:
			gui_options_open();
			break;
		case SPHONE_CMD_INSMOD:
			sphone_module_insmod(options->number);
			break;
		case SPHONE_CMD_RMMOD:
			sphone_module_unload(options->number);
			break;
		default:
			break;
	}
}

static void send_command(GDBusConnection *connection, const struct sphone_options *options)
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
		case SPHONE_CMD_HISTORY_CALLS:
			resp = g_dbus_connection_call_sync(connection,
				SPHONE_SERVICE, SPHONE_PATH,
				SPHONE_INTERFACE, "OpenCallHistory", NULL, NULL,
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
		case SPHONE_CMD_INSMOD:
			params = g_variant_new("(s)", options->number ?: "none");
			resp = g_dbus_connection_call_sync(connection,
				SPHONE_SERVICE, SPHONE_PATH,
				SPHONE_INTERFACE, "Insmod", params, NULL,
				(GDBusCallFlags)G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);
			break;
		case SPHONE_CMD_RMMOD:
			params = g_variant_new("(s)", options->number ?: "none");
			resp = g_dbus_connection_call_sync(connection,
				SPHONE_SERVICE, SPHONE_PATH,
				SPHONE_INTERFACE, "Rmmod", params, NULL,
				(GDBusCallFlags)G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1, NULL, &error);
			break;
		break;
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
	gchar** split = g_strsplit(uri, ":", 2);
	char* ret = NULL;
	if(split[1] != NULL)
		ret = g_strdup(split[0]);
	g_strfreev(split);
	return ret;
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
	else if(g_strcmp0(method_name, "OpenCallHistory") == 0)
		gui_history_calls();
	else if(g_strcmp0(method_name, "Insmod") == 0) {
		if(g_strcmp0(g_variant_get_type_string(parameters), "(s)") == 0) {
			char *path;
			g_variant_get(parameters, "(s)", &path);
			sphone_module_insmod(path);
		}
		else {
			sphone_log(LL_WARN, "%s called with invalid parameters", method_name);
		}
	}
	else if(g_strcmp0(method_name, "Rmmod") == 0) {
		if(g_strcmp0(g_variant_get_type_string(parameters), "(s)") == 0) {
			char *name;
			g_variant_get(parameters, "(s)", &name);
			sphone_module_unload(name);
		}
		else {
			sphone_log(LL_WARN, "%s called with invalid parameters", method_name);
		}
	}
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
		exit(2);
	}
	
	sphone_log(LL_DEBUG, "Registered dbus object");
}

static _Noreturn void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	(void)user_data;
	(void)name;

	if(!connection) {
		sphone_log(LL_CRIT, "Can not connect to dbus");
		exit(1);
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

static bool sphone_check_main(void)
{
	// GIO will implicitly create a private instance of g_main_loop for itself that you can not exit
	// This is deliterious for sphone since sphone will fork() after this point which breaks this mainloop,
	// Since it expects a 1 to 1 corrispondance of mainloop fds to processies. Additionally Sphone may want to use
	// A qt main loop, which the implicitly created loop will interfere with again.
	// To avoid this we perform the dbus operations in a child process that we can then exit, avoiding gio messing up our
	// environment.
	pid_t pid = fork();

	if(pid < 0) {
		sphone_log(LL_CRIT, "Unable to create dbus child");
		exit(1);
	}

	if(pid == 0) {
		GDBusConnection *dbus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
		if(!dbus) {
			sphone_log(LL_CRIT, "Unable to connect to dbus");
			exit(10);
		}

		GDBusProxy *proxy = g_dbus_proxy_new_sync(dbus,
												G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS | G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
												NULL, "org.freedesktop.DBus", "/", "org.freedesktop.DBus", NULL, NULL);
		if(!proxy) {
			sphone_log(LL_CRIT, "Unable to aquire dbus proxy for /org/freedesktop/DBus");
			exit(10);
		}

		GError *err = NULL;
		GVariant* var = g_dbus_proxy_call_sync(proxy, "ListNames", NULL, G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &err);
		if(!var) {
			sphone_log(LL_CRIT, "Unable to execute org.freedesktop.DBus.ListNames: %s", err->message);
			g_error_free(err);
			exit(10);
		}

		GVariantType *expected_type = g_variant_type_new("(as)");
		if(!g_variant_is_of_type(var, expected_type)) {
			sphone_log(LL_CRIT, "Dbus returned value of type %s but expected (as)", g_variant_type_dup_string(g_variant_get_type(var)));
			exit(10);
		}
		g_variant_type_free(expected_type);
		GVariant *array = g_variant_get_child_value(var, 0);

		size_t length;
		const char **services = g_variant_get_strv(array, &length);

		bool found = false;
		for(size_t i = 0; i < length; ++i) {
			if(g_strcmp0(services[i], SPHONE_SERVICE) == 0) {
				found = true;
				break;
			}
		}

		g_free(services);
		g_variant_unref(var);
		g_variant_unref(array);
		g_object_unref(proxy);
		g_object_unref(dbus);
		exit(!found);
	}
	else {
		int exit_code;
		waitpid(pid, &exit_code, 0);
		if(WEXITSTATUS(exit_code) > 1) {
			sphone_log(LL_CRIT, "Child exited with %i", WEXITSTATUS(exit_code));
			exit(exit_code);
		}
		return exit_code;
	}
}

static int sphone_secondary(const struct sphone_options *options)
{
	GDBusConnection *dbus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	if(!dbus) {
		sphone_log(LL_CRIT, "Unable to connect to dbus");
		return 1;
	}

	send_command(dbus, options);
	g_object_unref(dbus);
	return 0;
}

static int sphone_main(struct sphone_options options, int argc, char *argv[])
{
	guint owner_id;
	GModule *loop_module = NULL;


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

	append_filter_to_datapipe(&comm_backend_removed_pipe, drop, NULL);
	sphone_modules_exit();
	remove_filter_from_datapipe(&comm_backend_removed_pipe, drop, NULL);
	datapipes_exit();

	g_bus_unown_name(owner_id);

	return 0;
}

static pid_t sphone_create_child(struct sphone_options *options, int argc, char *argv[])
{
	pid_t pid = fork();
	if(pid < 0) {
		sphone_log(LL_ERR, "Unable to create child process");
		exit(1);
	}

	if(pid == 0) {
		sphone_log(LL_INFO, "Starting new instance");
		int ret = sphone_main(*options, argc, argv);
		exit(ret);
	}

	return pid;
}

static int sphone_supervise(struct sphone_options *options, int argc, char *argv[])
{
	while(true) {
		time_t starttime = time(NULL);
		pid_t pid = sphone_create_child(options, argc, argv);

		options->command = SPHONE_CMD_NONE;

		int exit_code;
		sphone_log(LL_DEBUG, "Waiting for sphone child to exit");
		waitpid(pid, &exit_code, 0);
		if(exit_code != 0) {
			sphone_log(LL_WARN, "--- Sphone has crashed ---");
			if(time(NULL) - starttime < 5) {
				sphone_log(LL_ERR, "Sphone is crashing to fast, aborting");
				return exit_code;
			}

			sphone_log(LL_WARN, "Restarting Sphone");
		}
		else {
			return 0;
		}
	}
}

int main(int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	struct sphone_options options = {};
	int c;
	int verbosity = LL_DEFAULT;
	
	options.command = SPHONE_CMD_NONE;
	options.number = NULL;
	options.unsupervised = false;
	
	while ((c = getopt (argc, argv, ":hsn:vc:")) != -1) {
		switch (c) {
			case '?':
				if (optopt == 'n')
					break;
			case 'h':
				printf("SPhone \n%s [hvc] \n"
				      "   -h\tDisplay this help\n"
				      "   -v\tEnable debug\n"
				      "   -n [number]\topen with number\n"
				      "   -s  do not self superivse"
				      "   -c [cmd]\tExecute command. Accepted commands are: dialer-open, history-calls, sms-new, history-sms, options, insmod, rmmod\n"
				      , argv[0]);
				return 0;
			case 'c':
				if(!g_strcmp0(optarg, "dialer-open"))
					options.command = SPHONE_CMD_DIALER_OPEN;
				else if(!g_strcmp0(optarg, "history-calls"))
					options.command = SPHONE_CMD_HISTORY_CALLS;
				else if(!g_strcmp0(optarg, "sms-new"))
					options.command = SPHONE_CMD_SMS_NEW;
				else if(!g_strcmp0(optarg,"history-sms"))
					options.command = SPHONE_CMD_HISTORY_SMS;
				else if(!g_strcmp0(optarg, "options"))
					options.command = SPHONE_CMD_OPTIONS;
				else if(!g_strcmp0(optarg, "insmod"))
					options.command = SPHONE_CMD_INSMOD;
				else if(!g_strcmp0(optarg, "rmmod"))
					options.command = SPHONE_CMD_RMMOD;
				break;
				break;
			case 's':
				options.unsupervised = true;
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

	datapipes_init();

	bool is_main = sphone_check_main();
	sphone_log(LL_DEBUG, "%s", is_main ? "Main Process" : "Secondary Process");

	if(is_main) {
		if(options.unsupervised) {
			sphone_log(LL_INFO, "Running without self-supervision");
			return sphone_main(options, argc, argv);
		}
		else {
			return sphone_supervise(&options, argc, argv);
		}
	}
	else {
		return sphone_secondary(&options);
	}
}
