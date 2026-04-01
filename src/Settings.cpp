#include "Settings.h"

Settings::Settings()
    : m_settings("wispr-flow", "wispr-flow")
{
}

Settings::Backend Settings::backend() const
{
    QString b = m_settings.value("api/backend", "groq").toString();
    return (b == "openai") ? Backend::OpenAI : Backend::Groq;
}

void Settings::setBackend(Backend b)
{
    m_settings.setValue("api/backend", b == Backend::OpenAI ? "openai" : "groq");
}

QString Settings::apiKey() const
{
    return m_settings.value("api/key").toString();
}

void Settings::setApiKey(const QString &key)
{
    m_settings.setValue("api/key", key);
}

QString Settings::hotkey() const
{
    return m_settings.value("hotkey/shortcut", "<Super>d").toString();
}

void Settings::setHotkey(const QString &shortcut)
{
    m_settings.setValue("hotkey/shortcut", shortcut);
}

QString Settings::audioDevice() const
{
    return m_settings.value("audio/device").toString();
}

void Settings::setAudioDevice(const QString &device)
{
    m_settings.setValue("audio/device", device);
}
