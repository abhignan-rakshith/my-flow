#pragma once

#include <QWidget>
#include <QLabel>

class OverlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWidget(QWidget *parent = nullptr);

    void showRecording();
    void hide();

private:
    QLabel *m_label;
};
