/*-
 * Copyright (c) 2012-2014 Robert N. M. Watson
 * Copyright (c) 2014 SRI International
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#if !__has_feature(capabilities)
#error "This code requires a CHERI-aware compiler"
#endif

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <machine/cheri.h>
#include <machine/cheric.h>
#include <machine/cpuregs.h>
#include <machine/sysarch.h>

#include <cheri/cheri_enter.h>
#include <cheri/cheri_fd.h>
#include <cheri/sandbox.h>

#include <cheritest-helper.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "cheritest.h"

struct sandbox_class	*cheritest_classp;
struct sandbox_object	*cheritest_objectp;

static int zero_fd = -1;
static struct cheri_object zero_fd_object;

void
cheritest_invoke_fd_op(int op)
{
	register_t v;

	v = sandbox_object_cinvoke(cheritest_objectp, op, 0, 0, 0, 0, 0, 0, 0,
	    sandbox_object_getsystemobject(cheritest_objectp).co_codecap,
	    sandbox_object_getsystemobject(cheritest_objectp).co_datacap,
	    cheri_zerocap(), cheri_zerocap(),
	    zero_fd_object.co_codecap, zero_fd_object.co_datacap,
	    cheri_zerocap(), cheri_zerocap());

	/*
	 * XXXRW: Pretty soon we'll want to break this one function out into
	 * test-specific functions that have more rich definitions of
	 * 'success'.
	 */
	cheritest_success();
}

void
cheritest_revoke_fd(void)
{

	cheri_fd_revoke(zero_fd_object);

	/*
	 * XXXRW: Soon we will rewrite this test to actually perform a series
	 * of operations and ensure that after a revoke, they stop happening.
	 */
	cheritest_success();
}

void
cheritest_invoke_simple_op(int op)
{
	register_t v;

	/*
	 * Test must be done in 10 seconds or less: not the ideal way to do
	 * this, as we'd rather time it out in the parent, I think, but works
	 * fine in practice.
	 */
	alarm(10);

	v = sandbox_object_cinvoke(cheritest_objectp, op, 0, 0, 0, 0, 0, 0, 0,
	    sandbox_object_getsystemobject(cheritest_objectp).co_codecap,
	    sandbox_object_getsystemobject(cheritest_objectp).co_datacap,
	    cheri_zerocap(), cheri_zerocap(), cheri_zerocap(),
	    cheri_zerocap(), cheri_zerocap(), cheri_zerocap());

	/*
	 * XXXRW: Pretty soon we'll want to break this one function out into
	 * test-specific functions that have more rich definitions of
	 * 'success'.
	 */
	cheritest_success();
}

static register_t
test_sandbox_op(int op)
{

	alarm(10);
	return (sandbox_object_cinvoke(cheritest_objectp, op, 0, 0, 0, 0, 0, 0, 0,
	    sandbox_object_getsystemobject(cheritest_objectp).co_codecap,
	    sandbox_object_getsystemobject(cheritest_objectp).co_datacap,
	    cheri_zerocap(), cheri_zerocap(), cheri_zerocap(),
	    cheri_zerocap(), cheri_zerocap(), cheri_zerocap()));
}

static void
signal_handler_clear(int sig)
{
	struct sigaction sa;

	/* XXXRW: Possibly should just not be registering it? */
	bzero(&sa, sizeof(sa));
	sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sig, &sa, NULL) < 0)
		cheritest_failure_err("clearing handler for sig %d", sig);
}

void
test_sandbox_cp2_bound_catch(void)
{

	test_sandbox_op(CHERITEST_HELPER_OP_CP2_BOUND);
	cheritest_failure_errx("invoke returned");
}

void
test_sandbox_cp2_bound_nocatch(void)
{
	register_t v;

	signal_handler_clear(SIGPROT);
	v = test_sandbox_op(CHERITEST_HELPER_OP_CP2_BOUND);
	if (v != -1)
		cheritest_failure_errx("invoke returned %d (expected %d)", v,
		    -1);
	cheritest_success();
}

