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
	bool (*contact_shown)(const Contact *contact);
	int id;
};

static GSList *uis;

bool gui_contact_shown(const Contact *contact)
{
	bool backend_avail = false;
	for(GSList *element = uis; element; element = element->next) {
		struct Ui *ui = element->data;
		if(ui->contact_shown && !ui->contact_shown(contact))
			return false;
		else if(ui->contact_shown)
			backend_avail = true;
	}
	if(!backend_avail) {
		sphone_log(LL_WARN, "No backend for %s", __func__);
		return false;
	}
	return true;
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
			 bool (*contact_shown)(const Contact *contact))
{
	static int id_counter = 0;
	struct Ui *ui = g_malloc(sizeof(*ui));

	ui->id = id_counter++;
	ui->dialer_show = dialer_show;
	ui->sms_send_show = sms_send_show;
	ui->options_open = options_open;
	ui->history_sms = history_sms;
	ui->contact_shown = contact_shown;
	
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
