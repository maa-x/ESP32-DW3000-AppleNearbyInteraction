#include <stdint-gcc.h>
#include "niq.h"

int niq_init(void (*start_uwb)(void),
             void (*stop_uwb)(void),
             void (*crypto_init)(void),
             void (*crypto_deinit)(void),
             void (*crypto_range_vector_geneator)(uint8_t *const data, size_t data_len))
{
    int return_value = 0;
    
}
