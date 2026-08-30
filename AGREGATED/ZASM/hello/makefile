# The ZASM assembler
ZASM = zasm

# Output binary name
OUT = hello

# Source files
SRCS = main.s \
       ../stdzio/stdzout.s \
       ../stdzio/stdzin.s \
       ../stdzio/uart/uart_out.s \
       ../stdzio/uart/uart_in.s

# Default rule
all: $(OUT)

# Rule to assemble
$(OUT): $(SRCS)
	$(ZASM) -o $(OUT) $(SRCS)

# Clean rule
clean:
	rm -f $(OUT)