void
test_sandbox_cp2_perm_catch(void)
{

	test_sandbox_op(CHERITEST_HELPER_OP_CP2_PERM);
	cheritest_failure_errx("invoke returned");
}

void
test_sandbox_cp2_perm_nocatch(void)
{
	register_t v;

	signal_handler_clear(SIGPROT);
	v = test_sandbox_op(CHERITEST_HELPER_OP_CP2_PERM);
	if (v != -1)
		cheritest_failure_errx("invoke returned %d (expected %d)", v,
		    -1);
	cheritest_success();
}

void
test_sandbox_cp2_tag_catch(void)
{

	test_sandbox_op(CHERITEST_HELPER_OP_CP2_TAG);
	cheritest_failure_errx("invoke returned");
}

void
test_sandbox_cp2_tag_nocatch(void)
{
	register_t v;

	signal_handler_clear(SIGPROT);
	v = test_sandbox_op(CHERITEST_HELPER_OP_CP2_TAG);
	if (v != -1)
		cheritest_failure_errx("invoke returned %d (expected %d)", v,
		    -1);
	cheritest_success();
}

void
test_sandbox_cp2_seal_catch(void)
{

	test_sandbox_op(CHERITEST_HELPER_OP_CP2_SEAL);
	cheritest_failure_errx("invoke returned");
}

void
test_sandbox_cp2_seal_nocatch(void)
{
	register_t v;

	signal_handler_clear(SIGPROT);
	v = test_sandbox_op(CHERITEST_HELPER_OP_CP2_SEAL);
	if (v != -1)
		cheritest_failure_errx("invoke returned %d (expected %d)", v,
		    -1);
	cheritest_success();
}

void
test_sandbox_divzero_catch(void)
{

	test_sandbox_op(CHERITEST_HELPER_OP_DIVZERO);
	cheritest_failure_errx("invoke returned");
}

void
test_sandbox_divzero_nocatch(void)
{
	register_t v;

	signal_handler_clear(SIGEMT);
	v = test_sandbox_op(CHERITEST_HELPER_OP_DIVZERO);
	if (v != -1)
		cheritest_failure_errx("invoke returned %d (expected %d)", v,
		    -1);
	cheritest_success();
}

void
test_sandbox_vm_rfault_catch(void)
{

	test_sandbox_op(CHERITEST_HELPER_OP_VM_RFAULT);
	cheritest_failure_errx("invoke returned");
}

void
test_sandbox_vm_rfault_nocatch(void)
{
	register_t v;

	signal_handler_clear(SIGSEGV);
	v = test_sandbox_op(CHERITEST_HELPER_OP_VM_RFAULT);
	if (v != -1)
		cheritest_failure_errx("invoke returned %d (expected %d)", v,
		    -1);
	cheritest_success();
}

void
test_sandbox_vm_wfault_catch(void)
{

	test_sandbox_op(CHERITEST_HELPER_OP_VM_WFAULT);
	cheritest_failure_errx("invoke returned");
}

void
test_sandbox_vm_wfault_nocatch(void)
{
	register_t v;

	signal_handler_clear(SIGSEGV);
	v = test_sandbox_op(CHERITEST_HELPER_OP_VM_WFAULT);
	if (v != -1)
		cheritest_failure_errx("invoke returned %d (expected %d)", v,
		    -1);
	cheritest_success();
}

void
test_sandbox_vm_xfault_catch(void)
{

	test_sandbox_op(CHERITEST_HELPER_OP_VM_XFAULT);
	cheritest_failure_errx("invoke returned");
}

void
test_sandbox_vm_xfault_nocatch(void)
{
	register_t v;

	signal_handler_clear(SIGBUS);
	v = test_sandbox_op(CHERITEST_HELPER_OP_VM_XFAULT);
	if (v != -1)
		cheritest_failure_errx("invoke returned %d (expected %d)", v,
		    -1);
	cheritest_success();
}

