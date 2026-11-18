#include "ui/MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QProxyStyle>
#include <QStyleOption>


int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Termind Desktop");
    app.setOrganizationName("Termind");
    app.setApplicationVersion("1.0.0");
    app.setDesktopFileName("termind-desktop");

    QFile qssFile(":styles/dark.qss");
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(qssFile.readAll()));
    }

    QString initialWorkspace = QDir::currentPath();
    if (argc > 1) {
        const QFileInfo fi(QString::fromLocal8Bit(argv[1]));
        if (fi.exists() && fi.isDir()) {
            initialWorkspace = fi.canonicalFilePath();
        }
    }

    MainWindow window(initialWorkspace);
    window.show();
    return app.exec();
}
