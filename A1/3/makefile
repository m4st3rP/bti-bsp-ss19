# file makefile
.PHONY: clean install
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -g

SRCDIR = src
OBJDIR = obj
BINDIR = bin
TSTDIR = tst

srcfiles = $(wildcard $(SRCDIR)/*.c)
objfiles = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(srcfiles:.c=.o))
deps = $(objfiles:.o=.d)

all: $(TSTDIR)/projectBs $(TSTDIR)/projectHallo $(TSTDIR)/projectHaw
-include $(deps)

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	@echo "computing dependencies for $<"
	@mkdir -p $(@D)
	@$(CC) -MM -MT "$@ $(patsubst %.d,%.o,$@)" -MF $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "compiling $< ..."
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

$(TSTDIR)/projectHallo: $(OBJDIR)/hallo.o $(OBJDIR)/print.o
	@mkdir -p $(@D)
	@ $(CC) $(LDFLAGS) -o $@ $^
	
$(TSTDIR)/projectBs: $(OBJDIR)/bs.o $(OBJDIR)/print.o
	@mkdir -p $(@D)
	@ $(CC) $(LDFLAGS) -o $@ $^
	
$(TSTDIR)/projectHaw: $(OBJDIR)/haw.o $(OBJDIR)/print.o
	@mkdir -p $(@D)
	@ $(CC) $(LDFLAGS) -o $@ $^

install:
	@echo "installing $< ..."
	@mkdir -p $(@D)
	@echo "copying..."
	@cp $(TSTDIR)/* $(BINDIR)
	
clean:
	rm -r -f $(OBJDIR)/* $(TSTDIR)/*
#EOF
