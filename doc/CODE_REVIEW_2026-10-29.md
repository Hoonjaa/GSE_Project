# 코드 품질 · 미사용 구현 · 구조 검토

> 이 문서는 리팩터링 전 코드의 검토 기록이다. 제안한 다섯 단계는 이후 적용했으며 현재 상태·파일명·검증 범위는 REFACTOR_REPORT_2026-10-29.md와 CODE_STRUCTURE.md를 참조한다. 아래 소스 링크와 지적은 검토 당시 기준이다.

검토일: 2026-10-29. 현재 프로젝트 자체 소스의 선언·생성·호출·상태 변경 경로를 정적으로 확인했다. 빌드·게임 실행·FPS/GPU 시간 측정은 하지 않았다. 게임 소스의 삭제·이름 변경·리팩터링은 이번 검토에서 수행하지 않았다.

## 전체 판단

프로토타입을 확장하는 기반은 갖추었다. Actor/SceneGraph, World/IslandLayout, System/Actor, 메시 캐시/인스턴싱, 카탈로그/경제 상태, UI/후처리 분리는 유지할 가치가 있다.

다만 초기 구현을 새 시스템으로 대체한 뒤 이전 경로를 제거하는 작업과, 커진 클래스의 책임을 분리하는 작업이 기능 추가를 따라가지 못하고 있다. 파일 수보다 중요한 문제는 **실제 사용 경로가 둘처럼 보이는 잔존 코드, 중앙 클래스의 책임 집중, 여러 파일에 반복된 규칙**이다.

현재 자체 .cpp 54개, .h 37개다. Prototype으로 시작하는 .cpp는 11개·물리적 2,509줄(빈 줄·주석 포함)이지만 모두 같은 Prototype 클래스의 구현이다. 파일 분할 자체는 유효하나 클래스 책임이 분리됐다는 의미는 아니다.

## 1. 우선 확인·수정할 동작 문제

### A1. HUD 클릭 영역과 공격 차단 영역이 어긋남

- 근거: [PrototypeHud.cpp:100](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/PrototypeHud.cpp:100)는 골드/안내 패널을 x=20~450, y=360~444에 그린다. [Prototype.cpp:223](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/Prototype.cpp:223)의 MouseAttack은 상단 HUD·지도·하단 안내·승선 상태창 등을 좌표로 따로 차단하지만 이 패널은 빠져 있다.
- 조건: 기본 1280×800, HUD 표시, 육상, 메뉴 닫힘, 공격 가능 상태에서 패널 안의 (100, 370) 클릭은 기존 차단 조건을 통과해 육상 공격 처리로 전달될 수 있다. 실제 명중 여부는 주변 몬스터와 방향에 달렸다.
- 판단: 단순 정리 선호가 아니라 소스상 확인되는 입력 처리 누락이다. 실행 재현은 미실시다.
- 개선: UI가 그릴 때와 클릭을 소비할 때 같은 Rect/레이아웃 정보를 사용하도록 한다. 인벤토리의 [PrototypeInventory.cpp:197](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/PrototypeInventory.cpp:197)는 이미 Layout::Cell/Gear를 그리기·클릭에 함께 사용하고 있어 재사용 가능한 방향이다.

### A2. 시작 섬 방식의 전투 좌표가 장거리 월드와 충돌

