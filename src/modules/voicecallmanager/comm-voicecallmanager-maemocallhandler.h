#ifndef __MAEMOCALLHANDLER_H__
#define __MAEMOCALLHANDLER_H__
// Prevent type issues/conficts between glib and qt
#include "datapipe.h"
#include "datapipes.h"
#include "types.h"

#include <QtCore>

#include "comm-voicecallmanager-maemoprovider.h"

#include <voicecallmanager.h>

class MaemoCallHandler : public QObject
{
	Q_OBJECT

public:
	MaemoCallHandler(MaemoManager*, VoiceCallHandler*);
	~MaemoCallHandler();

	void answer();
	void hangup();
	void hold(bool);

public slots:
	//    void error(const QString &error);
	void statusChanged();
#if 0
	void lineIdChanged();
	void durationChanged();
	void startedAtChanged();
	void emergencyChanged();
	void multipartyChanged();
	void forwardedChanged();
	void remoteHeldChanged();
	void childCallsChanged();
	void childCallsListChanged();
	void parentCallChanged();
#endif

private:
	void setupProvider();

	MaemoManager* mgr;
	VoiceCallHandler* voicecall_handler;
	MaemoProvider* backend;
	CallProperties* call_properties;
	int call_status;
	// VoiceCallHandler::VoiceCallStatus call_status;

	// Do we store just the QString handler ID, or do we store the pointer to
	// the handler? We can use the VoiceCallModel->instance(handler_id) code to
	// find a pointer.
};

#if 0
QString handlerId() const;
QString providerId() const;
int status() const;
QString statusText() const;
QString lineId() const;
QDateTime startedAt() const;
int duration() const;
bool isIncoming() const;
bool isMultiparty() const;
bool isEmergency() const;
bool isForwarded() const;
bool isRemoteHeld() const;
VoiceCallModel* childCalls() const;
VoiceCallHandler* parentCall() const;
Q_SIGNALS:
void error(const QString &error);
void statusChanged();
void lineIdChanged();
void durationChanged();
void startedAtChanged();
void emergencyChanged();
void multipartyChanged();
void forwardedChanged();
void remoteHeldChanged();
void childCallsChanged();
void childCallsListChanged();
void parentCallChanged();
#endif

#endif /* __MAEMOCALLHANDLER_H__ */
