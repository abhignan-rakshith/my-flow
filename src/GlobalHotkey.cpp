#include "GlobalHotkey.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QDBusMetaType>
#include <QVariantMap>
#include <QDebug>

static const QString PORTAL_SERVICE = "org.freedesktop.portal.Desktop";
static const QString PORTAL_PATH = "/org/freedesktop/portal/desktop";
static const QString SHORTCUTS_IFACE = "org.freedesktop.portal.GlobalShortcuts";

// Custom type for shortcut: (sa{sv})
struct PortalShortcut {
    QString id;
    QVariantMap properties;
};
Q_DECLARE_METATYPE(PortalShortcut)
Q_DECLARE_METATYPE(QList<PortalShortcut>)

QDBusArgument &operator<<(QDBusArgument &arg, const PortalShortcut &s)
{
    arg.beginStructure();
    arg << s.id << s.properties;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, PortalShortcut &s)
{
    arg.beginStructure();
    arg >> s.id >> s.properties;
    arg.endStructure();
    return arg;
}

GlobalHotkey::GlobalHotkey(const QString &preferredShortcut, QObject *parent)
    : QObject(parent)
    , m_shortcut(preferredShortcut)
{
    qDBusRegisterMetaType<PortalShortcut>();
    qDBusRegisterMetaType<QList<PortalShortcut>>();

    createSession();
}

bool GlobalHotkey::isAvailable() const
{
    return m_available;
}

void GlobalHotkey::createSession()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_SERVICE, PORTAL_PATH, SHORTCUTS_IFACE, "CreateSession");

    QVariantMap options;
    options["handle_token"] = QString("wispr_flow_session");
    options["session_handle_token"] = QString("wispr_flow_session");
    msg << options;

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "CreateSession failed:" << reply.errorMessage();
        emit error("Failed to create portal session: " + reply.errorMessage());
        return;
    }

    // Construct the predictable session path
    QString busName = QDBusConnection::sessionBus().baseService().mid(1).replace('.', '_');
    m_sessionPath = QDBusObjectPath(
        QString("/org/freedesktop/portal/desktop/session/%1/wispr_flow_session").arg(busName));

    qDebug() << "Session path:" << m_sessionPath.path();
    bindShortcuts();
}

void GlobalHotkey::bindShortcuts()
{
    PortalShortcut shortcut;
    shortcut.id = "record-toggle";
    shortcut.properties["description"] = QString("Toggle voice recording");
    shortcut.properties["preferred_trigger"] = m_shortcut;

    QList<PortalShortcut> shortcuts;
    shortcuts << shortcut;

    QVariantMap options;
    options["handle_token"] = QString("wispr_flow_bind");

    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_SERVICE, PORTAL_PATH, SHORTCUTS_IFACE, "BindShortcuts");
    msg << QVariant::fromValue(m_sessionPath);
    msg << QVariant::fromValue(shortcuts);
    msg << QString();  // parent_window
    msg << options;

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "BindShortcuts failed:" << reply.errorMessage();
        emit error("Failed to bind shortcuts: " + reply.errorMessage());
        return;
    }

    m_available = true;
    connectSignals();
    qDebug() << "Global hotkey bound:" << m_shortcut;
}

void GlobalHotkey::connectSignals()
{
    QDBusConnection::sessionBus().connect(
        PORTAL_SERVICE, PORTAL_PATH, SHORTCUTS_IFACE,
        "Activated",
        this, SLOT(onActivated(QDBusObjectPath,QString,qulonglong,QVariantMap)));

    QDBusConnection::sessionBus().connect(
        PORTAL_SERVICE, PORTAL_PATH, SHORTCUTS_IFACE,
        "Deactivated",
        this, SLOT(onDeactivated(QDBusObjectPath,QString,qulonglong,QVariantMap)));
}

void GlobalHotkey::onActivated(const QDBusObjectPath &sessionPath, const QString &shortcutId,
                                qulonglong timestamp, const QVariantMap &options)
{
    Q_UNUSED(sessionPath); Q_UNUSED(timestamp); Q_UNUSED(options);
    if (shortcutId == "record-toggle") {
        emit pressed();
    }
}

void GlobalHotkey::onDeactivated(const QDBusObjectPath &sessionPath, const QString &shortcutId,
                                  qulonglong timestamp, const QVariantMap &options)
{
    Q_UNUSED(sessionPath); Q_UNUSED(timestamp); Q_UNUSED(options);
    if (shortcutId == "record-toggle") {
        emit released();
    }
}
