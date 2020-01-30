#include "seastar/core/app-template.hh"
#include "seastar/core/reactor.hh"
#include "seastar/core/sleep.hh"
#include <iostream>
#include <time.h>
#include <chrono>

#include "15palindrome.h"

using namespace seastar;
using namespace std::chrono_literals;






future<> app_main() {
        std::cout << "Hello world\n";
        std::cout << smp::count << "\n";

        std::cout << "Sleeping... " << std::flush;
        return when_all(
                sleep(200ms).then([] { std::cout << "200ms " << std::flush; }),
                sleep(100ms).then([] {
                        std::cout << "100ms " << std::flush;
                        return when_all(
                                sleep(200ms).then([] { std::cout << "200ms " << std::flush; }),
                                sleep(2000ms).then([] { std::cout << "2000ms " << std::flush; })
                        );
                })
        ).then([] (auto tup     ) {
                std::cout << "Done.\n";
        });
}

int main(int argc, char** argv) {
    clock_t clock_begin = clock();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    app_template app;
    try {
        app.run(argc, argv, app_main);
    } catch(std::runtime_error &e) {
        std::cerr << "Couldn't start application: " << e.what() << "\n";
        return 1;
    }

    printf("clock elapsed: %lf ms\n", (double) (clock() - clock_begin) * 1000 / CLOCKS_PER_SEC);
    printf("chrono elapsed: %lf ms\n",
           std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::steady_clock::now() - begin).count());
}

