#include <clocale>
#include <QApplication>
#include "playerwindow.h"

#ifdef USE_DBUS
#include <QDBusConnection>
#include <QDBusInterface>
#else
#include <QLocalSocket>
#include <QLocalServer>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[]) {

#ifdef _WIN32
    // Launch with --debug to get a console window
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            AllocConsole();
            freopen("CONOUT$", "w", stderr);
            freopen("CONOUT$", "w", stdout);
            break;
        }
    }
#endif

    QApplication app(argc, argv);
    setlocale(LC_NUMERIC, "C");

    app.setApplicationName("LMAO");
    app.setApplicationVersion(APP_VERSION);
    app.setDesktopFileName("dev.elek.LMAO");

    QString filePath = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();

#ifdef USE_DBUS
    QString serviceName = "dev.elek.LMAO";
    QDBusConnection bus = QDBusConnection::sessionBus();

    if (bus.isConnected()) {
        QDBusInterface iface(serviceName, "/", "dev.elek.LMAO", bus);
        if (iface.isValid()) {
            if (!filePath.isEmpty())
                iface.call("openFile", filePath);
            else
                iface.call("raiseWindow");
            return 0;
        }
    }

    bus.registerService(serviceName);
#else
    QString serverName = "dev.elek.LMAO";

    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        if (!filePath.isEmpty()) {
            socket.write(filePath.toUtf8());
            socket.waitForBytesWritten(1000);
        }
        socket.disconnectFromServer();
        return 0;
    }

    QLocalServer::removeServer(serverName);
    QLocalServer server;
    server.listen(serverName);
#endif

    PlayerWindow window;
    window.show();

#ifdef USE_DBUS
    bus.registerObject("/", &window, QDBusConnection::ExportScriptableSlots);
#else
    QObject::connect(&server, &QLocalServer::newConnection, [&window, &server]() {
        QLocalSocket *client = server.nextPendingConnection();
        QObject::connect(client, &QLocalSocket::readyRead, [&window, client]() {
            QString path = QString::fromUtf8(client->readAll()).trimmed();
            if (!path.isEmpty()) {
                window.openFile(path);
                window.raiseWindow();
            }
            client->deleteLater();
        });
    });
#endif

    if (!filePath.isEmpty())
        window.openFile(filePath);

    QIcon appIcon = QIcon::fromTheme("dev.elek.LMAO");
    if (appIcon.isNull())
        appIcon = QIcon("../icons/hicolor/256x256/apps/dev.elek.LMAO.png");
    app.setWindowIcon(appIcon);

    return app.exec();
}