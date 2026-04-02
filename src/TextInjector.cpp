#include "TextInjector.h"
#include <QProcess>
#include <QStandardPaths>

// =============================================================================
// Linux: ydotool
// =============================================================================
#ifdef Q_OS_LINUX

TextInjector::TextInjector(QObject *parent)
    : QObject(parent)
{
    m_available = !QStandardPaths::findExecutable("ydotool").isEmpty();
    if (!m_available) {
        qWarning("ydotool not found in PATH");
    }
}

bool TextInjector::isAvailable() const
{
    return m_available;
}

void TextInjector::type(const QString &text)
{
    if (!m_available) {
        emit injectionError("ydotool is not installed");
        return;
    }

    auto *proc = new QProcess(this);
    proc->setProgram("ydotool");
    proc->setArguments({"type", "--key-delay", "0", "--clearmodifiers", "--", text});

    connect(proc, &QProcess::finished, this, [this, proc](int exitCode, QProcess::ExitStatus status) {
        proc->deleteLater();
        if (status != QProcess::NormalExit || exitCode != 0) {
            emit injectionError("ydotool failed: " + proc->readAllStandardError());
        }
    });

    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError err) {
        Q_UNUSED(err);
        proc->deleteLater();
        emit injectionError("Failed to start ydotool: " + proc->errorString());
    });

    proc->start();
}

#endif // Q_OS_LINUX

// =============================================================================
// Windows: SendInput
// =============================================================================
#ifdef Q_OS_WIN

#include <windows.h>

TextInjector::TextInjector(QObject *parent)
    : QObject(parent)
{
    m_available = true;
}

bool TextInjector::isAvailable() const
{
    return m_available;
}

void TextInjector::type(const QString &text)
{
    // Use SendInput to type Unicode characters
    std::vector<INPUT> inputs;
    inputs.reserve(text.length() * 2);

    for (const QChar &ch : text) {
        INPUT down = {};
        down.type = INPUT_KEYBOARD;
        down.ki.wScan = ch.unicode();
        down.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(down);

        INPUT up = {};
        up.type = INPUT_KEYBOARD;
        up.ki.wScan = ch.unicode();
        up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs.push_back(up);
    }

    UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent != inputs.size()) {
        emit injectionError("SendInput failed: only sent " + QString::number(sent) +
                            " of " + QString::number(inputs.size()) + " events");
    }
}

#endif // Q_OS_WIN

// =============================================================================
// macOS: CGEventPost
// =============================================================================
#ifdef Q_OS_MACOS

#include <ApplicationServices/ApplicationServices.h>
#include <QDebug>

TextInjector::TextInjector(QObject *parent)
    : QObject(parent)
{
    m_available = true;
}

bool TextInjector::isAvailable() const
{
    return m_available;
}

void TextInjector::type(const QString &text)
{
    // Check if we have accessibility permissions
    if (!AXIsProcessTrusted()) {
        emit injectionError("Accessibility permission not granted. Enable in System Settings → Privacy → Accessibility.");
        return;
    }

    // Use CGEventKeyboardSetUnicodeString to type text
    // Process in chunks of 20 chars (CGEvent limit)
    static const int kChunkSize = 20;

    for (int i = 0; i < text.length(); i += kChunkSize) {
        QString chunk = text.mid(i, kChunkSize);
        std::vector<UniChar> uniChars(chunk.length());
        for (int j = 0; j < chunk.length(); ++j) {
            uniChars[j] = chunk[j].unicode();
        }

        CGEventRef keyDown = CGEventCreateKeyboardEvent(nullptr, 0, true);
        CGEventRef keyUp = CGEventCreateKeyboardEvent(nullptr, 0, false);

        CGEventKeyboardSetUnicodeString(keyDown, uniChars.size(), uniChars.data());
        CGEventKeyboardSetUnicodeString(keyUp, uniChars.size(), uniChars.data());

        CGEventPost(kCGHIDEventTap, keyDown);
        CGEventPost(kCGHIDEventTap, keyUp);

        CFRelease(keyDown);
        CFRelease(keyUp);
    }
}

#endif // Q_OS_MACOS
