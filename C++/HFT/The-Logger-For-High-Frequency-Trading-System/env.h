#pragma once

#include "farm.h"
#include "macros.h"

#include <signal.h>
#include <fstream>
#include <dlfcn.h>
#include <string.h>

// farm env
inline static std::unique_ptr<Farm<farming*>> gFarm{};

inline auto farmInit(int n, cpuIDs c1, auto... cs) noexcept(false) -> void {
        struct farmInitException final : std::exception {
                const char* what() const noexcept override {
                        return "farmInitException";
                } 
                ~farmInitException() noexcept override = default;
        };

        
        if (gFarm) {
                throw farmInitException{};
        }
        gFarm = std::make_unique<Farm<farming*>>(
                (int)(n),
                std::move(c1),
                std::move(cs)...
        );
}
