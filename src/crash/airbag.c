/*
 * Copyright (C) 2011-2014 Chuck Coffing <clc@alum.mit.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "stacktrace.h"
#include "safecall.h"

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <arpa/inet.h>  /* for htonl */
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#include <sys/syscall.h>
#endif
#if !defined(AIRBAG_NO_BACKTRACE)
#include <execinfo.h>
#endif
#if !defined(AIRBAG_NO_PTHREAD)
#if defined(__FreeBSD__)
#include <pthread.h>
#include <pthread_np.h>
#endif
#endif


#define AIRBAG_EXPORT
#ifndef O_CLOEXEC
#define O_CLOEXEC 0  /* Supported starting in Linux 2.6.23 */
#endif


typedef void (*airbag_user_callback)(int fd);

static int s_fd = -1;
static const char *s_filename;
static const char *s_execname;
static airbag_user_callback s_cb;
#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
static uint32_t busy, pending;
#endif

#if defined(USE_GCC_DEMANGLE)
static char *s_demangleBuf;
static size_t s_demangleBufLen;
#endif

#define ALT_STACK_SIZE (MINSIGSTKSZ + 256 * sizeof(void *))  /* or let it all hang out: SIGSTKSZ */
static void *s_altStackSpace;

static const char comment[] = "# ";
static const char section[] = "=== ";
static const char unknown[] = "\?\?\?";
static const char termBt[] = "terminating backtrace";

#define MAX_SIGNALS 32
static const char *sigNames[MAX_SIGNALS] =
{
    /*[0        ] =*/ NULL,
    /*[SIGHUP   ] =*/ "HUP",
    /*[SIGINT   ] =*/ "INT",
    /*[SIGQUIT  ] =*/ "QUIT",
    /*[SIGILL   ] =*/ "ILL",
    /*[SIGTRAP  ] =*/ "TRAP",
    /*[SIGABRT  ] =*/ "ABRT",
    /*[SIGBUS   ] =*/ "BUS",
    /*[SIGFPE   ] =*/ "FPE",
    /*[SIGKILL  ] =*/ "KILL",
    /*[SIGUSR1  ] =*/ "USR1",
    /*[SIGSEGV  ] =*/ "SEGV",
    /*[SIGUSR2  ] =*/ "USR2",
    /*[SIGPIPE  ] =*/ "PIPE",
    /*[SIGALRM  ] =*/ "ALRM",
    /*[SIGTERM  ] =*/ "TERM",
    /*[SIGSTKFLT] =*/ "STKFLT",
    /*[SIGCHLD  ] =*/ "CHLD",
    /*[SIGCONT  ] =*/ "CONT",
    /*[SIGSTOP  ] =*/ "STOP",
    /*[SIGTSTP  ] =*/ "TSTP",
    /*[SIGTTIN  ] =*/ "TTIN",
    /*[SIGTTOU  ] =*/ "TTOU",
    /*[SIGURG   ] =*/ "URG",
    /*[SIGXCPU  ] =*/ "XCPU",
    /*[SIGXFSZ  ] =*/ "XFSZ",
    /*[SIGVTALRM] =*/ "VTALRM",
    /*[SIGPROF  ] =*/ "PROF",
    /*[SIGWINCH ] =*/ "WINCH",
    /*[SIGIO    ] =*/ "IO",
    /*[SIGPWR   ] =*/ "PWR",
    /*[SIGSYS   ] =*/ "SYS"
};

/*
 * Do not use strsignal; it is not async signal safe.
 */
static const char *_strsignal(int sigNum)
{
    return sigNum < 1 || sigNum >= MAX_SIGNALS ? unknown : sigNames[sigNum];
}


