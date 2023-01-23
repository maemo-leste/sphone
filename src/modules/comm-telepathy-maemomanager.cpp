#include "comm-telepathy-maemomanager.h"
#include "comm-telepathy-maemoprovider.h"

#include "moc_comm-telepathy-maemomanager.cpp"

MaemoManager::MaemoManager() {
}

MaemoManager::~MaemoManager() {
    /* disconnect all signals */
}

void MaemoManager::setup(void) {
    mgr = new VoiceCallManager(qApp);

    connect(mgr, &VoiceCallManager::providersChanged, this, &MaemoManager::providersChanged);
    connect(mgr, &VoiceCallManager::voiceCallsChanged, this, &MaemoManager::voiceCallsChanged);
    connect(mgr, &VoiceCallManager::defaultProviderChanged, this, &MaemoManager::defaultProviderChanged);
    connect(mgr, &VoiceCallManager::activeVoiceCallChanged, this, &MaemoManager::activeVoiceCallChanged);

    connect(mgr, &VoiceCallManager::audioModeChanged, this, &MaemoManager::audioModeChanged);
    connect(mgr, &VoiceCallManager::audioRoutedChanged, this, &MaemoManager::audioRoutedChanged);
    connect(mgr, &VoiceCallManager::microphoneMutedChanged, this, &MaemoManager::microphoneMutedChanged);
    connect(mgr, &VoiceCallManager::speakerMutedChanged, this, &MaemoManager::speakerMutedChanged);

    qDebug() << "default Provider" << mgr->defaultProviderId();
    // For every provider, add a MaemoProvider, store it in a list

    VoiceCallProviderModel * providermodel = mgr->providers();
    for (int i = 0; i < providermodel->count(); i++) {
        qDebug() << "provider model: " << i << providermodel->id(i) << providermodel->type(i) << providermodel->label(i);

        // XXX: Let's do only telepathy-ring atm
        if (providermodel->type(i) == "tel") {
            MaemoProvider* provider = new MaemoProvider(this, providermodel->id(i), providermodel->type(i), providermodel->label(i));
            provider->registerBackend();

            maemo_providers[providermodel->id(i)] = provider;
        }
    }
}

void MaemoManager::providersChanged(void) {
    // TODO: Implement this, what do we do with (active) calls that belong to a
    // provider if they disappear? Let's hope voicecall-manager deals with this
    // properly
    qDebug() << "providersChanged";
}

void MaemoManager::voiceCallsChanged(void) {
    // TODO: this is where incoming and outgoing calls will show, we can create
    // the necessary types here, but maybe leave all the other work to the
    // providers and the call lists, since they should also get signals after
    // this
    //
    // So we should check if there is a new call, and if so, create
    // MaemoCallHandler, the VoiceCallHandler already exists, just check
    // mgr->voiceCalls() - which returns the VoiceCallModel, which can be used
    // to find the VoiceCallHandlers (using voicecallmodel coint and
    // voicecallmodel->instance(idx)
    //
    // What do we match it against to see that already know it? I think we can
    // use VoiceCallHandler->handlerId() - so we compare this to the calls
    // registered with our backends, or directly with the manager? Not sure what
    // makes more sense yet.
    //
    // Could use a QHash of handlerIds (QString) to keep track of all the
    // currently active voice calls, perhaps? That way we can also act on
    // if/when they change - per change, through MaemoCallHandler?


    qDebug() << "voiceCallsChanged";

    VoiceCallModel* voicecallmodel = mgr->voiceCalls();
    for (int i = 0; i < voicecallmodel->count(); i++) {
        VoiceCallHandler* handler = voicecallmodel->instance(i);
        if (!voicecalls.contains(handler->handlerId())) {
            MaemoCallHandler* mh = new MaemoCallHandler(this, handler);
            voicecalls[handler->handlerId()] = mh;
        }
    }

    QMutableHashIterator<QString, MaemoCallHandler*> i(voicecalls);
    while (i.hasNext()) {
        i.next();

        QString handlerid = i.key();
        MaemoCallHandler* mh = i.value();

        qDebug() << "handlerid:" << handlerid;

        if (!voicecallmodel->instance(handlerid)) {
            qDebug() << "Removing MaemoCallHandler with id" << handlerid;

            // Remove from voicecalls using iterator (presumably this works)
            i.remove();

            // Free
            delete mh;
        }
    }

    // Now remove any in our QHash that are not in voicecallmodel
    // voicecallmodel.instance(handlerId)
    //
    // TODO: also see which voice calls need to be removed, right?

    // when found:
    // ->providerId() to find backend
    // ->handlerId() for hashmap
    // then create, and add to hashmap

#if 0
    VoiceCallHandler *handler = mgr->activeVoiceCall();

    if (handler) {
        connect(handler, &VoiceCallHandler::statusChanged, this, &MaemoManager::voiceCallStatusChanged);

        call = (CallProperties*)g_malloc0(sizeof(*call));
        call->backend = priv->backend_id;
        call->needs_route = true;
        // TODO: this isn't always inbound of course (!)
        call->outbound = false;

        // TODO: this isn't always incoming of course (!)
        call->state = SPHONE_CALL_INCOMING;
        call->emergency = false;
        call->line_identifier = "test";
        // TODO:
        // call->line_identifier
        // call->state
        // call->emergency
        // call->backend_data
        execute_datapipe(&call_new_pipe, call);
    }
#endif
}

void MaemoManager::defaultProviderChanged(void) {
    qDebug() << "defaultProviderChanged";
}

void MaemoManager::activeVoiceCallChanged(void) {
    // mgr->activeVoiceCall() will return the currently (newly) active voice call
    qDebug() << "activeVoiceCallChanged";

}

void MaemoManager::audioModeChanged(void) {
    qDebug() << "audioModeChanged" << mgr->audioMode();
}

void MaemoManager::audioRoutedChanged(void) {
    qDebug() << "audioRoutedChanged" << mgr->isAudioRouted();
}

void MaemoManager::microphoneMutedChanged(void) {
    qDebug() << "microphoneMutedChanged" << mgr->isMicrophoneMuted();
}

void MaemoManager::speakerMutedChanged(void) {
    qDebug() << "speakerMutedChanged" << mgr->isSpeakerMuted();
}

void MaemoManager::voiceCallStatusChanged(void) {
#if 0
    VoiceCallHandler *handler = mgr->activeVoiceCall();

    if (strcmp(handler->statusText().toStdString().c_str(), "active")) {
        call->state = SPHONE_CALL_ACTIVE;
    } else if (strcmp(handler->statusText().toStdString().c_str(), "incoming") == 0) {
        call->state = SPHONE_CALL_INCOMING;
    } else if (strcmp(handler->statusText().toStdString().c_str(), "alerting") == 0) {
        //call->state = SPHONE_CALL_ALERTING;
    }

    qDebug() << "voiceCallStatusChanged: status" << handler->statusText();
#endif
}

void MaemoManager::acceptTrigger(const CallProperties *call) {
    QString call_handler = QString(call->backend_data);
    qDebug() << "call handler id" << call_handler;

    if (voicecalls.contains(call_handler)) {
        MaemoCallHandler *mch = voicecalls[call_handler];
        mch->answer();
    }

}

void MaemoManager::hangupTrigger(const CallProperties *call) {
    QString call_handler = QString(call->backend_data);
    qDebug() << "call handler id" << call_handler;

    if (voicecalls.contains(call_handler)) {
        MaemoCallHandler *mch = voicecalls[call_handler];
        mch->hangup();
    }
}

void MaemoManager::dialTrigger(const CallProperties *call) {
    // TODO: Find out what backend to use, then use dial()
}
