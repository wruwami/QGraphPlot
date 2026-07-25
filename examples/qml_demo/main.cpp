#include <QtCore/QCoreApplication>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // QML 모듈 검색 경로 설정 (빌드 아웃풋 디렉토리)
    engine.addImportPath(QCoreApplication::applicationDirPath());
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../lib");
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../qml");

    // 실시간 데이터 스트리밍(Phase 0.7)은 StreamingDataSource가 main.qml 안에서
    // 직접 인스턴스화하므로(QML_ELEMENT), 여기서는 엔진만 띄운다.
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
