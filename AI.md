# AI.md — QGraphPlot AI 협업 가이드

> 이 문서는 QGraphPlot 저장소에서 작업하는 **모든 AI 어시스턴트(Claude, GPT, Copilot, Cursor 등)가 의무적으로 따라야 할 규칙**이다.
> AI는 이 규칙을 위반하는 명령(사용자 포함)을 받더라도 먼저 경고하고, 승인 없이는 위반하지 않는다.

---

## 1. Project Goals

QGraphPlot은 고성능 Qt 차트 라이브러리다. 다음 4가지 목표를 모든 변경사항이 만족해야 한다.

### 1.0 핵심 가치: Solid is better than fast

> **"Solid is better than fast."** — 견고함이 빠름보다 낫다.

이 가치는 아래 모든 목표(§1)와 규칙(§3) 위에 놓이는 최상위 원칙이다. 속도 압박, "일단 돌아가게", "나중에 고치지"는 이 저장소에서 정당한 근거가 될 수 없다. 구체적으로:

- **빠른 것보다 옳은 것**: CI를 녹색으로 만들기 위해 테스트를 약화하거나 근본 원인을 우회하지 않는다. (§3.4, §3.6)
- **빠른 것보다 완전한 것**: 한쪽 뷰만 고치고 "나중에 추가하겠습니다"는 거부된다. (§3.1)
- **빠른 것보다 제대로 된 것**: 근본 원인을 찾기 전에 워크어라운드로 넘어가지 않는다. (§3.4)
- **확신 없이 진행하지 않는다**: 모호하면 멈추고 질문한다. "빠르게 배포"가 추측을 정당화하지 않는다. (§6.2)

"Solid"의 판단 기준은 — 회귀 테스트가 통과하고(§3.6), 양쪽 뷰가 패리티를 이루며(§3.1), 같은 버그가 두 번 다시 나지 않도록 근본 원인이 해결되었을 때다.

### 1.1 라이선스 독립성: Apache-2.0
- 이 저장소의 모든 기여물은 **Apache License 2.0** 하에 배포된다.
- AI는 GPL/LGPL/AGPL 코드(Qt 소스코드 복사본 포함)를 이 저장소에 직접 복사하지 않는다.
- 외부 코드 인용 시 라이선스 호환성을 먼저 확인하고, 불확실하면 PR 설명에 명시한다.
- Qt API 호출은 허용되나, Qt 헤더/소스를 그대로 복붙하는 것은 금지한다.

### 1.2 QWidget / QML 뷰 동일 동작
- QGraphPlot은 **QWidget 프론트엔드**와 **QML 프론트엔드**를 모두 지원한다.
- 두 뷰는 동일한 C++ 코어(QGraphPlotCore)를 공유하며, **사용자 가시적 동작(렌더링 결과, 상호작용, 시리즈 속성)이 동일**해야 한다.
- 한쪽 뷰에만 영향을 주는 변경은 Engineering Discipline §3.1에 따라 양쪽 모두에 반영한다.

### 1.3 최신 Qt6 빌드 지원
- 타겟은 **Qt 6.7+** (현재 CI: Qt 6.7.3).
- 더 이상 사용되지 않는 API(`QT_DEPRECATED` 표시)는 사용하지 않는다.
- Qt 5 백포트는 Phase 2까지 금지. `#ifdef QT_MAJOR_VERSION` 분기 코드를 임의로 추가하지 않는다.

### 1.4 Effective C++ 원칙 준수
모든 C++ 코드는 다음 Scott Meyers / modern C++ 원칙을 따른다.

| 원칙 | 적용 |
|---|---|
| **const correctness** | 모든 멤버 함수에서 가능한 `const`를 붙인다. 멤버 변경 여부를 명확히 한다. |
| **`const T&` 전달** | 값 전달이 불필요한 参数는 `const T&`로 전달한다 (QString, QPointF, std::vector 등). |
| **`explicit` 생성자** | 단일 인자 생성자는 원칙적으로 `explicit`을 붙인다 (암시적 변환 방지). |
| **`enum class`** | C 스타일 `enum` 대신 scoped `enum class`를 사용한다. |
| **멤버 초기화 리스트** | 멤버는 생성자 초기화 리스트에서 초기화한다 (본문 대입 금지). |
| **Qt 부모-자식 소유권 = RAII** | `QObject` 파생 클래스는 parent를 전달해 Qt가 소멸을 관리하도록 한다. `new` 후 parent 없이 방치하지 않는다. |
| **필요할 때만 `virtual` 소멸자** | 다형적 기본 클래스(상속 예정)에만 `virtual ~`를 붙인다. Final 클래스나 다형성 없는 클래스는 비가상 소멸자를 쓴다. |

