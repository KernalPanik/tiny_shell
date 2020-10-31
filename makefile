CXXFLAGS= -I. -Wall -pedantic
CCFLAGS= -I. -Wall -pedantic

all: shell

quality: CXXFLAGS += -Werror
quality: CCFLAGS += -Werror
quality: shell 

debug: CXXFLAGS += -DDEBUG -g
debug: CCFLAGS += -DDEBUG -g
debug: shell

shell:
	gcc -o myShell shell.c $(CCFLAGS)