void
cheritest_invoke_syscall(void)
{
	size_t len;
	int old, new;

	/*
	 * Track whether or not the number of system-call violations increases
	 * as a result of triggering a system call in a sandbox.  Note that
	 * this isn't really authoritative (nor in the strictest sense
	 * correct), as we can race with other threads that trigger
	 * violations, but it's still a useful test case.
	 */
	len = sizeof(old);
	if (sysctlbyname("security.cheri.syscall_violations", &old, &len,
	    NULL, 0) < 0)
		cheritest_failure_errx(
		    "security.cheri.syscall_violations sysctl read (%d)",
		    errno);
	cheritest_invoke_simple_op(CHERITEST_HELPER_OP_SYSCALL);
	len = sizeof(new);
	if (sysctlbyname("security.cheri.syscall_violations", &new, &len,
	    NULL, 0) < 0)
		cheritest_failure_errx(
		    "security.cheri.syscall_violations sysctl read (%d)",
		    errno);
	if (new <= old)
		cheritest_failure_errx(
		    "security.cheri.syscall_violations unchanged");
	cheritest_success();
}

static char string_to_md5[] = "hello world";
static char string_md5[] = "5eb63bbbe01eeed093cb22bb8f5acdc3";

void
cheritest_invoke_md5(void)
{
	__capability void *md5cap, *bufcap, *cclear;
	char buf[33];
	register_t v;

	cclear = cheri_zerocap();
	md5cap = cheri_ptrperm(string_to_md5, sizeof(string_to_md5),
	    CHERI_PERM_LOAD);
	bufcap = cheri_ptrperm(buf, sizeof(buf), CHERI_PERM_STORE);

	v = sandbox_object_cinvoke(cheritest_objectp, CHERITEST_HELPER_OP_MD5,
	    0, strlen(string_to_md5), 0, 0, 0, 0, 0,
	    sandbox_object_getsystemobject(cheritest_objectp).co_codecap,
	    sandbox_object_getsystemobject(cheritest_objectp).co_datacap,
	    md5cap, bufcap, cclear, cclear, cclear, cclear);

	buf[32] = '\0';
	if (strcmp(buf, string_md5) != 0)
		cheritest_failure_errx(
		    "Incorrect MD5 checksum returned from sandbox");
	cheritest_success();
}

static register_t cheritest_libcheri_userfn_handler(register_t, register_t,
    register_t, register_t, register_t, register_t, register_t, register_t,
    struct cheri_object, __capability void *, __capability void *,
    __capability void *, __capability void *, __capability void *)
    __attribute__((cheri_ccall));

static register_t
cheritest_libcheri_userfn_handler(register_t methodnum, register_t arg,
    register_t a2 __unused, register_t a3 __unused,
    register_t a4 __unused, register_t a5 __unused, register_t a6 __unused,
    register_t a7 __unused, struct cheri_object system_object __unused,
    __capability void *c3 __unused, __capability void *c4 __unused,
    __capability void *c5 __unused, __capability void *c6 __unused,
    __capability void *c7 __unused)
{

	switch (methodnum) {
	case CHERITEST_USERFN_RETURNARG:
		return (arg);

	case CHERITEST_USERFN_GETSTACK:
		return (cheritest_libcheri_userfn_getstack());

	case CHERITEST_USERFN_SETSTACK:
		return (cheritest_libcheri_userfn_setstack(arg));

	default:
		cheritest_failure_errx("%s: unexpected method %ld", __func__,
		    methodnum);
	}
}

