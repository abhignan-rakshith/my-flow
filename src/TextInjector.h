#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

/// Types text into the currently focused window using platform APIs.
/// Linux: ydotool (works in both GUI and terminal, unlike clipboard paste).
/// macOS: CGEventPost with Unicode keyboard events.
/// Windows: SendInput with KEYEVENTF_UNICODE.
class TextInjector : public QObject {
    Q_OBJECT
public:
    explicit TextInjector(QObject *parent = nullptr);

    bool isAvailable() const;

public slots:
    void type(const QString &text);

signals:
    void injectionError(const QString &error);

private:
    bool m_available = false;
};
