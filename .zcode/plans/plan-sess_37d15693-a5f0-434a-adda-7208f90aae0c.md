# 이슈 #57 — QmlLineSeries → core QLineSeries 컴포지션 구조 전환

## 목표
`QmlLineSeries`가 core `QAbstractSeries`를 사용하지 않고 프로퍼티/검증 로직을 이중 구현하는 문제를, **core에 `QLineSeries` 구체 타입을 신설**하고 `QmlLineSeries`가 이를 **컴포지션으로 소유/위임**하는 구조로 해결한다. 검증·시그널·기본값이 core 한 곳에만 존재하게 된다.

이슈 #58(시리즈 컬렉션 관리)은 별도 PR로 분리한다.

---

## 1. core에 `QLineSeries` 구체 타입 신설

**새 파일 `src/core/series/QLineSeries.h` / `.cpp`**

```cpp
namespace qgraphplot {
class QGRAPHPLOT_EXPORT QLineSeries : public QAbstractSeries {
    Q_OBJECT
public:
    explicit QLineSeries(QObject* parent = nullptr);
    SeriesType type() const override { return SeriesType::Line; }
};
}
```
- `QAbstractSeries::type()` 순수가상의 유일한 구현체 → enum이 더 이상 죽은 코드가 아님.
- 프로퍼티/검증/시그널은 전부 기반 클래스 `QAbstractSeries`에서 상속 (재구현 없음).
- `.cpp`는 생성자 구현만 (부모 위임).

**`src/core/CMakeLists.txt`**: `_core_sources`와 `_core_public_headers`에 `series/QLineSeries.h` / `series/QLineSeries.cpp` 추가.

---

## 2. `QmlLineSeries`를 컴포지션 구조로 재작성

**`src/qml_frontend/qmllineseries.h`** 변경:
- 멤버 `QLineSeries* m_series` 추가 (QmlLineSeries가 소유 — 생성자에서 `new QLineSeries(this)`, Qt 부모-자식 정리).
- 멤버 `m_model`/`m_color`/`m_name`/`m_lineWidth`/`m_dashPattern` **삭제** (이중 구현 제거).
- Q_PROPERTY의 READ/WRITE는 위임 getter/setter로 교체:
  - `model()` → `m_series->model()`, `setModel()` → `m_series->setModel()`
  - `color()` → `m_series->color()`, `setColor()` → `m_series->setColor()`
  - `name()` → `m_series->name()`, `setName()` → `m_series->setName()`
  - `lineWidth()` → `m_series->lineWidth()`, `setLineWidth()` → `m_series->setLineWidth()`
  - `dashPattern()` → `m_series->dashPattern()`, `setDashPattern()` → `m_series->setDashPattern()`
  - **추가: `visible` Q_PROPERTY** (이슈 완료기준) — `isVisible()`/`setVisible()` 위임. 단 `QQuickItem`도 이미 `visible`을 가지므로, 시리즈 `visible`은 **렌더링 스킵 용도**로 `updatePaintNode` 시작부에서 체크 (`if (!m_series->isVisible()) return nullptr;`). Q_PROPERTY 이름 충돌을 피하기 위해 시리즈 visible은 기존 QQuickItem::visible에 의존하지 않고 별도 시맨틱 유지 — 이 부분은 구현 중 QQuickItem의 visible과 충돌하지 않는지 검증 후, 필요하면 프로퍼티를 `seriesVisible`로 명명하거나 QQuickItem::visible에 위임. (기본: `Q_PROPERTY(bool visible ...)` 시도, 충돌 시 `seriesVisible`로 폴백 — 하위호환 메모에 기록)
- **시그널 전달(forwarding)**: QML NOTIFY 시그널(`colorChanged` 등)을 선언하고, 생성자에서 `connect(m_series, &QLineSeries::colorChanged, this, &QmlLineSeries::colorChanged)` 등으로 연결. core 시그널은 인자형(`colorChanged(QColor)`)이고 QML 시그널은 무인자였으나, Qt는 인자형→무인자 시그널 연결을 허용하므로 하위호환 유지됨. `modelChanged`의 경우 core가 `modelChanged(QAbstractSeriesModel*)`를 emit하면 QML `modelChanged()`가 발화.
- **렌더 트리거**: 현재 각 setter가 `update()`를 호출. 컴포지션 후 setter는 core에 위임되므로, 생성자에서 core 시그널 → `this->update()` 연결로 대체 (`connect(m_series, &QLineSeries::colorChanged, this, [this]{ update(); });` 등). modelChanged → update 연결 포함.

