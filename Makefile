EXE = Popcorn
OBJS = popcorn.o rgbe.o
#SSE = -msse -DUSE_SSE2
FLAGS = $(shell sdl2-config --cflags) -march=native -O3 -flto -g $(SSE)
LIBS = $(shell sdl2-config --static-libs)

all: $(EXE)

$(EXE) : $(OBJS)
	g++ -o $(EXE) $(OBJS) $(FLAGS) $(LIBS)

%.o : %.cpp
	g++ $< -c $(FLAGS) -std=c++11
%.o : %.c
	gcc $< -c $(FLAGS)

clean:
	rm -f $(EXE) $(OBJS)