#if defined(__x86_64__)
#define NMCTXREGS NGREG
#define MCTXREG(uc, i) (uc->uc_mcontext.gregs[i])
#define MCTX_PC(uc) MCTXREG(uc, 16)
static const char *mctxRegNames[NMCTXREGS] =
{
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15", "RDI", "RSI", "RBP", "RBX",
    "RDX", "RAX", "RCX", "RSP", "RIP", "EFL", "CSGSFS", "ERR", "TRAPNO", "OLDMASK", "CR2"
};
#elif defined(__i386__)
#define NMCTXREGS NGREG
#define MCTXREG(uc, i) (uc->uc_mcontext.gregs[i])
#define MCTX_PC(uc) MCTXREG(uc, 14)
static const char *mctxRegNames[NMCTXREGS] =
{
    "GS", "FS", "ES", "DS", "EDI", "ESI", "EBP", "ESP", "EBX", "EDX",
    "ECX", "EAX", "TRAPNO", "ERR", "EIP", "CS", "EFL", "UESP", "SS"
};
#elif defined(__arm__)
#define NMCTXREGS 21
#define MCTXREG(uc, i) (((unsigned long *)(&uc->uc_mcontext))[i])
#define MCTX_PC(uc) MCTXREG(uc, 18)
static const char *mctxRegNames[NMCTXREGS] =
{
    "TRAPNO", "ERRCODE", "OLDMASK", "R0", "R1", "R2", "R3", "R4", "R5", "R6",
    "R7", "R8", "R9", "R10", "FP", "IP", "SP", "LR", "PC", "CPSR", "FAULTADDR"
};
static const int gregOffset = 3;
#elif defined(__mips__)
#define NMCTXREGS NGREG
#define MCTXREG(uc, i) (uc->uc_mcontext.gregs[i])
#define MCTX_PC(uc) (uc->uc_mcontext.pc)
static const char *mctxRegNames[NMCTXREGS] =
{
    "ZERO", "AT", "V0", "V1", "A0", "A1", "A2", "A3",
    #if _MIPS_SIM == _ABIO32
    "T0", "T1", "T2", "T3",
    #else
    "A4", "A5", "A6", "A7",
    #endif
    "T4", "T5", "T6", "T7",
    "S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7", "T8", "T9", "K0", "K1", "GP", "SP", "FP", "RA"
};
#endif

#if 0
#if defined(__MACH__)
#if __DARWIN_UNIX03
#if defined(__i386__)
pnt = (void *)uc->uc_mcontext->__ss.__eip;
#elif defined(__arm__)
/* don't see mcontext in iphone headers... */
#else
/* pnt = (void*) uc->uc_mcontext->__ss.__srr0; */
#endif
#else
#if defined(__i386__)
pnt = (void *)uc->uc_mcontext->ss.eip;
#elif defined(__arm__)
/* don't see mcontext in iphone headers... */
#else
pnt = (void *)uc->uc_mcontext->ss.srr0;
#endif
#endif
#elif defined(__FreeBSD__)
#if defined(__i386__)
pnt = (void *)uc->uc_mcontext.mc_eip;
#elif defined(__x86_64__)
pnt = (void *)uc->uc_mcontext.mc_rip;
#endif
#elif (defined (__ppc__)) || (defined (__powerpc__))
pnt = (void *)uc->uc_mcontext.regs->nip;
#elif defined(__i386__)
pnt = (void *)uc->uc_mcontext.gregs[REG_EIP];
#elif defined(__x86_64__)
pnt = (void *)uc->uc_mcontext.gregs[REG_RIP];
#elif defined(__mips__)
#ifdef CTX_EPC  /* Pre-2007 uclibc */
pnt = (void *)uc->uc_mcontext.gpregs[CTX_EPC];
#else
pnt = (void *)uc->uc_mcontext.pc;
#endif
#endif
#endif

static uint8_t load8(const void *_p, uint8_t *v)
{
    static int fds[2] = { -1, -1 };
    uint8_t b;
    int r;
    const uint8_t *p = (const uint8_t *)_p;

    if (fds[0] == -1) {
        r = pipe(fds);
        (void)r; /* even on failure, degrades gracefully if memory is readable */
    }

    if (v)
        *v = 0;
    errno = 0;
    while ((r = write(fds[1], p, 1)) < 1 && errno == EINTR) {
        ;
    }
    if (r == 1) {
        while ((r = read(fds[0], v ? v : &b, 1)) < 1 && errno == EINTR) {
            ;
        }
        if (r == 1)
            return 0;
    }
    if (errno == EFAULT)
        return 0xff;
    if (v)
        *v = *p;  /* Risk it... */
    return 0;
}

