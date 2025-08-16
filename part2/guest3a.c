#include <stddef.h>
#include <stdint.h>


static inline void outl(uint16_t port, uint32_t value){
	asm("outl %0,%1": /* empty */ : "a"(value), "Nd"(port) : "memory");
}

void send_base_addr(uint32_t* base_addr)
{
	outl(0xEB, (uint32_t)(intptr_t)base_addr);
}

void produce(){
	outl(0xED, 0);
}

void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{

	/* write code here */
	uint32_t a[5];
	uint32_t val = 0;
	send_base_addr(a);
	while(1){
		for(int i=0; i<5; i++){
			a[i] = val++;
		}
		produce();
	}
	*(long *)0x400 = 42;

	for (;;)
		asm("hlt" : /* empty */ : "a"(42) : "memory");
}
