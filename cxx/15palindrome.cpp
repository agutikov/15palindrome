#include "seastar/core/app-template.hh"
#include "seastar/core/reactor.hh"
#include "seastar/core/sleep.hh"
#include <iostream>
#include <time.h>
#include <chrono>

#include "15palindrome.h"

using namespace seastar;
using namespace std::chrono_literals;


#define BASE36_BUFFER_SIZE (4096*16)


future<> app_main() {
        std::cout << smp::count << "\n";

        init_base36_r2();

        int success = 0;
        ssize_t buffer_offset = 0; // for correct offset
        size_t total_offset = 0; // for correct offset

        primesieve_iterator it;
        primesieve_init(&it);
        char base36_buffer[BASE36_BUFFER_SIZE];
        char* ptr = base36_buffer;

        while(1) {
                while(ptr < (base36_buffer + sizeof(base36_buffer) - 16)) {
                        uint64_t prime = primesieve_next_prime(&it);
                        ptr = base36_r2(ptr, prime);
                }
                buffer_offset = search_15_palindrome(base36_buffer, ptr);
                if (buffer_offset >= 0) {
                        total_offset += buffer_offset;
                        success = 1;    
                        break;
                }
                total_offset += ptr - base36_buffer - 14;
                memcpy(base36_buffer, ptr-14, 14);
                ptr = base36_buffer + 14;
        }

        primesieve_free_iterator(&it);

        if (success) {
                printf("\n");
                printf("Palindrome found ");
                for (size_t i = 0; i < 15; i++) {
                        printf("%c", base36_buffer[buffer_offset + i]);
                }
                printf( " at byte %lu\n", total_offset);
        } else {
                printf("Can't find palindrome\n");
        }

        return seastar::make_ready_future<>();
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

