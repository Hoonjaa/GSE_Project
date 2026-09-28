# 성능 분석 변경 이력

실행별 원시 자료는 실행 파일 옆 Logs/run_*/에 보관한다. 이 문서는 분석 가정과 변경 이유를 누적하는 기록이며, 실행 결과 없이 측정치를 만들어 채우지 않는다.

## 2026-09-22 — 동일 메시 인스턴싱 / 월드 효과 파일 캐시

- 입력 자료: 사용자가 제공한 8개 콘솔 집계. FPS 49.5~51.4, draw call 48~62.
- 소스 관찰: CachedMesh Submit이 즉시 draw. 파도·바다·깃발·갈매기는 렌더 중 Mesh 생성. Tick은 다음 타이머를 16ms로 예약.
- 가설 A: 반복 메시의 API 호출 및 CPU 정점 생성/변환 비용을 줄일 수 있다.
- 가설 B: 약 50 FPS에는 프레임 페이싱/업데이트/swap 비용도 관여할 수 있다. 현 로그로 원인을 확정할 수 없다.
- 변경: 공유 메시별 제출 큐, glDrawArraysInstanced, GPU 애니메이션, 검증 가능한 .mesh 디스크 캐시, 실행별 CSV/이벤트 로그.
- 범위 제외: 고유 청크 메시 병합, 컬링, GPU 실행 시간 측정, 업데이트 타이머 변경.
- 분석 지표 추가: 패스별 draw, 제출/인스턴스 수, 업로드 요청량, 캐시 hit/miss/eviction, CPU update/render/swap, 프레임 간격, 장면 규모 및 상태.
- 검증 상태: 빌드/실행하지 않음. 성능 개선 및 시각 결과 미확인.
- 다음 자료: cold/warm cache 세션, 동일 설정의 안정 구간 summary.csv와 batches.csv, 필요 시 frames.csv.
- 상세 설계와 비교 절차: DRAW_CALL_OPTIMIZATION_REPORT.md.

## 다음 분석 기록 형식

- 날짜 / 변경 내용 / 실행 세션 폴더:
- 빌드 구성 / GPU / 드라이버 / 지형 시드:
- 화면 / 줌 / HUD / 격자 / HDR / 블러 / 장면:
- 비교 구간 / 워밍업 제외 기준:
- FPS 및 프레임 간격:
- draw 분류 / submits / instances:
- static·instance·stream 업로드:
- CPU update·render·swap:
- 캐시 이벤트 / 시각 오류:
- 관찰과 가설을 구분한 결론:
- 다음 변경 및 검증 항목:
