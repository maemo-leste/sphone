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

struct Ui {
	bool (*dialer_show)(const CallProperties* call);
	bool (*sms_send_show)(const MessageProperties* call);
	bool (*options_open)(void);
	bool (*history_sms)(void);
	bool (*contact_thread_shown)(const Contact *contact);
	void (*show_thread_for_contact)(const Contact *contact);
	void (*history_calls)(void);
	void (*contact_show)(const Contact *contact, void (*callback)(Contact*, void*), void *user_data);
	void (*close_contact_diag)(void);
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
		if(ui->contact_thread_shown && ui->contact_thread_shown(contact))
			return true;
		else if(ui->contact_thread_shown)
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
		if(ui->show_thread_for_contact) {
			ui->show_thread_for_contact(contact);
			backend_avail = true;
			break;
		}
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
	}
}

void gui_history_calls(void)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->history_calls) {
			ui->history_calls();
			backend_avail = true;
			break;
		}
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
	}
}

void gui_contact_show(const Contact *contact, void (*callback)(Contact*, void*), void *user_data)
{
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->contact_show) {
			ui->contact_show(contact, callback, user_data);
			return;
		}
	}
	sphone_log(LL_WARN, "No backend for %s", __func__);
}

void gui_close_contact_diag(void)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->close_contact_diag) {
			ui->close_contact_diag();
			backend_avail = true;
		}
	}
	if(!backend_avail)
		sphone_log(LL_WARN, "No backend for %s", __func__);
}

bool gui_dialer_show(const CallProperties* call)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->dialer_show && !ui->dialer_show(call))
			return false;
		else if(ui->dialer_show)
			backend_avail = true;
	}
	if(!backend_avail)
		sphone_log(LL_WARN, "No backend for %s", __func__);
	return true;
}

bool gui_sms_send_show(const MessageProperties* message)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->sms_send_show && !ui->sms_send_show(message))
			return false;
		else if(ui->sms_send_show)
			backend_avail = true;
	}
	if(!backend_avail)
		sphone_log(LL_WARN, "No backend for %s", __func__);
	return true;
}

bool gui_options_open(void)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->options_open && !ui->options_open())
			return false;
		else if(ui->options_open)
			backend_avail = true;
	}
	if(!backend_avail)
		sphone_log(LL_WARN, "No backend for %s", __func__);
	return true;
}

bool gui_history_sms(void)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->history_sms && !ui->history_sms())
			return false;
		else if(ui->history_sms)
			backend_avail = true;
	}
	if(!backend_avail)
		sphone_log(LL_WARN, "No backend for %s", __func__);
	return true;
}

int gui_register(bool (*dialer_show)(const CallProperties* call),
			 bool (*sms_send_show)(const MessageProperties* call),
			 bool (*options_open)(void),
			 bool (*history_sms)(void),
			 bool (*contact_thread_shown)(const Contact *contact),
			 void (*show_thread_for_contact)(const Contact *contact),
			 void (*history_calls)(void),
			 void (*contact_show)(const Contact *contact, void (*callback)(Contact*, void*), void *user_data),
			 void (*close_contact_diag)(void))
{
	static int id_counter = 0;
	struct Ui *ui = g_malloc(sizeof(*ui));

	ui->id = id_counter++;
	ui->dialer_show = dialer_show;
	ui->sms_send_show = sms_send_show;
	ui->options_open = options_open;
	ui->history_sms = history_sms;
	ui->contact_thread_shown = contact_thread_shown;
	ui->show_thread_for_contact = show_thread_for_contact;
	ui->history_calls = history_calls;
	ui->contact_show = contact_show;
	ui->close_contact_diag = close_contact_diag;
	
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
