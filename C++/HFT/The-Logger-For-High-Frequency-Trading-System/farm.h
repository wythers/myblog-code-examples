#pragma once

#include "queue.h"
#include "macros.h"
#include "thread_utils.h"
#include "atomic.h"

#include <vector>
#include <memory>
#include <tuple>

struct farming {
        using farming_t = bool (*)(farming*);

        auto operator()() -> bool {
                return (*task)(this);
        }

        farming(farming_t task_) : task(task_) {}

        farming_t task{};
};

template<typename T>
class Farm {
        using Que = queue<T, QueType::deque>;

        auto holding() noexcept -> void {
                while (!m_is_closed || m_cnt != 0) {
                        T f{};
                        if (fromLocal(f) || fromShared(f)) [[likely]] {
                                if ((*f)()) [[likely]] {
                                        reco(std::move(f));
                                } else {
                                        m_cnt.fetch_sub(1, std::memory_order_relaxed);
                                }
                        } else {
                                __ROTATION;
                        }
                }
        }

        auto fromLocal(T& f) noexcept -> bool {
                return m_localQue->tryGet(f);
        }

        auto fromShared(T& f) noexcept -> bool {
                int mod = m_ques.size();
                for (int i = 0; i < mod; i++) {
                        int idx = (m_index + i) & (mod-1);
                        if (m_ques[idx]->trySteal(f)) {
                                m_index = idx;
                                return true;
                        }
                }
                return false;
        }

        auto reco(T&& f) noexcept -> void {
                m_localQue->pushBack(std::move(f));
        }

        template<size_t...>
        struct _Sque {};

        template<size_t N>
        using _Indices = _Sque<__integer_pack(N)...>;

        template<size_t I, size_t... Is>
        inline constexpr void expand(auto& tp, _Sque<I, Is...>) {
                m_ths.push_back(createAndStartThread(std::move(std::get<I>(tp)), "Farm thread",
                [this]{
                        m_index = I+1;
                        m_localQue = m_ques[I].get();

                        holding();     
                }));
                expand(tp, _Sque<Is...>{});
        }

        inline constexpr void expand(auto& tp, _Sque<>) {}

public:
        template<typename... IDs>
        Farm(int n, IDs&&... is) {
                std::tuple<IDs...> tp{std::move(is)...};
                constexpr auto N = sizeof...(IDs);

                // TODO: static_assert

                for (int j = 0; j < n; j++) {
                        m_ques.emplace_back(new Que{});
                }

                expand(tp, _Indices<N>{});       
        }

        auto Add(T&& f) noexcept -> void {
                int idx = (m_cnt.load()) & (m_ques.size()-1);
                m_ques[idx]->pushBack(std::move(f));
                m_cnt.fetch_add(1, std::memory_order_relaxed);
        }

        auto Close() noexcept -> void {
                m_is_closed.store(true);
        }

        ~Farm() {
                Close();
                for (auto& th : m_ths) {
                        th.join();
                }
        }

private:
        atomic<bool> m_is_closed{};
        atomic<int> m_cnt{};

        std::vector<std::unique_ptr<Que>> m_ques{};
        std::vector<std::thread> m_ths{};

inline static thread_local int m_index{};
inline static thread_local Que* m_localQue{};
};