- 근거: [Prototype.cpp:283](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/Prototype.cpp:283)과 [Prototype.cpp:444](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/Prototype.cpp:444)는 플레이어 위치를 월드 원점에 대한 Vec3로 바꾸어 LevelOne에 넘긴다. [Prototype.cpp:514](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/Prototype.cpp:514)는 반대로 원점의 카메라 상대 위치를 전투 루트에 적용한다.
- [WorldPosition.cpp:29](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/WorldPosition.cpp:29)의 RelativeTo는 근처 청크용이다. 축별 청크 차이가 1000을 초과하면 실제 차이 대신 ±100000.f를 반환한다. 따라서 이 함수로 무한 월드의 전역 좌표를 표현하면 1000청크 밖에서 서로 다른 지점이 같은 값이 된다.
- 영향: 육상 슬래시/피해 숫자의 위치와 [LevelOne.cpp:136](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/LevelOne.cpp:136)의 효과 활성 구역 판정이 잘못될 수 있다. 현재 MonsterSystem의 근접 명중 판정은 WorldPosition/섬 로컬 좌표를 별도로 사용하므로 모든 공격 판정이 동일하게 깨진다고 단정하지 않는다.
- 판단: 초기 레벨 좌표 체계가 현재 월드 체계에 남은 구조 문제다. 단순히 float를 double로 바꾸면 위의 제한은 해결되지 않는다.
- 개선: 전투 효과도 WorldPosition 또는 해당 섬/근처 기준점에 대한 로컬 좌표를 보관하고 렌더 시 카메라 기준으로 변환한다. 시작 섬·음수 좌표·청크 경계·1000청크 밖을 확인한다.

## 2. 현재 미사용 경로가 확인되는 부분

정적 검색은 프로젝트 내부 호출 기준이다. 추후 외부 API나 디버그 호출을 추가할 계획이 있는지는 별도이며, 아래가 파일 전체 삭제 허가를 의미하지는 않는다.

### B1. 이전 EnemyActor / DropActor

- [GameplayActors.h:31](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/GameplayActors.h:31), [GameplayActors.cpp:72](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/GameplayActors.cpp:72)의 EnemyActor와 [GameplayActors.cpp:248](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/GameplayActors.cpp:248)의 DropActor는 선언·구현은 남아 있지만 현재 프로젝트에서 생성·사용하는 호출이 확인되지 않는다.
- 실제 육상 몬스터/드랍은 MonsterSystem → MonsterActor / MonsterItemDrop 경로다.
- 이전 GameplayContext의 Step·적/드랍용 콜백도 이 이전 구현에 연결된 부분이다. 현재 LevelOne이 GameplayContext의 player/safe 등을 일부 사용하므로 필요한 플레이어 프레임 상태를 분리한 뒤 정리해야 한다.
- **GameplayActors.cpp 전체 삭제는 부적절하다.** [GameplayActors.cpp:309](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/GameplayActors.cpp:309)의 DamageNumberActor와 [GameplayActors.cpp:332](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/GameplayActors.cpp:332)의 SlashActor는 현재 사용 중이다.

### B2. 사용하지 않는 이전 모델 10개를 초기화 때 생성

- [LevelMeshes.cpp:12](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/LevelMeshes.cpp:12)는 적 모델 4개, 예고 링 2개, 이전 드랍 2개, 산호 전리품 모델 2개를 무조건 구성한다.
- [LevelOne.h:77](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/LevelOne.h:77)의 LevelMeshes 멤버는 LevelOne 생성 시 생성된다. 현재 사용 경로에서 이 멤버의 slash는 쓰지만 위 10개 모델의 소비자는 B1의 이전 Actor 구현이다.
- 영향은 시작 시 CPU 정점 생성과 보관 메모리다. 이 정점들이 매 프레임 재생성되거나 미사용 모델 때문에 추가 Draw Call이 발생한다고 주장하지 않는다. Renderer에 제출하지 않은 자산의 GPU 업로드도 별개다.
- 개선: 활성 전투 효과 리소스만 남기고 이전 모델/필드를 함께 정리한다. 예상 절감량은 실행 측정 전에는 수치화하지 않는다.

### B3. 예전 무기 획득 함수와 새 인벤토리 상태의 공존

- [LevelOne.cpp:191](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/LevelOne.cpp:191)의 Collect(int weapon)는 선언/정의 외 호출이 확인되지 않는다.
- 이전 m_weapon/m_stones는 [PlayerProgressState.cpp:9](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/PlayerProgressState.cpp:9)의 스냅샷에도 남아 있다. 실제 강화는 PlayerEconomy의 UpgradeStone 아이템을 사용하며 실제 장착 무기는 장비 슬롯에서 조회한다.
- m_stones는 현재 강화 재화와 별도인 이전 필드이고, m_weapon은 여전히 Damage 식에 합산되므로 단순히 텍스트 검색 후 삭제하면 안 된다. 스냅샷 의미까지 함께 정리해 공격력과 강화석의 기준 상태를 하나로 만든다.
- 의도적으로 남긴 공통 강화 단계 m_upgrade까지 전부 미사용으로 분류하는 것은 아니다.

