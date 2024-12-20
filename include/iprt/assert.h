/** @file
 * IPRT - Assertions.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_assert_h
#define IPRT_INCLUDED_assert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/stdarg.h>
#include <iprt/assertcompile.h>

/** @defgroup grp_rt_assert     Assert - Assertions
 * @ingroup grp_rt
 *
 * Assertions are generally used to check preconditions and other
 * assumptions. Sometimes it is also used to catch odd errors or errors
 * that one would like to inspect in the debugger. They should not be
 * used for errors that happen frequently.
 *
 * IPRT provides a host of assertion macros, so many that it can be a bit
 * overwhelming at first. Don't despair, there is a system (surprise).
 *
 * First there are four families of assertions:
 *      - Assert        - The normal strict build only assertions.
 *      - AssertLogRel  - Calls LogRel() in non-strict builds, otherwise like Assert.
 *      - AssertRelease - Triggers in all builds.
 *      - AssertFatal   - Triggers in all builds and cannot be continued.
 *
 * Then there are variations wrt to argument list and behavior on failure:
 *      - Msg           - Custom RTStrPrintf-like message with the assertion message.
 *      - Return        - Return the specific rc on failure.
 *      - ReturnVoid    - Return (void) on failure.
 *      - Break         - Break (out of switch/loop) on failure.
 *      - Stmt          - Execute the specified statement(s) on failure.
 *      - RC            - Assert RT_SUCCESS.
 *      - RCSuccess     - Assert VINF_SUCCESS.
 *
 * @remarks As you might have noticed, the macros don't follow the
 * coding guidelines wrt to macros supposedly being all uppercase
 * and underscored. For various  reasons they don't, and nobody
 * has complained yet. Wonder why... :-)
 *
 * @remarks Each project has its own specific guidelines on how to use
 * assertions, so the above is just trying to give you the general idea
 * from the IPRT point of view.
 *
 * @{
 */

RT_C_DECLS_BEGIN

#if  !defined(IPRT_WITHOUT_ASSERT_STACK) \
  && defined(IN_RING3) \
  && !defined(IN_RT_STATIC) /* try keep static binaries small */ \
  && (defined(RT_ARCH_AMD64) /*|| defined(RT_ARCH_X86)*/)
/** @def IPRT_WITH_ASSERT_STACK
 * Indicates that we collect a callstack stack on assertion. */
# define IPRT_WITH_ASSERT_STACK
#endif

/**
 * The 1st part of an assert message.
 *
 * @param   pszExpr     Expression. Can be NULL.
 * @param   uLine       Location line number.
 * @param   pszFile     Location file name.
 * @param   pszFunction Location function name.
 */
RTDECL(void)    RTAssertMsg1(const char *pszExpr, unsigned uLine, const char *pszFile, const char *pszFunction);
/**
 * Weak version of RTAssertMsg1 that can be overridden locally in a module to
 * modify, redirect or otherwise mess with the assertion output.
 *
 * @copydoc RTAssertMsg1
 */
RTDECL(void)    RTAssertMsg1Weak(const char *pszExpr, unsigned uLine, const char *pszFile, const char *pszFunction);

/**
 * The 2nd (optional) part of an assert message.
 *
 * @param   pszFormat   Printf like format string.
 * @param   ...         Arguments to that string.
 */
RTDECL(void)    RTAssertMsg2(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);
/**
 * Weak version of RTAssertMsg2 that forwards to RTAssertMsg2WeakV.
 *
 * There is not need to override this, check out RTAssertMsg2WeakV instead!
 *
 * @copydoc RTAssertMsg2
 */
RTDECL(void)    RTAssertMsg2Weak(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * The 2nd (optional) part of an assert message.
 *
 * @param   pszFormat   Printf like format string.
 * @param   va          Arguments to that string.
 */
RTDECL(void)    RTAssertMsg2V(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);
/**
 * Weak version of RTAssertMsg2V that can be overridden locally in a module to
 * modify, redirect or otherwise mess with the assertion output.
 *
 * @copydoc RTAssertMsg2V
 */
RTDECL(void)    RTAssertMsg2WeakV(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);

/**
 * Additional information which should be appended to the 2nd part of an
 * assertion message.
 *
 * @param   pszFormat   Printf like format string.
 * @param   ...         Arguments to that string.
 */
RTDECL(void)    RTAssertMsg2Add(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);
/**
 * Weak version of RTAssertMsg2Add that forwards to RTAssertMsg2AddWeakV.
 *
 * There is not need to override this, check out RTAssertMsg2AddWeakV instead!
 *
 * @copydoc RTAssertMsg2Add
 */
RTDECL(void)    RTAssertMsg2AddWeak(const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(1, 2);

/**
 * Additional information which should be appended to the 2nd part of an
 * assertion message.
 *
 * @param   pszFormat   Printf like format string.
 * @param   va          Arguments to that string.
 */
RTDECL(void)    RTAssertMsg2AddV(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);
/**
 * Weak version of RTAssertMsg2AddV that can be overridden locally in a module
 * to modify, redirect or otherwise mess with the assertion output.
 *
 * @copydoc RTAssertMsg2AddV
 */
RTDECL(void)    RTAssertMsg2AddWeakV(const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(1, 0);

#ifdef IN_RING0
/**
 * Panics the system as the result of a fail assertion.
 */
RTR0DECL(void)  RTR0AssertPanicSystem(void);
#endif /* IN_RING0 */

/**
 * Overridable function that decides whether assertions executes the panic
 * (breakpoint) or not.
 *
 * The generic implementation will return true.
 *
 * @returns true if the breakpoint should be hit, false if it should be ignored.
 *
 * @remark  The RTDECL() makes this a bit difficult to override on Windows. So,
 *          you'll have to use RTASSERT_HAVE_SHOULD_PANIC or
 *          RTASSERT_HAVE_SHOULD_PANIC_PRIVATE there to control the kind of
 *          prototype.
 */
#if !defined(RTASSERT_HAVE_SHOULD_PANIC) && !defined(RTASSERT_HAVE_SHOULD_PANIC_PRIVATE)
RTDECL(bool)    RTAssertShouldPanic(void);
#elif defined(RTASSERT_HAVE_SHOULD_PANIC_PRIVATE)
bool            RTAssertShouldPanic(void);
#else
DECLEXPORT(bool) RTCALL RTAssertShouldPanic(void);
#endif

/**
 * Controls whether the assertions should be quiet or noisy (default).
 *
 * @returns The old setting.
 * @param   fQuiet              The new setting.
 */
RTDECL(bool)    RTAssertSetQuiet(bool fQuiet);

/**
 * Are assertions quiet or noisy?
 *
 * @returns True if they are quiet, false if noisy.
 */
RTDECL(bool)    RTAssertAreQuiet(void);

/**
 * Makes the assertions panic (default) or not.
 *
 * @returns The old setting.
 * @param   fPanic              The new setting.
 */
RTDECL(bool)    RTAssertSetMayPanic(bool fPanic);

/**
 * Can assertion panic.
 *
 * @returns True if they can, false if not.
 */
RTDECL(bool)    RTAssertMayPanic(void);


/** @name Globals for crash analysis
 * @remarks     This is the full potential set, it
 * @{
 */
/** The last assertion message, 1st part. */
extern RTDATADECL(char)                     g_szRTAssertMsg1[1024];
/** The last assertion message, 2nd part. */
extern RTDATADECL(char)                     g_szRTAssertMsg2[4096];
#ifdef IPRT_WITH_ASSERT_STACK
/** The last assertion message, stack part. */
extern RTDATADECL(char)                     g_szRTAssertStack[4096];
#endif
/** The last assertion message, expression. */
extern RTDATADECL(const char * volatile)    g_pszRTAssertExpr;
/** The last assertion message, file name. */
extern RTDATADECL(const char * volatile)    g_pszRTAssertFile;
/** The last assertion message, line number. */
extern RTDATADECL(uint32_t volatile)        g_u32RTAssertLine;
/** The last assertion message, function name. */
extern RTDATADECL(const char * volatile)    g_pszRTAssertFunction;
/** @} */

RT_C_DECLS_END

/** @def RTAssertDebugBreak()
 * Debugger breakpoint instruction.
 *
 * @remarks This macro does not depend on RT_STRICT.
 */
#define RTAssertDebugBreak()    do { RT_BREAKPOINT(); } while (0)



/** @name Assertions
 *
 * These assertions will only trigger when RT_STRICT is defined. When it is
 * undefined they will all be no-ops and generate no code.
 *
 * @{
 */


/** @def RTASSERT_QUIET
 * This can be defined to shut up the messages for a file where this would be
 * problematic because the message printing code path passes thru it.
 * @internal */
#ifdef DOXYGEN_RUNNING
# define RTASSERT_QUIET
#endif
#if defined(RTASSERT_QUIET) && !defined(DOXYGEN_RUNNING)
# define RTAssertMsg1Weak(pszExpr, uLine, pszfile, pszFunction) \
                                do { } while (0)
# ifdef RT_COMPILER_SUPPORTS_VA_ARGS
#  define RTAssertMsg2Weak(...) do { } while (0)
# else
#  define RTAssertMsg2Weak      if (1) {} else RTAssertMsg2Weak
# endif
#endif

/** @def RTAssertDoPanic
 * Raises an assertion panic appropriate to the current context.
 * @remarks This macro does not depend on RT_STRICT.
 */
#if defined(IN_RING0) \
 && (defined(RT_OS_DARWIN) || defined(RT_OS_HAIKU) || defined(RT_OS_SOLARIS))
# define RTAssertDoPanic()      RTR0AssertPanicSystem()
#else
# define RTAssertDoPanic()      RTAssertDebugBreak()
#endif

/** @def AssertBreakpoint()
 * Assertion Breakpoint.
 * @deprecated Use RTAssertPanic or RTAssertDebugBreak instead.
 */
#ifdef RT_STRICT
# define AssertBreakpoint()     RTAssertDebugBreak()
#else
# define AssertBreakpoint()     do { } while (0)
#endif

/** @def RTAssertPanic()
 * If RT_STRICT is defined this macro will invoke RTAssertDoPanic if
 * RTAssertShouldPanic returns true. If RT_STRICT isn't defined it won't do any
 * thing.
 */
#if defined(RT_STRICT) && !defined(RTASSERT_DONT_PANIC)
# define RTAssertPanic()        do { if (RTAssertShouldPanic()) RTAssertDoPanic(); } while (0)
#else
# define RTAssertPanic()        do { } while (0)
#endif

/** @def Assert
 * Assert that an expression is true. If false, hit breakpoint.
 * @param   expr    Expression which should be true.
 */
#ifdef RT_STRICT
# define Assert(expr)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
        } \
    } while (0)
#else
# define Assert(expr)     do { } while (0)
#endif


/** @def AssertStmt
 * Assert that an expression is true. If false, hit breakpoint and execute the
 * statement.
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute on failure.
 */
#ifdef RT_STRICT
# define AssertStmt(expr, stmt)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
            stmt; \
        } \
    } while (0)
