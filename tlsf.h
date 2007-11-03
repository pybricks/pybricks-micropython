/*
 * Two Levels Segregate Fit memory allocator (TLSF)
 * Version 2.3.2
 *
 * Written by Miguel Masmano Tello <mimastel@doctor.upv.es>
 *
 * Thanks to Ismael Ripoll for his suggestions and reviews
 *
 * Copyright (C) 2007, 2006, 2005, 2004
 *
 * This code is released using a dual license strategy: GPL/LGPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of the GNU General Public License Version 2.0
 * Released under the terms of the GNU Lesser General Public License Version 2.1
 *
 * Adapted to NxOS by David Anderson.
 */

#ifndef __NXOS_TLSF_H__
#define __NXOS_TLSF_H__

#include "mytypes.h"

extern size_t nx_mem_init(size_t, void *);
extern size_t nx_mem_used(void *);
extern void nx_mem_destroy(void *);

extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *realloc(void *ptr, size_t size);
extern void *calloc(size_t nelem, size_t elem_size);

#endif /* __NXOS_TLSF_H__ */
