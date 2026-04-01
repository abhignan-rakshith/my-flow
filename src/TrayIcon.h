#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>

class TrayIcon : public QObject {
    Q_OBJECT
public:
    enum class State { Idle, Recording, Transcribing };

    explicit TrayIcon(QObject *parent = nullptr);

    void setState(State state);
    void showError(const QString &message);

signals:
    void toggleRecording();
    void settingsRequested();
    void quitRequested();

private:
    void buildMenu();

    QSystemTrayIcon m_tray;
    QMenu m_menu;
    QAction *m_recordAction = nullptr;
    State m_state = State::Idle;
};
