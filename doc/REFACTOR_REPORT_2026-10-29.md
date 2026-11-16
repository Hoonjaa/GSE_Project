# 입력·좌표 수정 및 책임 분리 결과

작업일: 2026-10-29. CODE_REVIEW_2026-10-29.md에서 제안한 다섯 단계의 코드 변경을 적용했다. 빌드·게임 실행·회귀 검사 실행·렌더링 결과 확인은 사용자가 담당하므로 이번에는 수행하지 않았다.

## 1. 입력과 전투 좌표

- HudLayout/UiRect를 추가하고 골드·지도·정보·선박·하단 안내·전투 알림의 표시 영역과 클릭 소비 영역을 함께 정의했다. 기본 1280×800의 골드 패널 (100,370)도 GameUi가 소비한다.
- Prototype::MouseAttack은 UI가 소비한 클릭을 공격에 전달하지 않는다. 인벤토리의 선택/장비 클릭도 GameUi가 처리한다. 리사이즈 후 클릭 때 현재 Renderer 크기로 UI 뷰를 갱신한다.
- PlayerCombat은 시작 섬/월드 원점에 대한 전역 float 좌표를 받지 않는다. 슬래시 위치와 피해 숫자는 WorldPosition을 보관하고 Render 준비에서 카메라에 대한 근처 좌표로 변환한다.
- 피해 숫자의 활성 구역 판정도 저장한 WorldPosition을 사용한다. WorldPosition::RelativeTo의 원거리 제한 자체는 유지한다. 주변 좌표용 함수를 전역 좌표로 사용하던 경로를 제거한 것이다.
- 시작 섬 미니맵은 별도 IslandTerrainAdapter로 옮겼다. 플레이어 전투는 지형 어댑터에 의존하지 않는다.

## 2. 이전 코드와 상태 제거

- GameplayActors의 이전 EnemyActor/DropActor 및 GameplayContext를 제거했다. 현재 슬래시·피해 숫자는 CombatEffects에 보존했다.
- LevelMeshes의 사용하지 않던 적 4·예고 2·드랍 2·산호 전리품 2 모델 생성/보관을 제거했다. 현재 슬래시는 CombatMeshes가 player_slash_v2 키로 파일/메모리 캐시에 준비한다.
- 이전 Collect(int weapon), m_weapon/m_stones 및 스냅샷의 해당 필드를 제거했다. 장착 무기 능력치와 강화석 수량은 PlayerEconomy의 인벤토리를 사용한다. 공통 강화 단계·레벨·경험치·HP·기존 기록은 유지한다.
- 미사용 MapCatalog::Offers / TradePricing / PrototypeTradePricing 경로와 TutorialActive를 제거했다. 상점 견적은 TradeRules 경로로 통일했다.
- CoralLoot 전용 호환 API를 제거하고 조선소 1~4 단축키도 일반 StoreItem/UnloadItem을 호출한다. 기존 산호 아이템 ID·기준가·단축키 기능은 유지한다.

## 3. 이름과 파일 대응

| 이전 | 현재 |
|---|---|
| LevelOne.h/.cpp | PlayerCombat.h/.cpp |
| LevelOneHud.cpp | PlayerCombatHud.cpp |
| PlayerProgressState.cpp | PlayerCombatState.cpp |
| LevelOneTerrain.h/.cpp | IslandTerrainAdapter.h/.cpp; 차트 기능만 남김 |
| LevelMeshes.cpp | CombatMeshes.h/.cpp; 현재 효과만 남김 |
| GameplayActors.h/.cpp | CombatEffects.h/.cpp; 현재 사용하는 Actor만 남김 |
| PrototypePlayer.cpp / PrototypePirate.cpp | PlayerController.h/.cpp |
| PrototypeHud/Inventory/Npc/Trade/Services/PlayerHud | GameUi 계열 소스 |
| NpcRelations.cpp (PlayerEconomy 구현) | NpcState.h/.cpp (별도 상태/규칙 객체) |

Visual Studio의 프로젝트/필터에는 현재 소스·헤더를 각각 등록하고 삭제한 경로를 제거했다. 외부 라이브러리·셰이더 복사·stdafx PCH 생성 설정은 유지한다. 회귀 검사 main은 게임 프로젝트의 ClCompile에 넣지 않았다.

## 4. 중복 규칙과 생성 코드 통합

- 일반/테스트 맵은 Prototype::CreateMapSystems 한 곳에서 MarineSystem·MonsterSystem·NpcSystem을 생성하고 콜백을 연결한다. 해적 피격 상세 로그도 같은 경로를 사용한다.
- QuestRules가 사냥꾼/수집가/상인의 목표·설명·호감도/신뢰/존경 보상을 제공한다. 완료 판정과 진행 표시는 NpcState::Progress를 함께 사용한다. 목표는 기존처럼 각각 3, 호감도 완료 +10이다.
- NpcRole의 명명된 상수와 MonsterCatalog::SlotsPerIsland, ItemCatalog의 ID 구간 상수를 사용한다. 데이터 범위와 재료/몬스터 수에 대한 static_assert를 추가했다.
- ItemCatalog에서 InventoryIcons, MonsterCatalog에서 MonsterMeshes를 분리했다. GeometryTypes/IslandKey를 작은 공통 타입 헤더로 추출해 아이템·NPC 상태 헤더가 Renderer를 포함하지 않도록 했다.

## 5. 실제 클래스 책임 분리

