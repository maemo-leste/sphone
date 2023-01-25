#ifndef __MAEMOPROVIDER_H__
#define __MAEMOPROVIDER_H__
// Prevent type issues/conficts between glib and qt
#include "types.h"

#include <QtCore>

#include "comm-telepathy-maemocallhandler.h"
#include "comm-telepathy-maemomanager.h"

class MaemoProvider : public QObject {
    Q_OBJECT

public:
    MaemoProvider(MaemoManager*, QString, QString, QString);
    ~MaemoProvider();

    void registerBackend();
    void unregisterBackend();

    // XXX: make these private or at least protected?
    QString id;
    QString type;
    QString label;
    char* backend_name;
    QString backend_id;
    int sphone_backend_id;

private:
    MaemoManager* maemo_manager;
};

#endif /* __MAEMOPROVIDER_H__ */
