#pragma once

#include <stdint.h>

namespace vpual {
namespace tempread {

enum class action : char {
    INIT,
    GET_TEMPERATURE
};

typedef enum {
    INIT_SUCCESS = 0,
    INIT_ERROR,
    GET_SUCCESS,
    GET_ERROR,
    NOT_INITIALIZED,
    ALREADY_INITIALIZED,
} RetStatus_t;

typedef struct {
    uint32_t ret_status; // @param RetStatus_t
    uint32_t ret_code;
} StatusCode_t;

typedef enum {
    IDX_CSS = 0,
    IDX_MSS = 1,
    IDX_UPA = 2,
    IDX_DSS = 3,
    IDX_MAX = 4
} Idx_t;

typedef struct {
    float value[IDX_MAX];
} Entry_t;

} // namespace tempread
} // namespace vpual
