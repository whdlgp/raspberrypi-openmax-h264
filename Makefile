BIN = h264_with_preview

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

SRC = $(BIN).c \
	  $(COMPONENTS_SRC) \
	  $(DUMP_SRC) \

COMPONENTS_DIR = ./components
VPATH := $(COMPONENTS_DIR) 
COMPONENTS_SRC = $(notdir $(wildcard $(COMPONENTS_DIR)/*.c))

DUMP_DIR = ./dump
VPATH := $(VPATH):$(DUMP_DIR)
DUMP_SRC = $(notdir $(wildcard $(DUMP_DIR)/*.c))

OBJ_DIR = ./objs
OBJS = $(addprefix $(OBJ_DIR)/,$(SRC:.c=.o))

all: $(BIN)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations

$(BIN): $(OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

.PHONY: clean printval

clean:
	rm -f $(BIN) $(OBJ_DIR)/*.o

printval:
	$(info $(OBJS))