**`src/qml_frontend/qmllineseries.cpp`** 변경:
- `updatePaintNode`: `m_model` → `m_series->model()`, `m_color` → `m_series->color()`, `m_lineWidth` → `m_series->lineWidth()`, `m_dashPattern` → `m_series->dashPattern()` 로컬 변수로 한 번 캐시(핫패스 반복 호출 방지).
- `connectModelSignals`/`disconnectModelSignals`/`handleModelReset`/`handleDataChanged`: core `QAbstractSeries::setModel`이 이미 model destroyed 처리를 하므로, QML쪽의 model 시그널 연결(`dataChanged`/`pointsInserted` 등 → `update()`)은 **렌더 갱신용으로 유지**. 단 `handleModelReset`의 `m_model = nullptr` 직접 대입은 제거 (core가 관리).
- 기존 setter 구현체 전부 삭제, 위임 1줄로 교체.

---

## 3. 테스트 추가 (완료기준: "core 시리즈 검증 로직이 QML 경유로도 동일하게 동작")

이슈 완료기준은 "core 시리즈 검증 로직이 QML 경유로도 동일하게 동작함을 확인"이나, `QmlLineSeries`는 `QQuickItem`이라 GUI 없는 QtTest(`QTEST_GUILESS_MAIN`)에서 인스턴스화 어려움. 따라서:

**`tests/TestAbstractSeries.cpp`**에 `QLineSeries` 구체 타입 테스트 추가:
- `qLineSeriesTypeReturnsLine()` — `QLineSeries s; QCOMPARE(s.type(), SeriesType::Line)`
- 기존 TestSeries fixture가 `SeriesType::Line`을 반환하므로, `QLineSeries`로 동일 동작 보장 테스트.
- (선택) 누락된 `setLineWidth`/`setDashPattern` 검증 경로 테스트 보강 — Explore에서 발견된 커버리지 갭. 이건 #57 범위는 아니지만 컴포지션 전환이 검증 로직 단일화를 전제하므로, 핵심 검증 경로(잘못된 lineWidth 거부, 홀수 dashPattern 거부) 최소 테스트 추가.

QML 경유 테스트는 GUI 의존성이 커서 본 PR에서는 core `QLineSeries` 단위테스트로 검증 로직 단일화를 보증하고, QML 빌드/링크 통합(`qt_add_qml_module` 컴파일 통과)으로 QML 노출을 확인.

---

## 4. 완료기준 체크리스트 매핑
- ✅ "QML/Widget 시리즈의 공통 프로퍼티 상태·검증 로직이 core 단일 구현으로 수렴" — QmlLineSeries의 이중 멤버/검증 삭제, `QLineSeries`로 위임.
- ✅ "visible이 QML 시리즈에서도 동작" — `visible` Q_PROPERTY 추가 + updatePaintNode 스킵.
- ✅ "기존 QML API(프로퍼티 이름)는 하위 호환 유지" — 프로퍼티 이름 동일(model/color/name/lineWidth/dashPattern), `main.qml` 변경 불필요.
- ✅ "단위 테스트" — `QLineSeries` 타입 테스트 + 검증 경로 테스트.

---

## 5. 빌드/검증 단계
1. core 라이브러리 빌드 (`cmake --build` QGraphPlotCore 타겟).
2. QML 프론트엔드 빌드 (`qml_frontend` 타겟) — 컴포지션 컴파일 확인.
3. 테스트 빌드+실행 (ctest) — 기존 테스트 회귀 없음 + 새 QLineSeries 테스트 통과.
4. (가능하면) qml_demo 빌드로 main.qml 5개 프로퍼티 바인딩이 여전히 동작하는지 확인.

## 6. 브랜치
현재 `main`에 있음. 작업용 브랜치 `refactor/57-qml-series-composition` 생성 후 작업. (#59/#60 이슈 브랜치는 별개 이슈이므로 사용 안 함.)