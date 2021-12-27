#include <gtk/gtk.h>
#include <QApplication>
#include <QAbstractEventDispatcher>
#include <stdlib.h>

#include "sphone-log.h"

static QApplication *app;

extern "C" {

G_MODULE_EXPORT void sphone_loop_setup(int argc, char *argv[]);
void sphone_loop_setup(int argc, char *argv[])
{
	gtk_set_locale();
	gtk_init(&argc, &argv);
	app = new QApplication(argc, argv);
	app->setQuitOnLastWindowClosed(false);

	QAbstractEventDispatcher *dispatcher = QCoreApplication::eventDispatcher();
	if(!dispatcher && dispatcher->inherits("QEventDispatcherGlib")) {
		sphone_log(LL_CRIT, "CRITICAL FAILURE: qt-glib support is required!!");
		exit(-1);
	}
}

G_MODULE_EXPORT void sphone_loop_run(int argc, char *argv[]);
void sphone_loop_run(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	app->exec();
}

G_MODULE_EXPORT void sphone_loop_exit(void);
void sphone_loop_exit(void)
{
	if(app)
		app->exit(0);
	delete app;
}

}
