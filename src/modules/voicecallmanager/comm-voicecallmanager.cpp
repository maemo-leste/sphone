#include <glib.h>

#include "comm.h"
#include "datapipe.h"
#include "datapipes.h"
#include "sphone-log.h"
#include "sphone-modules.h"
#include "types.h"


#include <QtCore>
#include <QtDBus/QtDBus>

#include <voicecallmanager.h>

#include "comm-voicecallmanager.h"

#include "comm-voicecallmanager-maemocallhandler.h"
#include "comm-voicecallmanager-maemomanager.h"
#include "comm-voicecallmanager-maemoprovider.h"

extern "C" {
    /** Functionality provided by this module */
    static const gchar* const provides[] = { MODULE_NAME, NULL };

    /** Module information */
    G_MODULE_EXPORT module_info_struct module_info = {
        /** Name of the module */
        .name = MODULE_NAME,
        /** Module provides */
        .provides = provides,
        /** Module priority */
        .priority = 250
    };

    static void call_dial_trigger(gconstpointer data, gpointer user_data)
    {
        const CallProperties* call = (const CallProperties*)data;
        MaemoManager* maemo_mgr = (MaemoManager*)user_data;

        maemo_mgr->dialTrigger(call);
    }

    static void call_accept_trigger(gconstpointer data, gpointer user_data)
    {
        const CallProperties* call = (const CallProperties*)data;
        MaemoManager* maemo_mgr = (MaemoManager*)user_data;
        maemo_mgr->acceptTrigger(call);
    }

    static void call_hold_trigger(gconstpointer data, gpointer user_data)
    {
        const CallProperties* call = (const CallProperties*)data;
        MaemoManager* maemo_mgr = (MaemoManager*)user_data;
        maemo_mgr->holdTrigger(call);
    }

    static void call_hangup_trigger(gconstpointer data, gpointer user_data)
    {
        const CallProperties* call = (const CallProperties*)data;
        MaemoManager* maemo_mgr = (MaemoManager*)user_data;

        maemo_mgr->hangupTrigger(call);
    }

    G_MODULE_EXPORT const gchar* sphone_module_init(void** data);
    const gchar* sphone_module_init(void** data)
    {
        sphone_module_log(LL_DEBUG, "%s", __func__);

        MaemoManager* maemo_mgr = new MaemoManager();

        maemo_mgr->setup();

        *data = maemo_mgr;

        append_trigger_to_datapipe(&call_dial_pipe, call_dial_trigger, maemo_mgr);
        append_trigger_to_datapipe(&call_accept_pipe, call_accept_trigger, maemo_mgr);
        append_trigger_to_datapipe(&call_hold_pipe, call_hold_trigger, maemo_mgr);
        append_trigger_to_datapipe(&call_hangup_pipe, call_hangup_trigger, maemo_mgr);

        return NULL;
    }

    G_MODULE_EXPORT void sphone_module_exit(void* data);
    void sphone_module_exit(void* data)
    {
        MaemoManager* maemo_mgr = (MaemoManager*)data;

        remove_trigger_from_datapipe(&call_dial_pipe, call_dial_trigger, maemo_mgr);
        remove_trigger_from_datapipe(&call_accept_pipe, call_accept_trigger, maemo_mgr);
        remove_trigger_from_datapipe(&call_hold_pipe, call_hold_trigger, maemo_mgr);
        remove_trigger_from_datapipe(&call_hangup_pipe, call_hangup_trigger, maemo_mgr);

        delete maemo_mgr;
    }
}
