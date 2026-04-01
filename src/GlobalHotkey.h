#pragma once

#include <QObject>
#include <QDBusObjectPath>
#include <QString>

class GlobalHotkey : public QObject {
    Q_OBJECT
public:
    explicit GlobalHotkey(const QString &preferredShortcut = "<Super>d",
                          QObject *parent = nullptr);

    bool isAvailable() const;

signals:
    void pressed();
    void released();
    void error(const QString &message);

private:
    void createSession();
    void bindShortcuts();
    void connectSignals();

    QString m_shortcut;
    QDBusObjectPath m_sessionPath;
    bool m_available = false;
};
