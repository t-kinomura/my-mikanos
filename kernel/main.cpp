extern "C" void KernelMain() {
    // __asm__(): インラインアセンブラ
    // hlt: cpuを停止させ省電力な状態にする. 割り込みがあると再開するけどこのプログラムでは割り込みは起こらない.
    while (1) __asm__("hlt");
}