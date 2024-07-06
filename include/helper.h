#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include "protocol.h"
#include "user.h"
#include "globals.h"
#include "client_registry.h"

#include <stdlib.h>
#include "debug.h"
#include <semaphore.h>
#include <string.h>
#include "csapp.h"



// Macro for ref methods to increase the ref_count:
#define REF_COUNTED_OBJECT_REF(obj, why) \
     { \
        debug("Increasing the ref count: %s", why); \
        sem_wait(&(obj)->mutex); \
        (obj)->ref_count++; \
        sem_post(&(obj)->mutex); \
    } 


#endif
