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
#include "sphone-modules.h"
#include "sphone-log.h"
#include "sphone-conf.h"
#include "datapipes.h"
#include "datapipe.h"

/** Module name */
#define MODULE_NAME		"contacts-ui-abook"

/** Functionality provided by this module */
static const gchar *const provides[] = { "contacts-ui", NULL };

struct UiAbookPriv
{
	GtkWidget *chooser;
};

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
	struct UiAbookPriv *priv = data;
	GList *selection =
		osso_abook_contact_chooser_get_selection(OSSO_ABOOK_CONTACT_CHOOSER(priv->chooser));
		
	if (selection) {
		GList *l;

		sphone_module_log(LL_DEBUG, "Selected Contacts:\n");

		for (l = selection; l; l = l->next) {
				sphone_module_log(LL_DEBUG, "%s\n", osso_abook_contact_get_display_name(l->data));
		}
	} else {
			sphone_module_log(LL_DEBUG, "Nothing selected");
	}
	gtk_widget_destroy(priv->chooser);
	priv->chooser = NULL;
}

static void contact_show_trigger(const void *data, void *user_data)
{
	struct UiAbookPriv *priv = user_data;
	(void)data;

	if(!priv->chooser) {
		priv->chooser = osso_abook_contact_chooser_new_with_capabilities(NULL, "Choose contact",
		                                                                 OSSO_ABOOK_CAPS_PHONE |
		                                                                 OSSO_ABOOK_CAPS_VOICE,
		                                                                 OSSO_ABOOK_CONTACT_ORDER_NAME);
		osso_abook_contact_chooser_set_maximum_selection(OSSO_ABOOK_CONTACT_CHOOSER(priv->chooser), 1);
		osso_abook_contact_chooser_set_minimum_selection(OSSO_ABOOK_CONTACT_CHOOSER(priv->chooser), 1);
		g_signal_connect(G_OBJECT(priv->chooser), "response", contact_dialog_reponse_cb, priv);
	}

	gtk_widget_show_all(priv->chooser);
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	struct UiAbookPriv *priv = g_malloc0(sizeof(*priv));
	*data = priv;

	append_trigger_to_datapipe(&contact_show_pipe, contact_show_trigger, priv);
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	struct UiAbookPriv *priv = data;
	if(priv->chooser)
		gtk_widget_destroy(priv->chooser);
	remove_trigger_from_datapipe(&contact_show_pipe, contact_show_trigger, data);
}