### B4. 호출되지 않는 거래 추상화/호환 함수

- [MapCatalog.cpp:110](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/MapCatalog.cpp:110)의 Offers는 현재 프로젝트 내부 호출이 확인되지 않는다. 이 경로에 연결된 TradePricing / PrototypeTradePricing와 전체 상품 견적 구조가 남아 있다.
- 실제 상점은 [PrototypeTrade.cpp:70](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/PrototypeTrade.cpp:70)에서 TradeRules::Buy/Sell을 직접 사용한다. 두 경로가 같은 산식을 공유하므로 현재 가격 불일치라고 단정하지 않지만, 사용하지 않는 계층을 함께 유지하고 있다.
- World::TutorialActive([World.cpp:434](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/World.cpp:434)), EconomyRules::SellPrice/StackLimit([PlayerEconomy.cpp:39](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/PlayerEconomy.cpp:39)), PlayerEconomy::Collect(CoralLoot)도 정의/선언 외 호출이 확인되지 않는다.
- 개선: 현재 사용할 거래 진입 경로를 선택한다. 미사용 API의 보존 목적이 없으면 제거하고, 의도된 확장점이면 문서에 표시한다. 현재 쓰는 TradeRules 가격 계산 자체를 없애자는 뜻은 아니다.

## 3. 파일 수와 별개로 개선할 책임·의존성

### C1. Prototype에 조작·메뉴·월드 전환 책임 집중

- [Prototype.h:29](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/Prototype.h:29) 이후에 전투·이동·승선·인벤토리·상점·NPC·테스트 맵의 함수와 상태가 함께 선언되어 있다.
- .cpp 11개로 나눈 것은 읽기 편하게 하지만 이 클래스의 여러 상태를 동시에 변경할 수 있다는 결합은 그대로다. 입력 우선순위와 모달 창 닫기 같은 변경의 영향 범위가 크다.
- 개선 방향: Prototype은 시스템 생성/연결과 Update/Render 조율을 맡도록 유지하고, 먼저 PlayerController(이동·플레이 모드·승선)와 UI 화면 상태/입력 영역을 실제 객체로 분리한다. 기능 파일마다 무조건 새 클래스를 하나씩 만드는 방식은 피한다.

### C2. PlayerEconomy가 NPC 관계·의뢰·동료까지 소유

- [PlayerEconomy.h:75](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/PlayerEconomy.h:75)의 의뢰·관계·영입 API와 [PlayerEconomy.h:112](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/PlayerEconomy.h:112)의 NPC 상태/동료 명단은 경제 이상의 책임이다.
- NpcRelations.cpp로 구현 파일을 나눠도 같은 클래스다. NPC 동작 변경을 위해 아이템·선박 경제 클래스까지 알아야 한다.
- 개선 방향: 플레이어 인벤토리/장비 상태와 NPC 관계/퀘스트/동료 상태를 의미별로 나누고, 상위 GameState 같은 명시적인 값 구조에 묶는다. 맵 전환 스냅샷은 이 상위 상태를 복사하면 된다. 의뢰 전달의 아이템 차감과 완료 처리는 한 서비스/작업에서 원자적으로 조율한다.

### C3. 같은 규칙을 여러 파일에서 반복

- [NpcRelations.cpp:73](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/NpcRelations.cpp:73)의 수집 완료 조건, [NpcSystem.cpp:407](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/NpcSystem.cpp:407)의 진행 표시, [PrototypeNpc.cpp:81](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/PrototypeNpc.cpp:81)의 안내에 목표 수량 3과 섬별 몬스터 5종 계산이 반복된다.
- [ItemCatalog.h:5](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/ItemCatalog.h:5)와 [ItemCatalog.cpp:29](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/ItemCatalog.cpp:29)의 ID 구간 및 배열 인덱스, 섬 shape×5, role 0/1/2 같은 규칙이 여러 곳에 흩어져 있다.
- 지금 결과가 다르다는 증거는 아니지만, 목표 수량이나 아이템 목록을 바꿀 때 한쪽만 바꿀 가능성이 크다.
- 개선 방향: 작고 명확한 QuestDefinition/드랍 정의와 NpcRole enum, 카탈로그 조회 함수를 두고 UI도 동일한 목표·진행 계산을 사용한다. 외부 데이터 로더나 거대한 설정 프레임워크부터 만들 필요는 없다.

