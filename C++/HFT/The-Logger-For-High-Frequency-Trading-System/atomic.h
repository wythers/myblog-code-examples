#pragma once

#include <atomic>

template <typename T>
concept IsObject = std::is_object_v<T>;

enum MemOrder : int8_t { relax, rel_req };

template <IsObject _Tp, MemOrder O = MemOrder::relax>
class atomic : public std::atomic<_Tp> {
  using base = std::atomic<_Tp>;

  static constexpr auto M1 = (O == MemOrder::relax ? std::memory_order_relaxed
                                                   : std::memory_order_release);
  static constexpr auto M2 = (O == MemOrder::relax ? std::memory_order_relaxed
                                                   : std::memory_order_acquire);

 public:
  operator _Tp() const noexcept { return load(); }

  operator _Tp() const volatile noexcept { return load(); }

  _Tp operator=(_Tp __i) noexcept {
    store(__i);
    return __i;
  }

  _Tp operator=(_Tp __i) volatile noexcept {
    store(__i);
    return __i;
  }

  void store(_Tp __i) noexcept { base::store(__i, M1); }

  void store(_Tp __i) volatile noexcept { base::store(__i, M1); }

  _Tp load() const noexcept { return base::load(M2); }

  _Tp load() const volatile noexcept { return base::load(M2); }

  bool compare_exchange_weak(_Tp& __i1, _Tp __i2, std::memory_order __m) noexcept {
    return base::compare_exchange_weak(__i1, __i2, __m,
                                       std::memory_order_relaxed);
  }

  bool compare_exchange_weak(_Tp& __i1, _Tp __i2,
                             std::memory_order __m) volatile noexcept {
    return base::compare_exchange_weak(__i1, __i2, __m,
                                       std::memory_order_relaxed);
  }

  bool compare_exchange_strong(_Tp& __i1, _Tp __i2,
                               std::memory_order __m) volatile noexcept {
    return base::compare_exchange_strong(__i1, __i2, __m,
                                         std::memory_order_relaxed);
  }

  bool compare_exchange_strong(_Tp& __i1, _Tp __i2,
                               std::memory_order __m) noexcept {
    return base::compare_exchange_strong(__i1, __i2, __m,
                                         std::memory_order_relaxed);
  }
};
