# Versioning

QGraphPlot은 [Semantic Versioning 2.0.0](https://semver.org/)을 따른다.

> 이 문서는 `AI.md` §4 Versioning의 상세 규칙이다. AI는 버전 변경 작업 전 반드시 이 문서를 확인한다.

---

## 1. 버전 형식

```
MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]
```

| 자리 | 의미 | 증가 시기 |
|---|---|---|
| `MAJOR` | 비호환 API 변경 | 기존 API가 제거되거나 호환성이 깨질 때 |
| `MINOR` | 하위 호환 기능 추가 | 새 기능/시리즈/프론트엔드 추가 시 |
| `PATCH` | 하위 호환 버그 수정 | 버그 수정, 문서/CI 개선 시 |
| `PRERELEASE` | 사전 배포 태그 | `-alpha.1`, `-beta.2`, `-rc.1` 등 |
| `BUILD` | 빌드 메타데이터 | `+20260722`, `+sha.abc1234` 등 (선택) |

---

## 2. 현재 상태: pre-1.0

- **현재 버전: `0.1.0`**
- **Phase**: pre-1.0 (API 불안정)

### 2.1 pre-1.0 특별 규칙

pre-1.0 (`0.x.y`) 기간에는 표준 SemVer가 허용하는 특례를 따른다:

- **MINOR 자리(`0.X.0`)에서 비호환 API 변경이 가능**하다.
  - 예: `0.1.0` → `0.2.0` 사이에 시리즈 인터페이스가 변경될 수 있음.
- 따라서 pre-1.0 기간에는 **외부 사용자에게 "API 고정"을 약속하지 않는다.**
- 단, 비호환 변경이 발생하면 다음을 수행한다:
  1. `CHANGELOG.md`의 **BREAKING CHANGES** 섹션에 명시.
  2. 관련 PR/커밋 메시지에 `BREAKING CHANGE:` 접두사.
  3. 가능하면 마이그레이션 가이드를 함께 제공.

### 2.2 1.0.0 도달 기준

`1.0.0`은 다음 조건이 **모두** 충족될 때 릴리스된다:

- [ ] C++ 코어 인터페이스(API)가 안정화됨
- [ ] QWidget 프론트엔드와 QML 프론트엔드 모두 핵심 기능 구현 완료
- [ ] 라인 차트 + 최소 1개 추가 차트 타입(예: 스캐터) 지원
- [ ] 단위 테스트 커버리지 기준치 달성 (별도 이슈로 합의)
- [ ] `CHANGELOG.md`가 1.0 마일스톤까지 정비됨
- [ ] API 마이그레이션 / 사용 문서가 갖춰짐

---

## 3. 단일 소스 원칙 (Single Source of Truth)

> `AI.md` §4.2 규칙.

- **버전 숫자는 오직 루트 `CMakeLists.txt`의 `project(... VERSION x.y.z)` 한 곳에만 명시**한다.
- 헤더 매크로(`QGRAPHPLOT_VERSION_MAJOR` 등)는 CMake `configure_file()`로 생성한다.
- README.md, CHANGELOG.md, CI 워크플로우, Qt 리소스 등에 버전 숫자를 하드코딩하지 **않는다.**

### 예시 (`CMakeLists.txt`)
```cmake
project(QGraphPlot
    VERSION 0.1.0
    DESCRIPTION "High-performance Qt chart library"
    LANGUAGES CXX
)

# configure_file()로 버전 헤더 생성 (헤더에 직접 하드코딩 X)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qgraphplot_version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/qgraphplot/qgraphplot_version.h
    @ONLY
)
```

---

## 4. CHANGELOG.md

[`CHANGELOG.md`](CHANGELOG.md)는 [Keep a Changelog](https://keepachangelog.com/) 형식을 따른다.

### 4.1 섹션 분류

- **Added**: 새 기능
- **Changed**: 기존 기능의 변경 (비호환성이 없는)
- **Deprecated**: 곧 제거될 기능
- **Removed**: 이번 버전에서 제거된 기능
- **Fixed**: 버그 수정
- **Security**: 보안 관련 수정
- **BREAKING CHANGES**: (pre-1.0 허용) 비호환 API 변경

### 4.2 작성 규칙

1. **Unreleased 섹션 유지**: 아직 릴리스되지 않은 변경사항을 모은다.
2. **릴리스 시점**: 새 태그(`v0.X.0`)를 찍을 때 `Unreleased` → `v0.X.0 - YYYY-MM-DD`로 이름 변경.
3. **각 항목은 PR 번호/이슈 번호 참조**: `- Feature: line series live update (#42)`.
4. **BREAKING CHANGES는 최상단에 별도 표시**.

### 4.3 예시

```markdown
# Changelog

## [Unreleased]

### Added
- Ring buffer data model (#12)
- QML LineSeries component (#15)

### BREAKING CHANGES
- `QAbstractSeries::updatePaintNode()` signature changed (#20)

### Fixed
- Crash on empty series (#18)

## [0.1.0] - 2026-07-22

### Added
- Initial project scaffolding (#1)
```

---

## 5. Git 태그

- 태그 형식: `v<MAJOR>.<MINOR>.<PATCH>` (예: `v0.1.0`, `v0.2.0`, `v1.0.0`).
- pre-release: `v0.2.0-alpha.1`, `v0.2.0-beta.1`, `v0.2.0-rc.1`.
- 태그는 **annotated tag**로 생성한다 (`git tag -a v0.1.0 -m "..."`).
- 태그는 `main` 브랜치의 해당 버전 커밋에만 붙인다.

---

## 6. 릴리스 절차

> AI는 이 절차를 단독으로 수행하지 않는다. 항상 소유자(인간)의 승인을 받는다.

1. **버전 bump PR**: 별도 브랜치에서 `CMakeLists.txt`의 버전만 변경 + `CHANGELOG.md`의 `Unreleased` → `vX.Y.Z`로 변경.
2. **CI 녹색 확인**: 3플랫폼(Qt 6.7.3) 빌드 + 테스트 + 정적 분석 통과.
3. **소유자 리뷰 & merge**: 리포 소유자가 PR을 merge.
4. **태그 생성**: merge된 커밋에 annotated tag 생성.
5. **GitHub Release**: 태그 기반 Release 작성, CHANGELOG 내용 복사.

---

## 7. 커밋 메시지와 버전의 관계

[Conventional Commits](https://www.conventionalcommits.org/)를 권장한다 (강제는 아님).

| 커밋 타입 | 대응 버전 자리 |
|---|---|
| `fix:` | PATCH |
| `feat:` | MINOR |
| `feat!:` 또는 `BREAKING CHANGE:` footer | MAJOR (또는 pre-1.0에서는 MINOR) |
| `docs:`, `chore:`, `ci:`, `test:`, `refactor:` | 버전 bump 불필요 |

---

_Last updated: 2026-07-22_