### C4. 순수 데이터 헤더가 Renderer/OpenGL에 의존

- [ItemCatalog.h:2](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/ItemCatalog.h:2)는 Renderer.h를 포함하고, 같은 헤더에 InventoryIcons와 Mesh도 선언한다. PlayerEconomy는 이 헤더를 포함하므로 경제/퀘스트 코드를 읽거나 컴파일할 때 렌더링 헤더 의존성이 함께 따라온다.
- MonsterCatalog.h도 몬스터 정의와 MonsterMeshes 선언을 함께 가진다. MapCatalog.cpp의 Offers는 가격 정책을 직접 구성한다.
- 개선 방향: ItemDefinition/InventoryItem 같은 데이터와 InventoryIcons 같은 표시 리소스를 분리한다. 필요한 작은 공용 Vec/Color 타입도 GL 자원 코드와 구분하면 독립적인 규칙 테스트가 쉬워진다.
- 이는 현재 실행에 OpenGL 컨텍스트가 반드시 추가로 필요하다는 주장이 아니라 소스·컴파일 의존성에 대한 판단이다.

### C5. 일반 월드와 테스트 맵의 시스템 구성 코드 복제

- [Prototype.cpp:19](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/Prototype.cpp:19)와 [PrototypeTestMap.cpp:105](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/PrototypeTestMap.cpp:105)에서 MarineSystem/MonsterSystem 생성과 콜백 연결을 반복한다.
- 이미 일반 월드의 해적 피격 콜백에는 pirate_player_damage 상세 로그가 있지만 테스트 맵 생성 콜백에는 같은 로그가 없다. 게임 데미지가 다르다는 뜻은 아니며, 구성 복제에 따른 차이가 발생한 구체적인 사례다.
- 개선 방향: 공통 시스템 생성 함수/구성 함수를 한 곳에 둔다. 테스트 맵에 필요한 차이만 매개변수나 설정으로 전달한다.

## 4. 성능 관련 측정 후보

### D1. 한 Update에서 SceneGraph 전체 정리 반복

- MonsterSystem::Sync([MonsterSystem.cpp:256](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/MonsterSystem.cpp:256))와 NpcSystem::Sync([NpcSystem.cpp:206](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/NpcSystem.cpp:206))가 변경이 없어도 CollectDestroyed를 호출한다.
- SceneGraph::Update도 시작/끝에 호출한다([SceneGraph.cpp:118](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/SceneGraph.cpp:118)). 따라서 현재 일반 갱신 경로에서 정리 함수가 여러 차례 실행된다.
- [SceneGraph.cpp:75](C:/Users/Leejaehoon/Documents/GitHub/GSE/BaseProject_SE/SimpleGame/SceneGraph.cpp:75)는 삭제 요청 목록만 확인하는 구현이 아니라 전체 Actor의 Live/부모 경로 및 자식 목록을 순회한다. Actor가 늘어날 때 비용이 커질 후보다.
- **현재 FPS 저하의 원인으로 확정하지 않는다.** 함수별 CPU 시간과 Actor 수를 측정한 뒤, 삭제 요청이 없으면 빠르게 반환하거나 명확한 안전 시점에서 정리하는 방식을 검토한다. 순회 중 지연 삭제의 안전성은 유지해야 한다.

### D2. 시스템과 Actor의 갱신 방식 혼용

