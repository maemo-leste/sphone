/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * 
 * sphone is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * sphone is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include "keypad.h"
#include "sphone-log.h"

struct key {
	const gchar *text;
	const gchar *value;
};

struct key keys[]={
	{"<span font_size=\"x-large\">1</span>\n<span font_size=\"x-small\">     </span>", "1"},
	{" <span font_size=\"x-large\">2</span>\n<span font_size=\"x-small\">ABC</span>", "2"},
	{"<span font_size=\"x-large\">3</span>\n<span font_size=\"x-small\">DEF</span>", "3"},
	{"<span font_size=\"x-large\">4</span>\n<span font_size=\"x-small\">GHI</span>", "4"},
	{"<span font_size=\"x-large\">5</span>\n<span font_size=\"x-small\">JKL</span>", "5"},
	{" <span font_size=\"x-large\">6</span>\n<span font_size=\"x-small\">MNO</span>", "6"},
	{"<span font_size=\"x-large\"> 7</span>\n<span font_size=\"x-small\">PQRS</span>", "7"},
	{" <span font_size=\"x-large\">8</span>\n<span font_size=\"x-small\">TUV</span>", "8"},
	{"<span font_size=\"x-large\"> 9</span>\n<span font_size=\"x-small\">WXYZ</span>", "9"},
	{"*", "*"},
	{"0 +", "0"},
	{"#", "#"}
};

static void key_press_callback(GtkWidget *button, GdkEvent *event, void *data)
{
	(void)data;
	guint32 *time = g_object_get_data(G_OBJECT(button), "press_time");
	*time = gdk_event_get_time(event);
}

static void key_release_callback(GtkWidget *button, GdkEvent *event, GtkWidget *target)
{
	gtk_editable_set_position(GTK_EDITABLE(target),-1);
	gint position = gtk_editable_get_position(GTK_EDITABLE(target));
	const gchar *value = g_object_get_data(G_OBJECT(button), "key_value");
	
	if(*value == '0') {
		guint32 *presstime = g_object_get_data(G_OBJECT(button), "press_time");
		if(gdk_event_get_time(event) - *presstime > 500)
			gtk_editable_insert_text(GTK_EDITABLE(target), "+",-1, &position);
		else
			gtk_editable_insert_text(GTK_EDITABLE(target), "0",-1, &position);
	} else {
		gtk_editable_insert_text(GTK_EDITABLE(target), value,-1, &position);
	}

	gtk_widget_grab_focus(target);
	gtk_editable_set_position(GTK_EDITABLE(target),position);
}

GtkWidget *gui_keypad_setup(GtkWidget *target)
{
	GtkWidget *ret;
	unsigned int i = 0;

	ret=gtk_table_new (4,3,TRUE);
	for(unsigned int row = 0; row < 4; ++row)
		for(unsigned int column = 0; column < 3; ++column){
			GtkWidget *label=gtk_label_new(NULL);
			gtk_label_set_markup(GTK_LABEL(label), keys[i].text);
			GtkWidget *button = gtk_button_new();
			gtk_container_add (GTK_CONTAINER(button),label);
			gtk_widget_set_can_focus(button, FALSE);
			gtk_table_attach_defaults(GTK_TABLE(ret),button,column,column+1,row,row+1);
			g_signal_connect(G_OBJECT(button), "button-release-event", G_CALLBACK(key_release_callback), target);
			g_signal_connect(G_OBJECT(button), "button-press-event", G_CALLBACK(key_press_callback), NULL);
			g_object_set_data(G_OBJECT(button),"key_value", (gpointer)keys[i].value);
			g_object_set_data_full(G_OBJECT(button),"press_time", g_malloc0(sizeof(guint32)), g_free);
			++i;
		}

	return ret;
}
