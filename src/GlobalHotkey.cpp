#include "GlobalHotkey.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QProcess>
#include <QDebug>

const QString GlobalHotkey::DBUS_SERVICE = "com.wispr.Flow";
const QString GlobalHotkey::DBUS_PATH = "/com/wispr/Flow";
const QString GlobalHotkey::KEYBINDING_PATH =
    "/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/wispr-flow/";

GlobalHotkey::GlobalHotkey(const QString &preferredShortcut, QObject *parent)
    : QObject(parent)
    , m_shortcut(preferredShortcut)
{
    if (!setupDBusService()) {
        emit error("Failed to register DBus service");
        return;
    }

    if (!registerKeybinding()) {
        emit error("Failed to register GNOME keybinding");
        return;
    }

    m_available = true;
}

GlobalHotkey::~GlobalHotkey()
{
    removeKeybinding();
}

bool GlobalHotkey::isAvailable() const
{
    return m_available;
}

bool GlobalHotkey::setupDBusService()
{
    auto bus = QDBusConnection::sessionBus();

    // Register our service name
    if (!bus.registerService(DBUS_SERVICE)) {
        qWarning() << "Failed to register DBus service" << DBUS_SERVICE;
        return false;
    }

    // Register object and expose slots and scriptable methods
    if (!bus.registerObject(DBUS_PATH, this,
                            QDBusConnection::ExportScriptableSlots)) {
        qWarning() << "Failed to register DBus object";
        return false;
    }

    return true;
}

void GlobalHotkey::onToggle()
{
    qDebug() << "[HOTKEY] Toggle triggered via DBus";
    emit pressed();
}

bool GlobalHotkey::registerKeybinding()
{
    // Get current custom keybindings list
    QProcess get;
    get.start("gsettings", {"get", "org.gnome.settings-daemon.plugins.media-keys",
                            "custom-keybindings"});
    get.waitForFinished(2000);
    QString current = get.readAllStandardOutput().trimmed();

    // Add our keybinding path if not already present
    if (!current.contains(KEYBINDING_PATH)) {
        QString newList;
        if (current == "@as []" || current.isEmpty()) {
            newList = "['" + KEYBINDING_PATH + "']";
        } else {
            // Insert before closing bracket
            newList = current;
            newList.chop(1); // remove ]
            newList += ", '" + KEYBINDING_PATH + "']";
        }
        QProcess::execute("gsettings", {"set",
            "org.gnome.settings-daemon.plugins.media-keys",
            "custom-keybindings", newList});
    }

    // Set the keybinding properties
    QString schema = "org.gnome.settings-daemon.plugins.media-keys.custom-keybinding";
    QString path = KEYBINDING_PATH;

    QProcess::execute("gsettings", {"set", schema + ":" + path,
                                    "name", "Wispr Flow Toggle"});
    QProcess::execute("gsettings", {"set", schema + ":" + path,
                                    "command",
                                    "dbus-send --session --type=method_call "
                                    "--dest=com.wispr.Flow /com/wispr/Flow "
                                    "local.wispr_flow.GlobalHotkey.onToggle"});
    QProcess::execute("gsettings", {"set", schema + ":" + path,
                                    "binding", m_shortcut});

    qDebug() << "Registered GNOME keybinding:" << m_shortcut;
    return true;
}

void GlobalHotkey::removeKeybinding()
{
    // Remove our path from the custom keybindings list
    QProcess get;
    get.start("gsettings", {"get", "org.gnome.settings-daemon.plugins.media-keys",
                            "custom-keybindings"});
    get.waitForFinished(2000);
    QString current = get.readAllStandardOutput().trimmed();

    if (current.contains(KEYBINDING_PATH)) {
        current.remove("'" + KEYBINDING_PATH + "'");
        current.remove(", ,");  // clean up double commas
        current.replace("[, ", "[");
        current.replace(", ]", "]");
        if (current == "[]") current = "@as []";

        QProcess::execute("gsettings", {"set",
            "org.gnome.settings-daemon.plugins.media-keys",
            "custom-keybindings", current});
    }
}
