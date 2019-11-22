SQUIRREL= ..

UNAME := $(shell uname)
GCC=gcc
ifeq ($(UNAME), Linux)
        COMP=g++
endif
ifeq ($(UNAME), FreeBSD)
        COMP=c++
endif


GCC=$(GCC_PREFIX)$(COMP)
OUT= ../../lib/libsqstdlib.a
INCZ= -I$(SQUIRREL)/include -I. -Iinclude


SRCS = $(wildcard *.cpp)
OBJS = $(src:.cpp=.o)	
	
sq32:
	$(GCC) -O2 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ)
	ar rc $(OUT) *.o
	rm *.o

sqprof:
	$(GCC) -O2 -pg -fno-exceptions -fno-rtti -pie -gstabs -g3 -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ)
	ar rc $(OUT) *.o
	rm *.o
	
sq64:
	$(GCC) -O2 -m64 -fno-exceptions -D_SQ64 -fno-rtti -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ)
	ar rc $(OUT) *.o
	rm *.o
