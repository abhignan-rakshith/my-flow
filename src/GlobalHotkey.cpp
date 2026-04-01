#include "GlobalHotkey.h"
#include <QDebug>

// =============================================================================
// Linux: GNOME custom keybinding + DBus
// =============================================================================
#ifdef Q_OS_LINUX

#include <QDBusConnection>
#include <QDBusMessage>
#include <QProcess>

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

    if (!bus.registerService(DBUS_SERVICE)) {
        qWarning() << "Failed to register DBus service" << DBUS_SERVICE;
        return false;
    }

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
    QProcess get;
    get.start("gsettings", {"get", "org.gnome.settings-daemon.plugins.media-keys",
                            "custom-keybindings"});
    get.waitForFinished(2000);
    QString current = get.readAllStandardOutput().trimmed();

    if (!current.contains(KEYBINDING_PATH)) {
        QString newList;
        if (current == "@as []" || current.isEmpty()) {
            newList = "['" + KEYBINDING_PATH + "']";
        } else {
            newList = current;
            newList.chop(1);
            newList += ", '" + KEYBINDING_PATH + "']";
        }
        QProcess::execute("gsettings", {"set",
            "org.gnome.settings-daemon.plugins.media-keys",
            "custom-keybindings", newList});
    }

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
    QProcess get;
    get.start("gsettings", {"get", "org.gnome.settings-daemon.plugins.media-keys",
                            "custom-keybindings"});
    get.waitForFinished(2000);
    QString current = get.readAllStandardOutput().trimmed();

    if (current.contains(KEYBINDING_PATH)) {
        current.remove("'" + KEYBINDING_PATH + "'");
        current.remove(", ,");
        current.replace("[, ", "[");
        current.replace(", ]", "]");
        if (current == "[]") current = "@as []";

        QProcess::execute("gsettings", {"set",
            "org.gnome.settings-daemon.plugins.media-keys",
            "custom-keybindings", current});
    }
}

#endif // Q_OS_LINUX

// =============================================================================
// Windows: RegisterHotKey + native event filter
// =============================================================================
#ifdef Q_OS_WIN

#include <QApplication>
#include <windows.h>

GlobalHotkey::GlobalHotkey(const QString &preferredShortcut, QObject *parent)
    : QObject(parent)
    , m_shortcut(preferredShortcut)
{
    // Parse shortcut — default Ctrl+Alt+D
    UINT modifiers = MOD_CONTROL | MOD_ALT | MOD_NOREPEAT;
    UINT vk = 'D';

    m_hotkeyId = 1;
    if (RegisterHotKey(nullptr, m_hotkeyId, modifiers, vk)) {
        qApp->installNativeEventFilter(this);
        m_available = true;
        qDebug() << "Registered Windows hotkey:" << m_shortcut;
    } else {
        qWarning() << "Failed to register Windows hotkey";
        emit error("Failed to register hotkey");
    }
}

GlobalHotkey::~GlobalHotkey()
{
    if (m_available) {
        UnregisterHotKey(nullptr, m_hotkeyId);
        qApp->removeNativeEventFilter(this);
    }
}

bool GlobalHotkey::isAvailable() const
{
    return m_available;
}

bool GlobalHotkey::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY && msg->wParam == static_cast<WPARAM>(m_hotkeyId)) {
            qDebug() << "[HOTKEY] Toggle triggered via Windows hotkey";
            emit pressed();
            return true;
        }
    }
    return false;
}

#endif // Q_OS_WIN

// =============================================================================
// macOS: Carbon RegisterEventHotKey
// =============================================================================
#ifdef Q_OS_MACOS

#include <Carbon/Carbon.h>
#include <QApplication>

// Global callback — routes to the GlobalHotkey instance
static GlobalHotkey *s_instance = nullptr;

static OSStatus hotkeyHandler(EventHandlerCallRef /*nextHandler*/, EventRef event, void * /*userData*/)
{
    if (s_instance) {
        qDebug() << "[HOTKEY] Toggle triggered via macOS hotkey";
        QMetaObject::invokeMethod(s_instance, "pressed", Qt::QueuedConnection);
    }
    return noErr;
}

GlobalHotkey::GlobalHotkey(const QString &preferredShortcut, QObject *parent)
    : QObject(parent)
    , m_shortcut(preferredShortcut)
{
    s_instance = this;
    registerMacHotkey();
}

GlobalHotkey::~GlobalHotkey()
{
    unregisterMacHotkey();
    s_instance = nullptr;
}

bool GlobalHotkey::isAvailable() const
{
    return m_available;
}

void GlobalHotkey::registerMacHotkey()
{
    // Default: Ctrl+Option+D (macOS equivalent of Ctrl+Alt+D)
    // kVK_ANSI_D = 0x02, cmdKey=256, optionKey=2048, controlKey=4096
    UInt32 modifiers = optionKey | controlKey;  // Ctrl+Option
    UInt32 keyCode = kVK_ANSI_D;

    EventTypeSpec eventType;
    eventType.eventClass = kEventClassKeyboard;
    eventType.eventKind = kEventHotKeyPressed;

    EventHandlerRef handlerRef;
    OSStatus status = InstallApplicationEventHandler(&hotkeyHandler, 1, &eventType, nullptr, &handlerRef);
    if (status != noErr) {
        qWarning() << "Failed to install macOS hotkey handler";
        emit error("Failed to install hotkey handler");
        return;
    }
    m_eventHandler = handlerRef;

    EventHotKeyID hotkeyID;
    hotkeyID.signature = 'WSPR';
    hotkeyID.id = 1;

    EventHotKeyRef hotkeyRef;
    status = RegisterEventHotKey(keyCode, modifiers, hotkeyID,
                                  GetApplicationEventTarget(), 0, &hotkeyRef);
    if (status != noErr) {
        qWarning() << "Failed to register macOS hotkey";
        emit error("Failed to register hotkey");
        return;
    }

    m_hotkeyId = 1;
    m_available = true;
    qDebug() << "Registered macOS hotkey: Ctrl+Option+D";
}

void GlobalHotkey::unregisterMacHotkey()
{
    if (m_eventHandler) {
        RemoveEventHandler(static_cast<EventHandlerRef>(m_eventHandler));
        m_eventHandler = nullptr;
    }
}

#endif // Q_OS_MACOS
