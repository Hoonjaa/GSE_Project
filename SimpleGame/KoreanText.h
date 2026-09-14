#pragma once

// Player-facing copy is UTF-8 Korean. Future NPC dialogue uses the same encoding.
// Keep character voice and proper names consistent when adding generated dialogue.
namespace KoreanText
{
    constexpr const char* Title = "새싹 탐험가의 섬";
    constexpr const char* Subtitle = "대항해 시대 / OpenGL";
    constexpr const char* Version = "레벨 1 · 파밍 튜토리얼";
    constexpr const char* Chart = "주변 해도";
    constexpr const char* North = "북";
    constexpr const char* Sailing = "항해 중";
    constexpr const char* Walking = "도보 이동";
    constexpr const char* Chunk = " / 청크 ";
    constexpr const char* Movement = "WASD 이동 / 좌클릭 공격 / F 강화 / E 승선·하선";
    constexpr const char* Tools = "Space 달리기 / 휠 줌 / V 후처리 안내 / R 항구 복귀 / Esc 종료";
    constexpr const char* Welcome =
        "항구 밖의 게를 처치해 경험치와 장비를 모으세요. 붉은 표시는 북쪽 보스입니다.";
    constexpr const char* Boarded = "출항했습니다. 수평선 너머의 새로운 섬을 찾아보세요.";
    constexpr const char* TooFarToBoard = "배에 더 가까이 다가간 뒤 E를 눌러주세요.";
    constexpr const char* Landed = "상륙했습니다. 배는 이곳에서 기다립니다.";
    constexpr const char* TooFarToLand = "해변이나 부두에 더 가까이 다가가면 내릴 수 있습니다.";
    constexpr const char* Seed = "시드 ";
    constexpr const char* LoadedChunks = " / 로딩 청크 ";
    constexpr const char* Triangles = " / 삼각형 ";
    constexpr const char* Frames = " 프레임/초";
    constexpr const char* PostLabel = "후처리: ";
    constexpr const char* On = "켜짐";
    constexpr const char* Off = "꺼짐";
    constexpr const char* Unavailable = "사용 불가";
    constexpr const char* Exposure = " / 노출 ";
    constexpr const char* PostControls = "P 전체 / 1 톤 매핑 / 2 비넷 / 3 외곽 흐림";
    constexpr const char* ExposureControls = "[ ] 노출 조절 / 0 후처리 초기화";
    constexpr const char* ToneLabel = "톤 매핑 ";
    constexpr const char* VignetteLabel = " / 비넷 ";
    constexpr const char* BlurLabel = " / 흐림 ";
    constexpr const char* PostEnabled = "HDR 후처리를 켰습니다. 노출과 외곽 효과를 조절해 보세요.";
    constexpr const char* PostDisabled = "후처리를 껐습니다. 기존 렌더링 화면과 비교할 수 있습니다.";
}
