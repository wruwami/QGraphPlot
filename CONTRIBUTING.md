# Contributing to QGraphPlot

QGraphPlot에 기여해 주셔서 감사합니다! 이 문서는 기여 절차를 안내합니다.

> **모든 기여자(AI 어시스턴트 포함)는 [`AI.md`](AI.md)의 규칙을 준수해야 합니다.**
> 이 문서는 사람 기여자를 위한 친절한 안내이며, 권위 있는 규칙은 `AI.md`에 있습니다.

---

## 🚀 빠른 시작

### 요구 사항

| 항목 | 버전 |
|---|---|
| C++ 표준 | C++17 이상 |
| Qt | 6.7+ (현재 CI: 6.7.3) |
| CMake | 3.16+ |
| 컴파일러 | MSVC 2022 / Clang 15+ / GCC 11+ |

### 로컬 빌드

```bash
git clone https://github.com/wruwami/QGraphPlot.git
cd QGraphPlot
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.7.3
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### 코드 포맷 적용

커밋 전 반드시 실행:

```bash
find src tests examples -type f \( -name "*.cpp" -o -name "*.h" \) \
    -exec clang-format -i {} +
```

---

## 📋 기여 워크플로우

### 1. 이슈부터

- 모든 작업은 **GitHub 이슈**로 시작합니다 ([`AI.md` §2.3](AI.md)).
- 작업하고 싶은 이슈에 댓글로 할당을 요청하거나, 새 이슈를 엽니다.
- 이슈 없는 작업은 받아들여지지 않습니다.

### 2. 브랜치 만들기

- 이슈 1개당 브랜치 1개 ([`AI.md` §2.1](AI.md)).
- 네이밍 규칙:

| 타입 | 브랜치명 |
|---|---|
| 기능 | `feature/<issue-number>-<short-desc>` |
| 버그 수정 | `fix/<issue-number>-<short-desc>` |
| 문서 | `docs/<short-desc>` |
| 인프라/잡일 | `chore/<short-desc>` |

```bash
git checkout -b feature/42-add-scatter-series
```

### 3. 커밋 & 푸시

- **Conventional Commits** 형식 권장 ([`VERSIONING.md` §7](VERSIONING.md)):

```
feat(core): add ring buffer append benchmark

Detailed description if needed.

Refs #42
```

| 접두사 | 용도 |
|---|---|
| `feat:` | 새 기능 |
| `fix:` | 버그 수정 |
| `docs:` | 문서 |
| `refactor:` | 리팩터링 (동작 변경 없음) |
| `test:` | 테스트 추가/수정 |
| `chore:` | 빌드/CI/인프라 |
| `ci:` | CI 설정 |
| `BREAKING CHANGE:` | 비호환 변경 (pre-1.0에서는 MINOR로 반영) |

### 4. Pull Request

- `main` 브랜치로 직접 push 금지 ([`AI.md` §2.1](AI.md)).
- PR 템플릿의 체크리스트를 모두 채우세요.
- CI가 녹색이어야 merge 가능 ([`AI.md` §5](AI.md)).
- **리포 소유자만 merge 가능** ([`AI.md` §2.2](AI.md)).

---

## 🏗️ 아키텍처 개요

자세한 내용은 [`QGraphPlot_MVP_Plan.md`](QGraphPlot_MVP_Plan.md) 참조.

```
┌─ Shared C++ Core (src/core/) ────────────┐
│  Model / Transform / Series Interface    │
├──────────────────────────────────────────┤
│  QML Frontend        │  Widget Frontend  │
│  (src/qml_frontend/) │  (src/widget_/)   │
└──────────────────────┴───────────────────┘
```

**핵심 원칙**: 같은 C++ 코어를 QML과 Widget 양쪽이 공유. 뷰 동작은 양쪽이 동일해야 함 ([`AI.md` §3.1](AI.md)).

---

## ✅ 코드 리뷰 체크리스트

PR 오픈 전 자가 점검:

- [ ] 이슈 번호 참조 (`Refs #N` 또는 `Closes #N`)
- [ ] 단일 관심사 ([`AI.md` §3.5](AI.md))
- [ ] 뷰 변경 시 양쪽 프론트엔드 모두 수정 + 패리티 명시 ([`AI.md` §3.1](AI.md))
- [ ] 버그 수정 시 회귀 테스트 동반 ([`AI.md` §3.6](AI.md))
- [ ] `clang-format` 적용
- [ ] `cppcheck` 경고 없음
- [ ] Effective C++ 원칙 준수 ([`AI.md` §1.4](AI.md)): `const` correctness, `explicit`, `enum class`, RAII
- [ ] 파일명 = 클래스명 (CamelCase, [`AI.md` §1.5](AI.md))
- [ ] Apache-2.0 헤더 포함 (새 파일)
- [ ] Qt 소스 복붙 없음 ([`AI.md` §1.1](AI.md))
- [ ] CI 3플랫폼 녹색

---

## 🧪 테스트 가이드라인

- 모든 새 기능은 단위 테스트 동반.
- 테스트 파일명: `Test<Target>.cpp` (예: `TestRingBuffer.cpp`).
- 프레임워크: QtTest.
- 커버리지: Ubuntu CI에서 lcov → Codecov 수집.
- 회귀 테스트는 버그 수정과 같은 PR에 ([`AI.md` §3.6](AI.md)).

---

## 📦 릴리스 절차

자세한 내용은 [`VERSIONING.md`](VERSIONING.md).

- 버전은 `CMakeLists.txt` 한 곳에만 명시 ([`AI.md` §4.2](AI.md)).
- SemVer 준수, 현재 `0.1.0` (pre-1.0).
- 릴리스는 소유자가 진행.

---

## 💬 질문 / 토론

- 버그 / 기능 제안: [GitHub Issues](https://github.com/wruwami/QGraphPlot/issues)
- 설계 토론: 이슈를 열고 `discussion` 라벨 요청
- 보안 이슈: 공개 이슈로 올리지 말고 [security policy](SECURITY.md) 참조

---

## 📄 라이선스

기여물은 모두 [Apache License 2.0](LICENSE) 하에 배포됩니다. 기여 시 이에 동의하는 것으로 간주합니다.

CLA(기여자 라이선스 양도)는 요구하지 않습니다.

---

_Last updated: 2026-07-23_
