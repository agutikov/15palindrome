#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/sleep.hh>
#include <seastar/core/queue.hh>
#include <seastar/core/temporary_buffer.hh>
#include <seastar/core/thread.hh>
#include <iostream>
#include <time.h>
#include <chrono>
#include <memory>
#include <algorithm>
#include <tuple>

#include "15palindrome.h"

using namespace seastar;
using namespace std::chrono_literals;


#define PRIME_BUFFER_SIZE (1024*256)
#define BASE36_BUFFER_SIZE (PRIME_BUFFER_SIZE*10)


typedef std::vector<uint64_t> prime_buffer_t;
typedef std::pair<prime_buffer_t, std::unique_ptr<primesieve_iterator>>  prime_gen_result_t;
typedef seastar::temporary_buffer<char> char_buffer_t;
typedef std::pair<char_buffer_t, char_buffer_t> convert_result_t;
typedef std::tuple<bool, size_t, char_buffer_t> search_result_t;


prime_gen_result_t do_generate_primes(std::unique_ptr<primesieve_iterator> pi)
{
        prime_buffer_t primes(PRIME_BUFFER_SIZE);

        std::generate(primes.begin(), primes.end(), [p = pi.get()] { return primesieve_next_prime(p); });

        return std::make_pair(std::move(primes), std::move(pi));
}

seastar::future<prime_gen_result_t> generate_primes(std::unique_ptr<primesieve_iterator> pi)
{
        return seastar::async([] (std::unique_ptr<primesieve_iterator> pi) {
                return do_generate_primes(std::move(pi));
        }, std::move(pi));
}

convert_result_t do_convert_base36(prime_buffer_t primes, char_buffer_t head)
{
        char_buffer_t base36_buffer(BASE36_BUFFER_SIZE);

        if (head.size() > 0) {
                memcpy(base36_buffer.get_write(), head.get(), head.size()); 
        }

        size_t len = head.size() + base36_print_buffer_2(base36_buffer.get_write() + head.size(), primes.data(), primes.size());

        base36_buffer.trim(len);

        return std::make_pair(std::move(base36_buffer), base36_buffer.share(base36_buffer.size() - 14, 14));
}

seastar::future<convert_result_t> convert_base36(prime_buffer_t primes, char_buffer_t head)
{
        return seastar::async([] (prime_buffer_t primes, char_buffer_t head) {
                return do_convert_base36(std::move(primes), std::move(head));
        }, std::move(primes), std::move(head));
}

search_result_t do_palindrome15_search(char_buffer_t buffer, size_t total_offset)
{
        ssize_t buffer_offset = search_15_palindrome(buffer.begin(), buffer.end());
        if (buffer_offset >= 0) {
                return std::make_tuple(true, total_offset + buffer_offset, buffer.share(buffer_offset, 15));
        } else {
                return std::make_tuple(false, total_offset + buffer.size() - 14, char_buffer_t());
        }
}

seastar::future<search_result_t> palindrome15_search(char_buffer_t buffer, size_t total_offset)
{
        return seastar::async([] (char_buffer_t buffer, size_t total_offset) {
                return do_palindrome15_search(std::move(buffer), total_offset);
        }, std::move(buffer), total_offset);
}


// with fibers and pipes
future<> app_main_pipes()
{
        return seastar::make_ready_future<>();
}

// with recursive async tasks
future<> app_main_async_recursive()
{
        return seastar::make_ready_future<>();
}

// synchronous: app_main starts 3 tasks and waits for when_all() then checks results and continue
future<> app_main_sync_concurrent()
{
        return seastar::make_ready_future<>();
}

// with async continuations
future<> app_main_async_continuations()
{
        // std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();

        std::cout << "Initialize..." << std::endl;
        init_base36_r2();

        auto pi = std::make_unique<primesieve_iterator>();
        primesieve_init(pi.get());

        std::cout << "Start async processing..." << std::endl;

        return generate_primes(std::move(pi)).then(
                [] (auto r0) {
                        std::cout << "r0" << std::endl;

                        prime_buffer_t primes = std::move(r0.get0().first);
                        std::unique_ptr<primesieve_iterator> pi = std::move(r0.get0().second);

                        return when_all(
                                generate_primes(std::move(pi)),
                                convert_base36(primes, char_buffer_t())
                        ).then([] (auto r1) {
                                std::cout << "r1" << std::endl;

                                prime_gen_result_t gr = std::move(std::get<0>(r1).get0());
                                convert_result_t cr = std::move(std::get<1>(r1).get0());

                                return when_all(
                                        generate_primes(std::move(gr.second)),
                                        convert_base36(gr.first, cr.second),
                                        palindrome15_search(cr.first, 0)
                                ).then([] (auto r2) {
                                        std::cout << "r2" << std::endl;

                                        prime_gen_result_t gr = std::move(std::get<0>(r2).get0());
                                        convert_result_t cr = std::move(std::get<1>(r2).get0());
                                        search_result_t sr = std::move(std::get<2>(r2).get0());

                                        std::cout << gr.first.size() << std::endl;
                                        std::cout << gr.second.get() << std::endl;

                                        std::cout << cr.first.size() << std::endl;
                                        std::cout << cr.second.size() << std::endl;

                                        std::cout << std::get<0>(sr) << std::endl;
                                        std::cout << std::get<1>(sr) << std::endl;
                                        std::cout << std::get<2>(sr).size() << std::endl;

                                        //TODO: How to implement cycles with continuations?

                                        return seastar::make_ready_future<>();
                                });
                        });
                }
        );
}


// synchronous, single threaded
future<> app_main_sync_sequential()
{
        std::cout << smp::count << std::endl;

        auto started = std::chrono::steady_clock::now();

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

        auto finished = std::chrono::steady_clock::now();

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
                std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(finished - started).count());

        return seastar::make_ready_future<>();
}


int main(int argc, char** argv)
{
        app_template app;
        try {
                app.run(argc, argv, app_main_async_continuations);
        } catch(std::runtime_error &e) {
                std::cerr << "Couldn't start application: " << e.what() << "\n";
                return 1;
        }
}

