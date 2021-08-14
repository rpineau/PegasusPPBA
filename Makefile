# Makefile for libPegasusPPBAPower and libPegasusPPBAExtFocuser

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -I. -I./../../
CPPFLAGS = -std=c++17 -fPIC -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -I. -I./../../
LDFLAGS = -shared -lstdc++
RM = rm -f
STRIP = strip
TARGET_LIBFocuser = libPegasusPPBAExtFocuser.so
TARGET_LIBPower = libPegasusPPBAPower.so

SRCS_Focuser = focuserMain.cpp pegasus_ppbaExtFocuser.cpp x2focuser.cpp x2powercontrol.cpp pegasus_PPBAPower.cpp
SRCS_Power = powerMain.cpp pegasus_PPBAPower.cpp x2powercontrol.cpp x2focuser.cpp pegasus_ppbaExtFocuser.cpp

OBJS_Focuser = $(SRCS_Focuser:.cpp=.o)
OBJS_Power = $(SRCS_Power:.cpp=.o)

.PHONY: all
all: ${TARGET_LIBFocuser} ${TARGET_LIBPower}
.PHONY: power
power : ${TARGET_LIBPower}
.PHONY: focuser
focuser : ${TARGET_LIBFocuser}

$(TARGET_LIBFocuser): $(OBJS_Focuser)
	$(CC) ${LDFLAGS} -o $@ $^
	$(STRIP) $@ >/dev/null 2>&1  || true

$(TARGET_LIBPower): $(OBJS_Power)
	$(CC) ${LDFLAGS} -o $@ $^
	$(STRIP) $@ >/dev/null 2>&1  || true

%.cpp.o: %.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $< >$@

.PHONY: clean
clean:
	${RM} ${TARGET_LIBFocuser} ${TARGET_LIBPower} *.o *.d
