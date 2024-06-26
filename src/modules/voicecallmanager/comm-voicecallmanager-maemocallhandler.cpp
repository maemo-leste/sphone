#include <time.h>

#include "comm-voicecallmanager-maemocallhandler.h"

#include "moc_comm-voicecallmanager-maemocallhandler.cpp"

MaemoCallHandler::MaemoCallHandler(MaemoManager* mgr_, VoiceCallHandler* handler_)
{
	this->mgr = mgr_;
	this->voicecall_handler = handler_;
	this->backend = NULL;
	this->call_properties = NULL;
	this->call_status = VoiceCallHandler::STATUS_NULL;

	// Setup signals here, we could add more like line id changes, duration
	// changes (I suppose hold might influence the duration)
	connect(voicecall_handler, &VoiceCallHandler::statusChanged, this, &MaemoCallHandler::statusChanged);
}

MaemoCallHandler::~MaemoCallHandler()
{
	disconnect(voicecall_handler);
	call_properties_free(call_properties);
}

void MaemoCallHandler::setupProvider()
{
	if (backend != NULL)
		return;

	sphone_module_log(LL_DEBUG, "setupProvider");

	QString provider_id = voicecall_handler->providerId();
	if (provider_id == "")
		return;

	backend = mgr->maemo_providers[provider_id];
}

void MaemoCallHandler::statusChanged()
{
	// Since it's not setup upon creation, let's check here first
	setupProvider();

	int current_status = voicecall_handler->status();

	qDebug() << "statusChanged status:" << current_status << "old status:" << call_status;
	sphone_module_log(LL_DEBUG, "statusChanged status: %d old status: %d",
	                  current_status, call_status);

	if (call_status == VoiceCallHandler::STATUS_NULL) {
		sphone_module_log(LL_DEBUG, "Creating call");
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
			sphone_module_log(LL_DEBUG, "Not sure what initial state to assign to call!!!");

		call_properties->start_time = time(NULL);

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
	} else if (current_status == VoiceCallHandler::STATUS_HELD) {
		call_properties->state = SPHONE_CALL_HELD;
		execute_datapipe(&call_properties_changed_pipe, call_properties);
	} else if (current_status == VoiceCallHandler::STATUS_WAITING) {
		call_properties->state = SPHONE_CALL_WAITING;
		execute_datapipe(&call_properties_changed_pipe, call_properties);
	} else if (current_status == VoiceCallHandler::STATUS_DISCONNECTED) {
		sphone_module_log(LL_DEBUG, "call status: disconnected");

		
		if (!hangup_communicated) {
			call_properties->state = SPHONE_CALL_DISCONNECTED;
			call_properties->end_time = time(NULL);

			execute_datapipe(&call_properties_changed_pipe, call_properties);
			hangup_communicated = 1;
		}
	}

	call_status = current_status;
}

void MaemoCallHandler::answer()
{
	sphone_module_log(LL_DEBUG, "accept()");
	voicecall_handler->answer();
}

void MaemoCallHandler::hold(bool hold)
{
	sphone_module_log(LL_DEBUG, "hold()");
	voicecall_handler->hold(hold);
}

void MaemoCallHandler::hangup()
{
	sphone_module_log(LL_DEBUG, "hangup()");
	sphone_module_log(LL_DEBUG, "hangup() status %d", voicecall_handler->status());

	if (voicecall_handler->status() != VoiceCallHandler::STATUS_DISCONNECTED) {
		/* This will not trigger statusChanged per se unfortunately, and currently there the voicecall_handler status also isn't STATUS_DISCONNECTED */
		voicecall_handler->hangup();

		if (!hangup_communicated) {
			call_properties->state = SPHONE_CALL_DISCONNECTED;
			call_properties->end_time = time(NULL);
			execute_datapipe(&call_properties_changed_pipe, call_properties);
		}
	}

	hangup_communicated = 1;
}
