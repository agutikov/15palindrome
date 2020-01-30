#include <unistd.h>
#include <byteswap.h>
#include <primesieve.h>

const char jump_table[36] = {
        '0','1','2','3','4','5','6','7','8','9',
        'A','B','C','D','E','F','G','H','I','J',
        'K','L','M','N','O','P','Q','R','S','T',
        'U','V','W','X','Y','Z'
};

static inline uint64_t fastmod36(uint64_t n)
{
        const uint64_t c = UINT64_C(0xFFFFFFFFFFFFFFFF) / 36 + 1;
        uint64_t lowbits = c * n;
        return ((__uint128_t)lowbits * 36) >> 64; 
}

static inline uint64_t fastmod1296(uint64_t n)
{
        const uint64_t c = UINT64_C(0xFFFFFFFFFFFFFFFF) / 1296 + 1;
        uint64_t lowbits = c * n;
        return ((__uint128_t)lowbits * 1296) >> 64;
}

char* base36_r(char *dest, uint64_t num)
{
        uint64_t y = num % 36;
        //uint64_t y = fastmod36(num);
        uint64_t z = num / 36;
        if (z > 0) {
                dest = base36_r(dest, z);
        }
        // NOT tail-recursive, but faster than base36 if apply to batch
        *dest = jump_table[y];
        return dest + 1;
}

uint16_t jump_table_2[1296];

char* base36_r2(char *dest, uint64_t num)
{
        if (num < 36) {
                return base36_r(dest, num);
        }
        uint64_t y = num % 1296;
        //uint64_t y = fastmod1296(num);
        uint64_t z = num / 1296;
        if (z > 0) {
                dest = base36_r2(dest, z);
        }
        *((uint16_t*)dest) = jump_table_2[y];
        return dest + 2;
}

void init_base36_r2()
{
        memset(jump_table_2, 0x30, sizeof(jump_table_2));
        for (int i = 0; i < 36; i++) {
                base36_r((char*)&(jump_table_2[i]) + 1, i);
        }
        for (int i = 36; i < 1296; i++) {
                base36_r((char*)&(jump_table_2[i]), i);
        }
#if 0
        char b1[16];
        char b2[16];
        memset(b1, 0, sizeof(b1));
        memset(b2, 0, sizeof(b2));
        for (int i = 0; i < 100000000; i++) {
                char* end1 = base36_r(b1, i);
                char* end2 = base36_r2(b2, i);
                size_t s1 = end1 - b1;
                size_t s2 = end2 - b2;
                if (s1 != s2 || memcmp(b1, b2, s1)) {
                        printf("%d\n%lu %s\n%lu %s", i, s1, b1, s2, b2);
                        exit(1);
                }
        }
#endif
}

char* base36(char *dest, size_t num)
{
        static char buffer[16];
        char* ptr = buffer;
        while (num > 0) {
                *ptr = jump_table[num % 36];
                ptr++;
                num = num / 36;
        }
        while (ptr > buffer) {
                ptr--;
                *dest = *ptr;
                dest++;
        }
        return dest;
}

int is_palindrome_15_bswap(const char *ptr)
{
        uint64_t* first = (uint64_t*) ptr;

        uint64_t* second = (uint64_t*) (ptr + 7);

        return bswap_64(*first) == *second;
}


ssize_t search_15_palindrome(const char* begin, const char* end)
{
        const char* ptr = begin;
        while (ptr <= end - 15) {
                if (is_palindrome_15_bswap(ptr)) {
                        return ptr - begin;
                }
                ptr++;
        }
        return -1;
}


void prime_fill_buffer(uint64_t* buffer, size_t size, primesieve_iterator* it)
{
        while (size-- > 0) {
                *buffer++ = primesieve_next_prime(it);
        }
}

char* base36_print_buffer(char* dest, const uint64_t* src, size_t size)
{
        while(size-- > 0) {
                dest = base36_r(dest, *src++);
        }
        return dest;
}

size_t base36_print_buffer_2(char* dest, const uint64_t* src, size_t size)
{
        char* dest_begin = dest;
        while(size-- > 0) {
                dest = base36_r2(dest, *src++);
        }
        return dest - dest_begin;
}

