#pragma once

#include "db.h"

#include <map>
#include <stdio.h>
#include <string>
#include <mutex>
#include <memory>

class fileDB {
        struct fileDBException final : std::exception {
                const char* what() const noexcept override {
                        return "fileDBException";
                } 
                ~fileDBException() noexcept override = default;
        };

public:
        fileDB() {
                std::lock_guard<std::mutex> locked{m_mtx};
                db["stdout"] = {stdout, &mtx};
        }

        auto select(std::string sql) -> std::pair<FILE*, std::mutex*> {
                std::lock_guard<std::mutex> locked{m_mtx};
                if (db.count(sql)) {
                        auto& p = db[sql];
                        return {p.first, p.second};
                }

                FILE* fd = fopen(sql.c_str(), "a");
                if (fd == nullptr) {
                        FATAL("fileDBException");
                }
                std::mutex* m = new std::mutex{};
                db[std::move(sql)] = {fd, m};
                return {fd, m};
        }

        ~fileDB() {
                for (auto const& fm : db) {
                        auto const [f, m] = fm.second;
                        if (fm.first != "stdout") {
                                std::unique_ptr<std::mutex> cleanup{m};
                                fflush(f);
                                fclose(f);
                        }
                }
        }

private:
        std::mutex m_mtx{};
        std::map<std::string, std::pair<FILE*, std::mutex*>> db{};
};
