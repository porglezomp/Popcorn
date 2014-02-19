EXE = Popcorn
OBJS = popcorn.o
FLAGS = $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --static-libs)

all: $(EXE)

$(EXE) : $(OBJS)
	g++ -o $(EXE) $(OBJS) $(FLAGS) $(LIBS)

%.o : %.cpp
	g++ $< -c $(FLAGS)