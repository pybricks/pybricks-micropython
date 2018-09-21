
#ifndef _PBIO_MAIN_H_
#define _PBIO_MAIN_H_

void pbio_init(void);
void pbio_poll(void);
void pbio_reset(void);

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void pbio_deinit(void);
#else
static inline void pbio_deinit(void) { }
#endif

#endif // _PBIO_MAIN_H_
