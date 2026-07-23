# QGraphPlot — MVP 플랜 (최종본)

> 이 문서는 QGraphPlot의 Phase 0 (MVP) 구현 로드맵이다.
> `AI.md`의 규칙을 전제로 하며, 모든 구현 작업은 이 문서와 AI.md를 따른다.

---

## 📌 핵심 전략: Interface-First (인터페이스 선설계)

> 합의된 하이브리드 접근. 두 극단(동시 개발 vs 순차 연기)의 장점만 취한다.

**원칙**:
- C++ 코어 **인터페이스**는 QWidget/QML 양쪽 요구사항을 모두 고려해 **처음부터 설계**한다.
- 구현은 **QML 프론트엔드부터 완전히 완성**한다.
- Widget 프론트엔드는 **스텁(빈 구현)**부터 시작하여 Phase 1에서 고도화한다.

**근거**:
- 양쪽 요구를 동시에 보며 코어를 설계 → 추상화가 정확, 재사용 극대화
- 두 파이프라인 제약을 처음부터 반영 → 병목 없는 공통 인터페이스
- 공수는 QML+스텁 수준으로 억제 (동시 완전 구현 대비 -40%)
- Widget 렌더링 백엔드 결정은 데이터/경험 충분히 쌓인 뒤로 미룸

---

## 🎯 프로젝트 목표 (AI.md §1 재인용)

| 항목 | 결정 |
|---|---|
| **라이선스** | Apache-2.0 (상용 클로즈드 자유) |
| **프론트엔드** | QWidget + QML 양쪽 지원 (동일 동작 보장) |
| **Qt 버전** | Qt 6.7+ (CI: 6.7.3) |
| **코드 품질** | Effective C++ 원칙 (const correctness, explicit, enum class 등) |

---

## 🏗️ 아키텍처: 3계층 (공유 코어 + 독립 프론트엔드)

