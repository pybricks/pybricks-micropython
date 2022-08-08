// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _PBSYS_SYS_USER_PROGRAM_H_
#define _PBSYS_SYS_USER_PROGRAM_H_

#include <pbsys/config.h>
#include <pbsys/user_program.h>

void pbsys_user_program_poll(void);

#if PBSYS_CONFIG_USER_PROGRAM_LOAD
void pbsys_user_program_process_start(pbsys_user_program_info_t **program_info);
bool pbsys_user_program_process_complete(void);
#endif

#endif // _PBSYS_SYS_USER_PROGRAM_H_
