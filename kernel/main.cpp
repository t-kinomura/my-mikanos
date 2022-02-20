#include <cstdint>

extern "C" void KernelMain(uint64_t frame_buffer_base, uint64_t frame_buffer_size) {
    uint8_t* frame_buffer = reinterpret_cast<uint8_t*>(frame_buffer_base);
    for(uint64_t i = 0; i < frame_buffer_size; ++i) {
        frame_buffer[i] = i % 256;
    }
    // __asm__(): インラインアセンブラ
    // hlt: cpuを停止させ省電力な状態にする. 割り込みがあると再開するけどこのプログラムでは割り込みは起こらない.
    while (1) __asm__("hlt");
}