#include "TrayIcon.h"
#include <QApplication>

TrayIcon::TrayIcon(QObject *parent)
    : QObject(parent)
{
    buildMenu();
    m_tray.setContextMenu(&m_menu);
    setState(State::Idle);
    m_tray.setToolTip("Wispr Flow");
    m_tray.show();

    connect(&m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger) {
                    emit toggleRecording();
                }
            });
}

void TrayIcon::buildMenu()
{
    auto *settingsAction = m_menu.addAction("Settings...");
    connect(settingsAction, &QAction::triggered, this, &TrayIcon::settingsRequested);

    m_menu.addSeparator();

    auto *quitAction = m_menu.addAction("Quit");
    connect(quitAction, &QAction::triggered, this, &TrayIcon::quitRequested);
}

void TrayIcon::setState(State state)
{
    m_state = state;

    // Use built-in theme icons as fallback; custom icons loaded from resources
    switch (state) {
    case State::Idle:
        m_tray.setIcon(QIcon::fromTheme("audio-input-microphone",
                       QIcon(":/icons/mic-off.svg")));
        m_tray.setToolTip("Wispr Flow - Ready");
        break;
    case State::Recording:
        m_tray.setIcon(QIcon::fromTheme("media-record",
                       QIcon(":/icons/mic-on.svg")));
        m_tray.setToolTip("Wispr Flow - Recording...");
        break;
    case State::Transcribing:
        m_tray.setIcon(QIcon::fromTheme("system-run",
                       QIcon(":/icons/mic-on.svg")));
        m_tray.setToolTip("Wispr Flow - Transcribing...");
        break;
    }
}

void TrayIcon::showError(const QString &message)
{
    m_tray.showMessage("Wispr Flow Error", message,
                       QSystemTrayIcon::Critical, 5000);
}
