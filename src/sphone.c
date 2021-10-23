/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
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
#include <unique/unique.h>

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

#include "gui-calls-manager.h"
#include "gui-dialer.h"
#include "gui-options.h"
#include "utils.h"
#include "gui-sms.h"
#include "store.h"
#include "book-import.h"
#include "gui-contact-view.h"
#include "sphone-log.h"
#include "sphone-conf.h"
#include "sphone-modules.h"
#include "datapipes.h"

typedef enum {
	SPHONE_CMD_DIALER_OPEN=1,
	SPHONE_CMD_SMS_NEW,
	SPHONE_CMD_HISTORY_CALLS,
	SPHONE_CMD_HISTORY_SMS,
	SPHONE_CMD_OPTIONS,
	SPHONE_CMD_NONE,
} sphone_cmd;

static UniqueResponse main_message_received_callback(UniqueApp *app, gint command,
													 UniqueMessageData *message_data,
													 guint time_, gpointer user_data)
{
	(void)app;
	(void)time_;
	(void)user_data;

	gchar *number = NULL;
	if(message_data)
		number = unique_message_data_get_text(message_data);
	
	if(number)
		sphone_log(LL_DEBUG, "number: %s\n", number);

	switch(command){
		case SPHONE_CMD_DIALER_OPEN:
			gui_dialer_show(number);
			break;
		case SPHONE_CMD_SMS_NEW:
			gui_sms_send_show(number,NULL);
			break;
		case SPHONE_CMD_HISTORY_CALLS:
			gui_history_calls();
			break;
		case SPHONE_CMD_HISTORY_SMS:
			gui_history_sms();
			break;
		case SPHONE_CMD_OPTIONS:
			gui_options_open();
			break;
		default:
			sphone_log(LL_ERR, "Invalid command: %d\n",command);
			return UNIQUE_RESPONSE_OK;
	}

	return UNIQUE_RESPONSE_INVALID;
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

	sphone_cmd command = SPHONE_CMD_NONE;
	gboolean is_done=FALSE;
	int c;
	int verbosity = LL_DEFAULT;

	gchar *number = NULL;
	
	while ((c = getopt (argc, argv, ":hn:vc:i:")) != -1) {
		switch (c) {
			case '?':
				if (optopt == 'n')
					break;
			case 'h':
				printf("SPhone \n%s [hvc] \n"
				      "   -h\tDisplay this help\n"
				      "   -v\tEnable debug\n"
					  "   -n [number]\topen with number\n"
				      "   -c [cmd]\tExecute command. Accepted commands are: dialer-open, sms-new, history-calls, history-sms, options\n"
				      "   -i [file]\timport contacts XML\n", argv[0]);
				return 0;
			case 'c':
				if(!g_strcmp0(optarg,"dialer-open"))
					command = SPHONE_CMD_DIALER_OPEN;
				else if(!g_strcmp0(optarg,"sms-new"))
					command = SPHONE_CMD_SMS_NEW;
				else if(!g_strcmp0(optarg,"history-sms"))
					command = SPHONE_CMD_HISTORY_SMS;
				else if(!g_strcmp0(optarg,"history-calls"))
					command = SPHONE_CMD_HISTORY_CALLS;
				else if(!g_strcmp0(optarg,"options"))
					command = SPHONE_CMD_OPTIONS;
				break;
			case 'i':
				store_init();
				book_import(optarg);
				is_done=TRUE;
				break;
			case 'v':
				verbosity = LL_DEBUG;
				break;
			case 'n':
				printf("num: %s\n", optarg);
				number = optarg;
				break;
       }
	}
	
	sphone_log_open(PRG_NAME, LOG_USER, SPHONE_LOG_STDERR);
	sphone_log_set_verbosity(verbosity);
	
	gchar** numbersplit = NULL;
	if(number) {
		numbersplit = g_strsplit(number, ":", 2);
		if(numbersplit[1] != NULL) {
			number = numbersplit[1];
		}
		else {
			g_strfreev(numbersplit);
			numbersplit = NULL;
		}
	}

	UniqueApp *unique = unique_app_new_with_commands("org.maemo.sphone", NULL
	                                               ,"dialer-open", SPHONE_CMD_DIALER_OPEN
	                                               ,"history-sms", SPHONE_CMD_HISTORY_SMS
	                                               ,"history-calls", SPHONE_CMD_HISTORY_CALLS
	                                               ,"sms-new", SPHONE_CMD_SMS_NEW
	                                               ,"options", SPHONE_CMD_OPTIONS, NULL);

	if (!is_done && !unique_app_is_running(unique)) {
		sphone_log(LL_INFO,  "Staring new instance");
		
		datapipes_init();
		
		if(!sphone_conf_init()) {
			sphone_log(LL_ERR,  "sphone_conf_init failed");
			return -1;
		}
		
		store_init();
		gui_calls_manager_init();
		gui_dialer_init();
		gui_sms_init();

		if(!sphone_modules_init()) {
			sphone_log(LL_ERR,  "sphone_modules_init failed");
			return -1;
		}
		
		if(number)
			sphone_log(LL_DEBUG,  "number: %s", number);
		
		switch (command) {
			case SPHONE_CMD_DIALER_OPEN:
				gui_dialer_show(number);
				break;
			case SPHONE_CMD_SMS_NEW:
				gui_sms_send_show(number,NULL);
				break;
			case SPHONE_CMD_HISTORY_CALLS:
				gui_history_calls();
				break;
			case SPHONE_CMD_HISTORY_SMS:
				gui_history_calls();
				break;
			case SPHONE_CMD_OPTIONS:
				gui_options_open();
				break;
			case SPHONE_CMD_NONE:
			default:
				break;
		}

		g_signal_connect(G_OBJECT(unique), "message-received", G_CALLBACK(main_message_received_callback), NULL);
		
		gtk_main();
		
		gui_sms_exit();
		gui_calls_manager_exit();
		sphone_modules_exit();
		datapipes_exit();
	} else {
		sphone_log(LL_DEBUG, "Instance is already running, sending commands ...");
		if(number)
			sphone_log(LL_DEBUG,  "number: %s", number);
		if(command != SPHONE_CMD_NONE) {
			UniqueMessageData *message = NULL;
			if(number) {
				message = unique_message_data_new();
				unique_message_data_set_text(message, number, strlen(number));
			}
			unique_app_send_message(unique, command, message);
			if(message)
				unique_message_data_free(message);
		}
	}

	if(numbersplit)
		 g_strfreev(numbersplit);
	
	return 0;
}
