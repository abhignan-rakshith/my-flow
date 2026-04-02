#pragma once

#include <QWidget>
#include <QLabel>

/// Floating semi-transparent "Recording..." indicator shown during capture.
class OverlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWidget(QWidget *parent = nullptr);

    void showRecording();
    void hide();

private:
    QLabel *m_label;
};
