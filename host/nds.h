#pragma once
// host/nds.h — stub minimo da libnds p/ compilar a LOGICA no PC (gcc/WSL).
// So tipos e macros; nada de video/input. Usado por host/run_tests.sh.
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef int64_t s64;
#define BIT(n) (1u<<(n))
#define RGB15(r,g,b) ((r)|((g)<<5)|((b)<<10))
