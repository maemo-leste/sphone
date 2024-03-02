#include "comm-voicecallmanager.h"
#include "comm-voicecallmanager-maemoprovider.h"

#include "moc_comm-voicecallmanager-maemoprovider.cpp"

#include "comm.h"
#include "datapipe.h"
#include "datapipes.h"

static const Scheme call_scheme = {
	.scheme = (char*)"tel",
	.flags = BACKEND_FLAG_CALL
};

MaemoProvider::MaemoProvider(MaemoManager* mgr, QString id_, QString type_, QString label_)
{
	this->maemo_manager = mgr;
	this->id = id_;
	this->type = type_;
	this->label = label_;
}

static bool char_valid (uint32_t codepoint)
{
	(void)codepoint;
	return true;
}

void MaemoProvider::registerBackend()
{
	// This is different for ring than it is for sip (where we can do both sip and tel?)
	const Scheme* schemes[2] = {
		&call_scheme,
		NULL
	};

	backend_id = id;
	backend_name = g_strdup(backend_id.toStdString().c_str());
	sphone_backend_id = sphone_comm_add_backend(backend_name, schemes, BACKEND_FLAG_CALL, char_valid);

	sphone_module_log(LL_DEBUG, "Registered backend: %s", backend_name);
}

void MaemoProvider::unregisterBackend()
{
	sphone_module_log(LL_DEBUG, "Unregistering backend: %s", backend_id.toStdString().c_str());
	sphone_comm_remove_backend(sphone_backend_id);
	g_free(backend_name);
}

MaemoProvider::~MaemoProvider()
{
}
