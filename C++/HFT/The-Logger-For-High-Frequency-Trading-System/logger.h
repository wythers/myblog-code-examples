#pragma once

#include "env.h"
#include "logging.h"

#include <sstream>
#include <utility>
#include <iomanip>

// e.g: std::pow(2, 10)
template<int N>
concept IsBaseTwo = (N-1)&N ? false : true; 

template<typename T>
concept IsLogger = requires(T l, char const* format) {
        { l.log(format) } -> void;
};

template<IsLogger T>
class Logger {
public:
        Logger(auto&&... args) : m_log(std::forward<decltype(args)>(args)...) {}

        auto log(char const* format, auto&&... args) -> void {
                m_log.log(format, std::forward<decltype(args)>(args)...);
        }

        ~Logger() {
                m_log.close();
        }
private:
        T m_log{};
};

template<IsWriter T, IsBaseTwo CAP = 0>
class Log {
        using loggingType = logging<T>;
        using HeaderWithCS = typename logging<T>::HeaderWithCS;
        using tailer = typename logging<T>::tailer;

public:
        Log(
                std::string name,
                auto&&... args) : m_name(std::move(name)), m_writer(new T(std::forward<decltype(args)>(args)...)) {

                m_task = new logging{m_name + "-slave", CAP == 0 ? (int)sysconf(_SC_PAGESIZE) : CAP, m_writer};
                gFarm->Add(m_task);
        }

        auto close() noexcept -> void {
                if (m_task) {
                        m_task->close();
                        m_task = nullptr;
                        is_closed = true;
                }
        }

        auto name() noexcept -> std::string {
                return m_name;
        }

        template<typename... Args>
        auto log(char const* format, Args&&... args) -> void {
                constexpr size_t N = sizeof...(Args) + 1 + 1;

                if (!is_closed && m_task->touch() >= N) [[likely]] {
                        m_task->tryAssign(
                                loggingType::encode(HeaderWithCS{format}),
                                loggingType::encode(std::move(args))...,
                                loggingType::encode(tailer{})
                        );
                        return;
                }

                std::stringstream buf{};     
                buf.setf(std::ios::fixed);
                expand(buf, format, std::move(args)...);

                m_writer->write(std::move(buf).str());
        }

        ~Log() {
                if (m_task) {
                        m_task->close();
                }
        }

private:
        auto expand(std::stringstream& buf, char const* s, auto arg, auto... args) noexcept -> void {
                while (*s) {
                        if (*s == '%') {
                                if (*(s+1) == '%') [[unlikely]] {
                                        s++;
                                } else {
                                        buf << arg;
                                        expand(buf, s+1, std::move(args)...);
                                        return;
                                }
                        }
                        buf << *s;
                        s++;
                }
        }

        auto expand(std::stringstream& buf, char const* s) noexcept -> void {
                while (*s) {
                        if (*s == '%') [[unlikely]] {
                                s++;
                                if (*s == '\0') [[unlikely]]
                                        return;
                        }
                        buf << *s;
                        s++;
                }
        }



private:
        std::string m_name{};

        logging<T>* m_task{};
        bool is_closed{};

        T* m_writer{};
};