몬스터는 enabled=false 상태에서 MonsterSystem이 직접 Tick을 호출하고, 일부 효과·날씨는 SceneGraph::Update가 갱신한다. 현재 설계상 중복 갱신을 피하기 위한 방식이며 자체로 버그는 아니다. 다만 새 Actor를 추가할 때 소유 시스템과 갱신 경로를 명확히 정하지 않으면 이중 갱신/갱신 누락을 만들 수 있다. 문서·명명으로 정책을 드러내고, 성능 측정 없이 전부 같은 방식으로 바꾸지는 않는다.

## 5. 미사용으로 보인다고 즉시 삭제할 대상은 아닌 것

| 부분 | 유지/판단 이유 |
|---|---|
| 메시 캐시·인스턴싱·프로파일러 | 현재 반복 모델 재사용과 성능 관측을 담당하는 실제 사용 기능 |
| 작은 RendererIcon/InventoryIcons 소스 | 파일이 작아도 화면 변환과 아이콘 자산 생성이라는 유효한 역할이 있음 |
| 테스트 맵 | 수동 조작 검증을 위한 현재 사용 기능이며, 자동 테스트와는 별개 |
| NpcDialogue의 제공자 인터페이스 | 실제 API는 미연결이지만 사용자 요구인 AI 대화의 명시적 연결부. 한국어 기본 대사는 사용 중 |
| NpcRecord의 fear/resentment 등 | 아직 행동에 연결되지 않은 설계 필드. 미구현 상태로 표시할 대상이며 죽은 구버전 전투 코드와는 성격이 다름 |
| GameplayActors의 슬래시·피해 숫자 | 이전 적 구현과 같은 파일에 있지만 현재 플레이에서 사용 |

## 6. 권장 진행 순서

1. A1/A2의 입력 영역·전투 좌표 경로를 수정하고 사용자 실행으로 재현/회귀 확인한다.
2. B1~B3의 이전 적·드랍·모델·획득 상태를 함께 정리한다. 현재 효과 Actor와 진행 스냅샷을 보존한다.
3. LevelOne을 PlayerCombat 등 실제 책임에 맞게 이름 변경한다. 사용하지 않는 시작 섬 전투 Context 의존성도 함께 줄인다.
4. C5의 시스템 생성 코드를 통합한다. 일반/테스트 맵의 조우·피격·드랍·로그 경로를 비교한다.
5. UI 클릭/표시 레이아웃과 의뢰 목표/진행 계산을 공통 정의로 만든다.
6. PlayerEconomy의 NPC 상태·의뢰 책임 및 Prototype의 플레이어 조작·UI 책임을 작은 단위로 옮긴다.
7. D1의 실제 비용을 측정한 뒤 수명 정리 정책을 최적화한다.

전체 재작성, ECS 전환, 모든 호출에 이벤트 버스 도입, 파일 수를 줄이기 위한 무조건적 합치기는 지금 우선순위가 아니다. 역할에 맞는 .cpp가 늘어나는 것은 자연스럽다. 같은 규칙/상태를 고치려고 많은 파일을 동시에 열어야 하는지를 개선 기준으로 삼는다.

## 7. 확인할 회귀 항목

- HUD 위 클릭이 공격으로 전달되지 않음, HUD 밖 육상/갑판 공격 유지.
- 원점·음수 청크·경계·1000청크 밖에서 슬래시/피해 숫자/효과 활성 판정 유지.
- 아이템 획득·장착 교환·강화석 차감·골드 부족·판매 실패 시 수량 보존.
- 퀘스트 목표와 화면 진행 일치, 중복 완료 보상 없음, 재료 전달 실패 시 미차감.
- T 왕복 후 각 맵의 체력·경제·NPC/동료 상태 보존과 비활성 Actor 숨김.
- 기존 프레임 로그와 함수별 정리 비용 비교. 빌드/실행/측정은 이번 검토에서 수행하지 않음.

현재 프로젝트에서 별도 자동 테스트 소스/테스트 프로젝트는 이번 검색 범위에서 확인하지 못했다. T 테스트 맵은 유효한 수동 검증 도구지만 아이템 거래·퀘스트 상태 같은 규칙의 자동 회귀 검증을 대체하지 않는다. 데이터/표시 의존성을 분리할 때 이 규칙부터 작은 테스트로 검증하는 것이 적합하다.