```
┌─────────────────────────────────────────────────────────────┐
│   C++ 공유 코어 (QGraphPlotCore)                            │
│                                                             │
│   ┌─ Model Layer ──────────────────────────────────────┐   │
│   │  - QAbstractSeriesModel (인터페이스)                │   │
│   │  - QRingBufferSeriesModel (60fps 실시간, lock-free) │   │
│   │  [future] QStaticSeriesModel, ...                   │   │
│   └─────────────────────────────────────────────────────┘   │
│   ┌─ Transform Layer ───────────────────────────────────┐  │
│   │  - 좌표 변환 (data ↔ pixel, High-DPI 안전)          │   │
│   │  - 스케일 / 뷰포트 / 히트 테스트                      │   │
│   │  - 축 계산 (틱, 라벨)                                │   │
│   └─────────────────────────────────────────────────────┘   │
│   ┌─ Series Interface ──────────────────────────────────┐  │
│   │  - QAbstractSeries (렌더링 미포함 순수 추상)         │   │
│   │  - 데이터 접근 API만 정의                            │   │
│   └─────────────────────────────────────────────────────┘   │
│   [QML/Widget 양쪽이 동일하게 사용]                          │
├─────────────────────────────────────────────────────────────┤
│   QML 프론트엔드              │   Widget 프론트엔드           │
│   ┌──────────────────────┐    │   ┌──────────────────────┐  │
│   │ QQuickItem 파생       │    │   │ QWidget 파생          │  │
│   │ QSGGeometryNode      │    │   │ 백엔드: 미정           │  │
│   │ QSGRenderNode (QRhi) │    │   │   (QPainter 후보)     │  │
│   │ Render Thread        │    │   │ GUI Thread            │  │
│   │ Phase 0 완전 구현    │    │   │ Phase 0 스텁           │  │
│   └──────────────────────┘    │   │ Phase 1 고도화         │  │
│                               │   └──────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

**핵심 원칙**: 모델(데이터) ↔ 트랜스폼(변환) ↔ 시리즈(추상) ↔ 프론트엔드(렌더링).
새 차트 타입은 `QAbstractSeries` 구현체 1개 + 각 프론트엔드용 렌더러 1개씩 추가만으로 끝.

---

## 📂 디렉토리 구조 (Phase 0 기준)

```
QGraphPlot/
├── CMakeLists.txt                      # ✅ 작성됨 (루트, 버전 0.1.0)
├── AI.md                               # ✅ 작성됨 (AI 강제 규칙)
├── VERSIONING.md                       # ✅ 작성됨 (SemVer 상세)
├── README.md                           # ✅ 작성됨
├── LICENSE                             # ✅ 작성됨 (Apache-2.0)
├── .clang-format                       # ✅ 작성됨
├── cppcheck.options                    # ✅ 작성됨
├── .cppcheck-suppressions              # ✅ 작성됨
├── .gitignore / .gitattributes         # ✅ 작성됨
├── .github/workflows/ci.yml            # ✅ 작성됨 (3플랫폼 × Qt 6.7.3)
├── QGraphPlot_MVP_Plan.md              # ✅ 이 문서
│
├── src/
│   ├── core/                           # 공유 C++ 코어
│   │   ├── CMakeLists.txt
│   │   ├── qgraphplot_global.h         # QGRAPHPLOT_EXPORT 매크로
│   │   ├── model/
│   │   │   ├── QAbstractSeriesModel.h/.cpp
│   │   │   └── QRingBufferSeriesModel.h/.cpp
│   │   ├── transform/
│   │   │   ├── QCoordinateTransform.h/.cpp   # data ↔ pixel 변환
│   │   │   └── QScaleEngine.h/.cpp           # 축 틱/라벨 계산
│   │   └── series/
│   │       └── QAbstractSeries.h/.cpp        # 순수 인터페이스 (렌더링 미포함)
│   │
│   ├── qml_frontend/                   # QML 프론트엔드
│   │   ├── CMakeLists.txt
│   │   ├── qml/                         # QML 파일 (qt_add_qml_module)
│   │   │   ├── QGraphPlot/
│   │   │   │   ├── ChartView.qml
│   │   │   │   └── LineSeries.qml
│   │   ├── qmlchartview.h/.cpp          # QQuickItem 파생
│   │   ├── qmllineseries.h/.cpp         # QSGGeometryNode 기반 라인
│   │   ├── qmlaxis.h/.cpp               # 축 (QSGTextNode)
│   │   └── qmllegend.h/.cpp             # 범례
│   │
│   └── widget_frontend/                # Widget 프론트엔드 (Phase 0 스텁)
│       ├── CMakeLists.txt
│       ├── widgetchartview.h/.cpp       # QWidget 파생 (Phase 0: 빈 구현)
│       └── [Phase 1에서 렌더러 추가]
│
├── examples/
│   ├── qml_demo/                       # QML 데모
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   └── main.qml
│   └── widget_demo/                    # Widget 데모 (Phase 0: 최소)
│       ├── CMakeLists.txt
│       └── main.cpp
│
└── tests/
    ├── CMakeLists.txt
    ├── TestRingBuffer.cpp              # 링 버퍼 단위 테스트
    ├── TestCoordinate.cpp              # 좌표 변환 단위 테스트
    └── TestScaleEngine.cpp             # 스케일 엔진 단위 테스트
```

---

## 🔑 핵심 인터페이스 설계 (Phase 0.1~0.2)

### 1. `QAbstractSeriesModel` (데이터 인터페이스)

```cpp
class QGRAPHPLOT_EXPORT QAbstractSeriesModel : public QObject {
    Q_OBJECT
public:
    explicit QAbstractSeriesModel(QObject* parent = nullptr);
    virtual ~QAbstractSeriesModel() = default;

    // 차트 특화 API (QModelIndex 스타일 아님)
    virtual qsizetype pointCount() const = 0;
    virtual QPointF pointAt(qsizetype index) const = 0;

    // 범위 조회 (LOD/서브샘플링용)
    virtual std::span<const QPointF> points(
        qsizetype first, qsizetype last) const = 0;

