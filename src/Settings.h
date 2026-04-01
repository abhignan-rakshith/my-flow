#pragma once

#include <QSettings>
#include <QString>

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
