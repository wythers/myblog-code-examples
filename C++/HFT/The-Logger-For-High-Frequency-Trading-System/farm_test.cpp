#include "farm.h"

#include "env.h"

int main() {
        farmInit(cpus{2}, cpuIDs{0}, cpuIDs{1});
        pause();
}