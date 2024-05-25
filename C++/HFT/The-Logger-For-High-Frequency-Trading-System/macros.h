#pragma once 

#include <iostream>
#include <mutex>
#include <vector>
#include <sstream>

#include <string.h>

inline static std::mutex mtx{};

inline auto ASSERT(bool cond, char const* msg) -> void {
        using namespace std;

        if (!cond) [[unlikely]] {
                fprintf(stderr, "ASSERT: %s\n", msg);
                exit(EXIT_FAILURE);
        }
}

inline auto MSG(auto... msgs) -> void {
        using namespace std;
        auto spaceBefore = [](auto const& arg) {
                std::cout << ' ';
                return arg;
        };
        {
                std::lock_guard<std::mutex> locked{mtx};
                cout << "MSG:";
                (cout << ... << spaceBefore(msgs));
                cout << endl;   
        }
}


inline auto FATAL(auto... args) -> void {
        using namespace std;
        auto spaceBefore = [](auto const& arg) {
                std::cout << ' ';
                return arg;
        };
        {
                std::lock_guard<std::mutex> locked{mtx};
                cout << "FATAL:";
                (cout << ... << spaceBefore(args));
                cout << endl;
        }
                exit(EXIT_FAILURE);
}

class cpus {
public:
        explicit operator int() {
                return m_cpus;
        }
public:
        int m_cpus{};
};

class cpuIDs {
public:
        cpuIDs(auto... ids) : cs{ids...} {}  

        auto size() const {
                return cs.size();
        }      

        auto toString() const {
                std::stringstream ss;
                if (cs.size() == 0) {
                        ss << -1 <<" ";
                        return ss.str();
                }

                for (auto i : cs) {
                        ss << "core:" << i << " ";
                }
                
                return ss.str();
        }

        std::vector<int> cs{};
};


namespace std {
        template<typename _Traits>
        inline basic_ostream<char, _Traits>&
        operator<<(basic_ostream<char, _Traits>& out, chrono::_V2::system_clock::time_point tp) {
                char stamp[26]{};
                time_t tm{std::chrono::system_clock::to_time_t(tp)};
                ctime_r(&tm, stamp);
                stamp[strlen(stamp)-1] = '\0';

                return out << stamp;
        }
};

#define __ROTATION std::this_thread::yield();

