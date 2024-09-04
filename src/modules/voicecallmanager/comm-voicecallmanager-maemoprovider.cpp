#include "comm-voicecallmanager.h"
#include "comm-voicecallmanager-maemoprovider.h"

#include "moc_comm-voicecallmanager-maemoprovider.cpp"

#include "comm.h"

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
	const Scheme call_scheme = {
		.scheme = (char*)"tel",
		.flags = BACKEND_FLAG_CALL
	};

	// This is different for ring than it is for sip (where we can do both sip and tel?)
	const Scheme* schemes[2] = {
		&call_scheme,
		NULL
	};

	const sphone_contact_field_t fields[] = {
		SPHONE_FIELD_PHONE,
		SPHONE_FIELD_LISTEND
	};

	backend_id = id;

	QString tmp = QString(id).replace("/org/freedesktop/Telepathy/Account/", "");
	sphone_backend_id = sphone_comm_add_backend(tmp.toStdString().c_str(), tmp.toStdString().c_str(), schemes, BACKEND_FLAG_CALL, fields, char_valid);

	sphone_module_log(LL_DEBUG, "Registered backend: %s", tmp.toStdString().c_str());
}

void MaemoProvider::unregisterBackend()
{
	sphone_module_log(LL_DEBUG, "Unregistering backend: %s", backend_id.toStdString().c_str());
	sphone_comm_remove_backend(sphone_backend_id);
}

MaemoProvider::~MaemoProvider()
{
}
