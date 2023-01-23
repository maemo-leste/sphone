#include <glib.h>

extern "C"
{

#include "types.h"
#include "sphone-modules.h"
#include "sphone-log.h"
#include "datapipe.h"
#include "datapipes.h"
#include "comm.h"

}

/** Module name */
#define MODULE_NAME		"comm-telepathy"


#include <QtCore>
#include <QtDBus/QtDBus>

#include <voicecallmanager.h>

#include "comm-telepathy-maemomanager.h"
#include "comm-telepathy-maemoprovider.h"
#include "comm-telepathy-maemocallhandler.h"


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

extern "C" 
{

/* Pipes in play:
 *
 * - &call_properties_changed_pipe:
 *      trigger this when the call state or properties have changed
 * - &call_dial_pipe:
 *      read from this to start a new dial
 * - &call_accept_trigger:
 *      read from this to accept the call
 * - &call_hangup_trigger
 *      read from this to hang up the call
 * - &call_hold_trigger
 *      read from this to hold the call (how do we resume?) - probably hold
 *      again?
 */

static void call_hangup_trigger(gconstpointer data, gpointer user_data) {
    const CallProperties *call = (const CallProperties*)data;
    MaemoManager* maemo_mgr = (MaemoManager*)user_data;

    maemo_mgr->hangupTrigger(call);
}

static void call_accept_trigger(gconstpointer data, gpointer user_data) {
    const CallProperties *call = (const CallProperties*)data;
    MaemoManager* maemo_mgr = (MaemoManager*)user_data;
    maemo_mgr->acceptTrigger(call);
}

static void call_dial_trigger(gconstpointer data, gpointer user_data) {
    const CallProperties *call = (const CallProperties*)data;
    MaemoManager* maemo_mgr = (MaemoManager*)user_data;

    maemo_mgr->dialTrigger(call);
}

G_MODULE_EXPORT const gchar *sphone_module_init(void **data);
const gchar *sphone_module_init(void **data)
{
	sphone_module_log(LL_DEBUG, "%s", __func__);

    MaemoManager* maemo_mgr = new MaemoManager();

    maemo_mgr->setup();

    fprintf(stderr, "maemo_mgr: %x\n", (void*)maemo_mgr);
	*data = maemo_mgr;

	append_trigger_to_datapipe(&call_dial_pipe,   call_dial_trigger, maemo_mgr);
	append_trigger_to_datapipe(&call_accept_pipe, call_accept_trigger, maemo_mgr);
	//append_trigger_to_datapipe(&call_hold_pipe,   call_hold_trigger, maemo_mgr);
	append_trigger_to_datapipe(&call_hangup_pipe, call_hangup_trigger, maemo_mgr);

	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
    MaemoManager* maemo_mgr = (MaemoManager*)data;

    // TODO: clean up here

	remove_trigger_from_datapipe(&call_dial_pipe,   call_dial_trigger, maemo_mgr);
	remove_trigger_from_datapipe(&call_accept_pipe, call_accept_trigger, maemo_mgr);
	//remove_trigger_from_datapipe(&call_hold_pipe,   call_hold_trigger, maemo_mgr);
	remove_trigger_from_datapipe(&call_hangup_pipe, call_hangup_trigger, maemo_mgr);
}

}
