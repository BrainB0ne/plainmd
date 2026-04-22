#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Markdown Viewer");
    app.setOrganizationName("mdviewer");

    MainWindow window;
    window.show();

    if (argc > 1) {
        window.openFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