static uint32_t load32(const void *_p, uint32_t *_v)
{
    int i;
    uint32_t r = 0;
    uint32_t v = 0;
    const uint8_t *p = (const uint8_t *)_p;

    for (i = 0; i < 4; ++i) {
        uint8_t b;
        r <<= 8;
        v <<= 8;
        r |= load8(p + i, &b);
        v |= b;
    }
    v = htonl(v);
    r = htonl(r);
    if (_v)
        *_v = v;
    return r;
}

static const char *demangle(const char *mangled)
{
    if (!mangled)
        return unknown;
    return mangled;
}

static void _airbag_symbol(int fd, void *pc, const char *sname, void *saddr)
{
    int printed = 0;

#if !defined(AIRBAG_NO_DLADDR)
    Dl_info info;
    if (dladdr(pc, &info)) {
        int offset;
        if (info.dli_sname && info.dli_saddr) {
            sname = info.dli_sname;
            saddr = info.dli_saddr;
        } else if (!sname || !saddr) {  /* dladdr and heuristic both failed; offset from start of so */
            sname = "";
            saddr = info.dli_fbase;
        }
        offset = (ptrdiff_t)pc - (ptrdiff_t)saddr;
        safe_printf(fd, "%s[%x](%s+%x)[%x]", info.dli_fname, info.dli_fbase, demangle(sname), offset, pc);
        printed = 1;
    }
#endif
    if (!printed) {
        safe_printf(fd, "%s(+0)[%x]", unknown, pc);
    }
}

AIRBAG_EXPORT
void airbag_symbol(int fd, void *pc)
{
    _airbag_symbol(fd, pc, 0, 0);
}

#ifdef __arm__
/*
 * Search for function name embedded via gcc's -mpoke-function-name.
 * addr should point near the top of the function.
 * If found, populates fname and returns pointer to start of function.
 */
static void *getPokedFnName(int fd, uint32_t addr, char *fname)
{
    unsigned int i;
    void *faddr = 0;

    addr &= ~3;
    /* GCC man page suggests len is at "pc - 12", but prologue can vary, so scan back */
    for (i = 0; i < 16; ++i) {
        uint32_t len;
        addr -= 4;
        if (load32((void *)addr, &len) == 0 && (len & 0xffffff00) == 0xff000000) {
            uint32_t offset;
            len &= 0xff;
            faddr = (void *)(addr + 4);
            addr -= len;
            for (offset = 0; offset < len; ++offset) {
                uint8_t c;
                if (load8((void *)(addr + offset), &c))
                    break;
                fname[offset] = c;
            }
            fname[offset] = 0;
            safe_printf(fd, "%sFound poked function name: %s[%x]\n", comment, fname, faddr);
            break;
        }
    }
    return faddr;
}
#endif

