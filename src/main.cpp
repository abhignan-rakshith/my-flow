#include <QApplication>
#include "Application.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("wispr-flow");
    app.setOrganizationName("wispr-flow");
    app.setQuitOnLastWindowClosed(false);

    Application wispr;

    return app.exec();
}
