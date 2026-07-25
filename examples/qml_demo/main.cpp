#include <QtCore/QCoreApplication>
#include <QtCore/qmath.h>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

#include "model/QRingBufferSeriesModel.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    // 1. 샘플용 실시간 링 버퍼 모델 생성 (용량 200)
    qgraphplot::QRingBufferSeriesModel model(200);

    // 2. 사인 그래프 형태의 샘플 데이터 생성
    for (int i = 0; i < 200; ++i) {
        double x = i * 0.1;
        double y = qSin(x);
        model.append(QPointF(x, y));
    }

    QQmlApplicationEngine engine;

    // QML 모듈 검색 경로 설정 (빌드 아웃풋 디렉토리)
    engine.addImportPath(QCoreApplication::applicationDirPath());
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../lib");
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../qml");

    // 3. 모델을 QML 컨텍스트에 등록
    engine.rootContext()->setContextProperty("chartModel", &model);

    // 4. 메인 QML 파일 로드
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
