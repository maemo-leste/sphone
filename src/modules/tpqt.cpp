#include <glib.h>

extern "C" 
{

#include "sphone-modules.h"
#include "sphone-log.h"
#include "datapipe.h"
#include "datapipes.h"

}

/** Module name */
#define MODULE_NAME		"tpqt"


#include <QtCore>
#include <QtDBus/QtDBus>


#include <TelepathyQt/AbstractClientHandler>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/CallChannel>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/AccountSet>

class Telepathy : public QObject {
Q_OBJECT

public:
    explicit Telepathy(QObject *parent = nullptr);
    ~Telepathy() override;

    //QList<TelepathyAccount*> accounts;

signals:
    //void databaseAddition(ChatMessage *msg);
    void accountManagerReady();

public slots:
    //void sendMessage(const QString &local_uid, const QString &remote_uid, const QString &message);
    //v/oid onDatabaseAddition(ChatMessage *msg);

private slots:
    void onAccountManagerReady(Tp::PendingOperation *op);

private:
    Tp::ClientRegistrarPtr registrar;
    Tp::AccountManagerPtr m_accountmanager;
    Tp::AbstractClientPtr clienthandler;
};

class TelepathyHandler : public Tp::AbstractClientHandler {
public:
    TelepathyHandler(const Tp::ChannelClassSpecList &channelFilter);
    ~TelepathyHandler() { }
    bool bypassApproval() const;
    void handleChannels(const Tp::MethodInvocationContextPtr<> &context,
        const Tp::AccountPtr &account,
        const Tp::ConnectionPtr &connection,
        const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
        const QDateTime &userActionTime,
        const Tp::AbstractClientHandler::HandlerInfo &handlerInfo);

    void setTelepathyParent(Telepathy* parent);
public:
    Telepathy* m_telepathy_parent;

};

#include "tpqt.moc"

Telepathy::Telepathy(QObject *parent) : QObject(parent) {
    Tp::AccountFactoryPtr accountFactory = Tp::AccountFactory::create(
                QDBusConnection::sessionBus(),
                Tp::Features()
                    << Tp::Account::FeatureCore
                );

    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
                QDBusConnection::sessionBus(),
                Tp::Features()
                    << Tp::Connection::FeatureCore
                    << Tp::Connection::FeatureSelfContact
                );

    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(QDBusConnection::sessionBus());

    channelFactory->addCommonFeatures(Tp::Channel::FeatureCore);

    channelFactory->addFeaturesForCalls(
            Tp::Features()
            << Tp::CallChannel::FeatureCallState
            << Tp::CallChannel::FeatureCallMembers
            << Tp::CallChannel::FeatureContents
            << Tp::CallChannel::FeatureLocalHoldState
            );

    Tp::ContactFactoryPtr contactFactory = Tp::ContactFactory::create(
                Tp::Features()
                    << Tp::Contact::FeatureAlias
                    << Tp::Contact::FeatureAvatarData
                );

    m_accountmanager = Tp::AccountManager::create(accountFactory);
    connect(m_accountmanager->becomeReady(), &Tp::PendingReady::finished, this, &Telepathy::onAccountManagerReady);

    registrar = Tp::ClientRegistrar::create(accountFactory,
                                            connectionFactory,
                                            channelFactory,
                                            contactFactory);

    auto tphandler = new TelepathyHandler(Tp::ChannelClassSpecList() << Tp::ChannelClassSpec::audioCall()
	<< Tp::ChannelClassSpec::videoCall()
	<< Tp::ChannelClassSpec::videoCallWithAudio()
	<< Tp::ChannelClassSpec::streamedMediaCall()
	<< Tp::ChannelClassSpec::streamedMediaAudioCall()
	<< Tp::ChannelClassSpec::streamedMediaVideoCall()
	<< Tp::ChannelClassSpec::streamedMediaVideoCallWithAudio());

    Tp::AbstractClientPtr handler = Tp::AbstractClientPtr::dynamicCast(
            Tp::SharedPtr<TelepathyHandler>(tphandler));

    tphandler->setTelepathyParent(this);

    registrar->registerClient(handler, "sphone");
}

Telepathy::~Telepathy()
{
}

void Telepathy::onAccountManagerReady(Tp::PendingOperation *op) {
    qDebug() << "onAccountManagerReady";

    auto validaccounts = m_accountmanager->validAccounts();
    auto l = validaccounts->accounts();

    qDebug() << "account count:" << l.count();

    Tp::AccountPtr acc;

    for (int i = 0; i < l.count(); i++) {
        acc = l[i];
        //auto myacc = new TelepathyAccount(acc);
        //accounts << myacc;

        //connect(myacc, SIGNAL(databaseAddition(ChatMessage *)), SLOT(onDatabaseAddition(ChatMessage *)));
    }

    emit accountManagerReady();
}


TelepathyHandler::TelepathyHandler(const Tp::ChannelClassSpecList &channelFilter)
    : Tp::AbstractClientHandler(channelFilter) {
    // XXX: Do we want to do anything here?

}

void TelepathyHandler::setTelepathyParent(Telepathy* parent) {
    m_telepathy_parent = parent;
    return;
}

bool TelepathyHandler::bypassApproval() const {
    return false;
}

void TelepathyHandler::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
                                      const Tp::AccountPtr &account,
                                      const Tp::ConnectionPtr &connection,
                                      const QList<Tp::ChannelPtr> &channels,
                                      const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                                      const QDateTime &userActionTime,
                                      const Tp::AbstractClientHandler::HandlerInfo &handlerInfo) {
    context->setFinished();
}


static Telepathy *telepathy;

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

extern "C" 
{

#if 0
void contact_show_trigger(const void* data, void* user_data)
{
	(void)data;
	sphone_module_log(LL_DEBUG, "%s", __func__);
}
#endif

G_MODULE_EXPORT const gchar *sphone_module_init(void);
const gchar *sphone_module_init(void)
{
	sphone_module_log(LL_DEBUG, "%s", __func__);

    telepathy = new Telepathy(NULL);


	return NULL;
}

G_MODULE_EXPORT void g_module_unload(GModule *mod);
void g_module_unload(GModule *mod)
{
	(void)mod;
}

}
