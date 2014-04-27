#ifndef HELPER_H
#define HELPER_H 

#include <string>

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

static inline std::string trim(const std::string& str, const std::string& target=" \r\n\t") {
	const size_t first = str.find_first_not_of(target);

	if(first == std::string::npos) {
		return std::string();
	} else {
		const size_t last = str.find_last_not_of(target);
		return str.substr(first, last - first + 1);
	}
}

#endif /* HELPER_H */
