// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \addtogroup sys
 * @{
 */

/**
 * \defgroup subprocess Contiki subprocesses
 * @{
 *
 * A Contiki subprocess is a "process-in-a-process".
 */

/**
 * \file
 *         Subprocesses for Contiki
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef SUBPROCESS_H_
#define SUBPROCESS_H_

#define SUBPROCESS_BEGIN(strname)                                       \
    {                                                                       \
        static struct process subprocess_subprocess = {NULL, strname};        \
        subprocess_subprocess.thread = PROCESS_CURRENT()->thread;             \
        process_start(&subprocess_subprocess);                                \
        PT_INIT(&subprocess_subprocess.pt);                                   \
        LC_SET(subprocess_subprocess.pt.lc);                                  \
        if (PROCESS_CURRENT() == &subprocess_subprocess) {

#define SUBPROCESS_END()                        \
    PROCESS_EXIT();                             \
}                                             \
}

#endif /* SUBPROCESS_H_ */

/** @}*/
/** @}*/
