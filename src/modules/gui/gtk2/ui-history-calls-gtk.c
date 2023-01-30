/*
 * ui-history-calls-gtk.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * ui-history-calls-gtk.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ui-history-calls-gtk.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui-history-calls-gtk.h"

#ifdef ENABLE_LIBHILDON
#include <hildon/hildon.h>
#include <hildon/hildon-gtk.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-stackable-window.h>
#endif

#include <gtk/gtk.h>

#include "sphone-log.h"
#include "gui.h"
#include "comm.h"
#include "gtk-gui-utils.h"
#include "storage.h"
#include "gui.h"

#include "sphone-modules.h"

/** Module name */
#define MODULE_NAME		"ui-history-calls-gtk"

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, "ui-history-calls", NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

struct{
	GtkWidget *window;
	GtkWidget *dials_view;
} g_history_calls;

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
		GtkTreeModel *model=gtk_tree_view_get_model(view);
		GtkTreeIter iter;
		CallProperties msg = {0};
		CommBackend *backend = sphone_comm_default_backend();
		GValue value={0};
		gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
		gtk_tree_model_get_value(model, &iter, GTK_UI_MOD_LINE_ID, &value);
		msg.backend = backend ? backend->id : 0;
		msg.line_identifier = (char*)g_value_get_string(&value);
		gdk_window_destroy(gtk_widget_get_window(GTK_WIDGET(g_history_calls.window)));
		g_history_calls.window = NULL;
		gui_dialer_show(&msg);
		g_value_unset(&value);
	}
}

static GtkWidget *gui_dialer_build_list(void)
{
	GtkWidget *scroll;
	g_history_calls.dials_view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(g_history_calls.dials_view), TRUE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
#ifdef ENABLE_LIBHILDON
	hildon_init();
#endif

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", GTK_UI_MOD_NAME, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_min_width(column,100);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_history_calls.dials_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_NONE, NULL);
	column = gtk_tree_view_column_new_with_attributes("Line", renderer, "text", GTK_UI_MOD_LINE_ID, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_min_width(column,120);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_history_calls.dials_view), column);
	
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_NONE, "wrap-width", 140, "wrap-mode", PANGO_WRAP_WORD, NULL);
	column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", GTK_UI_MOD_TIME, NULL);
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

void gtk_gui_history_calls(void)
{
	sphone_log(LL_DEBUG, "gui_history_calls\n");
	GtkTreeModel *calls;

	if(g_history_calls.window){
		GList *calls_list = store_get_all_calls();
		calls = gtk_gui_new_model_from_calls(calls_list);
		store_free_call_list(calls_list);
		gtk_tree_view_set_model(GTK_TREE_VIEW(g_history_calls.dials_view), GTK_TREE_MODEL(calls));
		g_object_unref(G_OBJECT(calls));
		gtk_window_present(GTK_WINDOW(g_history_calls.window));
		return;
	}

	GtkWidget *v1 = gtk_vbox_new(FALSE, 0);
	
#ifdef ENABLE_LIBHILDON
	g_history_calls.window = hildon_stackable_window_new();
	hildon_gtk_window_set_portrait_flags(GTK_WINDOW(g_history_calls.window), HILDON_PORTRAIT_MODE_SUPPORT);
#else
	g_history_calls.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#endif

	GtkWidget *list = gui_dialer_build_list();

	gtk_window_set_title(GTK_WINDOW(g_history_calls.window),"Call History");
	gtk_window_set_default_size(GTK_WINDOW(g_history_calls.window), 600, 220);

	gtk_container_add (GTK_CONTAINER(v1), list);
	gtk_container_add (GTK_CONTAINER(g_history_calls.window), v1);

	g_signal_connect(G_OBJECT(g_history_calls.window), "delete-event", G_CALLBACK(gui_history_make_null), &g_history_calls.window);

	GList *calls_list = store_get_all_calls();
	calls = gtk_gui_new_model_from_calls(calls_list);
	store_free_call_list(calls_list);
	gtk_tree_view_set_model(GTK_TREE_VIEW(g_history_calls.dials_view), GTK_TREE_MODEL(calls));
	g_object_unref(G_OBJECT(calls));
	
	gtk_widget_show_all(g_history_calls.window);
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;

#ifdef ENABLE_LIBHILDON
	hildon_init();
#endif

	*data = GINT_TO_POINTER(gui_register(NULL, NULL, NULL, NULL, NULL, NULL, gtk_gui_history_calls, NULL, NULL));
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	gui_remove(GPOINTER_TO_INT(data));
}
