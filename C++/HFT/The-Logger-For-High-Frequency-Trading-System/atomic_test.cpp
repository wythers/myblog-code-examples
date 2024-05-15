#include "atomic.h"

#include <stdio.h>

int main() {
        atomic<int, MemOrder::relax> n{10};

        int i = n;
        auto b = n.compare_exchange_weak(i, 11, std::memory_order_relaxed);

        printf("%d\n", int(n));
}