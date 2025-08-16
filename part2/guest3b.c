#include <stddef.h>
#include <stdint.h>


static inline uint32_t in(uint16_t port)
{
	uint32_t value;
	asm("in %1,%0": "=a"(value) : "Nd"(port) : "memory");
	return value;
}

static inline void outl(uint16_t port, uint32_t value)
{
	asm("outl %0,%1": /* empty */ : "a"(value), "Nd"(port) : "memory");
}

void send_base_addr(uint32_t* base_addr)
{
	outl(0xEB, (uint32_t)(intptr_t)base_addr);
}


void consume(){
	outl(0xEF, 0);
}

void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{
	
	/* Write code here */
	// uint32_t temp;
	uint32_t a[5] = {0};
	send_base_addr(a);
	while(1){
		for(int i=0; i<5; i++){
			// temp = a[i];
			// consume the values
		}
		consume();
	}

	*(long *)0x400 = 42;

	for (;;)
		asm("hlt" : /* empty */ : "a"(42) : "memory");
}
