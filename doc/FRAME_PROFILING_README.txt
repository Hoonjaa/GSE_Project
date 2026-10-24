기본 콘솔 프로파일링
===================

2026-09-22 갱신:
현재는 glDrawArraysInstanced를 포함하고, 실행 파일 옆 Logs/run_*/에
summary.csv / batches.csv / events.log를 저장한다.
GSE_PROFILE_DETAIL=1로 실행하면 매 프레임 frames.csv도 추가한다.
CPU update/render/swap 시간과 캐시/업로드 지표도 수집한다.
최신 필드 정의와 분석 절차는 DRAW_CALL_OPTIMIZATION_REPORT.md를 기준으로 한다.
아래 내용은 초기 콘솔 로깅 형식에 대한 설명이다.

실행 중 약 1초마다 다음 형식의 집계 로그가 출력된다.
[Frame] FPS=60.0 | Draw calls/frame: last=125 avg=123.5 min=120 max=125
위 숫자는 형식 설명용 예시이며 실제 측정값이 아니다.

FPS: 집계 구간에서 버퍼 교환까지 완료한 프레임 수 / 실제 경과 시간(초).
last: 마지막 완료 프레임의 draw call 수.
avg / min / max: 해당 집계 구간의 프레임별 draw call 평균 / 최솟값 / 최댓값.
매 프레임 카운트하되 콘솔 출력은 초당 약 1회로 제한한다.
가려짐/표시 상태 변경 시 FPS 집계를 초기화하여 비표시 시간의 영향을 줄인다.

카운트 범위:
- 캐시 메시, 동적 메시, 한글/UI 배치의 실제 glDrawArrays 호출.
- 후처리 블러 2회와 최종 합성 1회. 실행되는 패스만 포함한다.
- 후처리가 꺼져 있거나 사용할 수 없으면 후처리 draw는 발생하지 않는다.
- glClear, glBlitFramebuffer, 상태 설정(glDrawBuffer/glDrawBuffers 등)은 제외한다.
- Submit/Actor 수나 삼각형 수를 draw call 수로 대신 사용하지 않는다.

RenderStatistics.h: 실제 GL draw와 카운터 증가를 묶은 공통 진입점.
Renderer::Begin에서 초기화하며 Renderer::End 이후 DrawCallCount로 전체 프레임을 읽는다.
새 glDrawElements/인스턴싱 등의 경로를 추가할 때에도 RenderStatistics에 해당
래퍼를 추가하고 실제 호출마다 한 번씩 카운트해야 한다.
FrameProfiler.h: steady_clock 기반 FPS와 프레임별 draw call 집계/출력.
SimpleGame.cpp: RenderScene에서 렌더링 전과 glutSwapBuffers 후에 프로파일러를 호출한다.

FPS에는 기존 16ms 업데이트 타이머, VSync/버퍼 교환 대기, 콘솔 출력 비용이 포함된다.
따라서 GPU 단독 처리 성능이나 무제한 렌더링 FPS를 의미하지 않는다.
glFinish 같은 강제 GPU 동기화나 GPU 시간 쿼리는 추가하지 않았다.

사용자 확인:
1. 실행 후 콘솔에서 FPS와 draw call 집계가 출력되는지 확인한다.
2. B로 후처리를 끄면 동일 장면에서 후처리 draw가 사라지는지 확인한다.
3. 블러 사용 조건에서 외곽 흐림만 끄면 후처리 draw가 3회에서 1회로 줄어드는지 확인한다.
4. 전체 draw 수는 오브젝트/프레임 배치 변화에 따라서도 달라질 수 있다.

이번 변경에서는 빌드와 실행을 수행하지 않았다.
