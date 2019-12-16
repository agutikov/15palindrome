all: 15palindrome 15palindrome_pipe_10_to_36 15palindrome_pipe_searcher 15palindrome_mmap_searcher

15palindrome: 15palindrome.c Makefile
	gcc -Wall -O3 15palindrome.c -o 15palindrome

15palindrome_pipe_10_to_36: 15palindrome_pipe_10_to_36.c Makefile
	gcc -Wall -O3 15palindrome_pipe_10_to_36.c -o 15palindrome_pipe_10_to_36

15palindrome_pipe_searcher: 15palindrome_pipe_searcher.c Makefile
	gcc -Wall -O3 15palindrome_pipe_searcher.c -o 15palindrome_pipe_searcher

15palindrome_mmap_searcher: 15palindrome_mmap_searcher.c Makefile
	gcc -Wall -O3 15palindrome_mmap_searcher.c -o 15palindrome_mmap_searcher
