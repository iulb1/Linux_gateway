CC := $(CROSS_COMPILE)gcc

# 需要改为你使用的文件夹名称
TOOLCHAIN_DIR := /home/atguigu/atguigupi

CFLAGS += -Wall -Wextra
CFLAGS += -I/home/atguigu/gateway
ifdef CROSS_COMPILE
CFLAGS += --sysroot=$(TOOLCHAIN_DIR)/sysroot
endif
LDFLAGS += -lpaho-mqtt3c -lcurl -lcrypto

#改为你的开发版IP
PEER := root@192.168.32.31

SRCS += $(shell find app -type f -name "*.c")
SRCS += $(shell find daemon -type f -name "*.c")
SRCS += $(shell find ota -type f -name "*.c")
SRCS += $(shell find thirdparty -type f -name "*.c")

OBJS = $(SRCS:.c=.o)

TARGET = gateway


.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJS) main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(OBJS) main.o $(TARGET)

cross:
	CROSS_COMPILE=$(TOOLCHAIN_DIR)/toolchain/bin/arm-linux-gnueabihf- make
	scp -O $(TARGET) $(PEER):/usr/bin/
	scp -O init/S99gateway $(PEER):/etc/init.d/

test_buffer: $(OBJS) test/test_buffer.o
	-@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	-@./$@
	-@$(RM) $@ $^

test_mqtt: $(OBJS) test/test_mqtt.o
	-@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	-@./$@
	-@$(RM) $@ $^
test_message: $(OBJS) test/test_message.o
	-@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	-@./$@
	-@$(RM) $@ $^
test_task: $(OBJS) test/test_task.o
	-@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	-@./$@
	-@$(RM) $@ $^
