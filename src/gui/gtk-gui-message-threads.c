#include "gtk-gui-message-threads.h"

#ifdef ENABLE_LIBHILDON
#include <hildon/hildon-gtk.h>
#include <hildon/hildon-pannable-area.h>
#endif

#include <gtk/gtk.h>

#include "sphone-log.h"
#include "sphone-store-tree-model.h"
#include "gui-contact-view.h"
#include "gui-dialer.h"
#include "gui.h"
#include "comm.h"
#include "storage.h"
#include "gtk-gui-utils.h"
#include "gtk-gui-thread-view.h"

static void gtk_gui_msg_threads_list_double_click_callback(GtkTreeView *view, GtkTreePath* path, GtkTreeViewColumn* column, gpointer func_data)
{
	(void)func_data;
	(void)column;
	sphone_log(LL_DEBUG, "%s", __func__);
	if(path){
		GtkTreeModel *model = gtk_tree_view_get_model (view);
		GtkTreeIter iter;
		Contact contact = {0};
		GValue line_id_value = {0};
		GValue backend_value = {0};
		GValue name_value = {0};

		gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
		gtk_tree_model_get_value(model, &iter, GTK_UI_MOD_LINE_ID, &line_id_value);
		gtk_tree_model_get_value(model, &iter, GTK_UI_MOD_NAME, &name_value);
		gtk_tree_model_get_value(model, &iter, GTK_UI_MOD_BACKEND, &backend_value);
		
		contact.backend = g_value_get_int(&backend_value);
		contact.line_identifier = (char*)g_value_get_string(&line_id_value);
		contact.name = (char*)g_value_get_string(&name_value);
		gtk_gui_show_thread_for_contact(&contact);
		g_value_unset(&line_id_value);
		g_value_unset(&backend_value);
		g_value_unset(&name_value);
	}
}

static GtkWidget *gtk_gui_msg_threads_build(GtkWidget *contacts_view)
{
	GtkWidget *scroll;
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(contacts_view),FALSE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", GTK_UI_MOD_NAME, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_min_width(column,100);
	gtk_tree_view_append_column(GTK_TREE_VIEW(contacts_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_NONE, NULL);
	column = gtk_tree_view_column_new_with_attributes("Text", renderer, "text", GTK_UI_MOD_LINE_ID, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_min_width(column,120);
	gtk_tree_view_append_column(GTK_TREE_VIEW(contacts_view), column);

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(contacts_view),TRUE);
#ifdef ENABLE_LIBHILDON
	scroll = hildon_pannable_area_new();
#else
	scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#endif
	gtk_widget_set_size_request(GTK_WIDGET(scroll), 0, 200);
	gtk_container_add (GTK_CONTAINER(scroll), contacts_view);

	g_signal_connect_after(G_OBJECT(contacts_view),"row-activated", G_CALLBACK(gtk_gui_msg_threads_list_double_click_callback),NULL);

	return scroll;
}

bool gtk_gui_msg_threads(void)
{
	sphone_log(LL_DEBUG, "gtk_gui_msg_threads_calls\n");
	GtkTreeModel *contacts;

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window),"Threads");
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 600);
	GtkWidget *v1 = gtk_vbox_new(FALSE, 0);
	GtkWidget *contacts_view = gtk_tree_view_new();
	GtkWidget *threads = gtk_gui_msg_threads_build(contacts_view);
	
#ifdef ENABLE_LIBHILDON
	hildon_gtk_window_set_portrait_flags(GTK_WINDOW(window), HILDON_PORTRAIT_MODE_SUPPORT);
#endif

	gtk_container_add (GTK_CONTAINER(v1), threads);
	gtk_container_add (GTK_CONTAINER(window), v1);

	GList *contacts_list = store_get_interacted_msg_contacts();
	contacts = gtk_gui_new_model_from_contacts(contacts_list);
	store_free_contacts_list(contacts_list);
	gtk_tree_view_set_model(GTK_TREE_VIEW(contacts_view), GTK_TREE_MODEL(contacts));
	g_object_unref(G_OBJECT(contacts));

	gtk_widget_show_all(window);

	return true;
}
