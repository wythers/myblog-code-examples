#pragma once

#include "atomic.h"

#include <deque>
#include <vector>
#include <mutex>
#include <array>
#include <tuple>
#include <unistd.h>

enum QueType : int8_t {
        deque,  // MT-safe
        ssque, // SPSC-safe(single producer-single consumer)
        smque // SPMC-safe
};
template<typename, QueType >
class queue;

template<typename T>
class queue<T, QueType::smque> {
        struct alignas(64) atomicIn64bytes : std::atomic<T*> {};
        static constexpr auto def_cap = 128;
public:
	auto push(T* val) noexcept -> bool {
		int len = m_slots.size();
		auto [tail, head] = m_headAndtail.load(std::memory_order_relaxed);
		if (tail + len == head) 
			return false;

		std::atomic<T *>& slot = m_slots[head & (len - 1)];
		if (slot.load(std::memory_order_relaxed))
			return false;

		slot.store(val, std::memory_order_release);
                m_headAndtail.store({tail, head+1}, std::memory_order_relaxed);
		return true;
	}

        auto delta() noexcept -> int32_t {
                auto [tail, head] = m_headAndtail.load(std::memory_order_relaxed);
                auto mod = m_slots.size()-1;
                auto i = tail&mod, j = head&mod;
                
                return j >= i ? def_cap-(j-i) : i-j;
        }

        auto size() noexcept -> int32_t {
                auto [tail, head] = m_headAndtail.load(std::memory_order_relaxed);
                return head-tail;
        }

	auto pop() noexcept -> std::pair<T*, bool> {
		int idx{}, len = m_slots.size();
		for (;;) {
			auto cur = m_headAndtail.load(std::memory_order_relaxed);
			auto [tail, head] = cur;

			if (head == tail) 
				return {nullptr, false};
			
			decltype(cur) tmp = {tail+1, head};
			if (m_headAndtail.compare_exchange_weak(cur, tmp, std::memory_order_relaxed, std::memory_order_relaxed)) {
				idx = tail & (len-1);
				break;
			}
		}

		auto& slot = m_slots[idx];
		T* ret = slot.exchange(nullptr, std::memory_order_acquire);

		return {ret, true};
	}

	queue() : m_slots{} {}

private:
	struct packed {
		uint32_t tail{};
		uint32_t head{};
	};
	std::atomic<packed>   m_headAndtail{};
        unsigned char __padding[56]{};

        std::array<atomicIn64bytes, def_cap> m_slots{};
};

template<typename T>
class queue<T, QueType::ssque> {

        auto push(size_t mod, size_t i, auto arg, auto... args) noexcept -> size_t {
                m_store[i&(mod-1)] = std::move(arg);
                return push(mod, i+1, args...);
        }

        auto push(size_t mod, size_t i) {
                return (i & (mod-1));
        }


public:
        queue(int size = sysconf(_SC_PAGESIZE)) : m_store(size) {}

        template<typename... Args>
        auto tryPush(Args... args) noexcept -> bool {
                int size = m_size.load(std::memory_order_relaxed);
                int diff = m_store.size() - size;

                constexpr auto N = sizeof...(Args);
                if (diff == 0 || N > diff) {
                        return false;
                } 

                int wx = m_wx;
                m_wx = push(m_store.size(), wx, args...);

                m_size.fetch_add(N, std::memory_order_release);
                return true;
        }

        auto tryGet() noexcept -> std::tuple<std::vector<T>*, size_t, size_t> {
                int size = m_size.load(std::memory_order_acquire);
                if (size == 0) {
                        return {nullptr, 0, 0};
                }
                return {&m_store, m_rx, size};
        }

        auto updateReadIdx(size_t off) -> void {
                m_rx = (m_rx + off) & (m_store.size()-1);

                m_size.fetch_sub(off, std::memory_order_relaxed);
        }

        auto size() noexcept -> size_t {
                return m_size.load(std::memory_order_relaxed);
        }

        auto cap() noexcept -> size_t {
                return m_store.size();
        }

        auto diff() noexcept -> size_t {
                return cap() - size();
        }

private:
        std::atomic<size_t> m_size{};
        std::vector<T>  m_store{};

        size_t m_wx{};
        size_t m_rx{};
};

template<typename T>
class queue<T, QueType::deque> {

        auto isEmpty() noexcept -> bool {
                return m_data.empty();
        }

public:

        auto push(T&& d) noexcept -> void {
                std::lock_guard<std::mutex> locked{m_mtx};
                m_data.push_front(std::forward<T>(d));
        }

        auto pushBack(T&& d) noexcept -> void {
                std::lock_guard<std::mutex> locked{m_mtx};
                m_data.push_back(std::forward<T>(d));
        }

        auto tryGet(T& val) noexcept -> bool {                
                std::lock_guard<std::mutex> locked{m_mtx};
                if (isEmpty()) {
                        return false;
                }
                val = std::move(m_data.front());
                m_data.pop_front();
                return true;
        }

        auto trySteal(T& val) noexcept -> bool {
                std::lock_guard<std::mutex> locked{m_mtx};
                if (isEmpty()) {
                        return false;
                }
                val = std::move(m_data.back());
                m_data.pop_back();
                return true;
        }

private:
        std::mutex m_mtx{};
        std::deque<T> m_data{};
};
