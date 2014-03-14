#ifndef HELPER_H
#define HELPER_H 

#define STRINGIFY(x)	#x
#define TOSTRING(x)	STRINGIFY(x)

#define AT()		__FILE__ ":" TOSTRING(__LINE__)

#ifdef DO_DEBUG
#include <iostream>
#include <mutex>
extern std::mutex log_mutex;
#define DEBUG(x) do { std::lock_guard<std::mutex> guard(log_mutex); std::cerr << "[libnice] " << x << " (@" << AT() << ")" << std::endl; } while (0)
#else
#define DEBUG(x)
#endif

#endif /* HELPER_H */
