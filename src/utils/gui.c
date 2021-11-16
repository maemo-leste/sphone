#include <glib.h>

#include "gui.h"
#include "sphone-log.h"

struct Ui {
	bool (*dialer_show)(const CallProperties* call);
	bool (*sms_send_show)(const MessageProperties* call);
	bool (*options_open)(void);
	bool (*history_sms)(void);
	bool (*contact_shown)(const Contact *contact);
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
	struct Ui *ui = g_malloc(sizeof(*ui));
	int len = g_slist_length(uis);

	ui->dialer_show = dialer_show;
	ui->sms_send_show = sms_send_show;
	ui->options_open = options_open;
	ui->history_sms = history_sms;
	ui->contact_shown = contact_shown;
	
	uis = g_slist_prepend(uis, ui);
	return len;
}

void gui_remove(int id)
{
	GSList *element = g_slist_nth(uis, id);
	uis = g_slist_remove(uis, element->data);
	g_free(element->data);
}
