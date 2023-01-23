#include "comm-telepathy-maemoprovider.h"

#include "moc_comm-telepathy-maemoprovider.cpp"

#include "datapipe.h"
#include "datapipes.h"
#include "comm.h"

static const Scheme call_scheme =
{
	.scheme = (char*)"tel",
	.flags = BACKEND_FLAG_CALL
};


MaemoProvider::MaemoProvider(MaemoManager* mgr, QString id_, QString type_, QString label_) {
    this->maemo_manager = mgr;
    this->id = id_;
    this->type = type_;
    this->label = label_;
}

void MaemoProvider::registerBackend() {
    // TODO: guard against this being called already

    // This is different for ring than it is for sip (where we can do both sip and tel?)
	const Scheme* schemes[2] =
	{
		&call_scheme,
		NULL
	};

    backend_name = strdup(id.toStdString().c_str());
	sphone_backend_id = sphone_comm_add_backend(backend_name, schemes, BACKEND_FLAG_CALL);

    qDebug() << "Registered backend";

}

void MaemoProvider::unregisterBackend() {
    // TODO
    // free(backend_name);
}

MaemoProvider::~MaemoProvider() {
}