#else
# define AssertStmt(expr, stmt)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            stmt; \
        } \
    } while (0)
#endif


/** @def AssertReturn
 * Assert that an expression is true and returns if it isn't.
 * In RT_STRICT mode it will hit a breakpoint before returning.
 *
 * @param   expr    Expression which should be true.
 * @param   rc      What is to be presented to return.
 */
#ifdef RT_STRICT
# define AssertReturn(expr, rc) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
            return (rc); \
        } \
    } while (0)
#else
# define AssertReturn(expr, rc) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
            return (rc); \
    } while (0)
#endif

/** @def AssertReturnStmt
 * Assert that an expression is true, if it isn't execute the given statement
 * and return rc.
 *
 * In RT_STRICT mode it will hit a breakpoint before executing the statement and
 * returning.
 *
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute before returning on failure.
 * @param   rc      What is to be presented to return.
 */
#ifdef RT_STRICT
# define AssertReturnStmt(expr, stmt, rc) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
            stmt; \
            return (rc); \
        } \
    } while (0)
#else
# define AssertReturnStmt(expr, stmt, rc) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            stmt; \
            return (rc); \
        } \
    } while (0)
#endif

/** @def AssertReturnVoid
 * Assert that an expression is true and returns if it isn't.
 * In RT_STRICT mode it will hit a breakpoint before returning.
 *
 * @param   expr    Expression which should be true.
 */
#ifdef RT_STRICT
# define AssertReturnVoid(expr) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
            return; \
        } \
    } while (0)
#else
# define AssertReturnVoid(expr) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
            return; \
    } while (0)
#endif

/** @def AssertReturnVoidStmt
 * Assert that an expression is true, if it isn't execute the given statement
 * and return.
 *
 * In RT_STRICT mode it will hit a breakpoint before returning.
 *
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute before returning on failure.
 */
#ifdef RT_STRICT
# define AssertReturnVoidStmt(expr, stmt) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
            stmt; \
            return; \
        } \
    } while (0)
#else
# define AssertReturnVoidStmt(expr, stmt) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            stmt; \
            return; \
        } \
    } while (0)
#endif


/** @def AssertBreak
 * Assert that an expression is true and breaks if it isn't.
 * In RT_STRICT mode it will hit a breakpoint before breaking.
 *
 * @param   expr    Expression which should be true.
 */
#ifdef RT_STRICT
# define AssertBreak(expr) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        break; \
    } else \
        break
#else
# define AssertBreak(expr) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else \
        break
#endif

/** @def AssertBreakStmt
 * Assert that an expression is true and breaks if it isn't.
 * In RT_STRICT mode it will hit a breakpoint before doing break.
 *
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 */
#ifdef RT_STRICT
# define AssertBreakStmt(expr, stmt) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        stmt; \
        break; \
    } else do {} while (0)
#else
# define AssertBreakStmt(expr, stmt) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        stmt; \
        break; \
    } else do {} while (0)
#endif

/** @def AssertContinue
 * Assert that an expression is true and continue if it isn't.
 * In RT_STRICT mode it will hit a breakpoint before continuing.
 *
 * @param   expr    Expression which should be true.
 */
#ifdef RT_STRICT
# define AssertContinue(expr) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        continue; \
    } else do {} while (0)
#else
# define AssertContinue(expr) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else \
        continue
#endif

/** @def AssertContinueStmt
 * Assert that an expression is true and continue if it isn't.
 * In RT_STRICT mode it will hit a breakpoint before continuing.
 *
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute before contine in case of a failed assertion.
 */
#ifdef RT_STRICT
# define AssertContinueStmt(expr, stmt) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        stmt; \
        continue; \
    } else do {} while (0)
#else
# define AssertContinueStmt(expr, stmt) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        stmt; \
        continue; \
    } else do {} while (0)
#endif


/** @def AssertMsg
 * Assert that an expression is true. If it's not print message and hit breakpoint.
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#ifdef RT_STRICT
# define AssertMsg(expr, a)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            RTAssertPanic(); \
        } \
    } while (0)
#else
# define AssertMsg(expr, a)  do { } while (0)
#endif

/** @def AssertMsgStmt
 * Assert that an expression is true.  If it's not print message and hit
 * breakpoint and execute the statement.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute in case of a failed assertion.
 *
 * @remarks The expression and statement will be evaluated in all build types.
 */
#ifdef RT_STRICT
# define AssertMsgStmt(expr, a, stmt)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            RTAssertPanic(); \
            stmt; \
        } \
    } while (0)
#else
# define AssertMsgStmt(expr, a, stmt)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            stmt; \
        } \
    } while (0)
#endif

/** @def AssertMsgReturn
 * Assert that an expression is true and returns if it isn't.
 * In RT_STRICT mode it will hit a breakpoint before returning.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   rc      What is to be presented to return.
 */
#ifdef RT_STRICT
# define AssertMsgReturn(expr, a, rc)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            RTAssertPanic(); \
            return (rc); \
        } \
    } while (0)
#else
# define AssertMsgReturn(expr, a, rc) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
            return (rc); \
    } while (0)
#endif

/** @def AssertMsgReturnStmt
 * Assert that an expression is true, if it isn't execute the statement and
 * return.
 *
 * In RT_STRICT mode it will hit a breakpoint before returning.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rc      What is to be presented to return.
 */
#ifdef RT_STRICT
# define AssertMsgReturnStmt(expr, a, stmt, rc)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            RTAssertPanic(); \
            stmt; \
            return (rc); \
        } \
    } while (0)
#else
# define AssertMsgReturnStmt(expr, a, stmt, rc) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            stmt; \
            return (rc); \
        } \
    } while (0)
#endif

/** @def AssertMsgReturnVoid
 * Assert that an expression is true and returns if it isn't.
 * In RT_STRICT mode it will hit a breakpoint before returning.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#ifdef RT_STRICT
# define AssertMsgReturnVoid(expr, a)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            RTAssertPanic(); \
            return; \
        } \
    } while (0)
#else
# define AssertMsgReturnVoid(expr, a) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
            return; \
    } while (0)
#endif

/** @def AssertMsgReturnVoidStmt
 * Assert that an expression is true, if it isn't execute the statement and
 * return.
 *
 * In RT_STRICT mode it will hit a breakpoint before returning.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before return in case of a failed assertion.
 */
#ifdef RT_STRICT
# define AssertMsgReturnVoidStmt(expr, a, stmt)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            RTAssertPanic(); \
            stmt; \
            return; \
        } \
    } while (0)
#else
# define AssertMsgReturnVoidStmt(expr, a, stmt) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            stmt; \
            return; \
        } \
    } while (0)
#endif


/** @def AssertMsgBreak
 * Assert that an expression is true and breaks if it isn't.
 * In RT_STRICT mode it will hit a breakpoint before returning.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#ifdef RT_STRICT
# define AssertMsgBreak(expr, a) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertPanic(); \
        break; \
    } else \
        break
#else
# define AssertMsgBreak(expr, a) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else \
        break
#endif

/** @def AssertMsgBreakStmt
 * Assert that an expression is true and breaks if it isn't.
 * In RT_STRICT mode it will hit a breakpoint before doing break.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 */
#ifdef RT_STRICT
# define AssertMsgBreakStmt(expr, a, stmt) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertPanic(); \
        stmt; \
        break; \
    } else \
        break
#else
# define AssertMsgBreakStmt(expr, a, stmt) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        stmt; \
        break; \
    } else \
        break
#endif

/** @def AssertFailed
 * An assertion failed, hit breakpoint.
 */
#ifdef RT_STRICT
# define AssertFailed()  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
    } while (0)
#else
# define AssertFailed()         do { } while (0)
#endif

/** @def AssertFailedStmt
 * An assertion failed, hit breakpoint and execute statement.
 */
#ifdef RT_STRICT
# define AssertFailedStmt(stmt) \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        stmt; \
    } while (0)
#else
# define AssertFailedStmt(stmt)     do { stmt; } while (0)
#endif

/** @def AssertFailedReturn
 * An assertion failed, hit breakpoint (RT_STRICT mode only) and return.
 *
 * @param   rc      The rc to return.
 */
#ifdef RT_STRICT
# define AssertFailedReturn(rc)  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        return (rc); \
    } while (0)
#else
# define AssertFailedReturn(rc)  \
    do { \
        return (rc); \
    } while (0)
#endif

/** @def AssertFailedReturnStmt
 * An assertion failed, hit breakpoint (RT_STRICT mode only), execute a
 * statement and return a value.
 *
 * @param   stmt    The statement to execute before returning.
 * @param   rc      The value to return.
 */
#ifdef RT_STRICT
# define AssertFailedReturnStmt(stmt, rc)  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        stmt; \
        return (rc); \
    } while (0)
#else
# define AssertFailedReturnStmt(stmt, rc)  \
    do { \
        stmt; \
        return (rc); \
    } while (0)
#endif

/** @def AssertFailedReturnVoid
 * An assertion failed, hit breakpoint (RT_STRICT mode only) and return.
 */
#ifdef RT_STRICT
# define AssertFailedReturnVoid()  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        return; \
    } while (0)
#else
# define AssertFailedReturnVoid()  \
    do { \
        return; \
    } while (0)
#endif

/** @def AssertFailedReturnVoidStmt
 * An assertion failed, hit breakpoint (RT_STRICT mode only), execute a
 * statement and return.
 *
 * @param stmt The statement to execute before returning.
 */
#ifdef RT_STRICT
# define AssertFailedReturnVoidStmt(stmt)  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        stmt; \
        return; \
    } while (0)
#else
# define AssertFailedReturnVoidStmt(stmt)  \
    do { \
        stmt; \
        return; \
    } while (0)
#endif


/** @def AssertFailedBreak
 * An assertion failed, hit breakpoint (RT_STRICT mode only) and break.
 */
#ifdef RT_STRICT
# define AssertFailedBreak()  \
    if (1) { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        break; \
    } else \
        break
#else
# define AssertFailedBreak()  \
    if (1) \
        break; \
    else \
        break
#endif

/** @def AssertFailedBreakStmt
 * An assertion failed, hit breakpoint (RT_STRICT mode only), execute
 * the given statement and break.
 *
 * @param   stmt    Statement to execute before break.
 */
#ifdef RT_STRICT
# define AssertFailedBreakStmt(stmt) \
    if (1) { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        stmt; \
        break; \
    } else \
        break
#else
# define AssertFailedBreakStmt(stmt) \
    if (1) { \
        stmt; \
        break; \
    } else \
        break
#endif


/** @def AssertMsgFailed
 * An assertion failed print a message and a hit breakpoint.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#ifdef RT_STRICT
# define AssertMsgFailed(a)  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertPanic(); \
    } while (0)
#else
# define AssertMsgFailed(a)     do { } while (0)
#endif

/** @def AssertMsgFailedReturn
 * An assertion failed, hit breakpoint with message (RT_STRICT mode only) and return.
 *
 * @param   a       printf argument list (in parenthesis).
 * @param   rc      What is to be presented to return.
 */
#ifdef RT_STRICT
# define AssertMsgFailedReturn(a, rc)  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertPanic(); \
        return (rc); \
    } while (0)
#else
# define AssertMsgFailedReturn(a, rc)  \
    do { \
        return (rc); \
    } while (0)
#endif

/** @def AssertMsgFailedReturnVoid
 * An assertion failed, hit breakpoint with message (RT_STRICT mode only) and return.
 *
 * @param   a       printf argument list (in parenthesis).
 */
#ifdef RT_STRICT
# define AssertMsgFailedReturnVoid(a)  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertPanic(); \
        return; \
    } while (0)
#else
# define AssertMsgFailedReturnVoid(a)  \
    do { \
        return; \
    } while (0)
#endif


/** @def AssertMsgFailedBreak
 * An assertion failed, hit breakpoint with message (RT_STRICT mode only) and break.
 *
 * @param   a       printf argument list (in parenthesis).
 */
#ifdef RT_STRICT
# define AssertMsgFailedBreak(a)  \
    if (1) { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertPanic(); \
        break; \
    } else \
        break
#else
# define AssertMsgFailedBreak(a)  \
    if (1) \
        break; \
    else \
        break
#endif

/** @def AssertMsgFailedBreakStmt
 * An assertion failed, hit breakpoint (RT_STRICT mode only), execute
 * the given statement and break.
 *
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break.
 */
#ifdef RT_STRICT
# define AssertMsgFailedBreakStmt(a, stmt) \
    if (1) { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertPanic(); \
        stmt; \
        break; \
    } else \
        break
#else
# define AssertMsgFailedBreakStmt(a, stmt) \
    if (1) { \
        stmt; \
        break; \
    } else \
        break
#endif

/** @} */



/** @name Release Log Assertions
 *
 * These assertions will work like normal strict assertion when RT_STRICT is
 * defined and LogRel statements when RT_STRICT is undefined. Typically used for
 * things which shouldn't go wrong, but when it does you'd like to know one way
 * or the other.
 *
 * @{
 */

/** @def RTAssertLogRelMsg1
 * RTAssertMsg1Weak (strict builds) / LogRel wrapper (non-strict).
 */