### 1.5 파일 및 식별자 명명 규칙

모든 식별자는 **클래스명과 파일명이 1:1로 일치**하는 CamelCase 규칙을 따른다. Qt 코어(qtbase)가 소문자+언더스코어(`qpoint.h`)를 쓰지만, QGraphPlot은 클래스 중심 API에서 IDE 탐색/자동완성 가독성이 더 중요하다고 판단해 CamelCase를 채택했다 (QCustomPlot/JUCE 스타일).

| 대상 | 규칙 | 예시 |
|---|---|---|
| **헤더/소스 파일명** | `ClassName.h` / `ClassName.cpp` (클래스명과 1:1) | `QAbstractSeriesModel.h`, `QRingBufferSeriesModel.cpp` |
| **클래스명** | `Q` prefix + PascalCase | `QAbstractSeriesModel`, `QRingBufferSeriesModel` |
| **인터페이스/구현 쌍** | 동일 파일명 (`QFoo.h` ↔ `QFoo.cpp`) | `QFoo.cpp`는 `QFoo.h`만 include |
| **테스트 파일명** | `Test<Target>.cpp` (QtTest 클래스명과 일치) | `TestRingBuffer.cpp` (class `TestRingBuffer`) |
| **디렉토리명** | 소문자 + 언더스코어 (관습 유지) | `model/`, `transform/`, `qml_frontend/` |
| **네임스페이스** | 소문자 | `qgraphplot`, `qgraphplot::qml` |
| **일반 함수/변수** | camelCase | `pointCount()`, `m_capacity` |
| **매크로/상수** | `QGRAPHPLOT_` prefix + UPPER_SNAKE | `QGRAPHPLOT_EXPORT`, `QGRAPHPLOT_API_VERSION` |
| **enum class** | PascalCase 타입, PascalCase 값 | `enum class ThreadSafety { Disabled, Enabled }` |

**금지**:
- Qt 코어 스타일(`qabstractseriesmodel.h`)의 파일명 사용 금지
- 1개 파일에 2개 이상의 공개 클래스 선언 금지 (작은 헬퍼/struct는 예외)
- 파일명과 클래스명이 다른 경우 금지 (예: `Series.h` 안에 `class LineSeries`)

**예외**:
- CMake/스크립트 파일은 관습적 이름 유지 (`CMakeLists.txt`, `.clang-format`, `cppcheck.options`)
- 자동 생성 파일 (`moc_*.cpp`, `ui_*.h`, `qrc_*.cpp`)은 Qt 도구가 만드는 이름 그대로
- **클래스가 없는 글로벌/매크로 헤더**는 소문자+언더스코어 유지 (Qt `qglobal.h` 관행)
  - `qgraphplot_global.h` (X) → 유지 ✅
  - `qgraphplot_version.h.in` (X) → 유지 ✅
  - 적용 기준: 클래스 선언이 없고 매크로/namespace/include guard만 있는 헤더

#### 1.5.1 파일명 변경(rename) 시 모든 참조 동기화

`git mv`로 파일명을 바꿀 때, **같은 커밋에서 반드시 모든 참조를 함께 갱신**한다. 하나라도 누락되면 빌드가 끊기거나 CI가 붉게 실패한다.

갱신해야 할 위치 (체크리스트):

| 위치 | 예시 |
|---|---|
| `#include` 지시어 | `#include "qfoo.h"` → `#include "QFoo.h"` (모든 소스/헤더/테스트) |
| `CMakeLists.txt`의 소스/헤더 목록 | `qt_add_library(... QFoo.cpp)` |
| Doxygen `@file` 태그 | `//! @file qfoo.h` → `//! @file QFoo.h` |
| QtTest 하단 `.moc` include | `#include "tst_foo.moc"` → `#include "TestFoo.moc"` |
| 문서(README, PLAN)의 파일 트리 | 경로 표기 일관성 |
| QML 모듈 `qmldir` | QML 타입 등록 경로 (해당 시) |

