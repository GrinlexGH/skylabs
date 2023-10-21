#include "common.hpp"

bool M_strcmp(const char* s1, const char* s2) {
    while (true) {
        if (*s1 != *s2)
            return false;       // strings not equal
        if (!*s1)
            return true;        // strings are equal
        ++s1;
        ++s2;
    }
}