static int airbag_walkstack(int fd, void **buffer, int *repeat, int size, ucontext_t *uc)
{
    (void)fd;
    (void)uc;

    memset(repeat, 0, sizeof(int) * size);
#if defined(__mips__)
    /* Algorithm derived from:
     * http://elinux.org/images/6/68/ELC2008_-_Back-tracing_in_MIPS-based_Linux_Systems.pdf
     */
    uint32_t *addr, *pc, *ra, *sp;
    unsigned int raOffset, stackSize;
    uint32_t invalid;

    pc = (uint32_t *)uc->uc_mcontext.pc;
    ra = (uint32_t *)uc->uc_mcontext.gregs[31];
    sp = (uint32_t *)uc->uc_mcontext.gregs[29];

    int depth = 0;
    buffer[depth++] = pc;
    if (size == 1)
        return depth;

    /* Scanning to find the size of the current stack frame */
    raOffset = stackSize = 0;
    for (addr = pc; !raOffset || !stackSize; --addr) {
        uint32_t v;
        if (load32(addr, &v)) {
            safe_printf(fd, "%sText at %x is not mapped; trying prior frame pointer.\n", comment, addr);
            uc->uc_mcontext.pc = (uint32_t)ra;
            goto backward;
        }
        switch (v & 0xffff0000) {
        case 0x27bd0000:      /* addiu   sp,sp,??? */
            stackSize = abs((short)(v & 0xffff));
            safe_printf(fd, "%s[%08x]: stack size %u\n", comment, addr, stackSize);
            break;
        case 0xafbf0000:      /* sw      ra,???(sp) */
            raOffset = (v & 0xffff);
            safe_printf(fd, "%s[%08x]: ra offset %u\n", comment, addr, raOffset);
            break;
        case 0x3c1c0000:      /* lui     gp,??? */
            goto out;
        default:
            break;
        }
    }
out:
    if (raOffset) {
        uint32_t *newRa;
        if (load32((uint32_t *)((uint32_t)sp + raOffset), (uint32_t *)&newRa))
            safe_printf(fd, "%sText at RA <- SP[raOffset] %x[%x] is not mapped; assuming blown stack.\n", comment, sp, raOffset);
        else
            ra = newRa;
    }
    if (stackSize)
        sp = (uint32_t *)((uint32_t)sp + stackSize);

backward:
    while (depth < size && ra) {
        if (buffer[depth - 1] == ra)
            repeat[depth - 1]++;
        else
            buffer[depth++] = ra;
        raOffset = stackSize = 0;
        for (addr = ra; !raOffset || !stackSize; --addr) {
            uint32_t v;
            if (load32(addr, &v)) {
                safe_printf(fd, "%sText at %x is not mapped; %s.\n", comment, addr, termBt);
                return depth;
            }
            switch (v & 0xffff0000) {
            case 0x27bd0000:      /* addiu   sp,sp,??? */
                stackSize = abs((short)(v & 0xffff));
                safe_printf(fd, "%s[%08x]: stack size %u\n", comment, addr, stackSize);
                break;
            case 0xafbf0000:      /* sw      ra,???(sp) */
                raOffset = (v & 0xffff);
                safe_printf(fd, "%s[%08x]: ra offset %u\n", comment, addr, raOffset);
                break;
            case 0x3c1c0000:      /* lui     gp,??? */
                return depth + 1;
            default:
                break;
            }
        }
        if (load32((uint32_t *)((uint32_t)sp + raOffset), (uint32_t *)&ra)) {
            safe_printf(fd, "%sText at RA <- SP[raOffset] %x[%x] is not mapped; %s.\n", comment, sp, raOffset, termBt);
            break;
        }
        sp = (uint32_t *)((uint32_t)sp + stackSize);
    }
    return depth;
#elif 0
    /*defined(__arm__)*/
    uint32_t pc = MCTX_PC(uc);
    uint32_t fp = MCTXREG(uc, 14);
    uint32_t lr = MCTXREG(uc, 17);
    int depth = 0;
    char fname[257];

    if (pc & 3 || load32((void *)pc, NULL)) {
        safe_printf(fd, "%sCalled through bad function pointer; assuming PC <- LR.\n", comment);
        pc = MCTX_PC(uc) = lr;
    }
    buffer[depth] = (void *)pc;

    /* Heuristic for gcc-generated code:
     *  - Know PC, FP for current frame.
     *  - Scan backwards from PC to find the push of prior FP.  This is the function's prologue.
     *  - Sometimes there's a prior push instruction to account for.
     *  - Load registers from start of frame based on the push instruction(s).
     *  - Leaf functions might not push LR.
     * TODO: what if lr&3 or priorPc&3
     * TODO: return heuristic fname faddr?
     */
    while (++depth < size) {
        /*
         * CondOp-PUSWLRn--Register-list---
         * 1110100???101101????????????????
         * Unconditional, op is "load/store multiple", "W" bit because SP is updated,
         * not "L" bit because is store, to SP
         */
        const uint32_t stmBits = 0xe82d0000;
        const uint32_t stmMask = 0xfe3f0000;
        int found = 0;
        int i;

        safe_printf(fd, "%sSearching frame %u (FP=%x, PC=%x)\n", comment, depth - 1, fp, pc);

        for (i = 0; i < 8192 && !found; ++i) {
            uint32_t instr, instr2;
            if (load32((void *)(pc - i * 4), &instr2)) {
                safe_printf(fd, "%sInstruction at %x is not mapped; %s.\n", comment, pc - i * 4, termBt);
                return depth;
            }
            if ((instr2 & (stmMask | (1 << 11))) == (stmBits | (1 << 11))) {
                void *faddr = 0;
                uint32_t priorPc = lr;  /* If LR was pushed, will find and use that.  For now assume leaf function. */
                uint32_t priorFp;
                found = 1;
                i++;
                if (load32((void *)(pc - i * 4), &instr) == 0 && (instr & stmMask) == stmBits) {
                    int pushes, dir, pre, regNum;
checkStm:
                    pushes = 0;
                    dir = (instr & (1 << 23)) ? 1 : -1; /* U bit: increment or decrement? */
                    pre = (instr & (1 << 24)) ? 1 : 0;  /* P bit: pre  TODO */
                    safe_printf(fd, "%sPC-%02x[%8x]: %8x stm%s%s sp!\n", comment, i * 4, pc - i * 4, instr,
                                pre == 1 ? "f" : "e", dir == 1 ? "a" : "d");
                    for (regNum = 15; regNum >= 0; --regNum) {
                        if (instr & (1 << regNum)) {
                            uint32_t reg;
                            if (load32((void *)(fp + pushes * 4 * dir), &reg)) {
                                safe_printf(fd, "%sStack at %x is not mapped; %s.\n", comment,
                                            fp + pushes * 4 * dir, termBt);
                                return depth;
                            }
                            safe_printf(fd, "%sFP%s%02x[%8x]: %8x {%s}\n", comment, dir == 1 ? "+" : "-", pushes * 4,
                                        fp + pushes * 4 * dir, reg, mctxRegNames[gregOffset + regNum]);
                            pushes++;
                            if (regNum == 11)
                                priorFp = reg;
                            else if (regNum == 14)
                                priorPc = reg;
                            else if (regNum == 15) {
                                /* When built with -mpoke-function-name, PC is also pushed in the
                                 * function prologue so that [FP] points near the top of the function
                                 * and the poked name can be found. */
                                faddr = getPokedFnName(fd, reg, fname);
                            }
                        }
                    }
                }
                if (instr2) {
                    i--;
                    instr = instr2;
                    instr2 = 0;
                    goto checkStm;
                }
                safe_printf(fd, "%s%s ", comment, depth == 1 ? "Crashed at" : "Called from");
                _airbag_symbol(fd, (void *)pc, fname, faddr);
                safe_printf(fd, "\n");
                pc = priorPc;
                fp = priorFp;
            }
        }

        if (!found) {
            safe_printf(fd, "%sFailed to find prior stack frame; %s.\n", comment, termBt);
            break;
        }
        if (buffer[depth - 1] == (void *)pc)
            repeat[depth - 1]++;
        else
            buffer[depth] = (void *)pc;
    }
    return depth;
#if defined(__mips__)
    MCTX_PC(uc) = MCTXREG(uc, 31);  /* RA */
#elif defined(__arm__)
    MCTX_PC(uc) = MCTXREG(uc, 17);  /* LR */
#elif defined(__i386__)
    /* TODO heuristic for -fomit-frame-pointer? */
    uint8_t *fp = (uint8_t *)MCTXREG(uc, 6) + 4;
    uint32_t eip;
    if (load32((void *)fp, &eip)) {
        safe_printf(fd, "%sText at %x is not mapped; cannot get backtrace.\n", comment, fp);
        size = 0;
    } else {
        MCTX_PC(uc) = eip;
    }
#elif defined(__x86_64__)
    /* TODO x84_64 abi encourages not saving fp */
    size = 0;
#else
    size = 0;
#endif
}

