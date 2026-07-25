#include <QtCore/QCoreApplication>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

#include "StreamingDataSource.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    // Streaming data source: 60K-point ring buffer, 1K points / 16 ms.
    // Declared before the engine so QML references are released first.
    StreamingDataSource dataSource;

    QQmlApplicationEngine engine;

    // QML 모듈 검색 경로 설정 (빌드 아웃풋 디렉토리)
    engine.addImportPath(QCoreApplication::applicationDirPath());
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../lib");
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../qml");

    // 데이터 소스를 QML 컨텍스트에 등록
    engine.rootContext()->setContextProperty("dataSource", &dataSource);

    // 메인 QML 파일 로드
    const QUrl url(QStringLiteral("qrc:/qml_demo/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
