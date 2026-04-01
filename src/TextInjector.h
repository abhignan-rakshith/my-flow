#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

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