if (size >= 1) {
    /* TODO: setjmp, catch SIGSEGV to longjmp back here, to more gracefully handle
                 * corrupted stack. */
    _unwind_Backtrace(backtrace_helper, &arg);
}
return arg.cnt != -1 ? arg.cnt : 0;
}
}
return 0;
#elif !defined(AIRBAG_NO_BACKTRACE)
    /*
     * Not preferred, because no way to explicitly start at failing PC, doesn't handle
     * bad PC, doesn't handle blown stack, etc.
     */
    return backtrace(buffer, size);
#else
    return 0;
#endif
}


static void printWhere(void *pc)
{
#if !defined(AIRBAG_NO_DLADDR)
    Dl_info info;
    if (dladdr(pc, &info)) {
        safe_printf(s_fd, " in %s\n", demangle(info.dli_sname));
        return;
    }
#endif
    safe_printf(s_fd, " at %x\n", pc);
}

#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
static uint64_t getNow()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}
#endif

static void sigHandler(int sigNum, siginfo_t *si, void *ucontext)
{
    ucontext_t *uc = (ucontext_t *)ucontext;
    const uint8_t *pc = (uint8_t *)MCTX_PC(uc);

#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
    __sync_fetch_and_add(&pending, 1);
    if (__sync_val_compare_and_swap(&busy, 0, 1) != 0) {
        uint64_t now, start = getNow();
        while (1) {
            usleep(1000);
            if (__sync_val_compare_and_swap(&busy, 0, 1) == 0)
                break;
            now = getNow();
            if (now < start || now > start + 1000000) {
                /* Timeout; perhaps another thread is recursively crashing or stuck
                 * in airbag_user_callback.  Shut it down now. */
                _exit(EXIT_FAILURE);
            }
        }
    }
#endif

    if (s_fd == -1 && s_filename){

        s_fd = open(s_filename, O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC | O_SYNC, 0600);
    }
    if (s_fd == -1)
        s_fd = 2;
    int fd = s_fd;

    safe_printf(STDERR_FILENO, "Caught SIG%s (%u)\n", _strsignal(sigNum), sigNum);
    safe_printf(STDERR_FILENO, "Collecting infomation about the crash...\n");

    safe_printf(fd, "Caught SIG%s (%u)", _strsignal(sigNum), sigNum);
    if (si->si_code == SI_USER)
        safe_printf(fd, " sent by user %u from process %u\n", si->si_uid, si->si_pid);
    else if (si->si_code == SI_TKILL)
        safe_printf(fd, " sent by tkill\n");
    else if (si->si_code == SI_KERNEL) {
        safe_printf(fd, " sent by kernel");  /* rare; well-behaved kernel gives us a real code, handled below */
        printWhere((void *)pc);
    } else {
        printWhere((void *)pc);

        const char *faultReason = 0;
        switch (sigNum) {
        case SIGABRT:
            break;
        case SIGBUS: {
            switch (si->si_code) {
            case BUS_ADRALN: faultReason = "invalid address alignment";
                break;
            case BUS_ADRERR: faultReason = "nonexistent physical address";
                break;
            case BUS_OBJERR: faultReason = "object-specific hardware error";
                break;
            default: faultReason = "unknown";
                break;
            }
            break;
        }
        case SIGFPE: {
            switch (si->si_code) {
            case FPE_INTDIV: faultReason = "integer divide by zero";
                break;
            case FPE_INTOVF: faultReason = "integer overflow";
                break;
            case FPE_FLTDIV: faultReason = "floating-point divide by zero";
                break;
            case FPE_FLTOVF: faultReason = "floating-point overflow";
                break;
            case FPE_FLTUND: faultReason = "floating-point underflow";
                break;
            case FPE_FLTRES: faultReason = "floating-point inexact result";
                break;
            case FPE_FLTINV: faultReason = "floating-point invalid operation";
                break;
            case FPE_FLTSUB: faultReason = "subscript out of range";
                break;
            default: faultReason = "unknown";
                break;
            }
            break;
        }
        case SIGILL: {
            switch (si->si_code) {
            case ILL_ILLOPC: faultReason = "illegal opcode";
                break;
            case ILL_ILLOPN: faultReason = "illegal operand";
                break;
            case ILL_ILLADR: faultReason = "illegal addressing mode";
                break;
            case ILL_ILLTRP: faultReason = "illegal trap";
                break;
            case ILL_PRVOPC: faultReason = "privileged opcode";
                break;
            case ILL_PRVREG: faultReason = "privileged register";
                break;
            case ILL_COPROC: faultReason = "coprocessor error";
                break;
            case ILL_BADSTK: faultReason = "stack error";
                break;
            default: faultReason = "unknown";
                break;
            }
            break;
        }
        case SIGINT:
            break;
        case SIGQUIT:
            break;
        case SIGTERM:
            break;
        case SIGSEGV: {
            switch (si->si_code) {
            case SEGV_MAPERR: faultReason = "address not mapped to object";
                break;
            case SEGV_ACCERR: faultReason = "invalid permissions for mapped object";
                break;
            default: faultReason = "unknown";
                break;
            }
            break;
        }
        }

        if (faultReason) {
            safe_printf(fd, "Fault at memory location 0x%x", si->si_addr);
            safe_printf(fd, " due to %s (%x).\n", faultReason, si->si_code);
        }
    }
#ifdef __linux__
    {
        char name[17];
        prctl(PR_GET_NAME, (unsigned long)name, 0, 0, 0);
        name[sizeof(name) - 1] = 0;
#ifdef SYS_gettid
        safe_printf(fd, "Thread %u: %s\n", syscall(SYS_gettid), name);
#else
        safe_printf(fd, "Thread: %s\n", name);
#endif
    }
#endif

#if 0
    /*
     * Usually unset and unused on Linux.  Note that strerror it not guaranteed to
     * be async-signal safe (it deals with the locale) so hit the array directly.
     * And yet the array is deprecated.  Bugger.
     */
    if (si->si_errno)
        safe_printf(fd, "Errno %u: %s.\n", si->si_errno, sys_errlist[si->si_errno]);
#endif

    safe_printf(fd, "%sContext:\n", section);
    int width = 0;
    int i;
    for (i = 0; i < NMCTXREGS; ++i) {
        if (!mctxRegNames[i])   /* Can trim junk per-arch by NULL-ing name. */
            continue;
        if (i) {
            if (width > 70) {
                safe_printf(fd, "\n");
                width = 0;
            } else
                width += safe_printf(fd, " ");
        }
        width += safe_printf(fd, "%s:%x", mctxRegNames[i], MCTXREG(uc, i));
    }
    safe_printf(fd, "\n");

    safe_printf(fd, "%sBinary:\n", section);
    safe_printf(fd, "Compile date: %s %s\n", __DATE__, __TIME__);
    safe_printf(fd, "GNU version: %u\n", __GNUC_VERSION__);

    {
        const int size = 32;
        void *buffer[size];
        int repeat[size];
        safe_printf(fd, "%sBacktrace:\n", section);
        int nptrs = airbag_walkstack(fd, buffer, repeat, size, uc);
        for (i = 0; i < nptrs; ++i) {
            airbag_symbol(fd, buffer[i]);
            if (repeat[i])
                safe_printf(fd, " (called %u times)", repeat[i] + 1);
            safe_printf(fd, "\n");
        }
        /* Reload PC; walkstack may have discovered better state. */
        pc = (uint8_t *)MCTX_PC(uc);
    }

    width = 0;
    ptrdiff_t bytes = 128;
#if defined(__x86_64__) || defined(__i386__)
    const uint8_t *startPc = pc;
    if (startPc < (uint8_t *)(bytes / 2))
        startPc = 0;
    else
        startPc = pc - bytes / 2;
    const uint8_t *endPc = startPc + bytes;
    const uint8_t *addr;
#else
    pc = (uint8_t *)(((uint32_t)pc) & ~3);
    const uint32_t *startPc = (uint32_t *)pc;
    if (startPc < (uint32_t *)(bytes / 2))
        startPc = 0;
    else
        startPc = (uint32_t *)(pc - bytes / 2);
    const uint32_t *endPc = (uint32_t *)((uint8_t *)startPc + bytes);
    const uint32_t *addr;
#endif
    safe_printf(fd, "%sCode:\n", section);
    for (addr = startPc; addr < endPc; ++addr) {
        if (width > 70) {
            safe_printf(fd, "\n");
            width = 0;
        }
        if (width == 0) {
            safe_printf(fd, "%x: ", addr);
        }
        width += safe_printf(fd, (const uint8_t *)addr == pc ? ">" : " ");
#if defined(__x86_64__) || defined(__i386__)
        uint8_t b;
        uint8_t invalid = load8(addr, &b);
        if (invalid)
            safe_printf(fd, "??");
        else
            safe_printf(fd, "%02x", b);
        width += 2;
#else
        uint32_t w;
        uint32_t invalid = load32(addr, &w);
        for (i = 3; i >= 0; --i) {
            int shift = i * 8;
            if ((invalid >> shift) & 0xff)
                safe_printf(fd, "??");
            else
                safe_printf(fd, "%02x", (w >> shift) & 0xff);
        }
        width += 8;
#endif
    }

    safe_printf(fd, "\n");
    printBacktrace(s_fd,s_execname);
    char sh[512] = {0};
    (void)sprintf(sh,"{ echo '%sSystem:'; cat /proc/version; cat /var/lib/dbus/machine-id; lsb_release -a; } >> %s", section, STACKTRACE_LOG);
    (void)system(sh);

    if (s_cb)
        s_cb(fd);

    /* Do not use abort(): Would re-raise SIGABRT. */
    /* Do not use exit(): Would run atexit handlers. */
    /* For threads: pthread_exit is not async-signal-safe. */
    /* Only option is to (optionally) wait and then _exit. */

#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
    __sync_fetch_and_add(&busy, -1);
    if (__sync_fetch_and_add(&pending, -1) > 1) {
        uint64_t now, start = getNow();
        do {
            usleep(1000);
            now = getNow();
            if (now < start || now > start + 1000000)
                break;
        } while (__sync_fetch_and_add(&pending, 0) > 0);
    }
#endif

    _exit(EXIT_FAILURE);
}

