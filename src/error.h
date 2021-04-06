#pragma once

#ifdef WIN32
#define ERROR_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define ERROR_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifdef ERROR
#undef ERROR
#endif

#define ERROR(message) printf("[%s:%d] %s\n", ERROR_FILENAME, __LINE__, message)