#ifndef __VECTOR_BASE_SET_H__
#define __VECTOR_BASE_SET_H__
#include "libc/bit-support.h"
#include "asm-helpers.h"

/*
 * vector base address register:
 *   3-121 --- let's us control where the exception jump table is!
 *
 * defines: 
 *  - vector_base_set  
 *  - vector_base_get
 */

cp_asm_set(vector_base_asm, p15, 0, c12, c0, 0)
cp_asm_get(vector_base_asm, p15, 0, c12, c0, 0)

// return the current value vector base is set to.
static inline void *vector_base_get(void) {
    return (void*)vector_base_asm_get();
}

// check that not null and alignment is good.
static inline int vector_base_chk(void *vector_base) {
    if(!vector_base) {
        // output("null vector base!\n");
        return 0;
    }

    uint32_t v = (uint32_t)vector_base;
    if(bits_get(v, 0, 4) != 0) {
        // output("lower 5 bits of the vector base should be 0, have: %x\n", v);
        return 0;
    }
    
    return 1;
}

// set vector base: must not have been set already.
static inline void vector_base_set(void *vec) {
    if(!vector_base_chk(vec))
        panic("illegal vector base %p\n", vec);

    void *v = vector_base_get();
    if(v) 
        panic("vector base register already set=%p\n", v);

    vector_base_asm_set((uint32_t)vec);

    v = vector_base_get();
    if(v != vec)
        panic("set vector=%p, but have %p\n", vec, v);
}

// reset vector base and return old value: could have
// been previously set.
static inline void *
vector_base_reset(void *vec) {
    if(!vector_base_chk(vec))
        panic("illegal vector base %p\n", vec);

    // void *old_vec = 0;
    void *old_vec = vector_base_get();
    vector_base_asm_set((uint32_t)vec);
    assert(vector_base_get() == vec);

    assert(vector_base_get() == vec);
    return old_vec;
}
#endif
