BIN = h264
TEST_BIN = h264_mod

CC = gcc
CFLAGS = -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS \
		-DTARGET_POSIX -D_LINUX -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
		-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -DHAVE_LIBOPENMAX=2 -DOMX \
		-DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX \
		-DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -fPIC \
		-ftree-vectorize -pipe -Werror -g -Wall
LDFLAGS = -L/opt/vc/lib -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread
INCLUDES = -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads \
		-I/opt/vc/include/interface/vmcs_host/linux

SRC = $(BIN).c dump.c
OBJS = $(BIN).o dump.o

TEST_SRC = $(TEST_BIN).c dump.c
TEST_OBJS = $(TEST_BIN).o dump.o

all: $(BIN) $(SRC)
test: $(TEST_BIN) $(TEST_SRC)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations

$(BIN): $(OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

$(TEST_BIN): $(TEST_OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(TEST_OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic


.PHONY: clean rebuild

clean:
	rm -f $(BIN) $(BIN).o dump.o video.h264 $(TEST_BIN) $(TEST_BIN).o

rebuild:
	make clean && make
