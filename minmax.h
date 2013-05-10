// minmax.h - min() and max() macros.

#pragma once
#ifndef __MINMAX_H__
#define __MINMAX_H__

//#pragma message("__MINMAX_H__")

#include "morrigan.h"

#define max(a,b)  ((a) > (b) ? (a) : (b))
#define min(a,b)  ((a) < (b) ? (a) : (b))

#endif /* __MINMAX_H__ */
