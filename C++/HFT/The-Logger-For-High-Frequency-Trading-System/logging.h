#pragma once

#include "farm.h"

#include <string>
#include <chrono>
#include <cstring>

template<typename T>
concept IsWriter = requires(T w, std::string s) {
        { w.write(s) } -> size_t;
};

template <IsWriter T>
class logging : public farming {
public:
        enum class LogType : int8_t {
                CHAR = 0,
                INTEGER = 1,
                LONG_INTEGER = 2,
                LONG_LONG_INTEGER = 3,
                UNSIGNED_INTEGER = 4,
                UNSIGNED_LONG_INTEGER = 5,
                UNSIGNED_LONG_LONG_INTEGER = 6,
                FLOAT = 7,
                DOUBLE = 8,
                CHAR_CONST_POINTER = 9,
                STD_STRING_POINTER = 10,

                BEGcs = 1 << 6,// begin of block
                END = 1 << 5 // end of block
        };

        struct Block {
                LogType type = LogType::CHAR;
                union {
                        char c;
                        int i;
                        long l;
                        long long ll;

                        unsigned u;
                        unsigned long ul;
                        unsigned long long ull;
                        
                        float f;
                        double d;
                        
                        char const* cs;
                        std::string* s;
                } u{};
        };
        
        logging(std::string name_,
                int size,
                T* writer)
        :farming(&log), m_q{size}, m_name(std::move(name_)), m_writer(writer) {}

        auto close() noexcept -> void {
                is_closed.store(true, std::memory_order_relaxed);
        }

        auto touch() noexcept -> size_t {
                return m_q.diff();
        }

        auto tryAssign(auto... blocks) noexcept -> bool {
                return m_q.tryPush(blocks...);
        }

        static auto encode(char c) noexcept  {
                return Block{LogType::CHAR, {.c = c}};
        }

        static auto encode(int i) noexcept {
                return Block{LogType::INTEGER, {.i = i}};
        }

        static auto encode(long l) noexcept {
                return Block{LogType::LONG_INTEGER, {.l = l}};
        }

        static auto encode(long long ll) noexcept {
                return Block{LogType::LONG_LONG_INTEGER, {.ll = ll}};
        }

        static auto encode(unsigned u) noexcept {
                return Block{LogType::UNSIGNED_INTEGER, {.u = u}};
        }

        static auto encode(unsigned long ul) noexcept {
                return Block{LogType::UNSIGNED_LONG_INTEGER, {.ul = ul}};
        }

        static auto encode(unsigned long long ull) noexcept {
                return Block{LogType::UNSIGNED_LONG_LONG_INTEGER, {.ull = ull}};
        }

        static auto encode(float f) noexcept {
                return Block{LogType::FLOAT, {.f = f}};
        }

        static auto encode(double d) noexcept {
                return Block{LogType::DOUBLE, {.d = d}};
        }

        static auto encode(char const* cs) noexcept {
                return Block{LogType::CHAR_CONST_POINTER, {.cs = cs}};
        }

        static auto encode(std::string&& s) noexcept {
                std::string* sp = new std::string{std::move(s)};
                return Block{LogType::STD_STRING_POINTER, {.s = sp}};
        }

        static auto encode(std::chrono::_V2::system_clock::time_point tp) noexcept {
                char stamp[26]{};
                time_t tm{std::chrono::system_clock::to_time_t(tp)};
                ctime_r(&tm, stamp);
                stamp[strlen(stamp)-1] = '\0';

                return encode(std::string{stamp});
        }

        struct HeaderWithCS {
                char const* cs;
        };

        struct tailer {
        };

        static auto encode(HeaderWithCS hdr) noexcept {
                return Block{LogType::BEGcs, {.cs = hdr.cs}};
        }

        static auto encode(tailer) noexcept {
                return Block{LogType::END, {}};
        }


private:
        static auto split(std::vector<Block>& buf, int pos, int mod) noexcept -> std::pair<int, int> {
                int len{};
                for (;;) {
                        if (buf[pos].type == LogType::END) {
                                len++;
                                return {pos, len};
                        }
                        pos = (pos+1)&mod;
                        len++;
                }
                return {};
        }

        static auto log(farming* m) -> bool {
                auto* myself = static_cast<logging*>(m);
                if (!myself->is_closed.load(std::memory_order_relaxed) || myself->m_q.size() != 0) {
                        auto [buf, pos, len] = myself->m_q.tryGet();
                        if (buf == nullptr) {
                                return true;
                        }
                        int mod = myself->m_q.cap()-1;
                        
                        size_t diff = len;
                        while (diff) {
                                auto [pos_, len_] = split(*buf, pos, mod);
                                myself->formatting(*buf, pos, mod);
                                pos = (pos_+1)&mod;
                                diff -= len_;
                        }

                        myself->m_q.updateReadIdx(len);
                        return true;
                }
                
                std::unique_ptr<logging> _{myself};
                return false;
        }

        auto formatting(std::vector<Block>& buf, int begin, int mod) noexcept -> void {
                if (buf[begin].type == LogType::END) {
                        return;
                }

                char const* s = buf[begin].u.cs;
                begin = (begin+1)&mod;

                
                std::stringstream ss{}; 
                while (*s) {
                        if (*s == '%') [[unlikely]] {
                                if (*(s+1) == '%') [[unlikely]] {
                                        s++;
                                } else {
                                        if (buf[begin].type != LogType::END) [[likely]] {
                                                decode(ss, buf[begin]);
                                                begin = (begin+1)&mod;
                                        }
                                        s++;
                                        continue;
                                }
                        }
                        ss << *s;
                        s++;
                }

                m_writer->write(std::move(ss).str());
        }


        auto decode(std::stringstream& buf, Block const& block) noexcept -> void {
//                std::lock_guard<std::mutex> locked{mtx};
                switch (block.type) {
                        case LogType::CHAR:
                                buf << block.u.c;
                                break;
                        case LogType::INTEGER:
                                buf << block.u.i;
                                break;
                        case LogType::LONG_INTEGER:
                                buf << block.u.l;
                                break;
                        case LogType::LONG_LONG_INTEGER:
                                buf << block.u.ll;
                                break;
                        case LogType::UNSIGNED_INTEGER:
                                buf << block.u.u;
                                break;
                        case LogType::UNSIGNED_LONG_INTEGER:
                                buf << block.u.ul;
                                break;
                        case LogType::UNSIGNED_LONG_LONG_INTEGER:
                                buf << block.u.ull;
                                break;
                        case LogType::FLOAT:
                                buf << block.u.f;
                                break;
                        case LogType::DOUBLE:
                                buf << block.u.d;
                                break;
                        case LogType::CHAR_CONST_POINTER:
                                buf << block.u.cs;
                                break;
                        case LogType::STD_STRING_POINTER:
                                buf << *(block.u.s);
                                std::unique_ptr<std::string> cleanup{block.u.s};
                }
        }

private:
        queue<Block, QueType::ssque> m_q{};
        std::atomic<bool> is_closed{};

        std::string m_name{};

        std::unique_ptr<T> m_writer{};
};