void
cheritest_libcheri_userfn(void)
{
	__capability void *cclear;
	register_t i, v;

	cclear = cheri_zerocap();
	for (i = 0; i < 10; i++) {
		v = sandbox_object_cinvoke(cheritest_objectp,
		    CHERITEST_HELPER_LIBCHERI_USERFN,
		    CHERITEST_USERFN_RETURNARG, i, 0, 0, 0, 0, 0,
		   sandbox_object_getsystemobject(cheritest_objectp).co_codecap,
		   sandbox_object_getsystemobject(cheritest_objectp).co_datacap,
		    cclear, cclear, cclear, cclear, cclear, cclear);
		if (v != i)
			cheritest_failure_errx("Incorrect return value "
			    "0x%lx (expected 0x%lx)\n", v, i);
	}
	cheritest_success();
}

void
cheritest_save_global(void)
{
	__capability void *carg, *cclear;
	register_t v;

	carg = (__capability void *)&v;
	cclear = cheri_zerocap();
	v = sandbox_object_cinvoke(cheritest_objectp,
	    CHERITEST_HELPER_SAVE_CAPABILITY_IN_HEAP, 0, 0, 0, 0, 0, 0, 0,
	    sandbox_object_getsystemobject(cheritest_objectp).co_codecap,
	    sandbox_object_getsystemobject(cheritest_objectp).co_datacap,
	    carg, cclear, cclear, cclear, cclear, cclear);
	if (v != 0)
		cheritest_failure_errx("Incorrect return value 0x%lx "
		    "(expected 0)\n", v);
	cheritest_success();
}

void
cheritest_save_ephemeral(void)
{
	__capability void *carg, *cclear;
	register_t v;

	carg = (__capability void *)&v;
	carg = cheri_ephemeral(carg);
	cclear = cheri_zerocap();
	(void)sandbox_object_cinvoke(cheritest_objectp,
	    CHERITEST_HELPER_SAVE_CAPABILITY_IN_HEAP, 0, 0, 0, 0, 0, 0, 0,
	    sandbox_object_getsystemobject(cheritest_objectp).co_codecap,
	    sandbox_object_getsystemobject(cheritest_objectp).co_datacap,
	    carg, cclear, cclear, cclear, cclear, cclear);
	cheritest_failure_errx("Method failed to properly fail\n");
}

int
cheritest_libcheri_setup(void)
{

	/*
	 * Prepare a CHERI object representing /dev/zero for fd-related tests.
	 */
	zero_fd = open("/dev/zero", O_RDWR);
	if (zero_fd < 0)
		err(EX_OSFILE, "open: /dev/zero");
	if (cheri_fd_new(zero_fd, &zero_fd_object) < 0)
		err(EX_OSFILE, "cheri_fd_new: /dev/zero");

	if (sandbox_class_new("/usr/libexec/cheritest-helper.bin",
	    4*1024*1024, &cheritest_classp) < 0)
		return (-1);
	if (sandbox_object_new(cheritest_classp, &cheritest_objectp) < 0)
		return (-1);
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_MD5, "md5");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_ABORT, "abort");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_SPIN, "spin");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_CP2_BOUND, "cp2_bound");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_CP2_PERM, "cp2_perm");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_CP2_TAG, "cp2_tag");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_CP2_SEAL, "cp2_seal");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_CS_HELLOWORLD, "helloworld");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_CS_PUTS, "puts");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_PRINTF, "printf");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_VM_RFAULT, "vm_rfault");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_VM_WFAULT, "vm_wfault");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_VM_XFAULT, "vm_xfault");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_SYSCALL, "syscall");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_DIVZERO, "divzero");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_SYSCAP, "syscap");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_MALLOC, "malloc");
	(void)sandbox_class_method_declare(cheritest_classp,
	    CHERITEST_HELPER_OP_CS_CLOCK_GETTIME, "clock_gettime");

	cheri_system_user_register_fn(&cheritest_libcheri_userfn_handler);

	return (0);
}

void
cheritest_libcheri_destroy(void)
{

	sandbox_object_destroy(cheritest_objectp);
	sandbox_class_destroy(cheritest_classp);
	cheri_fd_destroy(zero_fd_object);
	close(zero_fd);
}
