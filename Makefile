.PHONY: all server configure clean rebuild

BUILD_DIR := build
TARGET := Leteehal-Server
JOBS ?= $(shell nproc 2>/dev/null || echo 4)

all: server

configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

server: configure
	@$(MAKE) -C $(BUILD_DIR) $(TARGET) -j$(JOBS)

rebuild: clean server

clean:
	@rm -rf $(BUILD_DIR)
