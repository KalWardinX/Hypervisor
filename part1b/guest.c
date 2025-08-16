#include <stddef.h>
#include <stdint.h>

#define BUF_SIZE 256

static inline void outb(uint16_t port, uint8_t value)
{
	asm("outb %0,%1" : /* empty */ : "a"(value), "Nd"(port) : "memory");
}

static inline void outl(uint16_t port, uint32_t value)
{
	asm("outl %0,%1" : /* empty */ :"a"(value), "Nd"(port) : "memory");
}

static inline uint32_t in(uint16_t port){
	uint32_t retval;
	asm("in %1,%0" : "=a"(retval): "Nd"(port) : "memory");
	return retval;
}

void HC_print8bit(uint8_t val)
{
	outb(0xE9, val);
}

void HC_print32bit(uint32_t val)
{
	// val++;
	/* Write code here */
	outl(0xEB, val);
}

uint32_t HC_numExits()
{
	uint32_t val = 0;
	/* Write code here */
	val = in(0xEB);
	return val;
}

void HC_printStr(char *str)
{
	/* Write code here */
	outl(0xED, (uint32_t) (intptr_t)str);
}

char *HC_numExitsByType()
{
	/* Write code here */
	char* ret_str;
	ret_str = (char*)(intptr_t) in(0xED);
	return ret_str;
}

uint32_t HC_gvaToHva(uint32_t gva)
{
	// gva++;
	uint32_t hva = 0;
	/* Write code here */
	outl(0xEF, gva);
	hva = in(0xEF);
	return hva;
}

void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{
	const char *p;

	for (p = "Hello 695!\n"; *p; ++p)
		HC_print8bit(*p);

	/*----------Don't modify this section. We will use grading script---------*/
	/*---Your submission will fail the testcases if you modify this section---*/
	HC_print32bit(2048);
	HC_print32bit(4294967295);

	uint32_t num_exits_a, num_exits_b;
	num_exits_a = HC_numExits();

	char *str = "CS695 Assignment 2\n";
	HC_printStr(str);

	num_exits_b = HC_numExits();

	HC_print32bit(num_exits_a);
	HC_print32bit(num_exits_b);

	char *firststr = HC_numExitsByType();
	uint32_t hva;
	hva = HC_gvaToHva(1024);
	HC_print32bit(hva);
	hva = HC_gvaToHva(4294967295);
	HC_print32bit(hva);
	char *secondstr = HC_numExitsByType();

	HC_printStr(firststr);
	HC_printStr(secondstr);
	/*------------------------------------------------------------------------*/

	*(long *)0x400 = 42;

	for (;;)
		asm("hlt" : /* empty */ : "a"(42) : "memory");
}
