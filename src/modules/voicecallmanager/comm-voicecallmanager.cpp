#include <glib.h>
#include <gmodule.h>

#include "datapipe.h"
#include "datapipes.h"
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

    static void call_dtmf_trigger(gconstpointer data, gpointer user_data)
    {
        const DtmfRequest* request = (const DtmfRequest*)data;
        const CallProperties* call = request->call;
        MaemoManager* maemo_mgr = (MaemoManager*)user_data;

        QString qtone;
        switch (request->dtmf) {
            case SPHONE_DTMF_STOP:
                return;
            case SPHONE_DTMF_0:
                qtone = "0";
                break;
            case SPHONE_DTMF_1:
                qtone = "1";
                break;
            case SPHONE_DTMF_2:
                qtone = "2";
                break;
            case SPHONE_DTMF_3:
                qtone = "3";
                break;
            case SPHONE_DTMF_4:
                qtone = "4";
                break;
            case SPHONE_DTMF_5:
                qtone = "5";
                break;
            case SPHONE_DTMF_6:
                qtone = "6";
                break;
            case SPHONE_DTMF_7:
                qtone = "7";
                break;
            case SPHONE_DTMF_8:
                qtone = "8";
                break;
            case SPHONE_DTMF_9:
                qtone = "9";
                break;
            case SPHONE_DTMF_HASH:
                qtone = "#";
                break;
            case SPHONE_DTMF_STAR:
                qtone = "*";
                break;
            case SPHONE_DTMF_A:
                qtone = "A";
                break;
            case SPHONE_DTMF_B:
                qtone = "B";
                break;
            case SPHONE_DTMF_C:
                qtone = "C";
                break;
            case SPHONE_DTMF_D:
                qtone = "D";
                break;
            default:
                sphone_module_log(LL_WARN, "Unknown DTMF tone value: %d", request->dtmf);
                return;
        }

        maemo_mgr->sendDtmfTrigger(call, qtone);
    }

    G_MODULE_EXPORT const gchar* sphone_module_init(void** data);
    const gchar* sphone_module_init(void** data)
    {
        MaemoManager* maemo_mgr = new MaemoManager();

        maemo_mgr->setup();

        *data = maemo_mgr;

        append_trigger_to_datapipe(&call_dial_pipe, call_dial_trigger, maemo_mgr);
        append_trigger_to_datapipe(&call_accept_pipe, call_accept_trigger, maemo_mgr);
        append_trigger_to_datapipe(&call_hold_pipe, call_hold_trigger, maemo_mgr);
        append_trigger_to_datapipe(&call_hangup_pipe, call_hangup_trigger, maemo_mgr);
        append_trigger_to_datapipe(&call_dtmf_pipe, call_dtmf_trigger, maemo_mgr);

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
        remove_trigger_from_datapipe(&call_dtmf_pipe, call_dtmf_trigger, maemo_mgr);

        delete maemo_mgr;
    }
}