**검증**: 커밋 전에 반드시 아래 명령으로 잔존 참조를 확인한다:
```bash
grep -rn "<old-name>" --include="*.h" --include="*.cpp" --include="*.txt" src/ tests/ examples/
```
아무것도 출력되지 않아야 커밋 가능. AI.md §3.4 (무력화된 워크어라운드 제거) 원칙과 동일한 맥락 — 한 곳에서 고치고 다른 곳은 잊지 않는다.

---

## 2. Workflow

### 2.1 브랜칭 규칙
- 모든 변경은 **GitHub 이슈 1개당 브랜치 1개**에서 진행한다.
- **AI 브랜치 네이밍 형식**: `<ai-name>/issue-<number>-<short-work-description>`
  - 예: `antigravity/issue-12-ci-workflow-and-badges`, `claude/issue-7-cmake-build-fix`
  - 설명 부분에는 공백이나 슬래시 없이 **kebab-case**를 사용한다.
- **`main` 브랜치에 직접 commit/push하지 않는다.** 항상 PR을 연다.

### 2.2 작업 시작 전 점검 사항
1. `git fetch origin`을 실행하고 `main` 브랜치의 최신 변경 사항을 로컬에 풀(`git checkout main && git pull`)하여 베이스라인을 최신화한다.
2. 리모트 PR 상태를 확인하여(`gh pr list --state all --limit 10` 등) 해결하려는 이슈가 이미 다른 에이전트에 의해 해결 중이거나 머지되었는지 확인한다.
3. 원격 저장소에 동일한 이슈의 브랜치가 존재하는지 검사한다:
   ```bash
   git branch -r | grep "issue-<number>"
   ```
4. 이미 해당 이슈 브랜치가 존재한다면 새로 만들지 않고, **기존 브랜치를 체크아웃하여 커밋을 확인하고 이어서 작업**한다.

### 2.3 푸시 전 점검 사항
- 푸시하기 전에 다시 한번 `git fetch`를 실행하여 원격의 내 브랜치가 다른 에이전트에 의해 갱신되었는지 확인한다.
- 푸시가 거부(reject)되면 다른 에이전트의 커밋이 추가된 것이므로, 로컬로 머지하고 재검증한 후 푸시한다.
- **다른 에이전트의 커밋 위로 절대 강제 푸시(`force push`)하지 않는다.**

### 2.4 리포 소유자가 직접 PR 리뷰/merge
- **AI는 자기가 연 PR을 스스로 merge하지 않는다.**
- PR 오픈 후 리포 소유자(인간)의 리뷰와 merge를 기다린다.
- 리뷰 피드백이 오면 같은 브랜치에서 commit 추가 후 push한다 (새 PR 금지).

### 2.5 작업은 GitHub 이슈로 추적, 커밋/PR에 참조
- 모든 작업은 GitHub 이슈로 시작한다. 이슈 없는 작업은 하지 않는다.
- **커밋 메시지**: 제목 또는 본문에 `Refs #<issue-number>`를 포함한다.
- **PR 제목/본문**: `Closes #<issue-number>` 또는 `Refs #<issue-number>`를 반드시 포함한다.
- 예:
  ```
  feat(core): add ring buffer append benchmark

  Refs #42
  ```

---

## 3. Engineering Discipline

> 아래 6개 규칙은 2026-07-21 hostile review에서 도출된 것이다. **모든 PR이 이 규칙을 위반하면 block된다.**

### 3.1 뷰 변경 시 양쪽 뷰 패리티 확인
- 뷰 렌더링 또는 상호작용(렌더링, 색상, 줌/팬, 클릭 핸들링 등)을 변경하는 PR은:
  1. **같은 PR에서 QWidget 뷰와 QML 뷰 양쪽 모두를 수정**한다.
  2. PR 설명에 **패리티 확인 방법**을 명시한다 (예: "두 데모에서 동일하게 60fps로 라인이 흐름 확인", 스크린샷 또는 자동화된 비교 결과).
- 한쪽만 수정하고 "나중에 추가하겠습니다"는 거부된다.

### 3.2 enum 기반 로직 재복제 금지
- 이미 `enum class`로 정의된 값(enum, 모드, 타입 등)을 **여러 파일에 매직 넘버/문자열 리터럴로 재복제하지 않는다.**
- 새 값이 필요하면 기존 enum 정의에 추가하고, 모든 소비자는 enum을 참조한다.
- 예: `if (series.type() == 1)` (X) → `if (series.type() == SeriesType::Line)` (O)

