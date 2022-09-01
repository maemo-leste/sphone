/*
 * contacts-ui-exec.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * contacts-ui-exec.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * contacts-ui-exec.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <libosso-abook/osso-abook.h>
#include <gtk/gtk.h>
#include <libebook-contacts/libebook-contacts.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "sphone-conf.h"
#include "datapipes.h"
#include "datapipe.h"
#include "types.h"
#include "gui.h"
#include "comm.h"

/** Module name */
#define MODULE_NAME		"contacts-ui-abook"

/** Functionality provided by this module */
static const gchar *const provides[] = { "contacts-ui", NULL };

struct UiAbookPriv
{
	GtkWidget *chooser;
	void (*callback)(Contact*, void*);
	void *user_data;
	int ui_id;
} abook_priv;

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};

static void contact_dialog_reponse_cb(GtkDialog *dialog, int response_id, void *data)
{
	(void)data;
	(void)dialog;
	(void)response_id;

	GList *selection =
		osso_abook_contact_chooser_get_selection(OSSO_ABOOK_CONTACT_CHOOSER(abook_priv.chooser));
		
	gtk_widget_destroy(abook_priv.chooser);
	abook_priv.chooser = NULL;
		
	if (selection) {
		OssoABookContact *acontact = selection->data;
		OssoABookContactDetailSelector *detailDiag = 
		(OssoABookContactDetailSelector*)osso_abook_contact_detail_selector_new_for_contact(
			NULL, acontact, OSSO_ABOOK_CONTACT_DETAIL_PHONE | OSSO_ABOOK_CONTACT_DETAIL_IM_VOICE);
		gtk_dialog_run(GTK_DIALOG(detailDiag));
		
		EVCardAttribute *attribute = osso_abook_contact_detail_selector_get_detail(detailDiag);
		
		if(!attribute) {
			abook_priv.callback = NULL;
			return;
		}

		{
			Contact contact = {};

			contact.name = (char*)osso_abook_contact_get_display_name(acontact);
			//callContact.photo = osso_abook_contact_get_photo(acontact);
			//g_object_ref(G_OBJECT(callContact.photo));
			contact.line_identifier = e_vcard_attribute_get_value(attribute);

			if(abook_priv.callback)
				abook_priv.callback(&contact, abook_priv.user_data);

			g_list_free(selection);
			gtk_widget_destroy(GTK_WIDGET(detailDiag));
		}
	}

	abook_priv.callback = NULL;
}

static void abook_dialog_close(void)
{
	if (abook_priv.chooser) {
		gtk_widget_destroy(abook_priv.chooser);
		abook_priv.callback = NULL;
	}
}


static void abook_contact_show(const Contact *contact, void (*callback)(Contact*, void*), void *user_data)
{
	(void)contact; //TODO show specific contact

	if(abook_priv.callback) {
		sphone_module_log(LL_WARN, "Only one dialog can be shown at a time");
		return;
	}

	abook_priv.callback = callback;
	abook_priv.user_data = user_data;

	if(!abook_priv.chooser) {
		abook_priv.chooser = osso_abook_contact_chooser_new_with_capabilities(NULL, "Choose contact",
		                                                                 OSSO_ABOOK_CAPS_PHONE |
		                                                                 OSSO_ABOOK_CAPS_VOICE,
		                                                                 OSSO_ABOOK_CONTACT_ORDER_NAME);
		osso_abook_contact_chooser_set_maximum_selection(OSSO_ABOOK_CONTACT_CHOOSER(abook_priv.chooser), 1);
		osso_abook_contact_chooser_set_minimum_selection(OSSO_ABOOK_CONTACT_CHOOSER(abook_priv.chooser), 1);
		g_signal_connect(G_OBJECT(abook_priv.chooser), "response", G_CALLBACK(contact_dialog_reponse_cb), NULL);
	}

	gtk_widget_show_all(abook_priv.chooser);
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;

	hildon_init();
	osso_abook_init_with_name("sphone", NULL);

	abook_priv.ui_id = gui_register(NULL, NULL, NULL, NULL, NULL, abook_contact_show, abook_dialog_close);

	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	gui_remove(abook_priv.ui_id);
	abook_dialog_close();
	if(abook_priv.chooser)
		gtk_widget_destroy(abook_priv.chooser);
}
