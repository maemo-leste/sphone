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
#include <glib.h>

#include "gui.h"
#include "sphone-log.h"
#include "datapipes.h"

struct Ui {
	struct GuiFunctions functions;
	int id;
};

static GSList *uis;

bool gui_contact_thread_shown(const Contact *contact)
{
	if(!contact)
		return false;
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->functions.contact_thread_shown && ui->functions.contact_thread_shown(contact))
			return true;
		else if(ui->functions.contact_thread_shown)
			backend_avail = true;
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
		return false;
	}
	return false;
}

void gui_show_thread_for_contact(const Contact *contact)
{
	if(!contact)
		return;
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->functions.show_thread_for_contact) {
			ui->functions.show_thread_for_contact(contact);
			backend_avail = true;
			break;
		}
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
		gchar message[] = "No gui available for threaded messages";
		execute_datapipe(&gui_error_pipe, message);
	}
}

void gui_history_calls(void)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->functions.history_calls) {
			ui->functions.history_calls();
			backend_avail = true;
			break;
		}
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
		gchar message[] = "No gui available for call history";
		execute_datapipe(&gui_error_pipe, message);
	}
}

void gui_contact_show(const Contact *contact, void (*callback)(Contact*, void*), void *user_data)
{
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->functions.contact_show) {
			ui->functions.contact_show(contact, callback, user_data);
			return;
		}
	}
	sphone_log(LL_WARN, "No backend for %s", __func__);
	gchar message[] = "No contacts gui available";
	execute_datapipe(&gui_error_pipe, message);
}

void gui_close_contact_diag(void)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->functions.close_contact_diag) {
			ui->functions.close_contact_diag();
			backend_avail = true;
		}
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
		gchar message[] = "No contacts gui contacts";
		execute_datapipe(&gui_error_pipe, message);
	}
}

bool gui_dialer_show(const CallProperties* call)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->functions.dialer_show && !ui->functions.dialer_show(call))
			return false;
		else if(ui->functions.dialer_show)
			backend_avail = true;
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
		gchar message[] = "No dialer gui available";
		execute_datapipe(&gui_error_pipe, message);
	}
	return true;
}

bool gui_dtmf_show(const CallProperties *call)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->functions.dtmf_show && !ui->functions.dtmf_show(call))
			return false;
		else if(ui->functions.dtmf_show)
			backend_avail = true;
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
		gchar message[] = "No dtmf gui available";
		execute_datapipe(&gui_error_pipe, message);
	}
	return true;
}

bool gui_sms_send_show(const MessageProperties* message)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->functions.sms_send_show && !ui->functions.sms_send_show(message))
			return false;
		else if(ui->functions.sms_send_show)
			backend_avail = true;
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
		gchar errmessage[] = "No sms gui available";
		execute_datapipe(&gui_error_pipe, errmessage);
	}
	return true;
}

bool gui_options_open(void)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->functions.options_open && !ui->functions.options_open())
			return false;
		else if(ui->functions.options_open)
			backend_avail = true;
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
		gchar errmessage[] = "No options gui available";
		execute_datapipe(&gui_error_pipe, errmessage);
	}
	return true;
}

bool gui_history_sms(void)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->functions.history_sms && !ui->functions.history_sms())
			return false;
		else if(ui->functions.history_sms)
			backend_avail = true;
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
		gchar errmessage[] = "No sms history gpi available";
		execute_datapipe(&gui_error_pipe, errmessage);
	}
	return true;
}

int gui_register(const struct GuiFunctions functions)
{
	static int id_counter = 0;
	struct Ui *ui = g_malloc(sizeof(*ui));

	ui->id = id_counter++;
	ui->functions = functions;
	
	uis = g_slist_prepend(uis, ui);
	return id_counter-1;
}

void gui_remove(int id)
{
	for(GSList *element = uis; element; element = element->next) {
		if(((struct Ui*)element->data)->id == id) {
			g_free(element->data);
			uis = g_slist_remove(uis, element->data);
			break;
		}
	}
	sphone_log(LL_DEBUG, "Removed ui %i, %i remaining", id, g_slist_length(uis));
}
