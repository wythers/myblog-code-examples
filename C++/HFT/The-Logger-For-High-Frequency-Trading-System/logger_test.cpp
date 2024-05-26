#include "logger.h"

#include <stdio.h>

using std::chrono::system_clock;

static char const* format = R"({"OrderId":"%","ClientId":"%","TickerId":"%","Price":"%$","Qty":%,"When":"%","Side":"%"}
)";

// mock to output the records to the mock.log file
// 在mock.log文件中记录日志.
struct Mock {
        struct Records {
                Records(char const* name) : f(fopen(name, "a")) {}
                FILE* f{};
        };

        auto write(std::string str) -> size_t {
                fprintf(records.f, str.c_str());
                return str.size();
        }

        static inline Records records{"mock.log"};
};

int main() {
        farmInit(3, cpuIDs{0, 1}, cpuIDs{0, 1}, cpuIDs{0, 1});

        Logger<Log<Mock, 2>> logger{"logger#1"};
        std::thread th1([&]{
                Logger<Log<Mock, 1024>> logger{"logger#2"};
                logger.log(format, "9999", "AAAAA", "9999", "13.5", 100000, system_clock::now(), "buy");
                logger.log(format, "9999", "AAAAA", "9999", "13.5", 100000, system_clock::now(), "buy");
                logger.log(format, "9999", "AAAAA", "9999", "13.5", 100000, system_clock::now(), "buy");
                logger.log(format, "9999", "AAAAA", "9999", "13.5", 100000, system_clock::now(), "buy");     
        });

        logger.log(format, "9999", "BBBBB", "9999", "14", 100000, system_clock::now(), "sell");
        logger.log(format, "9999", "BBBBB", "9999", "14", 100000, system_clock::now(), "sell");
        logger.log(format, "9999", "BBBBB", "9999", "14", 100000, system_clock::now(), "sell");
        logger.log(format, "9999", "BBBBB", "9999", "14", 100000, system_clock::now(), "sell");

        th1.join();

        gFarm->Close();
}

/*
one possible output is:
MSG: Set core affinity for  Farm thread 139841915594496 to core:0 core:1 
{"OrderId":"9999","ClientId":"BBBBB","TickerId":"9999","Price":"14$","Qty":100000,"When":"Sun May 26 05:36:18 2024","Side":"sell"}
{"OrderId":"9999","ClientId":"BBBBB","TickerId":"9999","Price":"14$","Qty":100000,"When":"Sun May 26 05:36:18 2024","Side":"sell"}
{"OrderId":"9999","ClientId":"BBBBB","TickerId":"9999","Price":"14$","Qty":100000,"When":"Sun May 26 05:36:18 2024","Side":"sell"}
{"OrderId":"9999","ClientId":"BBBBB","TickerId":"9999","Price":"14$","Qty":100000,"When":"Sun May 26 05:36:18 2024","Side":"sell"}
{"OrderId":"9999","ClientId":"AAAAA","TickerId":"9999","Price":"13.5$","Qty":100000,"When":"Sun May 26 05:36:18 2024","Side":"buy"}
{"OrderId":"9999","ClientId":"AAAAA","TickerId":"9999","Price":"13.5$","Qty":100000,"When":"Sun May 26 05:36:18 2024","Side":"buy"}
{"OrderId":"9999","ClientId":"AAAAA","TickerId":"9999","Price":"13.5$","Qty":100000,"When":"Sun May 26 05:36:18 2024","Side":"buy"}
{"OrderId":"9999","ClientId":"AAAAA","TickerId":"9999","Price":"13.5$","Qty":100000,"When":"Sun May 26 05:36:18 2024","Side":"buy"}
MSG: Set core affinity for  Farm thread 139841907201792 to core:1 core:2 
*/