    // 전체 bounds (캐시 가능)
    virtual QRectF bounds() const = 0;

signals:
    void pointsInserted(qsizetype first, qsizetype last);
    void pointsRemoved(qsizetype first, qsizetype last);
    void dataChanged();
    void boundsChanged();
};
```

### 2. `QRingBufferSeriesModel` (실시간 60fps)

- `std::vector<QPointF>` + head_index + capacity 고정
- 새 데이터는 `appendBatch(span)` 으로 밀어넣기 (단일 포인트 append 대비 +10배 빠름)
- 초과분은 자동으로 삭제, 시그널 emit
- **스레드 안전**: 데이터 생산자가 별도 스레드인 경우 `QMutex` 사용 (옵션)

### 3. `QAbstractSeries` (차트 타입 인터페이스 — 순수 추상)

```cpp
class QGRAPHPLOT_EXPORT QAbstractSeries : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractSeriesModel* model READ model WRITE setModel
               NOTIFY modelChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
public:
    explicit QAbstractSeries(QObject* parent = nullptr);

    // 접근자 (const correctness — AI.md §1.4)
    QAbstractSeriesModel* model() const;
    QColor color() const;
    QString name() const;

    // ... setter / signals 생략

    // NOTE: 렌더링 메서드는 이 클래스에 없다.
    // 각 프론트엔드의 전용 서브클래스에서 별도 정의.
};
```

**중요**: 코어의 `QAbstractSeries`는 **렌더링을 모른다**. QML/Widget 프론트엔드가 각자 다음을 상속:

```cpp
// QML 쪽
class QmlLineSeries : public QAbstractSeries {
    QSGNode* updatePaintNode(QSGNode* old, QQuickWindow* window);
};

// Widget 쪽 (Phase 1)
class WidgetLineSeries : public QAbstractSeries {
    void paint(QPainter* painter);  // 또는 다른 백엔드
};
```

→ 코어 인터페이스는 양쪽에서 공유, 렌더링은 분리.

### 4. `QCoordinateTransform` (좌표 변환 — 공유)

```cpp
class QGRAPHPLOT_EXPORT QCoordinateTransform {
public:
    QCoordinateTransform(QRectF dataBounds, QRectF pixelRect,
                         bool xLog = false, bool yLog = false);

    QPointF toPixel(QPointF data) const noexcept;
    QPointF toData(QPointF pixel) const noexcept;

    QRectF dataBounds() const noexcept { return m_dataBounds; }
    QRectF pixelRect() const noexcept { return m_pixelRect; }

private:
    QRectF m_dataBounds;
    QRectF m_pixelRect;
    bool m_xLog, m_yLog;
};
```

QML/Widget 양쪽이 동일 변환 사용 → 패리티 보장 (AI.md §3.1).

---

## 🚀 Phase 0 (MVP) 구현 단계

> AI.md §2에 따라 각 스텝은 별도 GitHub 이슈 + 브랜치 + PR.

| Step | 이슈 | 작업 | 완료 기준 |
|---|---|---|---|
| **0.1** | #1 | `src/core/` CMakeLists.txt + 글로벌 헤더 | 빈 코어 라이브러리 빌드 성공 |
| **0.2** | #2 | `QAbstractSeriesModel` + `QRingBufferSeriesModel` | 단위 테스트 통과 (TestRingBuffer) |
| **0.3** | #3 | `QCoordinateTransform` + `QScaleEngine` | 단위 테스트 통과 (TestCoordinate, TestScaleEngine) |
| **0.4** | #4 | `QAbstractSeries` 순수 인터페이스 | 코어 라이브러리 완성, 헤더만 사용 가능 |
| **0.5** | #5 | `QmlChartView` (QQuickItem) + `QmlLineSeries` (QSGGeometryNode) | QML 데모에 라인 표시 |
| **0.6** | #6 | `QmlAxis`, `QmlLegend` (간단 버전) | 축/그리드 표시 |
| **0.7** | #7 | RingBuffer + 60fps 타이머 데모 | QML에서 60fps 데이터 흐름 |
| **0.8** | #8 | `WidgetChartView` 스텁 (QPaintEvent 빈 구현) | Widget 데모 빌드/실행 (빈 화면) |
| **0.9** | #9 | QML UI 폴리싱 (컬러, 두께, 테마 기초) | 시각적 완성도 |
| **0.10** | #10 | CI 3플랫폼 녹색 확인 + v0.1.0 태그 | ✅ MVP 완료 |

---

## 🔬 실시간 60fps 설계 핵심

### 스레드 모델 (AI.md §1.3 Qt 6 기준)

```
GUI Thread                    Render Thread (QSG)
─────────────                 ─────────────────────
RingBuffer.appendBatch()      
  ↓ emit pointsInserted()
