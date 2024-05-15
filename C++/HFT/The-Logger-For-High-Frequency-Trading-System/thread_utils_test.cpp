#include "thread_utils.h"

auto dummyFunction(int a, int b, bool sleep) {

        MSG("dummyFunction(", a, b, ')');
        MSG("dummyFunction output=", a+b);


  if(sleep) {
    MSG("dummyFunction sleeping...");

    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(2s);
  }

  MSG("dummyFunction done.");
}

int main(int, char **) {

  auto t1 = createAndStartThread(2, "dummyFunction1", dummyFunction, 12, 21, false);
  auto t2 = createAndStartThread(1, "dummyFunction2", dummyFunction, 15, 51, true);


        MSG("main waiting for threads to be done.");
  t1->join();
  t2->join();
  std::cout << "main exiting." << std::endl;
          MSG("main exiting.");

  return 0;
}
