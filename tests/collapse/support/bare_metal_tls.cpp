/** Link-time support for bare-metal evidence builds (collapse_evidence.py, cm33 builds only)
 *
 * Code using thread_local on ARM EABI calls __aeabi_read_tp, which a bare-metal image must provide (an RTOS
 * port normally does: Zephyr with CONFIG_THREAD_LOCAL_STORAGE). This single-thread stub lets such images link
 * so their final ELF can be analysed; it is NOT a runnable TLS implementation (no .tdata initialisation).
 * The evidence report still lists "TLS" as a dependency of any variant that pulls this symbol in; variants
 * that do not use TLS let --gc-sections discard it.
 */
#if defined(__arm__)
alignas(8) static unsigned char tlsBlock[256];

extern "C" __attribute__((naked, used)) void* __aeabi_read_tp()
{
    // EABI: result in r0, every other register preserved
    asm volatile("ldr r0, =%c0\n\tbx lr" : : "i"(tlsBlock));
}
#endif
