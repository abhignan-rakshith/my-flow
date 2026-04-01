#include "GlobalHotkey.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QVariantMap>
#include <QDebug>

static const QString PORTAL_SERVICE = "org.freedesktop.portal.Desktop";
static const QString PORTAL_PATH = "/org/freedesktop/portal/desktop";
static const QString SHORTCUTS_IFACE = "org.freedesktop.portal.GlobalShortcuts";

GlobalHotkey::GlobalHotkey(const QString &preferredShortcut, QObject *parent)
    : QObject(parent)
    , m_shortcut(preferredShortcut)
{
    createSession();
}

bool GlobalHotkey::isAvailable() const
{
    return m_available;
}

void GlobalHotkey::createSession()
{
    QDBusInterface portal(PORTAL_SERVICE, PORTAL_PATH, SHORTCUTS_IFACE,
                          QDBusConnection::sessionBus());

    if (!portal.isValid()) {
        qWarning() << "GlobalShortcuts portal not available";
        emit error("XDG GlobalShortcuts portal not available");
        return;
    }

    QVariantMap options;
    options["handle_token"] = "wispr_flow_session";
    options["session_handle_token"] = "wispr_flow_session";

    QDBusReply<QDBusObjectPath> reply = portal.call("CreateSession", options);
    if (!reply.isValid()) {
        qWarning() << "CreateSession failed:" << reply.error().message();
        emit error("Failed to create portal session: " + reply.error().message());
        return;
    }

    // The reply is a request path; we need to listen for the Response signal
    QString requestPath = reply.value().path();

    // The response will come asynchronously but we proceed synchronously

    // For simplicity, use a synchronous approach: wait briefly and attempt bind
    // The session path follows a predictable pattern
    m_sessionPath = QDBusObjectPath(
        QString("/org/freedesktop/portal/desktop/session/%1/wispr_flow_session")
            .arg(QDBusConnection::sessionBus().baseService().mid(1).replace('.', '_')));

    bindShortcuts();
}

void GlobalHotkey::bindShortcuts()
{
    QDBusInterface portal(PORTAL_SERVICE, PORTAL_PATH, SHORTCUTS_IFACE,
                          QDBusConnection::sessionBus());

    // Build shortcuts list: array of (id, properties) structs
    QDBusArgument shortcutsArg;
    shortcutsArg.beginArray(QMetaType(QMetaType::fromName("(sa{sv})").id()
                                      ? QMetaType::fromName("(sa{sv})").id()
                                      : qMetaTypeId<QVariant>()));
    shortcutsArg.beginStructure();
    shortcutsArg << QString("record-toggle");
    shortcutsArg.beginMap(QMetaType(QMetaType::QString), QMetaType(QMetaType::fromType<QDBusVariant>()));
    shortcutsArg.beginMapEntry();
    shortcutsArg << QString("description");
    shortcutsArg << QDBusVariant("Toggle voice recording");
    shortcutsArg.endMapEntry();
    shortcutsArg.beginMapEntry();
    shortcutsArg << QString("preferred_trigger");
    shortcutsArg << QDBusVariant(m_shortcut);
    shortcutsArg.endMapEntry();
    shortcutsArg.endMap();
    shortcutsArg.endStructure();
    shortcutsArg.endArray();

    QVariantMap options;
    options["handle_token"] = "wispr_flow_bind";

    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_SERVICE, PORTAL_PATH, SHORTCUTS_IFACE, "BindShortcuts");
    msg << QVariant::fromValue(m_sessionPath);
    msg << QVariant::fromValue(shortcutsArg);
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
