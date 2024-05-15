#include "queue.h"

#include <unistd.h>

#include <iostream>

int main() {
        queue<int, QueType::ssque> q{16};

        for (int j = 0; j < 3; j++) {
        for (int i = 1; i <= 16; i++) {
                if(!q.tryPush(i + j*10)) {
                        std::cout << "fail " << j << std::endl; 
                        return -1;
                }
        
        }

        auto [p, off, size] = q.tryGet();
        if (size == 0) {
                std::cout << "fail" << std::endl;
                return -1;
        }

        for (int i = 0; i < size; i++) {
                int idx = (off+i) & (p->size()-1);
                std::cout << (*p)[idx] << std::endl;
        }
        q.updateReadIdx(size);

        std::cout << "-----------" << std::endl;
        }

        queue<int, QueType::smque> q2{};

}