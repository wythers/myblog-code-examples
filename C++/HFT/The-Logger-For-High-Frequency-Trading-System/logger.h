#pragma once

#include "env.h"
#include "logging.h"

class Logger {
public:
        Logger(std::string name, std::string file, int size = sysconf(_SC_PAGESIZE)) : m_name(std::move(name)) {
                auto [f, m] = glog_file_db.select(file);
                m_task = new logging{m_name + "-slave", f, m, size};

                m_fd = f; m_mtx = m;

                gFarm->Add(m_task);
        }

        auto close() noexcept -> void {
                if (m_task) {
                        m_task->close();
                        m_task = nullptr;
                        is_closed = true;
                }
        }

        template<typename... Args>
        auto log(char const* format, Args&&... args) -> void {
                constexpr size_t N = sizeof...(Args) + 1 + 1;

                if (!is_closed && m_task->touch() >= N) [[likely]] {
                        m_task->tryAssign(
                                logging::encode(logging::HeaderWithCS{format}),
                                logging::encode(std::move(args))...,
                                logging::encode(logging::tailer{})
                        );
                        return;
                }

                auto tp = std::chrono::system_clock::now();
                char stamp[26]{};
                time_t tm{std::chrono::system_clock::to_time_t(tp)};
                ctime_r(&tm, stamp);
                stamp[strlen(stamp)-1] = '\0';

                std::lock_guard<std::mutex> locked{*m_mtx};
                fprintf(m_fd, "[%s] %s: ", stamp, m_name.c_str());
                expand(format, std::move(args)...);
        }

        ~Logger() {
                if (m_task) {
                        m_task->close();
                }
        }

private:
        auto expand(char const* s, auto arg, auto... args) noexcept -> void {
                struct dynamic_strait {
                        auto operator()(char c) noexcept {
                                fprintf(fd, "%c", c);
                        }
                        auto operator()(int i) noexcept {
                                fprintf(fd, "%d", i);
                        }
                        auto operator()(long l) noexcept {
                                fprintf(fd, "%ld", l);
                        }
                        auto operator()(long long ll) noexcept {
                                fprintf(fd, "%lld", ll);
                        }
                        auto operator()(unsigned u) noexcept {
                                fprintf(fd, "%u", u);
                        }
                        auto operator()(unsigned long ul) noexcept {
                                fprintf(fd, "%lu", ul);
                        }
                        auto operator()(unsigned long long ull) noexcept {
                                fprintf(fd, "%llu", ull);
                        }
                        auto operator()(float f) noexcept {
                                fprintf(fd, "%.6f", f);
                        }
                        auto operator()(double d) noexcept {
                                fprintf(fd, "%.6lf", d);
                        }
                        auto operator()(char const* cs) noexcept {
                                fprintf(fd, "%s", cs);
                        }
                        auto operator()(std::string const& s) noexcept {
                                fprintf(fd, "%s", s.c_str());
                        }                       
                        FILE* fd{};
                } strait{m_fd} ;

                while (*s) {
                        if (*s == '%') {
                                if (*(s+1) == '%') [[unlikely]] {
                                        s++;
                                } else {
                                        strait(arg);
                                        expand(s+1, std::move(args)...);
                                        return;
                                }
                        }
                        strait(*s);
                        s++;
                }
        }

        auto expand(char const* s) noexcept -> void {
                while (*s) {
                        if (*s == '%') [[unlikely]] {
                                s++;
                                if (*s == '\0') [[unlikely]]
                                        return;
                        }
                        fprintf(m_fd, "%c", *s);
                        s++;
                }
                fflush(m_fd);
        }

private:
        std::string m_name{};

        logging* m_task{};
        bool is_closed{};

        FILE* m_fd{};
        std::mutex* m_mtx{};
};