QmlLineSeries.onModelChange
  ↓ QQuickItem::update()
                              QmlLineSeries.updatePaintNode()
                                ↓ geometry 서브세트만 갱신
                                  QSGGeometryNode::markDirty(
                                      QSGNode::DirtyGeometry)
```

**핵심**:
- `updatePaintNode()`는 Render Thread에서 호출되지만 **GUI 스레드는 블록된 상태** (Qt Quick 공식 보장)
- 따라서 모델 데이터 접근 시 **별도 락 불필요** (단, 모델 자체가 별도 스레드에서 밀어넣는 경우만 mutex 사용)
- 매 프레임 전체 재빌드 없이 **버퍼 서브세트만 갱신** → 60fps 달성

### 성능 목표

| 시나리오 | 목표 |
|---|---|
| 10K 포인트 정적 | 60fps 여유 |
| 100K 포인트 정적 | 60fps (LOD 미적용 시) |
| 60K 포인트 60fps 스트리밍 (1K/프레임) | 60fps 유지 |
| Phase 2: 1M 포인트 | LOD/서브샘플링으로 60fps |

---

## ⚖️ 렌더링 백엔드 결정 (Phase 1)

Widget 프론트엔드 렌더링 백엔드는 **Phase 1에서 결정**. 후보:

| 백엔드 | 성능 | Mac | 복잡도 | 비고 |
|---|---|---|---|---|
| QPainter | 낮~중 | ✅ | 낮 | Phase 1 시작점 후보 |
| QOpenGLWidget | 높 | ⚠️ dep | 중 | Mac 경고 |
| QWidget + QRhi (Qt 6.6+) | 높 | ✅ Metal | 높 | 실험적 |

→ Phase 0에서 QML 구현하면서 얻은 경험으로 결정.

---

## 🛠️ 기술 스택 (AI.md와 일치)

| 항목 | 선택 | 비고 |
|---|---|---|
| Qt 버전 | Qt 6.7+ | AI.md §1.3 |
| 빌드 시스템 | CMake (`qt_add_qml_module`) | 루트 CMakeLists.txt 완성됨 |
| C++ 표준 | C++17 | CMakeLists.txt 설정됨 |
| QML 렌더링 | QSGGeometryNode + QSGRenderNode | QRhi 기반 |
| 데이터 모델 | 시그널 기반 커스텀 API | 차트 특화 |
| CI | GitHub Actions Win/Mac/Linux × Qt 6.7.3 | ✅ 구축 완료 |
| 정적 분석 | cppcheck + clang-format | ✅ 설정 완료 |
| 커버리지 | lcov → Codecov | ✅ CI에 포함 |
| 라이선스 | Apache-2.0 | ✅ LICENSE 추가 |
| 버전 관리 | SemVer 0.1.0 (CMakeLists.txt 단일) | ✅ 적용 |

---

## ✅ MVP 완료 기준 (Definition of Done)

- [ ] **Phase 0.10 CI 녹색**: Win/Mac/Linux 3플랫폼 빌드 + 테스트 통과
- [ ] **QML 데모**: 라인 차트 + 실시간 60fps 데이터 흐름
- [ ] **Widget 데모**: 스텁이지만 빌드/실행 가능 (빈 화면)
- [ ] **확장 가능**: 새 시리즈 타입은 `QAbstractSeries` 구현체 1개 + 프론트엔드 렌더러만 추가
- [ ] **단위 테스트**: RingBuffer, CoordinateTransform, ScaleEngine 통과
- [ ] **AI.md 규칙 100% 준수**: 모든 PR이 워크플로우/엔지니어링 규칙 준수
- [ ] **v0.1.0 태그**: VERSIONING.md 절차에 따라 릴리스

---

## 📦 Phase 0 산출물 (예정)

**소스 (라이브러리):**
- `src/core/CMakeLists.txt`
- `src/core/qgraphplot_global.h`
- `src/core/model/{QAbstractSeriesModel, QRingBufferSeriesModel}.{h,cpp}`
- `src/core/transform/{QCoordinateTransform, QScaleEngine}.{h,cpp}`
- `src/core/series/qabstractseries.{h,cpp}`
- `src/qml_frontend/{CMakeLists.txt, qml/, qmlchartview, qmllineseries, qmlaxis, qmllegend}.{h,cpp}`
- `src/widget_frontend/{CMakeLists.txt, widgetchartview}.{h,cpp}`

**예제:**
- `examples/qml_demo/{CMakeLists.txt, main.cpp, main.qml}`
- `examples/widget_demo/{CMakeLists.txt, main.cpp}`

**테스트:**
- `tests/{CMakeLists.txt, TestRingBuffer.cpp, TestCoordinate.cpp, TestScaleEngine.cpp}`

예상 총 파일 수: **약 30개** (Phase 0 완료 시).

---

## 🗺️ Phase 로드맵 (전체)

| Phase | 내용 | 예상 기간 |
|---|---|---|
| **Phase 0 (MVP)** | 코어 인터페이스 + QML 라인 + Widget 스텁 | 4~6개월 |
| **Phase 1** | Widget 렌더링 백엔드 구현, 양쪽 패리티 완성 | 2~3개월 |
| **Phase 2** | 추가 차트 타입 (Scatter, Bar, Area) + 줌/팬 + LOD | 3~4개월 |
| **Phase 3** | 테마 시스템, 내보내기(PNG/SVG/PDF), Qt 5 백포트 검토 | 2~3개월 |
| **v1.0.0** | API 안정화, 문서 완성 | 별도 합의 |

---

## 📌 라이선스 / 외부 의존성 전략

### Apache-2.0 채택 이유 (AI.md §1.1)
- 상용 클로즈드 소스에 자유롭게 사용 가능 (LGPL 대비 제약 없음)
- Qt Quick(QML) 사용 → Qt 자체는 LGPL v3이지만, **동적 링크 시 클로즈드 앱에 사용 가능**
- 본 라이브러리 자체는 Apache-2.0로 **사용자에게 최대 자유** 보장

### 주의사항
- Qt 소스코드를 복붙하지 않는다 (AI.md §1.1)
- 외부 코드 인용 시 라이선스 호환성 먼저 확인
- `QGRAPHPLOT_EXPORT` 매크로로 DLL 경계 명확화

---

## 📚 관련 기술 조사 결과 (부록)

### 렌더링 엔진 선택 배경 (Qt Scene Graph + QSGRenderNode)
- **QRhi**: Qt 6의 로우레벨 GPU 추상화 (Vulkan/Metal/D3D 자동 선택). 성능 최고 but 텍스트/클리핑 직접 구현 필요
- **QSGRenderNode**: Scene Graph 내에서 직접 QRhi 명령 호출 가능 + 텍스트/이벤트는 Qt가 처리 → 1인 개발에 최적
- **bgfx** (BSD-2-Clause): 가장 자유로운 라이선스, 12개 백엔드. 단, Qt 통합 비용 큼
- **Skia**: 2D 품질 최고 but ~50MB 의존성 + Qt Quick과 GL 컨텍스트 충돌
- **NanoVG**: 가벼움, 빠름 but 텍스트 렌더링 부족

→ QSGGeometryNode + QSGRenderNode 조합이 1인 개발에 최적의 트레이드오프.

### Widget+QML 동시 지원 관련 검토 (이전 플랜에서 철회한 부분)
- `createWindowContainer` 방식: Airspace 문제(Z-order, 드롭다운 가림), 이벤트 루프 충돌, DPI 불일치
- `QQuickRenderControl`+FBO: 60fps 성능 저하, 구현 복잡도 폭발
- → 본 플랜은 **독립 Widget 프론트엔드 자체 구현**으로 회피 (공유 코어 + 분리 렌더링)

---

_Last updated: 2026-07-22_
_관련 문서: [AI.md](AI.md), [VERSIONING.md](VERSIONING.md), [README.md](README.md)_
