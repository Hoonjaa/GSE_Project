Actor / SceneGraph 구조
=======================

1. 공통 계층
Actor.h/.cpp
  로컬 위치, Y축 회전, visible/enabled, ID, 부모 ID, 삭제 요청을 가진다.
  Update / Render / Overlay를 상속하여 새 오브젝트의 동작을 추가한다.
  MeshActor는 CachedMesh를 보관한다. 같은 리소스를 복사하면 GPU 캐시를 공유한다.
  CallbackActor는 HUD 등 기존 기능을 그래프의 갱신/표시 단계에 연결하는 어댑터다.

SceneGraph.h/.cpp
  unique_ptr로 모든 Actor의 수명을 소유한다. 게임 로직에는 ActorId를 보관한다.
  Create<T>(parentId, 생성자 인자...)로 배치하고 Get<T>(id) 또는 Find(id)로 조회한다.
  ID는 그래프 내에서 재사용하지 않는다. 삭제된 Actor의 조회 결과는 nullptr이다.
  부모 회전과 위치를 자식의 로컬 변환에 합성한다. 현재 변환은 이동과 Y축 회전이다.
  SetParent는 로컬 변환을 유지하며 자기 자신/자손 아래로 이동하는 순환을 거부한다.
  부모 visible=false는 하위 트리의 월드 및 오버레이 표시를 숨긴다.
  부모 enabled=false는 하위 트리의 Update를 중지한다. 표시는 별도로 제어한다.

2. 현재 그래프
Prototype이 하나의 SceneGraph를 소유하고 World와 LevelOne에 참조로 전달한다.

월드
  바다
  스트리밍 청크
    청크별 루트
      파도/디버그 격자
      지형 MeshActor
      건물/장식 MeshActor
  레벨 1 지형 MeshActor
레벨 1
  EnemyActor (일반 몬스터, 보스)
  전투 효과 및 드랍
    SlashActor (공격 궤적)
    DropActor (무기/강화석, 동적 생성/삭제)
    DamageNumberActor (피해량, 동적 생성/삭제)
  레벨 HUD
플레이어 및 항구
  선박 WorldActor
    그림자 / 항적 / 선체
  플레이어 WorldActor
    발밑 표시
    플레이어 모델
      장착 무기
  항구 WorldActor
    NPC 모델 / FlagActor / GullActor
  게임 HUD

정적 지형과 건물/나무 장식은 기존 청크 단위 배치 메시를 유지한다.
각 삼각형이나 지형 타일을 Actor로 만들지 않는다. 개별 상호작용이 필요한
건물/나무는 해당 배치 메시에서 분리한 뒤 별도의 Actor로 등록하면 된다.
체력 바와 보스 공격 예고는 EnemyActor의 표시 구성 요소다.
HUD의 개별 글자/패널 역시 별도 Actor가 아니라 HUD Actor의 배치 출력이다.

3. 책임 분리
LevelOne.cpp
  스폰 배치, 경험치/장비/보상 정책, 플레이어 피격 및 근접 공격 판정.
GameplayActors.h/.cpp
  EnemyActor: 추적, 공격 예고, 공격, 복귀/리스폰, 피격 상태와 체력 바.
  DropActor: 부유 표시, 자석 이동, 습득 요청, 시간 만료 및 삭제.
  DamageNumberActor: 피해량 표시 및 수명.
  SlashActor: 공격 궤적의 위치/방향/수명.
  GameplayContext: 프레임 입력, 지형/경로 탐색, 피해 및 습득 콜백.
LevelMeshes.cpp
  재사용하는 전투 메시 생성. Actor 인스턴스마다 다시 생성하지 않는다.
LevelOneHud.cpp
  레벨 UI와 미니맵 표시.
WorldActors.h/.cpp
  큰 월드 좌표 앵커, 바다/파도/깃발/갈매기.