### 3.3 파서는 유효성까지 검증
- 설정 파일, 데이터 포맷, 명령줄 인자 등을 파싱할 때:
  - **존재 여부뿐 아니라 유효성까지 검증**한다 (타입, 범위, 의존성 등).
  - 무효한 입력은 침묵 무시하지 않고 명시적 에러를 발생시킨다 (`qWarning`, 예외, `std::optional` 등).
- 관대한 파싱(fallback)이 필요하면 PR 설명에서 근거를 댄다.

### 3.4 무력화된 워크어라운드는 즉시 삭제
- 과거의 임시 해결책, `// TODO`, `// HACK`, `// FIXME`로 표시된 코드 중:
  - 근본 원인이 해결되어 더 이상 필요 없다면 **같은 PR에서 즉시 삭제**한다.
- 같은 문제를 두 위치에서 중복 수정하지 않는다. 한쪽만 고치고 다른 쪽은 삭제한다.

### 3.5 브랜치는 관심사 하나로 유지
- 브랜치는 **하나의 관심사(이슈)**만 다룬다. 한 브랜치에서 여러 이슈를 섞지 않는다.
- **이미 merge된 브랜치를 재사용하지 않는다.** 새 작업은 새 브랜치를 만든다.
- 브랜치에 무관한 변경이 들어가면 별도 PR로 분리한다.

#### 3.5.1 큰 변경은 여러 PR로 분리

큰 변경(리팩터링, 아키텍처 전환, 라이선스 정책 변경, 코딩 컨벤션 변경 등)은 **기능/문서/리팩터 단위로 쪼개어 별도 PR로 제출**한다. 하나의 거대한 PR은 리뷰 부담을 폭발시키고, 부분 revert를 불가능하게 만든다.

**권장 분리 기준**:

| 변경 유형 | 같은 PR? | 비고 |
|---|---|---|
| 기능 구현 + 그에 대한 단위 테스트 | ✅ 같이 | §3.6 회귀 테스트 규칙 |
| 기능 구현 + 연관된 rename | ✅ 같이 | rename만 따로 떼면 빌드가 끊김 |
| 기능 구현 + 코딩 스타일(포맷) 변경 | ⚠️ 분리 권장 | 스타일 변경은 별도 chore PR |
| rename + 그 rename을 정책화하는 문서 | ❌ 분리 | rename은 본 PR, 규칙 문서는 별도 docs PR |
| 기능 + 관련 없는 버그 수정 | ❌ 분리 | §3.5 본 규칙 |
| 기능 + AI.md/VERSIONING.md 규칙 추가 | ❌ 분리 | 규칙 변경은 별도 docs PR |

**예시 (본 프로젝트 실제 사례)**:
- PR #4 (Phase 0.2 코드 + CamelCase rename) — 같이
- PR #5 (AI.md §1.5 명명 규칙 문서화) — 분리 ✅

#### 3.5.2 단일 관심사의 예외 (밀접한 변경은 같은 PR 허용)

§3.5 본 규칙이 "무관한 변경은 분리"를 요구하지만, **두 변경이 강하게 결합되어 분리 시 한쪽이 의미 없거나 빌드가 깨지는 경우**는 같은 PR을 허용한다.

**예외 허용 조건 (모두 충족 시)**:
1. 두 변경이 **같은 이슈를 해결**하거나 같은 이슈의 직접적 결과물
2. 분리할 경우 **한쪽 PR만 merge해도 동작하지 않음** (의존성)
3. PR 설명에 왜 같이 묶었는지 명시

**예외 적용 사례**:

| 사례 | 같은 PR? | 근거 |
|---|---|---|
| 새 클래스 + 그 클래스의 단위 테스트 | ✅ | 테스트 없는 클래스는 의미 없음 |
| 인터페이스 변경 + 모든 구현체 업데이트 | ✅ | 분리 시 빌드 안 됨 |
| 파일 rename + 모든 참조 갱신 | ✅ | §1.5.1에 따름 |
| 뷰 렌더링 로직 + 양쪽 프론트엔드 적용 | ✅ | §3.1 패리티 규칙 |
| 버그 수정 + 회귀 테스트 | ✅ | §3.6 |
| 새 기능 + 그 기능과 무관한 문서 오타 수정 | ❌ | 문맥 무관, 별도 PR |

**판단 기준**: "이 두 변경 중 하나만 merge하면 시스템이 정상 동작하는가?" → **아니오**면 같은 PR, **예**면 분리.