#ifdef RT_STRICT
# define RTAssertLogRelMsg1(pszExpr, iLine, pszFile, pszFunction) \
    RTAssertMsg1Weak(pszExpr, iLine, pszFile, pszFunction)
#else
# define RTAssertLogRelMsg1(pszExpr, iLine, pszFile, pszFunction) \
    LogRel(("AssertLogRel %s(%d) %s: %s\n",\
            (pszFile), (iLine), (pszFunction), (pszExpr) ))
#endif

/** @def RTAssertLogRelMsg2
 * RTAssertMsg2Weak (strict builds) / LogRel wrapper (non-strict).
 */
#ifdef RT_STRICT
# define RTAssertLogRelMsg2(a)  RTAssertMsg2Weak a
#else
# define RTAssertLogRelMsg2(a)  LogRel(a)
#endif

/** @def AssertLogRel
 * Assert that an expression is true.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 */
#define AssertLogRel(expr) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
        } \
    } while (0)

/** @def AssertLogRelReturn
 * Assert that an expression is true, return \a rc if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   rc      What is to be presented to return.
 */
#define AssertLogRelReturn(expr, rc) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
            return (rc); \
        } \
    } while (0)

/** @def AssertLogRelReturnStmt
 * Assert that an expression is true, execute \a stmt & return \a rc if it
 * isn't. Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute before break in case of a failed
 *                  assertion.
 * @param   rc      What is to be presented to return.
 */
#define AssertLogRelReturnStmt(expr, stmt, rc) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
            stmt; \
            return (rc); \
        } \
    } while (0)

/** @def AssertLogRelReturnVoid
 * Assert that an expression is true, return void if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 */
#define AssertLogRelReturnVoid(expr) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
            return; \
        } \
    } while (0)

/** @def AssertLogRelReturnVoidStmt
 * Assert that an expression is true, execute \a stmt & return void if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute before break in case of a failed
 *                  assertion.
 */
#define AssertLogRelReturnVoidStmt(expr, stmt) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
            stmt; \
            return; \
        } \
    } while (0)

/** @def AssertLogRelBreak
 * Assert that an expression is true, break if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 */
#define AssertLogRelBreak(expr) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        break; \
    } \
    else \
        break

/** @def AssertLogRelBreakStmt
 * Assert that an expression is true, execute \a stmt and break if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 */
#define AssertLogRelBreakStmt(expr, stmt) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        stmt; \
        break; \
    } else \
        break

/** @def AssertLogRelStmt
 * Assert that an expression is true, return \a rc if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute in case of a failed assertion.
 */
#define AssertLogRelStmt(expr, stmt) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertPanic(); \
            stmt; \
        } \
    } while (0)

/** @def AssertLogRelMsg
 * Assert that an expression is true.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#define AssertLogRelMsg(expr, a) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else\
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertLogRelMsg2(a); \
            RTAssertPanic(); \
        } \
    } while (0)

/** @def AssertLogRelMsgStmt
 * Assert that an expression is true, execute \a stmt and break if it isn't
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute in case of a failed assertion.
 */
#define AssertLogRelMsgStmt(expr, a, stmt) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else\
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertLogRelMsg2(a); \
            RTAssertPanic(); \
            stmt; \
        } \
    } while (0)

/** @def AssertLogRelMsgReturn
 * Assert that an expression is true, return \a rc if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   rc      What is to be presented to return.
 */
#define AssertLogRelMsgReturn(expr, a, rc) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else\
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertLogRelMsg2(a); \
            RTAssertPanic(); \
            return (rc); \
        } \
    } while (0)

/** @def AssertLogRelMsgReturnStmt
 * Assert that an expression is true, execute @a stmt and return @a rcRet if it
 * isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 */
#define AssertLogRelMsgReturnStmt(expr, a, stmt, rcRet) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else\
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertLogRelMsg2(a); \
            RTAssertPanic(); \
            stmt; \
            return (rcRet); \
        } \
    } while (0)

/** @def AssertLogRelMsgReturnVoid
 * Assert that an expression is true, return (void) if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#define AssertLogRelMsgReturnVoid(expr, a) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else\
        { \
            RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertLogRelMsg2(a); \
            RTAssertPanic(); \
            return; \
        } \
    } while (0)

/** @def AssertLogRelMsgBreak
 * Assert that an expression is true, break if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#define AssertLogRelMsgBreak(expr, a) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertLogRelMsg2(a); \
        RTAssertPanic(); \
        break; \
    } \
    else \
        break

/** @def AssertLogRelMsgBreakStmt
 * Assert that an expression is true, execute \a stmt and break if it isn't.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 */
#define AssertLogRelMsgBreakStmt(expr, a, stmt) \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertLogRelMsg1(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertLogRelMsg2(a); \
        RTAssertPanic(); \
        stmt; \
        break; \
    } else \
        break

/** @def AssertLogRelFailed
 * An assertion failed.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 */
#define AssertLogRelFailed() \
    do { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
    } while (0)

/** @def AssertLogRelFailedReturn
 * An assertion failed.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   rc      What is to be presented to return.
 */
#define AssertLogRelFailedReturn(rc) \
    do { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        return (rc); \
    } while (0)

/** @def AssertLogRelFailedReturnVoid
 * An assertion failed, hit a breakpoint and return.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 */
#define AssertLogRelFailedReturnVoid() \
    do { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        return; \
    } while (0)

/** @def AssertLogRelFailedBreak
 * An assertion failed, break.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 */
#define AssertLogRelFailedBreak() \
    if (1) \
    { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        break; \
    } else \
        break

/** @def AssertLogRelFailedBreakStmt
 * An assertion failed, execute \a stmt and break.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   stmt    Statement to execute before break.
 */
#define AssertLogRelFailedBreakStmt(stmt) \
    if (1) \
    { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertPanic(); \
        stmt; \
        break; \
    } else \
        break

/** @def AssertLogRelMsgFailed
 * An assertion failed.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#define AssertLogRelMsgFailed(a) \
    do { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertLogRelMsg2(a); \
        RTAssertPanic(); \
    } while (0)

/** @def AssertLogRelMsgFailedStmt
 * An assertion failed, execute @a stmt.
 *
 * Strict builds will hit a breakpoint, non-strict will only do LogRel. The
 * statement will be executed in regardless of build type.
 *
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute after raising/logging the assertion.
 */
#define AssertLogRelMsgFailedStmt(a, stmt) \
    do { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertLogRelMsg2(a); \
        RTAssertPanic(); \
        stmt; \
    } while (0)

/** @def AssertLogRelMsgFailedReturn
 * An assertion failed, return \a rc.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a   printf argument list (in parenthesis).
 * @param   rc  What is to be presented to return.
 */
#define AssertLogRelMsgFailedReturn(a, rc) \
    do { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertLogRelMsg2(a); \
        RTAssertPanic(); \
        return (rc); \
    } while (0)

/** @def AssertLogRelMsgFailedReturnStmt
 * An assertion failed, execute @a stmt and return @a rc.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rc      What is to be presented to return.
 */
#define AssertLogRelMsgFailedReturnStmt(a, stmt, rc) \
    do { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertLogRelMsg2(a); \
        RTAssertPanic(); \
        stmt; \
        return (rc); \
    } while (0)

/** @def AssertLogRelMsgFailedReturnVoid
 * An assertion failed, return void.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#define AssertLogRelMsgFailedReturnVoid(a) \
    do { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertLogRelMsg2(a); \
        RTAssertPanic(); \
        return; \
    } while (0)

/** @def AssertLogRelMsgFailedReturnVoidStmt
 * An assertion failed, execute @a stmt and return void.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 */
#define AssertLogRelMsgFailedReturnVoidStmt(a, stmt) \
    do { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertLogRelMsg2(a); \
        RTAssertPanic(); \
        stmt; \
        return; \
    } while (0)