static int initCrashHandlers()
{
#if defined(USE_GCC_DEMANGLE)
    if (!s_demangleBuf) {
        s_demangleBufLen = 512;
        s_demangleBuf = (char *)malloc(s_demangleBufLen);
        if (!s_demangleBuf)
            return -1;
    }
#endif

    if (!s_altStackSpace) {
        stack_t altStack;
        s_altStackSpace = (void *)malloc(ALT_STACK_SIZE);
        if (!s_altStackSpace)
            return -1;
        altStack.ss_sp = s_altStackSpace;
        altStack.ss_flags = 0;
        altStack.ss_size = ALT_STACK_SIZE;
        if (sigaltstack(&altStack, NULL) != 0) {
            free(s_altStackSpace);
            s_altStackSpace = 0;
            return -1;
        }
    }

    sigset_t mysigset;
    sigemptyset(&mysigset);
    sigaddset(&mysigset, SIGABRT);
    sigaddset(&mysigset, SIGBUS);
    sigaddset(&mysigset, SIGILL);
    sigaddset(&mysigset, SIGSEGV);
    sigaddset(&mysigset, SIGFPE);

    struct sigaction sa;
    sa.sa_sigaction = sigHandler;
    sa.sa_mask = mysigset;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;

    sigaction(SIGABRT, &sa, 0);
    sigaction(SIGBUS, &sa, 0);
    sigaction(SIGILL, &sa, 0);
    sigaction(SIGSEGV, &sa, 0);
    sigaction(SIGFPE, &sa, 0);

    return 0;
}


