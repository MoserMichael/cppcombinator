
TEST_FILES=test_stream.cpp \
		   test_rules.cpp \
		   test_grammar.cpp \
		   test_trace.cpp \
		   test_analyse.cpp \
		   test_main.cpp


TEST_OBJS:=$(subst .cpp,.o,$(TEST_FILES))

#GCC_CPP_OPTS=-std=c++17  -pedantic -Wall -Wextra -Wshadow -Werror  
GCC_CPP_OPTS=-std=c++17  -pedantic -Wall -Wshadow -Werror  

GCC_LD_OPTS=-lstdc++ -lgtest -lpthread 

.PHOHY: all
all : test-gcc


test-gcc :  $(TEST_OBJS)
	gcc -o test-gcc $(TEST_OBJS) -lstdc++ -lgtest -lpthread

.PHONY: test
test : test-gcc
	@echo "test with gcc"
	./test-gcc

.PHONY: clean
clean :
	rm *.o test-gcc

%.o : %.cpp
	gcc $(GCC_CPP_OPTS) -c $<