| 객체 | 소유 상태/책임 |
|---|---|
| Prototype | 시스템 생성·연결, 월드/테스트 맵 수명, 프레임 실행 순서, 카메라·표시 Actor 준비 |
| PlayerController | 키 상태, 플레이 모드, 육상/갑판 이동, 승선·타륜·하선, 해적선 왕복, 공격 시작·낚시 |
| GameUi | 열린 창, 선택 품목, HUD/메뉴 입력·클릭 소비·표시·후처리 표시 옵션 |
| PlayerCombat | HP·레벨·경험치·강화·피격·회복·전투 효과와 전투 스냅샷 |
| PlayerEconomy | 골드·가방·장비·소유 선박·적재품 |
| NpcState | NPC 관계·의뢰·기억·동료 명단과 상태 전이 |
| GameState | 경제/NPC 상태를 묶어 맵별로 복사하는 값 객체 |

Prototype 구현 파일은 11개에서 3개로 줄었다. 전체 .cpp 수는 54개에서 56개가 됐으며, 목적은 파일 수 축소가 아니라 상태·규칙의 담당 객체를 분리하는 것이다. 데이터/표시 타입 분리로 헤더는 48개다. 새 UI도 CallbackActor와 UI SceneGraph를 통해 표시한다.

NpcState는 인벤토리를 소유하지 않는다. 전리품 전달 완료 때 인벤토리를 인자로 받아 검증 후 차감하고 의뢰를 완료한다. 외부에는 NPC 레코드의 const 조회만 제공하며 임의의 mutable 레코드 조회를 제거했다.

## 맵 전환과 유지한 기능

- T 전환 시 GameState 전체, PlayerCombat::ProgressState, PlayerControlState 및 플레이어/배/카메라 위치를 저장한다. economy/npcs 객체의 주소는 유지한 채 값을 복원해 각 System의 참조가 유효하게 남는다.
- 이전 맵에서 플레이어/공격 Actor의 부모를 먼저 분리하고 비활성 시스템을 보관한 다음 새 시스템에 Controller/UI를 다시 연결한다. 해적선 갑판에서 전환한 경우 복귀 시 유효한 갑판 부모를 복원한다.
- I 인벤토리, K 적재창, E 승선/시설/대화, F 강화/낚시, O/P 해상 소환, T/Y/F1~F8 테스트 조작을 유지한다.
- 초기 골드 50, 8m 돛단배 50골드·동료 정원 2·적재 5종, 몬스터 항목별 드랍 50%, 거래 가격·장비 능력치·해상 패배 손실 규칙을 변경하지 않았다.
- NPC AI 서비스는 계속 미연결 상태이며 한국어 기본 대사·비동기 제공자 인터페이스를 유지한다. 실제 모델/네트워크 연결 기능을 추가한 작업은 아니다.

## 검증 범위

정적 확인 대상으로 현재 프로젝트/필터 XML, 소스/헤더 등록·중복·include 경로, 이전 이름/클래스/콜백 잔존, 분리 클래스의 멤버 호출과 선언, C++ 포맷을 확인한다. C++ 컴파일러로 확인한 결과는 아니며 링크·실행 성공을 보장하는 표현으로 해석하면 안 된다.

실제 정적 점검 결과: 게임 소스 56개·헤더 48개 등록, 누락 파일/미등록 소스/include 경로 오류/프로젝트 중복 항목/필터 등록 누락 0개. 구조 문서에 104개 코드 파일을 모두 연결했고 끊어진 링크 0개다. 삭제한 클래스/거래 경로/이전 이름은 현재 코드·프로젝트에서 검색되지 않았다. 수정 C++ 및 회귀 소스의 clang-format --dry-run --Werror 검사를 통과했다. 분리한 클래스의 멤버 이름/선언 연결도 확인했다. Prototype의 세 구현 파일은 총 773줄이다(주석·빈 줄 포함).

Tests/GameplayRegression.cpp를 추가했다. 거래 실패 시 상태 보존·장비 교환, 의뢰 중복 완료 방지·재료 전달 실패, 동료 정원, GameState 복사 독립성, 지역/호감도 가격, 드랍 정의, 원거리 주변 좌표와 HUD 사각 영역을 검사한다. 이 검사도 이번에는 빌드/실행하지 않았다. 방법은 REFACTOR_TESTS.md에 기록했다.

사용자 게임 확인 순서:

1. HUD 골드 패널과 각 안내창을 클릭해 공격이 발생하지 않는지 확인하고, 패널 밖 육상/갑판 공격은 유지되는지 확인한다.
2. 이동·승선·타륜·하선·낚시, 해적 승선/약탈/복귀를 확인한다.
3. I에서 장비 교체·해제, F 강화, K 적재/하역, 상점 거래를 확인한다.
4. 각 NPC 의뢰 수락/완료/영입을 확인한다. 일반 3마리·재료 3개·구매 3개와 화면 진행이 일치하고 완료가 반복되지 않아야 한다.
5. T 왕복을 육상·내 배·해적선 갑판에서 확인한다. 일반/테스트 맵의 아이템·골드·HP·의뢰·동료가 섞이지 않아야 한다.
6. 먼 섬에서 슬래시/피해 숫자의 위치를 확인한다. 회귀 소스는 ±8000청크 주변의 좌표 변환을 검사하지만 실제 전투 화면 검사를 대체하지 않는다.

## 성능 분석

이전 모델 10개와 시작 섬 어댑터의 불필요한 경로장 생성을 제거했다. 현재 효과 리소스만 캐시하며 프레임별 모델 생성은 추가하지 않았다. 효과 활성 판정은 월드 좌표를 직접 사용한다.

FPS·메모리 절감량·Draw Call 개선 수치는 측정하지 않았다. SceneGraph::CollectDestroyed 호출 횟수처럼 기존 검토에서 측정 후보로 분류한 부분은 안전 시점과 실제 비용을 확인한 후 별도 변경한다. 기존 프로파일러·인스턴싱·청크 스트리밍은 유지했다.
