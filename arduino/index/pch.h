#pragma once 
#define DEBUG_MODE

#ifdef DEBUG_MODE
#define OnDebug(x) x
#else
#define OnDebug(x)
#endif