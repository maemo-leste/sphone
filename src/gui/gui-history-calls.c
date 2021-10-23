#include "gui-history-calls.h"

#ifdef ENABLE_LIBHILDON
#include <hildon/hildon-gtk.h>
#include <hildon/hildon-pannable-area.h>
#endif

#include <gtk/gtk.h>

#include "store.h"
#include "sphone-log.h"
#include "sphone-store-tree-model.h"
#include "gui-contact-view.h"
#include "gui-dialer.h"

struct{
	GtkWidget *window;
	GtkWidget *dials_view;
}g_history_calls;

static gboolean gui_history_make_null(GtkWidget *w, GdkEvent *event, GtkWidget **window)
{
	(void)w;
	(void)event;
	*window=NULL;
	return FALSE;
}

static void gui_history_list_delete_model(void)
{
	gtk_tree_view_set_model(GTK_TREE_VIEW(g_history_calls.dials_view), NULL);
}

static void gui_history_list_double_click_callback(GtkTreeView *view, GtkTreePath* path, GtkTreeViewColumn* column, gpointer func_data)
{
	(void)func_data;
	(void)column;
	sphone_log(LL_DEBUG, "%s", __func__);
	if(path){
		GtkTreeModel *model=gtk_tree_view_get_model (view);
		GtkTreeIter iter;
		GValue value={0};
		gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
		gtk_tree_model_get_value(model,&iter,SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,&value);
		const gchar *dial = g_value_get_string(&value);
		gui_dialer_show(g_strdup(dial));
		g_value_unset(&value);
	}
}

static GtkWidget *gui_dialer_build_list(void)
{
	GtkWidget *scroll;
	g_history_calls.dials_view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(g_history_calls.dials_view),FALSE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_NAME, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_min_width(column,100);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_history_calls.dials_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_NONE, NULL);
	column = gtk_tree_view_column_new_with_attributes("Dial", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_DIAL, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_min_width(column,120);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_history_calls.dials_view), column);
	
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_NONE, "wrap-width", 140, "wrap-mode", PANGO_WRAP_WORD, NULL);
	column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DATE, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(column, FALSE);
	gtk_tree_view_column_set_min_width(column,140);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_history_calls.dials_view), column);

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(g_history_calls.dials_view),TRUE);
#ifdef ENABLE_LIBHILDON
	scroll = hildon_pannable_area_new();
#else
	scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#endif
	gtk_widget_set_size_request(GTK_WIDGET(scroll), 0, 200);
	gtk_container_add (GTK_CONTAINER(scroll),g_history_calls.dials_view);

	g_signal_connect_after(G_OBJECT(g_history_calls.dials_view),"row-activated", G_CALLBACK(gui_history_list_double_click_callback),NULL);
	g_signal_connect(G_OBJECT(g_history_calls.window),"unmap-event", G_CALLBACK(gui_history_list_delete_model),NULL);

	return scroll;
}

gint gui_history_calls(void)
{
	sphone_log(LL_DEBUG, "gui_history_calls\n");
	SphoneStoreTreeModel *calls;

	if(g_history_calls.window){
		calls = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_CALLS_ALL, NULL);
		gtk_tree_view_set_model(GTK_TREE_VIEW(g_history_calls.dials_view), GTK_TREE_MODEL(calls));
		g_object_unref(G_OBJECT(calls));
		gtk_window_present(GTK_WINDOW(g_history_calls.window));
		return 0;
	}

	g_history_calls.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(g_history_calls.window),"Call History");
	gtk_window_set_default_size(GTK_WINDOW(g_history_calls.window), 600, 220);
	GtkWidget *v1 = gtk_vbox_new(FALSE, 0);

	GtkWidget *list = gui_dialer_build_list();
	
#ifdef ENABLE_LIBHILDON
	hildon_gtk_window_set_portrait_flags(GTK_WINDOW(g_history_calls.window), HILDON_PORTRAIT_MODE_SUPPORT);
#endif

	gtk_container_add (GTK_CONTAINER(v1), list);
	gtk_container_add (GTK_CONTAINER(g_history_calls.window), v1);

	g_signal_connect(G_OBJECT(g_history_calls.window),"delete-event", G_CALLBACK(gui_history_make_null), &g_history_calls.window);

	calls = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_CALLS_ALL, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(g_history_calls.dials_view), GTK_TREE_MODEL(calls));
	g_object_unref(G_OBJECT(calls));
	
	gtk_widget_show_all(g_history_calls.window);

	return 0;
}