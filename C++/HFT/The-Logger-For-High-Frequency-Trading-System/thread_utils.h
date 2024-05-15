#include "macros.h"

#include <iostream>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <memory>

#include <sys/syscall.h>

inline auto setThreadAffinity(cpuIDs const& core_ids) noexcept -> bool {
        cpu_set_t set{};
        
        for (auto core_id : core_ids.cs) {
                CPU_ZERO(&set);
                CPU_SET(core_id, &set);
        }

        return (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &set) == 0);
}

template<typename F, typename... Args>
inline auto createAndStartThread(cpuIDs core_id, std::string name, F&& task, Args&&... args) -> std::thread {
        auto t = std::thread([&, task_ = std::move(task), core_id_ = std::move(core_id), name_ = std::move(name)]{
                if (core_id_.size() > 0 && !setThreadAffinity(core_id_)) {
                        FATAL("Failed to set core affinity for ", name_, pthread_self(), "to",  core_id_.toString());
                }
                MSG("Set core affinity for ", name_, pthread_self(), "to",  core_id_.toString());

                task_(std::forward<Args>(args)...);
        });

        return t;
}
