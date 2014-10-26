/*-
 * Copyright (c) 2014 Robert N. M. Watson
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

/*
 * A few non-faulting CHERI-related virtual-memory tests.
 */

#include <sys/cdefs.h>

#if !__has_feature(capabilities)
#error "This code requires a CHERI-aware compiler"
#endif

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/ucontext.h>
#include <sys/wait.h>

#include <machine/cheri.h>
#include <machine/cheric.h>
#include <machine/cpuregs.h>
#include <machine/frame.h>
#include <machine/trap.h>

#include <cheri/cheri_fd.h>
#include <cheri/sandbox.h>

#include <cheritest-helper.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "cheritest.h"

/*
 * Exercise copy-on-write:
 *
 * 1) Create a new anonymous shared memory object, extend to page size, map,
 * and write a tagged capability to it.
 *
 * 2) Create a second copy-on-write mapping; read back the tagged value via
 * the second mapping, and confirm that it still has a tag.
 * (cheritest_vm_cow_read)
 *
 * 3) Write an adjacent word in the second mapping, which should cause a
 * copy-on-write, then read back the capability and confirm that it still has
 * a tag.  (cheritest_vm_cow_write)
 */
void
cheritest_vm_cow_read(const struct cheri_test *ctp __unused)
{
	__capability void * volatile *cp_copy;
	__capability void * volatile *cp_real;
	__capability void *cp;
	int fd;

	/*
	 * Create anonymous shared memory object.
	 */
	fd = shm_open(SHM_ANON, O_RDWR, 0600);
	if (fd < 0)
		cheritest_failure_err("shm_open");
	if (ftruncate(fd, getpagesize()) < 0)
		cheritest_failure_err("ftruncate");

	/*
	 * Create 'real' and copy-on-write mappings.
	 */
	cp_real = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0);
	if (cp_real == MAP_FAILED)
		cheritest_failure_err("mmap cp_real");
	cp_copy = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE,
	    MAP_PRIVATE, fd, 0);
	if (cp_copy == MAP_FAILED)
		cheritest_failure_err("mmap cp_copy");

	/*
	 * Write out a tagged capability to 'real' mapping -- doesn't really
	 * matter what it points at.  Confirm it has a tag.
	 */
	cp = cheri_ptr(&fd, sizeof(fd));
	cp_real[0] = cp;
	cp = cp_real[0];
	if (cheri_gettag(cp) == 0)
		cheritest_failure_errx("pretest: tag missing");

	/*
	 * Read in tagged capability via copy-on-write mapping.  Confirm it
	 * has a tag.
	 */
	cp = cp_copy[0];
	if (cheri_gettag(cp) == 0)
		cheritest_failure_errx("tag missing, cp_real");

	/*
	 * Clean up.
	 */
	if (munmap((void *)cp_real, getpagesize()) < 0)
		cheritest_failure_err("munmap cp_real");
	if (munmap((void *)cp_copy, getpagesize()) < 0)
		cheritest_failure_err("munmap cp_copy");
	if (close(fd) < 0)
		cheritest_failure_err("close");
	cheritest_success();
}

void
cheritest_vm_cow_write(const struct cheri_test *ctp __unused)
{
	__capability void * volatile *cp_copy;
	__capability void * volatile *cp_real;
	__capability void *cp;
	int fd;

	/*
	 * Create anonymous shared memory object.
	 */
	fd = shm_open(SHM_ANON, O_RDWR, 0600);
	if (fd < 0)
		cheritest_failure_err("shm_open");
	if (ftruncate(fd, getpagesize()) < 0)
		cheritest_failure_err("ftruncate");

	/*
	 * Create 'real' and copy-on-write mappings.
	 */
	cp_real = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0);
	if (cp_real == MAP_FAILED)
		cheritest_failure_err("mmap cp_real");
	cp_copy = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE,
	    MAP_PRIVATE, fd, 0);
	if (cp_copy == MAP_FAILED)
		cheritest_failure_err("mmap cp_copy");

	/*
	 * Write out a tagged capability to 'real' mapping -- doesn't really
	 * matter what it points at.  Confirm it has a tag.
	 */
	cp = cheri_ptr(&fd, sizeof(fd));
	cp_real[0] = cp;
	cp = cp_real[0];
	if (cheri_gettag(cp) == 0)
		cheritest_failure_errx("pretest: tag missing");

	/*
	 * Read in tagged capability via copy-on-write mapping.  Confirm it
	 * has a tag.
	 */
	cp = cp_copy[0];
	if (cheri_gettag(cp) == 0)
		cheritest_failure_errx("tag missing, cp_real");

	/*
	 * Diverge from cheritest_vm_cow_read(): write via the second mapping
	 * to force a copy-on-write rather than continued sharing of the page.
	 */
	cp = cheri_ptr(&fd, sizeof(fd));
	cp_copy[1] = cp;

	/*
	 * Confirm that the tag is still present on the 'real' page.
	 */
	cp = cp_real[0];
	if (cheri_gettag(cp) == 0)
		cheritest_failure_errx("tag missing after COW, cp_real");

	cp = cp_copy[0];
	if (cheri_gettag(cp) == 0)
		cheritest_failure_errx("tag missing after COW, cp_copy");

	/*
	 * Clean up.
	 */
	if (munmap((void *)cp_real, getpagesize()) < 0)
		cheritest_failure_err("munmap cp_real");
	if (munmap((void *)cp_copy, getpagesize()) < 0)
		cheritest_failure_err("munmap cp_copy");
	if (close(fd) < 0)
		cheritest_failure_err("close");
	cheritest_success();
}