PrototypeScene.cpp
  플레이어/배/NPC의 생성, 자식 배치, 카메라 상대 위치 및 표시 상태 연결.
PrototypeHud.cpp
  게임 HUD. CallbackActor를 통해 SceneGraph의 Overlay 단계에서 호출된다.

입력과 이동/충돌 처리는 Prototype 컨트롤러에 남긴다.
플레이어와 배의 실제 WorldPosition은 WorldActor가 소유한다.
정수 청크 + double 로컬 좌표를 유지하고, 렌더링 직전에 카메라 상대 float 좌표로
변환한다. 무한 월드 전체를 단일 float 좌표로 바꾸지 않는다.

4. 프레임과 수명
Prototype::Update:
  입력/이동 -> LevelOne::BeginFrame -> SceneGraph::Update -> LevelOne::EndFrame.
SceneGraph::Update:
  삭제 회수 -> 부모 우선 순회 목록 작성 -> 각 Actor 갱신 -> 삭제 회수.
  Update 중 생성된 Actor는 다음 Update부터 갱신하며 이번 프레임부터 표시될 수 있다.
  Destroy는 즉시 삭제하지 않고 표시한다. Find/Get은 삭제 요청된 노드와 그 자손을
  곧바로 조회 불가로 처리하고 순회 후 실제 메모리를 해제한다.
  부모 삭제 시 자손도 함께 제거된다. 청크 해제도 이 경로를 사용한다.
  장기 참조는 ActorId를 사용하고 Find/Get의 nullptr를 확인한다.
  Create가 반환한 참조/포인터는 해당 Actor의 삭제 회수까지만 유효하다.
Prototype::Render:
  카메라 상대 변환 설정 -> SceneGraph::Render -> 후처리 -> SceneGraph::Overlay.
  렌더링 콜백에서 Create나 재부모화를 하지 않는다. 갱신 단계에서 요청한다.

SceneGraph는 World/LevelOne보다 먼저 생성되고 나중에 해제된다.
World/LevelOne은 소유한 루트를 제거한 뒤, Actor가 참조하던 문맥/지형을 해제한다.
캐시된 GPU 메시는 기존 Renderer의 약한 참조 회수 규칙을 그대로 따른다.

5. 확장 예
auto& actor = scene.Create<MeshActor>(parentId, sharedMesh, "보물 상자");
actor.transform.position = {4, 0, 6};
const ActorId chestId = actor.Id();
// 이후에는 핸들로 조회한다.
if (auto* chest = scene.Get<MeshActor>(chestId))
{
    chest->visible = false;
}
scene.Destroy(chestId);
// Update 종료 또는 순회 외부의 CollectDestroyed에서 회수한다.

새 몬스터 행동은 Actor 파생 클래스와 GameplayContext 서비스로 추가할 수 있다.
현재 경로 탐색 서비스는 LevelOneTerrain의 격자/충돌 API를 사용한다.
다른 지형 표현을 도입할 때는 이 서비스 경계를 교체하면 된다.
스케일 변환, 물리 엔진, 저장/로드, ECS, 공간 분할 검색은 이번 범위에 포함하지 않는다.

6. 확인 사항
소스 포맷과 프로젝트/필터 파일 등록을 정적으로 점검했다.
빌드, 게임 실행, 렌더링 확인은 수행하지 않았다.
사용자 확인 항목:
  이동/공격 중 장착 무기가 플레이어 위치와 회전을 따라가는지
  승하선과 선체 흔들림/항적, 항구 NPC/깃발/갈매기가 유지되는지
  일반 몬스터/보스 추적과 예고 공격, 피격 색상, 복귀와 리스폰
  드랍 부유/자석 습득/만료, 피해량 표시와 공격 궤적의 소멸
  청크 왕복 로딩 시 지형/장식 중복 또는 잔존이 없는지
  사망/귀환, HUD/격자 토글, 한글, HDR 및 화면 크기/줌 변경
