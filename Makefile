BIN = pishade

DMX_INC = -I/opt/vc/include
INCLUDES = $(DMX_INC)
CFLAGS = -g -O0 -Wall -Wextra $(INCLUDES)

DMX_LIBS = -L/opt/vc/lib -lbcm_host
LDFLAGS = $(DMX_LIBS)

.PHONY: all
all: $(BIN)

.PHONY: clean
clean:
	@rm -vf $(BIN)
