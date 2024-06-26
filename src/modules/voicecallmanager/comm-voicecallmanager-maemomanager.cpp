#include "comm-voicecallmanager.h"
#include "comm-voicecallmanager-maemomanager.h"
#include "comm-voicecallmanager-maemoprovider.h"

#include "moc_comm-voicecallmanager-maemomanager.cpp"

MaemoManager::MaemoManager()
{
}

MaemoManager::~MaemoManager()
{
	/* TODO: delete all voicecalls */

	/* TODO: delete all providers */

	disconnect(qt_voicecall_manager);
	delete qt_voicecall_manager;
}

void MaemoManager::setup(void)
{
	qt_voicecall_manager = new VoiceCallManager(qApp);

	connect(qt_voicecall_manager, &VoiceCallManager::voiceCallsChanged, this, &MaemoManager::voiceCallsChanged);
	connect(qt_voicecall_manager, &VoiceCallManager::providersChanged, this, &MaemoManager::providersChanged);

	VoiceCallProviderModel* providermodel = qt_voicecall_manager->providers();
	for (int i = 0; i < providermodel->count(); i++) {
		MaemoProvider* provider = new MaemoProvider(this, providermodel->id(i), providermodel->type(i),
		        providermodel->label(i));
		provider->registerBackend();

		maemo_providers[providermodel->id(i)] = provider;
	}
}

void MaemoManager::voiceCallsChanged(void)
{
	// So we should check if there is a new call, and if so, create
	// MaemoCallHandler, the VoiceCallHandler already exists, just check
	// qt_voicecall_manager->voiceCalls() - which returns the VoiceCallModel, which can be used
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

	sphone_module_log(LL_DEBUG, "voiceCallsChanged");

	VoiceCallModel* voicecallmodel = qt_voicecall_manager->voiceCalls();
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

		sphone_module_log(LL_DEBUG, "handlerid: %s", handlerid.toStdString().c_str());

		/* voicecall is gone */
		if (!voicecallmodel->instance(handlerid)) {
			// If the remote party hung up on us, we still
			// need to execute the datapipe, hangup does this
			// for us
			mh->hangup(); // If not already

			sphone_module_log(LL_DEBUG, "Removing MaemoCallHandler with id %s", handlerid.toStdString().c_str());

			// Remove from voicecalls using iterator
			i.remove();
			delete mh;
		}
	}
}

void MaemoManager::providersChanged(void)
{
	sphone_module_log(LL_DEBUG, "providersChanged");

	VoiceCallProviderModel* providermodel = qt_voicecall_manager->providers();

	/* Add new providers that we do not know of yet */
	for (int i = 0; i < providermodel->count(); i++) {
		if (!maemo_providers.contains(providermodel->id(i))) {
			MaemoProvider* provider = new MaemoProvider(this, providermodel->id(i), providermodel->type(i),
			        providermodel->label(i));
			provider->registerBackend();

			maemo_providers[providermodel->id(i)] = provider;
		}
	}

	/* Remove providers that are no longer in the provider model */
	QHashIterator<QString, MaemoProvider*> i(maemo_providers);
	while (i.hasNext()) {
		i.next();

		QString providerid = i.key();
		MaemoProvider* mp = i.value();

		bool found = false;
		for (int c = 0; c < providermodel->count(); c++) {
			if (providermodel->id(c) == mp->backend_id) {
				found = true;
				break;
			}
		}

		if (!found) {
			mp->unregisterBackend();
			maemo_providers.remove(mp->backend_id);
			delete mp;
		}
	}
}

void MaemoManager::acceptTrigger(const CallProperties* call)
{
	QString call_handler = QString(call->backend_data);
	sphone_module_log(LL_DEBUG, "call handler id: %s", call_handler.toStdString().c_str());

	if (voicecalls.contains(call_handler)) {
		MaemoCallHandler* mch = voicecalls[call_handler];
		mch->answer();
	}
}

void MaemoManager::holdTrigger(const CallProperties* call)
{
	QString call_handler = QString(call->backend_data);
	sphone_module_log(LL_DEBUG, "hold handler id: %s", call_handler.toStdString().c_str());

	if (voicecalls.contains(call_handler)) {
		MaemoCallHandler* mch = voicecalls[call_handler];
		mch->hold(!(call->state == SPHONE_CALL_HELD));
	}
}

void MaemoManager::hangupTrigger(const CallProperties* call)
{
	QString call_handler = QString(call->backend_data);
	sphone_module_log(LL_DEBUG, "hangup call handler id: %s", call_handler.toStdString().c_str());

	if (voicecalls.contains(call_handler)) {
		MaemoCallHandler* mch = voicecalls[call_handler];
		mch->hangup();
	}
}

void MaemoManager::dialTrigger(const CallProperties* call)
{
	QHashIterator<QString, MaemoProvider*> i(maemo_providers);
	while (i.hasNext()) {
		i.next();

		QString providerid = i.key();
		MaemoProvider* mp = i.value();

		if ((mp->sphone_backend_id) == call->backend) {
			sphone_module_log(LL_DEBUG, "found matching backend for call");
			qt_voicecall_manager->dial(mp->id, call->line_identifier);
			break;
		}
	}
}
