#ifndef AIRBAG_FD_H
#define AIRBAG_FD_H

/**
 * @mainpage
 * Drop-in crash handlers for POSIX, particularly embedded Linux.
 * Please see @ref airbag_fd.h for more details.
 *
 * @file airbag_fd.h
 * @brief Drop-in crash handlers for POSIX, particularly embedded Linux.
 *
 * Dumps registers, backtrace, and instruction stream to a file descriptor.  Intended to be
 * self-contained and resilient.  Where possible, will detect and intelligently handle corrupt
 * state, such as jumping through a bad pointer or a blown stack.  The harvesting and reporting
 * of the crash log is left as an exercise for the reader.
 *
 * The common case requires no <tt>\#defines</tt>.  Optional defines:
 * - AIRBAG_NO_PTHREADS
 * - AIRBAG_NO_DLADDR
 * - AIRBAG_NO_BACKTRACE
 *
 * Should compile as C or C++.  C++ users are covered; airbag_fd catches SIGABRT.  By default,
 * std::terminate and std::unexpected abort() the program.  Be sure to compile as C++ if you
 * want name demangling.
 *
 * @todo
 * - chaining handlers?
 * - better symbols on x86-64
 * - improve GCC's unwind with bad PC, blown stack, etc
 * - test on more OSs: bsd
 * - if failed to get any backtrace, scan /proc/pid/maps for library offsets
 * - stop other threads, get their backtraces
 * - expose airbag_walkstack
 * - arm: thumb mode
 * - arm: http://www.mcternan.me.uk/ArmStackUnwinding/
 * @sa https://github.com/ccoffing/airbag_fd
 * @author Chuck Coffing <clc@alum.mit.edu>
 * @copyright Copyright 2011-2014 Chuck Coffing <clc@alum.mit.edu>, MIT licensed
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Optional user callback, to print additional state at time of crash (build #, uptime, etc).
 */
typedef void (*airbag_user_callback)(int fd);

/** Registers crash handlers to output to the file descriptor.
 * @return 0 iff registered; else errno is set.
 */
int airbag_init_fd(int fd, airbag_user_callback cb,const char* execname);

/** Registers crash handlers to output to the named file.  The file is created only if and when
 * a crash occurs.
 * @return 0 iff registered; else errno is set.
 */
int airbag_init_filename(const char *filename, airbag_user_callback cb,const char* execname);

/** Names the current thread.
 * @return 0 iff name is set; else errno is set.
 */
int airbag_name_thread(const char *name);


/** Looks up the file name, function name, and offset corresponding to pc.
 * Writes text representation to fd.
 */
void airbag_symbol(int fd, void *pc);

/** Deregisters the crash handlers.
 */
void airbag_deinit();

#ifdef __cplusplus
}
#endif


#endif
