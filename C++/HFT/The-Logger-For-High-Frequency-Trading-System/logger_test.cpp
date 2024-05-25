#include "logger.h"

using std::chrono::system_clock;

static char const* format = R"({"OrderId":"%","ClientId":"%","TickerId":"%","Price":"%$","Qty":%,"When":"%","Side":"%"}
)";

// mock to display the record logged in the terminal
// 在终端上显示日志
struct Mock {
        auto Write(std::string str) -> size_t {
                // locked for using the stdout
                std::lock_guard locked{mtx};
                std::cout << str;
                return str.size();
        }
};

int main() {
        farmInit(cpus{2}, cpuIDs{0, 1}, cpuIDs{1, 2});

        Logger<Mock> logger{"logger#1", 2};
        std::thread th1([&]{
                Logger<Mock> logger{"logger#2", 1024};
                logger.log(format, "9999", "AAAAA", "9999", "13.5", 100000, system_clock::now(), "buy");
                logger.log(format, "9999", "AAAAA", "9999", "13.5", 100000, system_clock::now(), "buy");
                logger.log(format, "9999", "AAAAA", "9999", "13.5", 100000, system_clock::now(), "buy");
                logger.log(format, "9999", "AAAAA", "9999", "13.5", 100000, system_clock::now(), "buy");
                logger.close();
        });

        logger.log(format, "9999", "BBBBB", "9999", "14", 100000, system_clock::now(), "sell");
        logger.log(format, "9999", "BBBBB", "9999", "14", 100000, system_clock::now(), "sell");
        logger.log(format, "9999", "BBBBB", "9999", "14", 100000, system_clock::now(), "sell");
        logger.log(format, "9999", "BBBBB", "9999", "14", 100000, system_clock::now(), "sell");
        logger.close();

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