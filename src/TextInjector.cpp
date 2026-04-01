#include "TextInjector.h"
#include <QProcess>
#include <QStandardPaths>

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
