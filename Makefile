PREVIEW_BIN = h264_with_preview
PREVIEW_UDP_BIN = h264_udp_stream

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

VPATH = $(COMPONENTS_DIR) \
		$(DUMP_DIR) \

PREVIEW_SRC = $(PREVIEW_BIN).c \
			  $(COMPONENTS_SRC) \
			  $(DUMP_SRC) \

PREVIEW_UDP_SRC = $(PREVIEW_UDP_BIN).c \
				  $(COMPONENTS_SRC) \
				  $(DUMP_SRC) \

COMPONENTS_DIR = ./components
COMPONENTS_SRC = $(notdir $(wildcard $(COMPONENTS_DIR)/*.c))

DUMP_DIR = ./dump
DUMP_SRC = $(notdir $(wildcard $(DUMP_DIR)/*.c))

OBJ_DIR = ./objs
PREVIEW_OBJS = $(addprefix $(OBJ_DIR)/,$(PREVIEW_SRC:.c=.o))
PREVIEW_UDP_OBJS = $(addprefix $(OBJ_DIR)/,$(PREVIEW_UDP_SRC:.c=.o)) 

all: $(PREVIEW_BIN) $(PREVIEW_UDP_BIN)

preview: $(PREVIEW_BIN)

preview_udp: $(PREVIEW_UDP_BIN)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations

$(PREVIEW_BIN): $(PREVIEW_OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(PREVIEW_OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

$(PREVIEW_UDP_BIN): $(PREVIEW_UDP_OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(PREVIEW_UDP_OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

.PHONY: clean printval

clean:
	rm -f $(PREVIEW_BIN) $(PREVIEW_UDP_BIN) $(OBJ_DIR)/*.o

printval:
	$(info $(PREVIEW_OBJS))
