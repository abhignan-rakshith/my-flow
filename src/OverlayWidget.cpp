#include "OverlayWidget.h"
#include <QHBoxLayout>
#include <QScreen>
#include <QGuiApplication>

OverlayWidget::OverlayWidget(QWidget *parent)
    : QWidget(parent, Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(180, 40);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);

    m_label = new QLabel(this);
    m_label->setStyleSheet(
        "QLabel {"
        "  background-color: rgba(220, 38, 38, 200);"
        "  color: white;"
        "  border-radius: 8px;"
        "  padding: 6px 14px;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "}");
    m_label->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_label);
}

void OverlayWidget::showRecording()
{
    m_label->setText("\xE2\x97\x89  Recording...");  // Unicode red circle

    // Position top-right of primary screen
    if (auto *screen = QGuiApplication::primaryScreen()) {
        QRect geo = screen->availableGeometry();
        move(geo.right() - width() - 20, geo.top() + 20);
    }

    QWidget::show();
}

void OverlayWidget::hide()
{
    QWidget::hide();
}
