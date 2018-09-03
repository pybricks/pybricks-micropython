#include "control.h"
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include <stdio.h>

#if defined EV3
    #include "../bricks/EV3/motorio.h"
#endif