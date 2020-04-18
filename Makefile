.PHONY: all
all :
	make -f Makefile.gcc
	make -f Makefile.clang


.PHONY: clean
clean :
	make -f Makefile.gcc clean
	make -f Makefile.clang clean

.PHONY: test
test :
	make -f Makefile.gcc test
	make -f Makefile.clang test

