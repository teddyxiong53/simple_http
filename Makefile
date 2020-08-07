csrc = $(wildcard ./*.c)



obj = $(csrc:.c=.o)
dep = $(obj:.o=.d)

CXX=g++
CC=gcc
CPP=g++
CFLAGS += -I./   -g -O0


LDFLAGS = -lpthread

TARGET = test

$(TARGET): $(obj)
	$(CXX) -o $@ $^ $(LDFLAGS)

-include $(dep)

%.d:%.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@



.PHONY: clean cleandep distclean

distclean:clean cleandep

clean:
	rm -f $(obj) $(TARGET)

cleandep:
	rm -rf $(dep)
