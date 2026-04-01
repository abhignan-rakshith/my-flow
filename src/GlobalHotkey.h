#pragma once

#include <QObject>
#include <QDBusObjectPath>
#include <QString>

class GlobalHotkey : public QObject {
    Q_OBJECT
public:
    explicit GlobalHotkey(const QString &preferredShortcut = "<Control><Alt>d",
                          QObject *parent = nullptr);

    bool isAvailable() const;

signals:
    void pressed();
    void released();
    void error(const QString &message);

private slots:
    void onActivated(const QDBusObjectPath &sessionPath, const QString &shortcutId,
                     qulonglong timestamp, const QVariantMap &options);
    void onDeactivated(const QDBusObjectPath &sessionPath, const QString &shortcutId,
                       qulonglong timestamp, const QVariantMap &options);

private:
    void createSession();
    void bindShortcuts();
    void connectSignals();

    QString m_shortcut;
    QDBusObjectPath m_sessionPath;
    bool m_available = false;
};
