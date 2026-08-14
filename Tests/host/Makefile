RUNTIME_DIR := ../../EdgeFlex_Runtime
MODELS_DIR  := ../../Models
BASELINE_DIR := ../../Baseline
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -g -O0 \
          -fsanitize=address,undefined \
          -I$(RUNTIME_DIR)/Inc -I$(MODELS_DIR) -I$(BASELINE_DIR)/Inc
LDFLAGS := -fsanitize=address,undefined -lm

MEM_SRCS := test_memory.c $(RUNTIME_DIR)/Src/edgeflex_memory.c
INFER_SRCS := test_inference.c $(RUNTIME_DIR)/Src/edgeflex_memory.c \
              $(RUNTIME_DIR)/Src/edgeflex_model.c $(RUNTIME_DIR)/Src/edgeflex_inference.c \
              $(MODELS_DIR)/tinymlp_model.c
BASELINE_SRCS := test_baseline.c $(RUNTIME_DIR)/Src/edgeflex_memory.c \
              $(RUNTIME_DIR)/Src/edgeflex_model.c $(RUNTIME_DIR)/Src/edgeflex_inference.c \
              $(BASELINE_DIR)/Src/baseline_inference.c $(MODELS_DIR)/tinymlp_model.c

.PHONY: test clean
test: test_memory test_inference test_baseline
	@echo "=================== test_memory ==================="
	./test_memory
	@echo "================== test_inference =================="
	./test_inference
	@echo "================== test_baseline ==================="
	./test_baseline

test_memory: $(MEM_SRCS)
	$(CC) $(CFLAGS) $(MEM_SRCS) -o test_memory $(LDFLAGS)

test_inference: $(INFER_SRCS)
	$(CC) $(CFLAGS) $(INFER_SRCS) -o test_inference $(LDFLAGS)

test_baseline: $(BASELINE_SRCS)
	$(CC) $(CFLAGS) $(BASELINE_SRCS) -o test_baseline $(LDFLAGS)

clean:
	rm -f test_memory test_inference test_baseline
