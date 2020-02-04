#include "seastar/core/app-template.hh"
#include "seastar/core/reactor.hh"
#include "seastar/core/sleep.hh"
#include <iostream>
#include <time.h>
#include <chrono>

#include "15palindrome.h"

using namespace seastar;
using namespace std::chrono_literals;


#define PRIME_BUFFER_SIZE (1024*256)
#define BASE36_BUFFER_SIZE (PRIME_BUFFER_SIZE*10)


future<> app_main() {
        std::cout << smp::count << std::endl;

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        int success = 0;
        ssize_t buffer_offset = 0; // for correct offset
        size_t total_offset = 0; // for correct offset

        primesieve_iterator it;
        primesieve_init(&it);
        uint64_t prime_buffer[PRIME_BUFFER_SIZE];
        char base36_buffer[BASE36_BUFFER_SIZE];
        char* ptr = base36_buffer;

        init_base36_r2();

        while(1) {
                for (size_t i = 0; i < sizeof(prime_buffer)/sizeof(prime_buffer[0]); i++) {
                        prime_buffer[i] = primesieve_next_prime(&it);
                }
                
                for (size_t i = 0; i < sizeof(prime_buffer)/sizeof(prime_buffer[0]); i++) {
                        ptr = base36_r2(ptr, prime_buffer[i]);
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

        printf("chrono elapsed: %lf ms\n",
                std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::steady_clock::now() - begin).count());

        return seastar::make_ready_future<>();
}


int main(int argc, char** argv) {
        app_template app;
        try {
                app.run(argc, argv, app_main);
        } catch(std::runtime_error &e) {
                std::cerr << "Couldn't start application: " << e.what() << "\n";
                return 1;
        }
}

