#include "logger.h"

int main() {
        farmInit(cpus{2}, cpuIDs{0, 1}, cpuIDs{1, 2});

        Logger logger{"Master#1", "default.log", 8};
        std::thread th1([&]{
                Logger logger2{"Master#2", "default.log", 8};
                logger2.log("Logging a char:% an int:% and an unsigned:%\n", 'c');
                logger2.log("Logging a float:% and a double:%\n", 1.0f, (double)(34.65));
                logger2.log("Logging a C-string:'%'\n", "slave doing well");
                logger2.log("Logging a string:'%'\n", std::string("a string"));

                logger2.close();
        });

        logger.log("Logging a char:% an int:% and an unsigned:%\n", 'c');
        logger.log("Logging a float:% and a double:%\n", 1.0f, (double)(34.65));
        logger.log("Logging a C-string:'%'\n", "slave doing well");
        logger.log("Logging a string:'%'\n", std::string("a string"));
        logger.close();

        th1.join();

        gFarm->Close();
}