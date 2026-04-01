#pragma once

#include <QObject>
#include <QString>

#ifdef Q_OS_LINUX
#include <QDBusObjectPath>
#endif

#ifdef Q_OS_WIN
#include <QAbstractNativeEventFilter>
#endif

#ifdef Q_OS_MACOS
class QTimer;
#endif

class GlobalHotkey : public QObject
#ifdef Q_OS_WIN
    , public QAbstractNativeEventFilter
#endif
{
    Q_OBJECT
public:
    explicit GlobalHotkey(const QString &preferredShortcut = "<Control><Alt>d",
                          QObject *parent = nullptr);
    ~GlobalHotkey();

    bool isAvailable() const;

signals:
    void pressed();
    void released();
    void error(const QString &message);

public slots:
#ifdef Q_OS_LINUX
    Q_SCRIPTABLE void onToggle();
#endif

private:
    QString m_shortcut;
    bool m_available = false;

#ifdef Q_OS_LINUX
    bool setupDBusService();
    bool registerKeybinding();
    void removeKeybinding();

    static const QString DBUS_SERVICE;
    static const QString DBUS_PATH;
    static const QString KEYBINDING_PATH;
#endif

#ifdef Q_OS_WIN
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
    int m_hotkeyId = 0;
#endif

#ifdef Q_OS_MACOS
    void registerMacHotkey();
    void unregisterMacHotkey();
    quint32 m_hotkeyId = 0;
    void *m_eventHandler = nullptr;
#endif
};
