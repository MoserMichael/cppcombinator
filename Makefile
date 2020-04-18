.PHONY: all
all :
	make -f Makefile.gcc
	make -f Makefile.clang


.PHONY: clean
clean :
	make -f Makefile.gcc clean
	make -f Makefile.clang clean


