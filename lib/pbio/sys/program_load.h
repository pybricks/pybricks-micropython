// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _PBSYS_SYS_PROGRAM_LOAD_H_
#define _PBSYS_SYS_PROGRAM_LOAD_H_

#include <pbsys/config.h>
#include <pbsys/program_load.h>

void pbsys_program_load_process_start(pbsys_program_load_info_t **program_info);
bool pbsys_program_load_process_complete(void);

#endif // _PBSYS_SYS_PROGRAM_LOAD_H_
