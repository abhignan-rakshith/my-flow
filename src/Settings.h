#pragma once

#include <QSettings>
#include <QString>

/// Persistent app configuration via QSettings (~/.config/wispr-flow/ on Linux).
/// Stores backend choice, API key, hotkey shortcut, and audio device.
class Settings {
public:
    Settings();

    enum class Backend { OpenAI, Groq };

    Backend backend() const;
    void setBackend(Backend b);

    QString apiKey() const;
    void setApiKey(const QString &key);

    QString hotkey() const;
    void setHotkey(const QString &shortcut);

    QString audioDevice() const;
    void setAudioDevice(const QString &device);

private:
    QSettings m_settings;
};