static void deinitCrashHandlers()
{
    struct sigaction sa;
    sigset_t mysigset;

    sigemptyset(&mysigset);

    sa.sa_handler = SIG_DFL;
    sa.sa_mask = mysigset;
    sa.sa_flags = 0;

    sigaction(SIGABRT, &sa, 0);
    sigaction(SIGBUS, &sa, 0);
    sigaction(SIGILL, &sa, 0);
    sigaction(SIGSEGV, &sa, 0);
    sigaction(SIGFPE, &sa, 0);

    if (s_altStackSpace) {
        stack_t altStack;
        altStack.ss_sp = 0;
        altStack.ss_flags = SS_DISABLE;
        altStack.ss_size = 0;
        sigaltstack(&altStack, NULL);
        free(s_altStackSpace);
        s_altStackSpace = 0;
    }
}


AIRBAG_EXPORT
int airbag_init_fd(int fd, airbag_user_callback cb,const char* execname)
{
    s_fd = fd;
    s_filename = 0;
    s_cb = cb;
    s_execname = execname;
    return initCrashHandlers();
}

AIRBAG_EXPORT
int airbag_init_filename(const char *filename, airbag_user_callback cb,const char* execname)
{
    s_fd = -1;
    s_filename = filename;
    s_cb = cb;
    s_execname = execname;
    return initCrashHandlers();
}

AIRBAG_EXPORT
int airbag_name_thread(const char *name)
{
#if defined(__linux__)
    prctl(PR_SET_NAME, (unsigned long)name);
    return 0;
#elif defined(__FreeBSD__) && !defined(AIRBAG_NO_PTHREAD)
    pthread_set_name_np(pthread_self(), name);
    return 0;
#else
    (void)name;
    errno = ENOTSUP;
    return -1;
#endif
}

AIRBAG_EXPORT
void airbag_deinit()
{
    s_fd = -1;
    s_filename = 0;
    s_cb = 0;
    deinitCrashHandlers();
}
