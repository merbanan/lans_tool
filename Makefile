all: lans_tool

lans_tool: lans_tool.c
	gcc -o lans_tool lans_tool.c

clean:
	rm -f *.o lans_tool


