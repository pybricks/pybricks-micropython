#include "control.h"

static int mycounter = 0;
int global_test = 0;

// This task should be called at (almost) fixed intervals
void handler(){
    if (mycounter++ == 500){
        // printf("Five seconds passed and global_test is %d \n", global_test);
    }
}
