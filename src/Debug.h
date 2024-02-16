#pragma once
#define DEBUG_MODE

#ifdef DEBUG_MODE
#define LOG(msg) std::cout<<msg;
#define LOG_ENDL(msg) std::cout<<msg<<std::endl;
#define LOG_VAR(var) std::cout << #var << " = " << var << std::endl
#else
#define LOG(msg)
#define LOG_ENDL(msg)
#define LOG_VAR(var)
#endif
