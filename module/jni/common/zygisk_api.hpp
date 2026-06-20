#pragma once

// The public Zygisk API uses dev_t and ino_t in its PLT-hook declarations.
// Include the defining platform header first so every translation unit sees a
// complete and consistent API surface, independent of incidental include order.
#include <sys/types.h>

#include "../zygisk.hpp"
