#pragma once

#include <cassert>

#define UNREACHABLE { assert(0); __builtin_unreachable(); }