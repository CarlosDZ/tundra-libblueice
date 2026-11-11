#ifndef BLUEICE_INTERNAL_H
#define BLUEICE_INTERNAL_H

#include <tundra/blueice.h>

#define BLUEICE_API __attribute__((visibility("default")))

int blueice_safe_token(const char *s);

#endif
