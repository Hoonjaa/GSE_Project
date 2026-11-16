# 로컬 월드 모니터

게임을 빌드해서 실행한 뒤, 같은 PC의 브라우저에서 [월드 모니터](http://127.0.0.1:8765/)를 연다. 별도 서버 실행이나 패키지 설치 없이 게임이 HTML과 JSON을 제공한다. Visual Studio 빌드 과정에서 HTML이 실행 파일 옆 `Monitor/index.html`로 복사된다.

브라우저는 [상태 JSON](http://127.0.0.1:8765/api/state)을 약 1초마다 요청한다. 게임을 닫거나 갱신이 멈추면 열린 화면은 마지막으로 받은 상태에 연결 끊김 또는 갱신 지연을 표시한다. 포트가 사용 중이거나 HTML이 빠져 있으면 콘솔과 게임 로그에 `[Monitor] Unavailable` 또는 `monitor_unavailable`로 기록하며 게임은 계속 실행된다.

## 표시 범위

- 일반 월드에서는 현재 청크가 로딩된 섬만 표시한다. 테스트 맵에서는 8개 섬을 모두 표시하며, 미로딩 섬의 몹과 마을 NPC 수는 미확인으로 표시한다.
- 섬별로 방문 여부, 최대 폭, 육지·마을 면적, 로딩·활성 청크와 몹·NPC 인원을 표시한다. 활성은 월드의 활성 범위 기준이다.
- 몹 수는 섬에 생성된 육상 몬스터 기준이다. 생존, 에픽 생존, 활성 생존과 현재 처치 상태를 집계한다. 리스폰하면 생존 수에 다시 포함된다.
- NPC는 호감도·신뢰·존중·공포·원한, 의뢰 진행과 최근 기억을 표시한다. 영입된 NPC는 마을 인원 집계에서 빠지며, 출신 섬이 로딩 범위 밖으로 나가도 동료 목록에 남는다.
- 현재 플레이 중인 맵의 상태를 표시한다. `T`로 일반 월드와 테스트 맵을 전환하면 다음 상태 메시지에 반영된다.

## JSON 연결

`GET /api/state`는 UTF-8 `application/json`으로 최신 전체 상태 한 건을 반환한다. 첫 상태가 아직 준비되지 않았으면 HTTP 503을 반환한다.

| 필드 | 의미 |
|---|---|
| `schemaVersion`, `timestampMs` | 형식 버전 1과 Unix epoch 밀리초 |
| `map`, `overview` | `adventure` 또는 `test`, 테스트 전경 모드 여부 |
| `player`, `chunks` | 플레이어의 청크·청크 내 위치·현재 섬과 월드 청크 집계 |
| `islands[]` | 섬 키, 이름, 중심점, 방문, 폭·면적, 청크, 몬스터·NPC 상태 |
| `islands[].monsters` | 생성 `spawned`, 생존 `alive`, 활성 생존 `active`, 에픽 생존 `epicAlive`. 미로딩은 `null` |
| `islands[].npcs[]` | 출신 섬, 역할, 이름, 배치 상태, 관계 수치, 의뢰, 최근 기억 |
| `companions[]` | 현재 맵의 영입 동료 |

섬 키의 `x`·`z`와 위치의 `chunkX`·`chunkZ`는 큰 좌표를 브라우저에서 정확히 다루도록 10진 문자열로 전달한다. 섬의 NPC 수를 사용할 때 `loadedChunks == 0`이면 현재 인원은 미확인이다. `npcs[]`의 `presence`는 `active`, `resident`, `unloaded`, `companion` 중 하나다.

게임 실행 중 PowerShell에서도 같은 상태를 읽을 수 있다.

```powershell
$state = Invoke-RestMethod 'http://127.0.0.1:8765/api/state'
$state.islands | Select-Object name, loadedChunks, residentNpcCount, monsters
```

게임 루프가 약 1초마다 JSON을 만들어 게시한다. 로컬 HTTP 처리 스레드는 완성된 JSON을 읽으며, 바인딩 주소는 `127.0.0.1:8765`로 고정한다.
