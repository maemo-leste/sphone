#ifndef __MAEMOMANAGER_H__
#define __MAEMOMANAGER_H__
// Prevent type issues/conficts between glib and qt
#include "types.h"

#include <QtCore>
#include <voicecallmanager.h>

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
    void dialTrigger(const CallProperties*);

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
    QHash <QString, MaemoCallHandler*> voicecalls;
    VoiceCallManager *mgr; // TODO: rename

public:
    QHash <QString, MaemoProvider*> maemo_providers;

};

#endif /* __MAEMOMANAGER_H__ */
