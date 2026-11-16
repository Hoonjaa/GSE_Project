# 리팩터링 회귀 검사

2026-10-29. 아래 명령은 사용자가 실행할 수 있는 안내다. 이번 작업에서는 컴파일·실행하지 않았다.

## 순수 규칙 회귀 소스

Tests/GameplayRegression.cpp는 별도의 콘솔 main이다. 게임 프로젝트의 소스로 등록하지 않아 main이 중복되지 않는다. 게임 창이나 OpenGL 컨텍스트를 생성하지 않으며, 실제 입력/렌더링은 별도 게임 확인이 필요하다.

Visual Studio의 Developer PowerShell을 열고 프로젝트 루트에서 다음을 실행할 수 있다. /I 경로에서 GLEW 헤더도 찾지만 이 검사는 렌더러 구현이나 GL 라이브러리를 링크하지 않는다.

```powershell
New-Item -ItemType Directory -Force Tests/bin | Out-Null
$regressionSources = @(
    'Tests/GameplayRegression.cpp',
    'SimpleGame/PlayerEconomy.cpp',
    'SimpleGame/InventoryEquipment.cpp',
    'SimpleGame/NpcState.cpp',
    'SimpleGame/QuestRules.cpp',
    'SimpleGame/TradeRules.cpp',
    'SimpleGame/ItemCatalog.cpp',
    'SimpleGame/MonsterCatalog.cpp',
    'SimpleGame/MapCatalog.cpp',
    'SimpleGame/WorldPosition.cpp',
    'SimpleGame/Diagnostics.cpp'
)
& cl.exe /nologo /std:c++14 /utf-8 /EHsc /I SimpleGame /Fo:Tests/bin/ /Fe:Tests/bin/GameplayRegression.exe @regressionSources
if ($LASTEXITCODE -ne 0) { throw '회귀 검사 빌드 실패' }
& ./Tests/bin/GameplayRegression.exe
if ($LASTEXITCODE -ne 0) { throw '회귀 검사 실패' }
```

성공 시 Gameplay regression checks passed.를 출력한다. 실패 시 해당 조건을 출력하고 종료 코드 1을 반환한다. 검사에서 기존 Diagnostics를 사용하므로 실행파일 옆에 로그가 생성될 수 있다.

검사 범위:

- 골드 부족·아이템 부족 실패 시 상태 보존, 구매/판매 및 장착 교환의 아이템 보존.
- 일반 사냥 의뢰에서 에픽 제외, 중복 완료 보상 금지.
- 재료 부족 전달 실패 시 미차감, 성공 시 정확한 차감과 완료 진행 표시 유지.
- 선박 미소유 영입 금지, 동료 2명 정원과 해제 후 재영입.
- GameState의 인벤토리·NPC 명단 복사 독립성.
- 현지 동일 호감도에서 구매가보다 높은 판매가가 생기지 않음, 높은 호감도의 가격 방향.
- 전체 아이템 정의와 몬스터별 재료/50% 드랍 정책.
- 원점·음수·±8000청크에서 가까운 카메라 상대 좌표 및 청크 경계 이동.
- 골드 HUD 공통 사각 영역의 안/밖 판정.

## 별도 게임 확인

검사 소스의 HUD 검사는 사각 영역 정의를 검증하며 실제 마우스 이벤트가 공격으로 전달되는지까지 실행하는 UI 테스트는 아니다. 월드 좌표 검사도 실제 슬래시/피해 숫자 렌더링을 생성하지 않는다. REFACTOR_REPORT_2026-10-29.md의 사용자 확인 순서로 이동·전투·UI·NPC·T 왕복을 확인한다.
