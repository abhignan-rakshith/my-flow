#pragma once

#include <QObject>
#include <QString>
#include <QDBusObjectPath>

class QDBusServiceWatcher;

class GlobalHotkey : public QObject {
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
    Q_SCRIPTABLE void onToggle();

private:
    bool setupDBusService();
    bool registerKeybinding();
    void removeKeybinding();

    QString m_shortcut;
    bool m_available = false;

    static const QString DBUS_SERVICE;
    static const QString DBUS_PATH;
    static const QString KEYBINDING_PATH;
};
