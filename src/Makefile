CC=cc -D NO_OS_SET

# Linux 
CC=cc $(LFA)
LIBS=-lm -lcrypt

# MacOS
CC=cc $(LFA)
LIBS=-lm

# ARM
#CC=aarch64-poky-linux-gcc --sysroot=/opt/fsl-imx-wayland/4.19-warrior/sysroots/aarch64-poky-linux

# Old versions of Linux that only support DES in crypt()
#CC=cc $(LFA) -D DES_ONLY
#LIBS=-lm 

# FreeBSD 6
#CC=cc $(LFA) -D DES_ONLY -D NO_LOG2
#LIBS=-lm -lcrypt
 
# For OS's where crypt just won't build
#CC=cc $(LFA) -D NO_CRYPT 
#LIBS=-lm

# -fsanitize is for runtime OOB memory checking. Debug use only as it has a 
# severe speed penalty and doesn't work with -O. Linker needs it too
#SANI=-fsanitize=address

# Standard compile args
ARGS=-Wall -pedantic -c -g -O
# -Wextra can add a lot of irrelevant warnings but sometimes useful
#ARGS=-Wall -Wextra -pedantic -c -g -O
# Sanitize doesn't work with optimisation
#ARGS=-Wall -pedantic -c -g $(SANI)

OBJS=\
	main.o \
	keyboard.o \
	tokeniser.o \
	execute.o \
	commands.o \
	functions.o \
	variables.o \
	expressions.o \
	strings.o \
	defexp.o \
	defmod.o \
	values.o \
	program.o \
	path.o \
	draw.o \
	watch.o \
	process.o \
	argv.o \
	labels.o \
	misc.o
DEPS=globals.h Makefile

basic: $(OBJS) Makefile
	$(CC) $(OBJS) $(LIBS) $(SANI) -o basic

build_date:
	echo "#define BUILD_DATE \"`date +'%Y-%m-%d %T %Z'`\"" > build_date.h

main.o: main.c $(DEPS) build_date
	$(CC) $(ARGS) main.c

keyboard.o: keyboard.c $(DEPS)
	$(CC) $(ARGS) keyboard.c

tokeniser.o: tokeniser.c $(DEPS)
	$(CC) $(ARGS) tokeniser.c

program.o: program.c $(DEPS)
	$(CC) $(ARGS) program.c

execute.o: execute.c $(DEPS)
	$(CC) $(ARGS) execute.c

commands.o: commands.c $(DEPS)
	$(CC) $(ARGS) commands.c

functions.o: functions.c $(DEPS)
	$(CC) $(ARGS) functions.c

variables.o: variables.c $(DEPS) build_date
	$(CC) $(ARGS) variables.c

expressions.o: expressions.c $(DEPS)
	$(CC) $(ARGS) expressions.c

strings.o: strings.c $(DEPS)
	$(CC) $(ARGS) strings.c

defexp.o: defexp.c $(DEPS)
	$(CC) $(ARGS) defexp.c

defmod.o: defmod.c $(DEPS)
	$(CC) $(ARGS) defmod.c

values.o: values.c $(DEPS)
	$(CC) $(ARGS) values.c

path.o: path.c $(DEPS)
	$(CC) $(ARGS) path.c

draw.o: draw.c $(DEPS)
	$(CC) $(ARGS) draw.c

watch.o: watch.c $(DEPS)
	$(CC) $(ARGS) watch.c 

process.o: process.c $(DEPS)
	$(CC) $(ARGS) process.c 

argv.o: argv.c $(DEPS)
	$(CC) $(ARGS) argv.c 

labels.o: labels.c $(DEPS)
	$(CC) $(ARGS) labels.c 

misc.o: misc.c $(DEPS)
	$(CC) $(ARGS) misc.c

clean:
	rm -f basic *.o core* build_date.h