/** @def AssertLogRelMsgFailedBreak
 * An assertion failed, break.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#define AssertLogRelMsgFailedBreak(a) \
    if (1)\
    { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertLogRelMsg2(a); \
        RTAssertPanic(); \
        break; \
    } else \
        break

/** @def AssertLogRelMsgFailedBreakStmt
 * An assertion failed, execute \a stmt and break.
 * Strict builds will hit a breakpoint, non-strict will only do LogRel.
 *
 * @param   a   printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break.
 */
#define AssertLogRelMsgFailedBreakStmt(a, stmt) \
    if (1) \
    { \
        RTAssertLogRelMsg1((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertLogRelMsg2(a); \
        RTAssertPanic(); \
        stmt; \
        break; \
    } else \
        break

/** @} */



/** @name Release Assertions
 *
 * These assertions are always enabled.
 * @{
 */

/** @def RTAssertReleasePanic()
 * Invokes RTAssertShouldPanic and RTAssertDoPanic.
 *
 * It might seem odd that RTAssertShouldPanic is necessary when its result isn't
 * checked, but it's done since RTAssertShouldPanic is overrideable and might be
 * used to bail out before taking down the system (the VMMR0 case).
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define RTAssertReleasePanic()  do { RTAssertShouldPanic(); RTAssertDoPanic(); } while (0)
#else
# define RTAssertReleasePanic()  do { } while (0)
#endif


/** @def AssertRelease
 * Assert that an expression is true. If it's not hit a breakpoint.
 *
 * @param   expr    Expression which should be true.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertRelease(expr)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertReleasePanic(); \
        } \
    } while (0)
#else
# define AssertRelease(expr)  do { } while (0)
#endif

/** @def AssertReleaseStmt
 * Assert that an expression is true. If false, hit breakpoint and execute the
 * statement.
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute on failure.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseStmt(expr, stmt)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertReleasePanic(); \
            stmt; \
        } \
    } while (0)
#else
# define AssertReleaseStmt(expr, stmt)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            stmt; \
        } \
    } while (0)
#endif

/** @def AssertReleaseReturn
 * Assert that an expression is true, hit a breakpoint and return if it isn't.
 *
 * @param   expr    Expression which should be true.
 * @param   rc      What is to be presented to return.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseReturn(expr, rc)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertReleasePanic(); \
            return (rc); \
        } \
    } while (0)
#else
# define AssertReleaseReturn(expr, rc) \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
            return (rc); \
    } while (0)
#endif

/** @def AssertReleaseReturnVoid
 * Assert that an expression is true, hit a breakpoint and return if it isn't.
 *
 * @param   expr    Expression which should be true.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseReturnVoid(expr)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertReleasePanic(); \
            return; \
        } \
    } while (0)
#else
# define AssertReleaseReturnVoid(expr)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
            return; \
    } while (0)
#endif


/** @def AssertReleaseBreak
 * Assert that an expression is true, hit a breakpoint and break if it isn't.
 *
 * @param   expr    Expression which should be true.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseBreak(expr)  \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertReleasePanic(); \
        break; \
    } else \
        break
#else
# define AssertReleaseBreak(expr)  \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else \
        break
#endif

/** @def AssertReleaseBreakStmt
 * Assert that an expression is true, hit a breakpoint and break if it isn't.
 *
 * @param   expr    Expression which should be true.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseBreakStmt(expr, stmt)  \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertReleasePanic(); \
        stmt; \
        break; \
    } else \
        break
#else
# define AssertReleaseBreakStmt(expr, stmt)  \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        stmt; \
        break; \
    } else \
        break
#endif


/** @def AssertReleaseMsg
 * Assert that an expression is true, print the message and hit a breakpoint if it isn't.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseMsg(expr, a)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            RTAssertReleasePanic(); \
        } \
    } while (0)
#else
# define AssertReleaseMsg(expr, a) do { } while (0)
#endif

/** @def AssertReleaseMsgReturn
 * Assert that an expression is true, print the message and hit a breakpoint and return if it isn't.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   rc      What is to be presented to return.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseMsgReturn(expr, a, rc)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            RTAssertReleasePanic(); \
            return (rc); \
        } \
    } while (0)
#else
# define AssertReleaseMsgReturn(expr, a, rc)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
            return (rc); \
    } while (0)
#endif

/** @def AssertReleaseMsgReturnVoid
 * Assert that an expression is true, print the message and hit a breakpoint and return if it isn't.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseMsgReturnVoid(expr, a)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
        { \
            RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            RTAssertReleasePanic(); \
            return; \
        } \
    } while (0)
#else
# define AssertReleaseMsgReturnVoid(expr, a)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
            return; \
    } while (0)
#endif


/** @def AssertReleaseMsgBreak
 * Assert that an expression is true, print the message and hit a breakpoint and break if it isn't.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseMsgBreak(expr, a)  \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertReleasePanic(); \
        break; \
    } else \
        break
#else
# define AssertReleaseMsgBreak(expr, a)  \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
        break; \
    else \
        break
#endif

/** @def AssertReleaseMsgBreakStmt
 * Assert that an expression is true, print the message and hit a breakpoint and break if it isn't.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseMsgBreakStmt(expr, a, stmt)  \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertReleasePanic(); \
        stmt; \
        break; \
    } else \
        break
#else
# define AssertReleaseMsgBreakStmt(expr, a, stmt)  \
    if (RT_LIKELY(!!(expr))) \
    { /* likely */ } \
    else if (1) \
    { \
        stmt; \
        break; \
    } else \
        break
#endif


/** @def AssertReleaseFailed
 * An assertion failed, hit a breakpoint.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseFailed()  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertReleasePanic(); \
    } while (0)
#else
# define AssertReleaseFailed() do { } while (0)
#endif

/** @def AssertReleaseFailedStmt
 * An assertion failed, hit breakpoint and execute statement.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseFailedStmt(stmt) \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertReleasePanic(); \
        stmt; \
    } while (0)
#else
# define AssertReleaseFailedStmt(stmt)     do { stmt; } while (0)
#endif

/** @def AssertReleaseFailedReturn
 * An assertion failed, hit a breakpoint and return.
 *
 * @param   rc      What is to be presented to return.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseFailedReturn(rc)  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertReleasePanic(); \
        return (rc); \
    } while (0)
#else
# define AssertReleaseFailedReturn(rc)  \
    do { return (rc); } while (0)
#endif

/** @def AssertReleaseFailedReturnVoid
 * An assertion failed, hit a breakpoint and return.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseFailedReturnVoid()  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertReleasePanic(); \
        return; \
    } while (0)
#else
# define AssertReleaseFailedReturnVoid()  \
    do { return; } while (0)
#endif

/** @def AssertReleaseFailedBreak
 * An assertion failed, hit a breakpoint and break.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseFailedBreak()  \
    if (1) { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertReleasePanic(); \
        break; \
    } else \
        break
#else
# define AssertReleaseFailedBreak()  \
    if (1) \
        break; \
    else \
        break
#endif

/** @def AssertReleaseFailedBreakStmt
 * An assertion failed, hit a breakpoint and break.
 *
 * @param   stmt    Statement to execute before break.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseFailedBreakStmt(stmt)  \
    if (1) { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertReleasePanic(); \
        stmt; \
        break; \
    } else \
        break
#else
# define AssertReleaseFailedBreakStmt(stmt)  \
    if (1) { \
        stmt; \
        break; \
    } else \
        break
#endif

/** @def AssertReleaseMsgFailed
 * An assertion failed, print a message and hit a breakpoint.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseMsgFailed(a)  \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertReleasePanic(); \
    } while (0)
#else
# define AssertReleaseMsgFailed(a)  do { } while (0)
#endif

/** @def AssertReleaseMsgFailedReturn
 * An assertion failed, print a message, hit a breakpoint and return.
 *
 * @param   a   printf argument list (in parenthesis).
 * @param   rc      What is to be presented to return.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseMsgFailedReturn(a, rc) \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertReleasePanic(); \
        return (rc); \
    } while (0)
#else
# define AssertReleaseMsgFailedReturn(a, rc) \
    do { return (rc); } while (0)
#endif

/** @def AssertReleaseMsgFailedReturnVoid
 * An assertion failed, print a message, hit a breakpoint and return.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseMsgFailedReturnVoid(a) \
    do { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertReleasePanic(); \
        return; \
    } while (0)
#else
# define AssertReleaseMsgFailedReturnVoid(a) \
    do { return; } while (0)
#endif


/** @def AssertReleaseMsgFailedBreak
 * An assertion failed, print a message, hit a breakpoint and break.
 *
 * @param   a   printf argument list (in parenthesis).
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseMsgFailedBreak(a) \
    if (1) { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertReleasePanic(); \
        break; \
    } else \
        break
#else
# define AssertReleaseMsgFailedBreak(a) \
    if (1) \
        break; \
    else \
        break
#endif

/** @def AssertReleaseMsgFailedBreakStmt
 * An assertion failed, print a message, hit a breakpoint and break.
 *
 * @param   a   printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break.
 */
#if defined(RT_STRICT) || !defined(RTASSERT_NO_RELEASE_ASSERTIONS)
# define AssertReleaseMsgFailedBreakStmt(a, stmt) \
    if (1) { \
        RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
        RTAssertMsg2Weak a; \
        RTAssertReleasePanic(); \
        stmt; \
        break; \
    } else \
        break
#else
# define AssertReleaseMsgFailedBreakStmt(a, stmt) \
    if (1) { \
        stmt; \
        break; \
    } else \
        break
#endif
/** @} */



/** @name Fatal Assertions
 * These are similar to release assertions except that you cannot ignore them in
 * any way, they will loop for ever if RTAssertDoPanic returns.
 *
 * @{
 */

/** @def AssertFatal
 * Assert that an expression is true. If it's not hit a breakpoint (for ever).
 *
 * @param   expr    Expression which should be true.
 */
#define AssertFatal(expr)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
            for (;;) \
            { \
                RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
                RTAssertReleasePanic(); \
            } \
    } while (0)

