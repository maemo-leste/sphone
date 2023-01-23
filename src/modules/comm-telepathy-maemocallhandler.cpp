#include "comm-telepathy-maemocallhandler.h"

#include "moc_comm-telepathy-maemocallhandler.cpp"

MaemoCallHandler::MaemoCallHandler(MaemoManager* mgr_, VoiceCallHandler* handler_) {
    this->mgr = mgr_;
    this->voicecall_handler = handler_;
    this->backend = NULL;
    this->call_properties = NULL;
    this->call_status = VoiceCallHandler::STATUS_NULL;

    // Setup signals here
    connect(voicecall_handler, &VoiceCallHandler::statusChanged, this, &MaemoCallHandler::statusChanged);
    // TODO: Set up signals for duration changes, line id changes, etc?
}

MaemoCallHandler::~MaemoCallHandler() {
    disconnect(voicecall_handler);
    call_properties_free(call_properties);

    // XXX: I don't think we free this
    //if (call_properties) {
    //    g_free(call_properties->line_identifier);
    //    g_free(call_properties->backend_data);

    //    g_free(call_properties);
    //}
}

void MaemoCallHandler::setupProvider() {
    if (backend != NULL)
        return;

    qDebug() << "setupProvider";

    QString provider_id = voicecall_handler->providerId();
    if (provider_id == "")
        return;

    backend = mgr->maemo_providers[provider_id];
}

void MaemoCallHandler::statusChanged() {
    // Since it's not setup upon creation, let's check here first
    setupProvider();

    int current_status = voicecall_handler->status();

    qDebug() << "statusChanged status:" << current_status << "old status:" << call_status;

    if (call_status == VoiceCallHandler::STATUS_NULL) {
        qDebug() << "Creating call";
        call_properties = (CallProperties*)g_malloc0(sizeof(*call_properties));

        call_properties->backend = backend->sphone_backend_id;
        call_properties->needs_route = backend->type == "tel";
        call_properties->outbound = !voicecall_handler->isIncoming();

        if (current_status == VoiceCallHandler::STATUS_INCOMING)
            call_properties->state = SPHONE_CALL_INCOMING;
        else if (current_status == VoiceCallHandler::STATUS_DIALING)
            call_properties->state = SPHONE_CALL_DIALING;
        else if (current_status == VoiceCallHandler::STATUS_ALERTING)
            call_properties->state = SPHONE_CALL_ALERTING;
        else
            qDebug() << "Not sure what initial state to assign to call!!!";


        call_properties->emergency = voicecall_handler->isEmergency();
        call_properties->line_identifier = g_strdup(voicecall_handler->lineId().toStdString().c_str());
        call_properties->backend_data = g_strdup(voicecall_handler->handlerId().toStdString().c_str());

        execute_datapipe(&call_new_pipe, call_properties);
    } else if (current_status == VoiceCallHandler::STATUS_ACTIVE) {
#if 0
        // Is this the right place?
        if (call_properties->outbound)
            call_properties->answered = true;
#endif
        call_properties->answered = true;

        call_properties->state = SPHONE_CALL_ACTIVE;
        execute_datapipe(&call_properties_changed_pipe, call_properties);
    } else if (current_status == VoiceCallHandler::STATUS_DIALING) {
        call_properties->state = SPHONE_CALL_DIALING;
        execute_datapipe(&call_properties_changed_pipe, call_properties);
    } else if (current_status == VoiceCallHandler::STATUS_ALERTING) {
        call_properties->state = SPHONE_CALL_ALERTING;
        execute_datapipe(&call_properties_changed_pipe, call_properties);
    } else if (current_status == VoiceCallHandler::STATUS_DISCONNECTED) {
        qDebug() << "call status: disconnected";
        call_properties->state = SPHONE_CALL_DISCONNECTED;
        execute_datapipe(&call_properties_changed_pipe, call_properties);
    }

    call_status = current_status;

    // TODO: STATUS_HELD, STATUS_DIALING, STATUS_ALERTING, STATUS_WAITING
}

void MaemoCallHandler::answer() {
    qDebug() << "accept()";
    voicecall_handler->answer();
}

void MaemoCallHandler::hangup() {
    qDebug() << "hangup()";
    voicecall_handler->hangup();
    call_properties->state = SPHONE_CALL_DISCONNECTED;
    execute_datapipe(&call_properties_changed_pipe, call_properties);
}


