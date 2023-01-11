#ifndef __MAEMOMANAGER_H__
#define __MAEMOMANAGER_H__
// Prevent type issues/conficts between glib and qt
#include "types.h"

#include <QtCore>
#include <voicecallmanager.h>

#include "comm-voicecallmanager.h"

class MaemoProvider;
class MaemoCallHandler;

class MaemoManager : public QObject
{
	Q_OBJECT

public:
	MaemoManager();
	~MaemoManager();
	void setup(void);

	void acceptTrigger(const CallProperties*);
	void hangupTrigger(const CallProperties*);
	void holdTrigger(const CallProperties*);
	void dialTrigger(const CallProperties*);

public slots:
	void voiceCallsChanged();
	void providersChanged();
#if 0

	void defaultProviderChanged();
	void activeVoiceCallChanged();

	void audioModeChanged();
	void audioRoutedChanged();
	void microphoneMutedChanged();
	void speakerMutedChanged();

	void voiceCallStatusChanged();
#endif

signals:

private:
	/* List of (active) voice calls, mapping the VoiceCallHandler->handlerId()
	 * to our class, so that we can find the right calls in the callbacks */
	QHash<QString, MaemoCallHandler*> voicecalls;

	/* qt voicecall manager */
	VoiceCallManager* qt_voicecall_manager;

public:
	QHash<QString, MaemoProvider*> maemo_providers;
};

#endif /* __MAEMOMANAGER_H__ */