/** @def AssertFatalMsg
 * Assert that an expression is true, print the message and hit a breakpoint (for ever) if it isn't.
 *
 * @param   expr    Expression which should be true.
 * @param   a       printf argument list (in parenthesis).
 */
#define AssertFatalMsg(expr, a)  \
    do { \
        if (RT_LIKELY(!!(expr))) \
        { /* likely */ } \
        else \
            for (;;) \
            { \
                RTAssertMsg1Weak(#expr, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
                RTAssertMsg2Weak a; \
                RTAssertReleasePanic(); \
            } \
    } while (0)

/** @def AssertFatalFailed
 * An assertion failed, hit a breakpoint (for ever).
 */
#define AssertFatalFailed()  \
    do { \
        for (;;) \
        { \
            RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertReleasePanic(); \
        } \
    } while (0)

/** @def AssertFatalMsgFailed
 * An assertion failed, print a message and hit a breakpoint (for ever).
 *
 * @param   a   printf argument list (in parenthesis).
 */
#define AssertFatalMsgFailed(a)  \
    do { \
        for (;;) \
        { \
            RTAssertMsg1Weak((const char *)0, __LINE__, __FILE__, RT_GCC_EXTENSION __PRETTY_FUNCTION__); \
            RTAssertMsg2Weak a; \
            RTAssertReleasePanic(); \
        } \
    } while (0)

/** @} */



/** @name Convenience Assertions Macros
 * @{
 */

/** @def AssertRC
 * Asserts a iprt status code successful.
 *
 * On failure it will print info about the rc and hit a breakpoint.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRC(rc)                AssertMsgRC(rc, ("%Rra\n", (rc)))

/** @def AssertRCStmt
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and execute
 * @a stmt if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCStmt(rc, stmt)   AssertMsgRCStmt(rc, ("%Rra\n", (rc)), stmt)

/** @def AssertRCReturn
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCReturn(rc, rcRet)   AssertMsgRCReturn(rc, ("%Rra\n", (rc)), rcRet)

/** @def AssertRCReturnStmt
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and returns @a rcRet if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCReturnStmt(rc, stmt, rcRet) AssertMsgRCReturnStmt(rc, ("%Rra\n", (rc)), stmt, rcRet)

/** @def AssertRCReturnVoid
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCReturnVoid(rc)      AssertMsgRCReturnVoid(rc, ("%Rra\n", (rc)))

/** @def AssertRCReturnVoidStmt
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), and
 * execute the given statement/return if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before returning on failure.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCReturnVoidStmt(rc, stmt) AssertMsgRCReturnVoidStmt(rc, ("%Rra\n", (rc)), stmt)

/** @def AssertRCBreak
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCBreak(rc)           AssertMsgRCBreak(rc, ("%Rra\n", (rc)))

/** @def AssertRCBreakStmt
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCBreakStmt(rc, stmt) AssertMsgRCBreakStmt(rc, ("%Rra\n", (rc)), stmt)

/** @def AssertMsgRC
 * Asserts a iprt status code successful.
 *
 * It prints a custom message and hits a breakpoint on FAILURE.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertMsgRC(rc, msg) \
    do { AssertMsg(RT_SUCCESS_NP(rc), msg); NOREF(rc); } while (0)

/** @def AssertMsgRCStmt
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and
 * execute @a stmt if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertMsgRCStmt(rc, msg, stmt) \
    do { AssertMsgStmt(RT_SUCCESS_NP(rc), msg, stmt); NOREF(rc); } while (0)

/** @def AssertMsgRCReturn
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return
 * @a rcRet if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertMsgRCReturn(rc, msg, rcRet) \
    do { AssertMsgReturn(RT_SUCCESS_NP(rc), msg, rcRet); NOREF(rc); } while (0)

/** @def AssertMsgRCReturnStmt
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and return @a rcRet if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertMsgRCReturnStmt(rc, msg, stmt, rcRet) \
    do { AssertMsgReturnStmt(RT_SUCCESS_NP(rc), msg, stmt, rcRet); NOREF(rc); } while (0)

/** @def AssertMsgRCReturnVoid
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and return
 * void if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertMsgRCReturnVoid(rc, msg) \
    do { AssertMsgReturnVoid(RT_SUCCESS_NP(rc), msg); NOREF(rc); } while (0)

/** @def AssertMsgRCReturnVoidStmt
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and return void if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertMsgRCReturnVoidStmt(rc, msg, stmt) \
    do { AssertMsgReturnVoidStmt(RT_SUCCESS_NP(rc), msg, stmt); NOREF(rc); } while (0)

/** @def AssertMsgRCBreak
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only) and break
 * if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertMsgRCBreak(rc, msg) \
    if (1) { AssertMsgBreak(RT_SUCCESS(rc), msg); NOREF(rc); } else do {} while (0)

/** @def AssertMsgRCBreakStmt
 * Asserts a iprt status code successful, bitch (RT_STRICT mode only), execute
 * @a stmt and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertMsgRCBreakStmt(rc, msg, stmt) \
    if (1) { AssertMsgBreakStmt(RT_SUCCESS_NP(rc), msg, stmt); NOREF(rc); } else do {} while (0)

/** @def AssertRCSuccess
 * Asserts an iprt status code equals VINF_SUCCESS.
 *
 * On failure it will print info about the rc and hit a breakpoint.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCSuccess(rc)                 do { AssertMsg((rc) == VINF_SUCCESS, ("%Rra\n", (rc))); NOREF(rc); } while (0)

/** @def AssertRCSuccessReturn
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCSuccessReturn(rc, rcRet)    AssertMsgReturn((rc) == VINF_SUCCESS, ("%Rra\n", (rc)), rcRet)

/** @def AssertRCSuccessReturnVoid
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and return if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCSuccessReturnVoid(rc)       AssertMsgReturnVoid((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def AssertRCSuccessBreak
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCSuccessBreak(rc)            AssertMsgBreak((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def AssertRCSuccessBreakStmt
 * Asserts that an iprt status code equals VINF_SUCCESS, bitch (RT_STRICT mode only) and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times. In release mode is NOREF()'ed.
 */
#define AssertRCSuccessBreakStmt(rc, stmt)  AssertMsgBreakStmt((rc) == VINF_SUCCESS, ("%Rra\n", (rc)), stmt)


/** @def AssertLogRelRC
 * Asserts a iprt status code successful.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRC(rc)                      AssertLogRelMsgRC(rc, ("%Rra\n", (rc)))

/** @def AssertLogRelRCReturn
 * Asserts a iprt status code successful, returning \a rc if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRCReturn(rc, rcRet)         AssertLogRelMsgRCReturn(rc, ("%Rra\n", (rc)), rcRet)

/** @def AssertLogRelRCReturnStmt
 * Asserts a iprt status code successful, executing \a stmt and returning \a rc
 * if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRCReturnStmt(rc, stmt, rcRet) AssertLogRelMsgRCReturnStmt(rc, ("%Rra\n", (rc)), stmt, rcRet)

/** @def AssertLogRelRCReturnVoid
 * Asserts a iprt status code successful, returning (void) if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRCReturnVoid(rc)            AssertLogRelMsgRCReturnVoid(rc, ("%Rra\n", (rc)))

/** @def AssertLogRelRCBreak
 * Asserts a iprt status code successful, breaking if it isn't.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRCBreak(rc)                 AssertLogRelMsgRCBreak(rc, ("%Rra\n", (rc)))

/** @def AssertLogRelRCBreakStmt
 * Asserts a iprt status code successful, execute \a statement and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRCBreakStmt(rc, stmt)       AssertLogRelMsgRCBreakStmt(rc, ("%Rra\n", (rc)), stmt)

/** @def AssertLogRelMsgRC
 * Asserts a iprt status code successful.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelMsgRC(rc, msg)              AssertLogRelMsg(RT_SUCCESS_NP(rc), msg)

/** @def AssertLogRelMsgRCReturn
 * Asserts a iprt status code successful.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelMsgRCReturn(rc, msg, rcRet) AssertLogRelMsgReturn(RT_SUCCESS_NP(rc), msg, rcRet)

/** @def AssertLogRelMsgRCReturnStmt
 * Asserts a iprt status code successful, execute \a stmt and return on
 * failure.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before returning in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelMsgRCReturnStmt(rc, msg, stmt, rcRet) AssertLogRelMsgReturnStmt(RT_SUCCESS_NP(rc), msg, stmt, rcRet)

/** @def AssertLogRelMsgRCReturnVoid
 * Asserts a iprt status code successful.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelMsgRCReturnVoid(rc, msg)    AssertLogRelMsgReturnVoid(RT_SUCCESS_NP(rc), msg)

/** @def AssertLogRelMsgRCBreak
 * Asserts a iprt status code successful.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelMsgRCBreak(rc, msg)         AssertLogRelMsgBreak(RT_SUCCESS(rc), msg)

/** @def AssertLogRelMsgRCBreakStmt
 * Asserts a iprt status code successful, execute \a stmt and break if it isn't.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelMsgRCBreakStmt(rc, msg, stmt) AssertLogRelMsgBreakStmt(RT_SUCCESS_NP(rc), msg, stmt)

/** @def AssertLogRelRCSuccess
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRCSuccess(rc)               AssertLogRelMsg((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def AssertLogRelRCSuccessReturn
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * @param   rc      iprt status code.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRCSuccessReturn(rc, rcRet)  AssertLogRelMsgReturn((rc) == VINF_SUCCESS, ("%Rra\n", (rc)), rcRet)

/** @def AssertLogRelRCSuccessReturnVoid
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRCSuccessReturnVoid(rc)     AssertLogRelMsgReturnVoid((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def AssertLogRelRCSuccessBreak
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRCSuccessBreak(rc)          AssertLogRelMsgBreak((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def AssertLogRelRCSuccessBreakStmt
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times.
 */
#define AssertLogRelRCSuccessBreakStmt(rc, stmt) AssertLogRelMsgBreakStmt((rc) == VINF_SUCCESS, ("%Rra\n", (rc)), stmt)


/** @def AssertReleaseRC
 * Asserts a iprt status code successful.
 *
 * On failure information about the error will be printed and a breakpoint hit.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseRC(rc)                 AssertReleaseMsgRC(rc, ("%Rra\n", (rc)))

/** @def AssertReleaseRCReturn
 * Asserts a iprt status code successful, returning if it isn't.
 *
 * On failure information about the error will be printed, a breakpoint hit
 * and finally returning from the function if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseRCReturn(rc, rcRet)    AssertReleaseMsgRCReturn(rc, ("%Rra\n", (rc)), rcRet)

/** @def AssertReleaseRCReturnVoid
 * Asserts a iprt status code successful, returning if it isn't.
 *
 * On failure information about the error will be printed, a breakpoint hit
 * and finally returning from the function if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseRCReturnVoid(rc)       AssertReleaseMsgRCReturnVoid(rc, ("%Rra\n", (rc)))

/** @def AssertReleaseRCBreak
 * Asserts a iprt status code successful, breaking if it isn't.
 *
 * On failure information about the error will be printed, a breakpoint hit
 * and finally breaking the current statement if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseRCBreak(rc)            AssertReleaseMsgRCBreak(rc, ("%Rra\n", (rc)))

/** @def AssertReleaseRCBreakStmt
 * Asserts a iprt status code successful, break if it isn't.
 *
 * On failure information about the error will be printed, a breakpoint hit
 * and finally the break statement will be issued if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseRCBreakStmt(rc, stmt)  AssertReleaseMsgRCBreakStmt(rc, ("%Rra\n", (rc)), stmt)

/** @def AssertReleaseMsgRC
 * Asserts a iprt status code successful.
 *
 * On failure a custom message is printed and a breakpoint is hit.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseMsgRC(rc, msg)         AssertReleaseMsg(RT_SUCCESS_NP(rc), msg)

/** @def AssertReleaseMsgRCReturn
 * Asserts a iprt status code successful.
 *
 * On failure a custom message is printed, a breakpoint is hit, and finally
 * returning from the function if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseMsgRCReturn(rc, msg, rcRet)    AssertReleaseMsgReturn(RT_SUCCESS_NP(rc), msg, rcRet)

/** @def AssertReleaseMsgRCReturnVoid
 * Asserts a iprt status code successful.
 *
 * On failure a custom message is printed, a breakpoint is hit, and finally
 * returning from the function if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseMsgRCReturnVoid(rc, msg)    AssertReleaseMsgReturnVoid(RT_SUCCESS_NP(rc), msg)

/** @def AssertReleaseMsgRCBreak
 * Asserts a iprt status code successful.
 *
 * On failure a custom message is printed, a breakpoint is hit, and finally
 * breaking the current status if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseMsgRCBreak(rc, msg)        AssertReleaseMsgBreak(RT_SUCCESS(rc), msg)

/** @def AssertReleaseMsgRCBreakStmt
 * Asserts a iprt status code successful.
 *
 * On failure a custom message is printed, a breakpoint is hit, and finally
 * the break statement is issued if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseMsgRCBreakStmt(rc, msg, stmt)  AssertReleaseMsgBreakStmt(RT_SUCCESS_NP(rc), msg, stmt)

/** @def AssertReleaseRCSuccess
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * On failure information about the error will be printed and a breakpoint hit.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseRCSuccess(rc)                  AssertReleaseMsg((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def AssertReleaseRCSuccessReturn
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * On failure information about the error will be printed, a breakpoint hit
 * and finally returning from the function if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @param   rcRet   What is to be presented to return.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseRCSuccessReturn(rc, rcRet)     AssertReleaseMsgReturn((rc) == VINF_SUCCESS, ("%Rra\n", (rc)), rcRet)

/** @def AssertReleaseRCSuccessReturnVoid
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * On failure information about the error will be printed, a breakpoint hit
 * and finally returning from the function if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseRCSuccessReturnVoid(rc)     AssertReleaseMsgReturnVoid((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def AssertReleaseRCSuccessBreak
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * On failure information about the error will be printed, a breakpoint hit
 * and finally breaking the current statement if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseRCSuccessBreak(rc)         AssertReleaseMsgBreak((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))

/** @def AssertReleaseRCSuccessBreakStmt
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * On failure information about the error will be printed, a breakpoint hit
 * and finally the break statement will be issued if the breakpoint is somehow ignored.
 *
 * @param   rc      iprt status code.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 * @remark  rc is referenced multiple times.
 */
#define AssertReleaseRCSuccessBreakStmt(rc, stmt)   AssertReleaseMsgBreakStmt((rc) == VINF_SUCCESS, ("%Rra\n", (rc)), stmt)


/** @def AssertFatalRC
 * Asserts a iprt status code successful.
 *
 * On failure information about the error will be printed and a breakpoint hit.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertFatalRC(rc)               AssertFatalMsgRC(rc, ("%Rra\n", (rc)))

/** @def AssertReleaseMsgRC
 * Asserts a iprt status code successful.
 *
 * On failure a custom message is printed and a breakpoint is hit.
 *
 * @param   rc      iprt status code.
 * @param   msg     printf argument list (in parenthesis).
 * @remark  rc is referenced multiple times.
 */
#define AssertFatalMsgRC(rc, msg)       AssertFatalMsg(RT_SUCCESS_NP(rc), msg)

/** @def AssertFatalRCSuccess
 * Asserts that an iprt status code equals VINF_SUCCESS.
 *
 * On failure information about the error will be printed and a breakpoint hit.
 *
 * @param   rc  iprt status code.
 * @remark  rc is referenced multiple times.
 */
#define AssertFatalRCSuccess(rc)        AssertFatalMsg((rc) == VINF_SUCCESS, ("%Rra\n", (rc)))


/** @def AssertPtr
 * Asserts that a pointer is valid.
 *
 * @param   pv      The pointer.
 */
#define AssertPtr(pv)                   AssertMsg(RT_VALID_PTR(pv), ("%p\n", (pv)))

/** @def AssertPtrReturn
 * Asserts that a pointer is valid.
 *
 * @param   pv      The pointer.
 * @param   rcRet   What is to be presented to return.
 */
#define AssertPtrReturn(pv, rcRet)      AssertMsgReturn(RT_VALID_PTR(pv), ("%p\n", (pv)), rcRet)

/** @def AssertPtrReturnVoid
 * Asserts that a pointer is valid.
 *
 * @param   pv      The pointer.
 */
#define AssertPtrReturnVoid(pv)         AssertMsgReturnVoid(RT_VALID_PTR(pv), ("%p\n", (pv)))

/** @def AssertPtrReturnStmt
 * Asserts that a pointer is valid.
 *
 * @param   pv      The pointer.
 * @param   stmt    Statement to execute before returninig in case of a failed
 *                  assertion.
 * @param   rcRet   What is to be presented to return.
 */
#define AssertPtrReturnStmt(pv, stmt, rcRet) AssertMsgReturnStmt(RT_VALID_PTR(pv), ("%p\n", (pv)), stmt, rcRet)

/** @def AssertPtrReturnVoidStmt
 * Asserts that a pointer is valid.
 *
 * @param   pv      The pointer.
 * @param   stmt    Statement to execute before returninig in case of a failed
 *                  assertion.
 */
#define AssertPtrReturnVoidStmt(pv, stmt) AssertMsgReturnVoid(RT_VALID_PTR(pv), ("%p\n", (pv)), stmt)

/** @def AssertPtrBreak
 * Asserts that a pointer is valid.
 *
 * @param   pv      The pointer.
 */
#define AssertPtrBreak(pv)              AssertMsgBreak(RT_VALID_PTR(pv), ("%p\n", (pv)))

/** @def AssertPtrBreakStmt
 * Asserts that a pointer is valid.
 *
 * @param   pv      The pointer.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 */
#define AssertPtrBreakStmt(pv, stmt)    AssertMsgBreakStmt(RT_VALID_PTR(pv), ("%p\n", (pv)), stmt)

/** @def AssertPtrNull
 * Asserts that a pointer is valid or NULL.
 *
 * @param   pv      The pointer.
 */
#define AssertPtrNull(pv)               AssertMsg(RT_VALID_PTR(pv) || (pv) == NULL, ("%p\n", (pv)))

/** @def AssertPtrNullReturn
 * Asserts that a pointer is valid or NULL.
 *
 * @param   pv      The pointer.
 * @param   rcRet   What is to be presented to return.
 */
#define AssertPtrNullReturn(pv, rcRet)  AssertMsgReturn(RT_VALID_PTR(pv) || (pv) == NULL, ("%p\n", (pv)), rcRet)

/** @def AssertPtrNullReturnVoid
 * Asserts that a pointer is valid or NULL.
 *
 * @param   pv      The pointer.
 */
#define AssertPtrNullReturnVoid(pv)     AssertMsgReturnVoid(RT_VALID_PTR(pv) || (pv) == NULL, ("%p\n", (pv)))

/** @def AssertPtrNullBreak
 * Asserts that a pointer is valid or NULL.
 *
 * @param   pv      The pointer.
 */
#define AssertPtrNullBreak(pv)          AssertMsgBreak(RT_VALID_PTR(pv) || (pv) == NULL, ("%p\n", (pv)))

/** @def AssertPtrNullBreakStmt
 * Asserts that a pointer is valid or NULL.
 *
 * @param   pv      The pointer.
 * @param   stmt    Statement to execute before break in case of a failed assertion.
 */
#define AssertPtrNullBreakStmt(pv, stmt) AssertMsgBreakStmt(RT_VALID_PTR(pv) || (pv) == NULL, ("%p\n", (pv)), stmt)

/** @def AssertGCPhys32
 * Asserts that the high dword of a physical address is zero
 *
 * @param   GCPhys      The address (RTGCPHYS).
 */
#define AssertGCPhys32(GCPhys)          AssertMsg(VALID_PHYS32(GCPhys), ("%RGp\n", (RTGCPHYS)(GCPhys)))

/** @def AssertGCPtr32
 * Asserts that the high dword of a physical address is zero
 *
 * @param   GCPtr       The address (RTGCPTR).
 */
#if GC_ARCH_BITS == 32
# define AssertGCPtr32(GCPtr)           do { } while (0)
#else
# define AssertGCPtr32(GCPtr)           AssertMsg(!((GCPtr) & UINT64_C(0xffffffff00000000)), ("%RGv\n", GCPtr))
#endif

/** @def AssertForEach
 * Equivalent to Assert for each value of the variable from the starting
 * value to the finishing one.
 *
 * @param   var     Name of the counter variable.
 * @param   vartype Type of the counter variable.
 * @param   first   Lowest inclusive value of the counter variable.
 *                  This must be free from side effects.
 * @param   end     Highest exclusive value of the counter variable.
 *                  This must be free from side effects.
 * @param   expr    Expression which should be true for each value of @a var.
 */
#define AssertForEach(var, vartype, first, end, expr) \
    do { \
        vartype var; \
        Assert((first) == (first) && (end) == (end)); /* partial check for side effects */ \
        for (var = (first); var < (end); var++) \
            AssertMsg(expr, ("%s = %#RX64 (%RI64)", #var, (uint64_t)var, (int64_t)var)); \
    } while (0)

#ifdef RT_OS_WINDOWS

/** @def AssertNtStatus
 * Asserts that the NT_SUCCESS() returns true for the given NTSTATUS value.
 *
 * @param   a_rcNt  The NTSTATUS to check.  Will be evaluated twice and
 *                  subjected to NOREF().
 * @sa      AssertRC()
 */
# define AssertNtStatus(a_rcNt) \
    do { AssertMsg(NT_SUCCESS(a_rcNt), ("%#x\n", (a_rcNt))); NOREF(a_rcNt); } while (0)

/** @def AssertNtStatusSuccess
 * Asserts that the given NTSTATUS value equals STATUS_SUCCESS.
 *
 * @param   a_rcNt  The NTSTATUS to check.  Will be evaluated twice and
 *                  subjected to NOREF().
 * @sa      AssertRCSuccess()
 */
# define AssertNtStatusSuccess(a_rcNt) \
    do { AssertMsg((a_rcNt) == STATUS_SUCCESS, ("%#x\n", (a_rcNt))); NOREF(a_rcNt); } while (0)

#endif /* RT_OS_WINDOWS */

/** @} */

/** @} */

#endif /* !IPRT_INCLUDED_assert_h */

