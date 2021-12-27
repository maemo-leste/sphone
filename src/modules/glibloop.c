#include <gtk/gtk.h>

static GMainLoop *loop;

G_MODULE_EXPORT void sphone_loop_setup(int argc, char *argv[]);
void sphone_loop_setup(int argc, char *argv[])
{
	gtk_set_locale();
	gtk_init(&argc, &argv);
	loop = g_main_loop_new (NULL, FALSE);
}

G_MODULE_EXPORT void sphone_loop_run(int argc, char *argv[]);
void sphone_loop_run(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	g_main_loop_run(loop);
}

G_MODULE_EXPORT void sphone_loop_exit(void);
void sphone_loop_exit(void)
{
	if(loop)
		g_main_loop_quit(loop);
}
