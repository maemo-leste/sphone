// #include <gtk/gtk.h>
#include "types.h"
#include "sphone-modules.h"
#include "datapipe.h"
#include "datapipes.h"
#include "sphone-log.h"
#include "gui.h"
#include "gtk-gui-utils.h"

/** Module name */
#define MODULE_NAME		"ui-message-recived-gtk"

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

static void gui_sms_receive_show(const MessageProperties *message);

static void gui_sms_open_contact_callback(GtkButton *button)
{
	gchar *dial = g_object_get_data(G_OBJECT(button), "dial");
	(void)dial;
	// TODO replace:
	//gui_contact_open_by_dial(dial);
}

static void gui_sms_incoming_callback(gconstpointer data, gpointer user_data)
{
	const MessageProperties *message = (const MessageProperties*)data;
	(void)user_data;

	Contact contact = {0};
	contact.line_identifier = message->line_identifier;
	contact.backend = message->backend;
	if(gui_contact_shown(&contact))
		return;

	gui_sms_receive_show(message);
}

static void gui_sms_cancel_callback(GtkWidget *button, GtkWidget *main_window)
{
	(void)button;
	gtk_widget_destroy(main_window);
}

static void gui_sms_reply_callback(GtkWidget *button)
{
	MessageProperties *msg = g_object_get_data(G_OBJECT(button), "message-proparties");
	gui_sms_send_show(msg);
}

static void gui_sms_receive_show(const MessageProperties *message)
{
	gchar *desc;
	gchar *time_str;
	GdkPixbuf *photo = NULL;

	if(message->contact && message->contact->name) {
		desc = g_strdup_printf("%s\n%s",message->contact->name, message->line_identifier);
		if(message->contact->photo)
			photo = message->contact->photo;
	} else {
		desc = g_strdup_printf("<Unknown>\n%s\n", message->line_identifier);
	}

	time_str = gtk_gui_date_to_new_string(message->time);

	GtkWidget *main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(main_window),"New SMS");
	gtk_window_set_default_size(GTK_WINDOW(main_window),400,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,0);
	GtkWidget *to_bar=gtk_hbox_new(FALSE,0);
	GtkWidget *actions_bar=gtk_hbox_new(TRUE,0);
	GtkWidget *photo_image=gtk_image_new_from_pixbuf(photo);
	GtkWidget *from_entry=gtk_button_new_with_label(desc);
	GtkWidget *time_label=gtk_label_new(time_str);
	GtkWidget *send_button=gtk_button_new_with_label("Reply");
	GtkWidget *cancel_button=gtk_button_new_with_label("Cancel");
	GtkWidget *text_edit=gtk_label_new(message->text);
	GtkWidget *s = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (s),
		       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_label_set_line_wrap(GTK_LABEL(text_edit), TRUE);
	gtk_misc_set_alignment(GTK_MISC(text_edit),0.0,0.0);
	gtk_misc_set_padding(GTK_MISC(text_edit),2,2);
	gtk_button_set_relief(GTK_BUTTON(from_entry),GTK_RELIEF_NONE);
	gtk_button_set_alignment(GTK_BUTTON(from_entry),0,0.5);
	//gtk_widget_set_can_focus(from_entry,FALSE);
	g_object_set(G_OBJECT(from_entry),"can-focus",FALSE,NULL);

	gtk_box_pack_start(GTK_BOX(to_bar), photo_image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), from_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), time_label, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(actions_bar), send_button);
	gtk_container_add(GTK_CONTAINER(actions_bar), cancel_button);
	gtk_box_pack_start(GTK_BOX(v1), to_bar, FALSE, FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(s), text_edit);
	gtk_box_pack_start(GTK_BOX(v1), s, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(v1), actions_bar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(main_window), v1);

	gtk_widget_show_all(main_window);

	MessageProperties *message_copy = message_properties_copy(message);

	g_object_set_data_full(G_OBJECT(send_button), "message-proparties",
						   message_copy, (void (*)(void *))message_properties_free);

	g_signal_connect(G_OBJECT(from_entry), "clicked", G_CALLBACK(gui_sms_open_contact_callback), NULL);
	g_signal_connect(G_OBJECT(send_button), "clicked", G_CALLBACK(gui_sms_reply_callback), NULL);
	g_signal_connect(G_OBJECT(cancel_button), "clicked", G_CALLBACK(gui_sms_cancel_callback), main_window);

	g_free(time_str);
	g_free(desc);
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	append_trigger_to_datapipe(&message_received_pipe, gui_sms_incoming_callback, NULL);
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	remove_trigger_from_datapipe(&message_received_pipe, gui_sms_incoming_callback, NULL);
}
