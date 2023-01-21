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
#define MODULE_NAME		"tpqt"


#include <QtCore>
#include <QtDBus/QtDBus>

#include <voicecallmanager.h>

struct tp_if_priv_s {
	int backend_id;
};

class MaemoCallManager : public QObject
{
    Q_OBJECT

public:
    MaemoCallManager();
    void setup(void);

    void acceptTrigger(void);
    void rejectTrigger(void);
    void dialTrigger(void);

public slots:
    void providersChanged();
    void voiceCallsChanged();
    void defaultProviderChanged();
    void activeVoiceCallChanged();

    void audioModeChanged();
    void audioRoutedChanged();
    void microphoneMutedChanged();
    void speakerMutedChanged();


    void voiceCallStatusChanged();

    /*
     * TODO: we can hook these up just for testing:
    */

signals:

private:

public:
    struct tp_if_priv_s *priv;
    VoiceCallManager *mgr;

};

static MaemoCallManager* maemo_mgr;

#include "tpqt.moc"

MaemoCallManager::MaemoCallManager() {
}

void MaemoCallManager::setup(void) {
    mgr = new VoiceCallManager(qApp);

    connect(mgr, &VoiceCallManager::providersChanged, this, &MaemoCallManager::providersChanged);
    connect(mgr, &VoiceCallManager::voiceCallsChanged, this, &MaemoCallManager::voiceCallsChanged);
    connect(mgr, &VoiceCallManager::defaultProviderChanged, this, &MaemoCallManager::defaultProviderChanged);
    connect(mgr, &VoiceCallManager::activeVoiceCallChanged, this, &MaemoCallManager::activeVoiceCallChanged);

    connect(mgr, &VoiceCallManager::audioModeChanged, this, &MaemoCallManager::audioModeChanged);
    connect(mgr, &VoiceCallManager::audioRoutedChanged, this, &MaemoCallManager::audioRoutedChanged);
    connect(mgr, &VoiceCallManager::microphoneMutedChanged, this, &MaemoCallManager::microphoneMutedChanged);
    connect(mgr, &VoiceCallManager::speakerMutedChanged, this, &MaemoCallManager::speakerMutedChanged);

    qDebug() << "default Provider" << mgr->defaultProviderId();
}

void MaemoCallManager::providersChanged(void) {
    qDebug() << "voiceCallsChanged";
}

void MaemoCallManager::voiceCallsChanged(void) {
    qDebug() << "voiceCallsChanged";
}

void MaemoCallManager::defaultProviderChanged(void) {
    qDebug() << "defaultProviderChanged";
}

void MaemoCallManager::activeVoiceCallChanged(void) {
    qDebug() << "activeVoiceCallChanged";

    VoiceCallHandler *handler = mgr->activeVoiceCall();

    if (handler) {
        connect(handler, &VoiceCallHandler::statusChanged, this, &MaemoCallManager::voiceCallStatusChanged);

        CallProperties *call = (CallProperties*)g_malloc0(sizeof(*call));
        call->backend = priv->backend_id;
        call->needs_route = false;
        call->outbound = false;
        // TODO:
        // call->line_identifier
        // call->state
        // call->emergency
        // call->backend_data
        execute_datapipe(&call_new_pipe, call);
    }
}

void MaemoCallManager::audioModeChanged(void) {
    qDebug() << "audioModeChanged" << mgr->audioMode();
}

void MaemoCallManager::audioRoutedChanged(void) {
    qDebug() << "audioRoutedChanged" << mgr->isAudioRouted();
}

void MaemoCallManager::microphoneMutedChanged(void) {
    qDebug() << "microphoneMutedChanged" << mgr->isMicrophoneMuted();
}

void MaemoCallManager::speakerMutedChanged(void) {
    qDebug() << "speakerMutedChanged" << mgr->isSpeakerMuted();
}

void MaemoCallManager::voiceCallStatusChanged(void) {
    VoiceCallHandler *handler = mgr->activeVoiceCall();
    qDebug() << "voiceCallStatusChanged: status" << handler->statusText();
}

static void call_hangup_trigger(gconstpointer data, gpointer user_data) {
    // TODO: just use maemo_mgr->call_hangup_trigger ?
    fprintf(stderr, "hangup\n");
    VoiceCallHandler *handler = maemo_mgr->mgr->activeVoiceCall();
    handler->hangup();
}

static void call_accept_trigger(gconstpointer data, gpointer user_data) {
    // TODO: just use maemo_mgr->call_accept_trigger
    fprintf(stderr, "accept\n");
    VoiceCallHandler *handler = maemo_mgr->mgr->activeVoiceCall();
    handler->answer();
}

static void call_dial_trigger(gconstpointer data, gpointer user_data) {
    // TODO: just use maemo_mgr->call_dial_trigger
	const CallProperties *call = (const CallProperties*)data;
    fprintf(stderr, "dial: %s\n", call->line_identifier);
    maemo_mgr->mgr->dial(call->line_identifier);
}

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

static const Scheme call_scheme =
{
	.scheme = (char*)"tel",
	.flags = BACKEND_FLAG_CALL
};

extern "C" 
{


G_MODULE_EXPORT const gchar *sphone_module_init(void **data);
const gchar *sphone_module_init(void **data)
{
	struct tp_if_priv_s *priv = (struct tp_if_priv_s*)g_malloc0(sizeof(*priv));
	*data = priv;
	sphone_module_log(LL_DEBUG, "%s", __func__);

    maemo_mgr = new MaemoCallManager();
    maemo_mgr->setup();
    maemo_mgr->priv = priv;

	const Scheme* schemes[2] =
	{
		&call_scheme,
		NULL
	};

	priv->backend_id = sphone_comm_add_backend("tptest", schemes, BACKEND_FLAG_CALL);

	append_trigger_to_datapipe(&call_dial_pipe,   call_dial_trigger, priv);
	append_trigger_to_datapipe(&call_accept_pipe, call_accept_trigger, priv);
	//append_trigger_to_datapipe(&call_hold_pipe,   call_hold_trigger, priv);
	append_trigger_to_datapipe(&call_hangup_pipe, call_hangup_trigger, priv);

	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	struct tp_if_priv_s *priv = (struct tp_if_priv_s*)data;

	sphone_comm_remove_backend(priv->backend_id);

	remove_trigger_from_datapipe(&call_dial_pipe,   call_dial_trigger, priv);
	remove_trigger_from_datapipe(&call_accept_pipe, call_accept_trigger, priv);
	//remove_trigger_from_datapipe(&call_hold_pipe,   call_hold_trigger, priv);
	remove_trigger_from_datapipe(&call_hangup_pipe, call_hangup_trigger,priv);
}

}
