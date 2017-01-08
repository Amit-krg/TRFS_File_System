INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: treplay trctl

treplay:
		gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi treplay.c -o treplay 

trctl:
		gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi trctl.c -o trctl 	

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f *.o
	rm -f treplay trctl
