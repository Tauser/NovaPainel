// NovaPainel - board/waveshare_board.hpp
// Real IBoard implementation for the confirmed board (Waveshare
// ESP32-P4-WIFI6-Touch-LCD-7B, see docs/HARDWARE.md). Fase 0 validated the
// hardware in firmware/experiments/gate15_coexistence; this brings the same
// bring-up sequence into product code, behind IBoard, so core/ never depends
// on the real BSP.
//
// Kept dependency-free of ESP-IDF/BSP headers here on purpose (mirrors
// mock_board.hpp) - only waveshare_board.cpp pulls in the real driver
// headers. This keeps app_main.cpp host-checkable even though
// waveshare_board.cpp itself is hardware-only (see tools/scripts/host_check.sh).
//
// network_ready here means the ESP-Hosted/SDIO transport to the ESP32-C6 is
// up (Gate 9) - it does NOT mean a Wi-Fi AP is associated. Connecting to a
// real network with credentials is a service-layer concern (Fase 5).
#pragma once

#include "mock_board.hpp"  // IBoard, BoardStatus

namespace nova {

class WaveshareBoard : public IBoard {
public:
    const char* name() const override { return "WaveshareBoard"; }
    BoardStatus bring_up() override;
    BoardStatus status() const override { return status_; }

    // Real BSP/LVGL mutex - the official BSP runs its own LVGL port task;
    // any other code touching lv_obj_* must hold this first.
    bool lock(uint32_t timeout_ms) override;
    void unlock() override;

private:
    BoardStatus status_{};
};

}  // namespace nova
