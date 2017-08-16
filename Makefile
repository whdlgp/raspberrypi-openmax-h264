PREVIEW_BIN = h264_with_preview
FFPREVIEW_BIN = h264_with_ffpreview
PREVIEW_UDP_BIN = h264_udp_stream

BINS = $(PREVIEW_BIN) $(FFPREVIEW_BIN) $(PREVIEW_UDP_BIN) 

CC = gcc
CFLAGS = -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS \
		-DTARGET_POSIX -D_LINUX -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE \
		-D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -DHAVE_LIBOPENMAX=2 -DOMX \
		-DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX \
		-DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -fPIC \
		-ftree-vectorize -pipe -Werror -g -Wall

INCLUDES = -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads \
		-I/opt/vc/include/interface/vmcs_host/linux

#Linker setting 
LDFLAGS = -L/opt/vc/lib -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread
LDFLAGS_AV = -lavcodec -lavformat -lavutil  # for ffmpeg  


ifneq "$(findstring preview, $(MAKECMDGOALS))" ""
VPATH = $(COMPONENTS_DIR) $(DUMP_DIR) $(PREVIEW_DIR) 
endif

ifneq "$(findstring preview_udp, $(MAKECMDGOALS))" ""
VPATH = $(COMPONENTS_DIR) $(DUMP_DIR) $(PREVIEW_UDP_DIR)
endif

ifneq "$(findstring ffpreview, $(MAKECMDGOALS))" ""
VPATH = $(COMPONENTS_DIR) $(DUMP_DIR) $(FFPREVIEW_DIR)
endif

COMMON_SRC = $(COMPONENTS_SRC) $(DUMP_SRC) 

PREVIEW_DIR = ./h264_with_preview_dir
PREVIEW_SRC = $(notdir $(wildcard $(PREVIEW_DIR)/*.c)) \
			  $(COMMON_SRC) \

PREVIEW_UDP_DIR = ./h264_udp_stream_dir
PREVIEW_UDP_SRC = $(notdir $(wildcard $(PREVIEW_UDP_DIR)/*.c)) \
				  $(COMMON_SRC) \

FFPREVIEW_DIR = ./h264_with_ffpreview_dir
FFPREVIEW_SRC = $(notdir $(wildcard $(FFPREVIEW_DIR)/*.c)) \
			  $(COMMON_SRC) \

COMPONENTS_DIR = ./components
COMPONENTS_SRC = $(notdir $(wildcard $(COMPONENTS_DIR)/*.c))

DUMP_DIR = ./dump
DUMP_SRC = $(notdir $(wildcard $(DUMP_DIR)/*.c))

OBJ_DIR = ./objs
PREVIEW_OBJS = $(addprefix $(OBJ_DIR)/,$(PREVIEW_SRC:.c=.o))
FFPREVIEW_OBJS = $(addprefix $(OBJ_DIR)/,$(FFPREVIEW_SRC:.c=.o))
PREVIEW_UDP_OBJS = $(addprefix $(OBJ_DIR)/,$(PREVIEW_UDP_SRC:.c=.o)) 

#<HEEJUNE 2017.8.13
# to automatically make object file directory (only needed first time) 
directories : $(OBJ_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

preview: directories $(PREVIEW_BIN)

ffpreview: directories $(FFPREVIEW_BIN)

preview_udp: directories $(PREVIEW_UDP_BIN)

#HEEJUNE>

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations

$(PREVIEW_BIN): $(PREVIEW_OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(PREVIEW_OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

$(FFPREVIEW_BIN): $(FFPREVIEW_OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(FFPREVIEW_OBJS) $(LDFLAGS) $(LDFLAGS_AV) -Wl,--no-whole-archive -rdynamic

$(PREVIEW_UDP_BIN): $(PREVIEW_UDP_OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(PREVIEW_UDP_OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

.PHONY: clean printval

clean:
	rm -f $(BINS) $(OBJ_DIR)/*.o

printval:
	$(info $(PREVIEW_SRC))
