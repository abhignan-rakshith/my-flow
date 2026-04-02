#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include "Settings.h"

/// Modal dialog for configuring backend, API key, and audio input device.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(Settings &settings, QWidget *parent = nullptr);

signals:
    void settingsChanged();

private:
    void save();

    Settings &m_settings;
    QComboBox *m_backendCombo;
    QLineEdit *m_apiKeyEdit;
    QComboBox *m_audioDeviceCombo;
};