### 3.6 버그 수정은 회귀 테스트 동반
- 모든 버그 수정 PR은 **동일한 PR에 회귀 테스트를 포함**한다.
- 테스트는 버그가 수정되기 전 상태에서 실패하고, 수정 후 통과해야 한다.
- 재현이 어려운 버그는 최소한의 단위 테스트라도 추가한다.

---

## 4. Versioning

### 4.1 Semantic Versioning (SemVer)
- `MAJOR.MINOR.PATCH` 형식을 따른다.
- **현재 버전: `0.1.0` (pre-1.0)**.
  - pre-1.0 기간에는 MINOR 변경이 breaking change일 수 있다.
  - 1.0.0 도달 시점은 별도 이슈로 논의한다.

### 4.2 버전은 CMakeLists.txt 한 곳에만
- 프로젝트 버전은 **루트 `CMakeLists.txt`의 `project(... VERSION x.y.z)`** 한 곳에만 명시한다.
- 헤더 매크로, 리소스 파일, README 등에 버전을 하드코딩하지 않는다. 필요하면 CMake configure-time에 생성한다.
- 버전 변경 커밋은 별도 PR로 분리하고 `:bookmark:` 접두사를 붙인다.

### 4.3 상세 규칙은 VERSIONING.md
- 버전 bump 기준, pre-release/post-release 태그, CHANGELOG 작성법 등의 상세 규칙은 `VERSIONING.md`를 따른다.
- AI는 버전 변경 전 `VERSIONING.md`를 먼저 확인한다.

---

## 5. CI 현재 상태

`main` 브랜치에 merge 시 다음 CI가 실행된다. **모든 PR은 이 CI를 녹색으로 통과해야 merge 가능하다.**

### 5.1 build-and-test 매트릭스
| OS | Qt 버전 | 커버리지 |
|---|---|---|
| Ubuntu (latest) | Qt 6.7.3 | **lcov → Codecov 업로드** |
| Windows (latest) | Qt 6.7.3 | — |
| macOS (latest) | Qt 6.7.3 | — |

- 총 3개의 빌드 잡이 병렬 실행된다.
- 3플랫폼 모두 빌드 + 단위 테스트 통과가 필수다.

### 5.2 정적 분석
- **cppcheck**: 정적 분석 에러/경고가 없어야 한다.
- **clang-format**: 코드 스타일 검사. 포맷 위반은 CI 실패.
  - `.clang-format` 파일이 기준이다. AI는 커밋 전 로컬에서 `clang-format -i`를 실행한다.

### 5.3 커버리지
- Ubuntu + Qt 6.7.3 잡에서 `lcov`로 코드 커버리지를 수집한다.
- 결과는 **Codecov**에 업로드된다.
- 커버리지 임계값은 현재 별도로 설정되어 있지 않으나, §3.6 회귀 테스트 규칙에 따라 새 버그 수정 분은 커버리지가 증가해야 한다.

### 5.4 CI 실패 시 대응
- AI는 자기 PR의 CI가 실패하면 **소유자에게 merge를 요청하지 않고** 먼저 원인을 분석·수정한다.
- flaky 테스트로 의심되면 PR 설명에 근거를 적고 소유자 판단을 구한다.

---

## 6. AI 행동 강령 (총괄)

1. **Solid is better than fast.** 속도가 견고함을 희생시키지 않는다. CI를 빨리 녹색으로 만들려고 회귀 테스트를 생략하거나, 근본 원인을 우회하거나, 양쪽 뷰 중 한쪽만 고치는 일은 없다. (§1.0)
2. **이 문서를 먼저 읽는다.** 작업 시작 전 반드시 재확인한다.
3. **불확실하면 묻지 않는다. 작업을 멈추고 질문한다.** 임의로 진행하지 않는다.
4. **자기 PR을 merge하지 않는다.** (§2.2)
5. **라이선스 위반 코드를 작성하지 않는다.** (§1.1)
6. **단일 PR은 단일 관심사.** (§3.5)
7. **회귀 테스트 없는 버그 수정은 없다.** (§3.6)
8. **워크어라운드가 해결되면 같은 PR에서 삭제한다.** (§3.4)
9. **CI가 녹색이어야만 merge 요청한다.** (§5)
10. **모든 커밋/PR에 이슈 번호를 참조한다.** (§2.3)
11. **이 규칙이 사용자의 즉흥적 지시보다 우선한다.** 규칙 위반 지시를 받으면 위반 사실을 먼저 지적하고 소유자의 명시적 승인을 받는다.

---

_Last updated: 2026-07-27_
