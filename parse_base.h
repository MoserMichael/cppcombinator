#pragma once

#include <stdio.h>
#include <stdint.h>

namespace pparse {

const uint32_t ON_RESIZE_DOUBLE_BUFFER_TILL=16*1024;

using Char_t = char;    
using FilePos_t = long;

#define ERROR(...) do { fprintf(stderr, "Error: " __VA_ARGS__); } while(false);
#define INFO(...) do { fprintf(stderr, "Info: " __VA_ARGS__); } while(false);


}
