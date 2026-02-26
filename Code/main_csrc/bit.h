#pragma once

#define BIT(x) (1UL << (x))
#define RBIT(b) (__RBIT((uint32_t)(b)) >> 24) /* Reverse bit order of a byte*/