#include <stddef.h>
#include <stdint.h>

#define BUFSZ 20

void HC_print8bit(uint8_t val);
typedef struct {
	int32_t prod_p;
	int32_t cons_p;
}buf_state;

static inline int32_t in(uint16_t port)
{
	int32_t value;
	__asm__ __volatile__("in %1,%0": "=a"(value) : "Nd"(port) : "memory");
	return value;
}
static inline void insl(uint16_t port, void* addr,uint32_t count)
{
	// void* addr;
	// HC_print8bit('h');
	__asm__ __volatile__("cld; rep insl"
		: "=D"(addr), "=c"(count)
		: "d"(port), "0" (addr), "1"(count)
		: "memory");
	// HC_print8bit('d');
}

static inline void outsl(uint16_t port , void* addr, uint32_t count){
	__asm__ __volatile__("rep outsl"
		:
		: "d"(port), "S"(addr), "c"(count)
		: "memory");
}
static inline void outl(uint16_t port, uint32_t value)
{
	__asm__ __volatile__("outl %0,%1": /* empty */ : "a"(value), "Nd"(port) : "memory");
}

static inline void outb(uint16_t port, uint8_t value)
{
	__asm__ __volatile__("outb %0,%1": /* empty */ : "a"(value), "Nd"(port) : "memory");
}

static inline uint32_t rdtsc()
{
	uint32_t low;
	uint32_t high;
	__asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high): : "memory");

	return ((uint64_t)high << 32) | low;
}

uint8_t random(uint8_t lb, uint8_t ub)
{
	uint8_t rand = rdtsc() % (ub - lb + 1);
	return rand;
}
void HC_print8bit(uint8_t val)
{
	outb(0xE9, val);
}

void send_base_addr(int8_t* base_addr)
{
	outl(0xEB, (uint32_t)(intptr_t)base_addr);
}



// void send_prod_state(void* b)
// {
// 	outsl(0xED, b, sizeof(buf_state)/4);
// }

// void get_buff_state(void* addr)
// {
// 	insl(0xEF, addr, sizeof(buf_state)/4);
// }
void send_prod(uint32_t val){
	outl(0xED, val);
}

int32_t recv_cons()
{
	int32_t retval;
	retval = (int32_t)(intptr_t)in(0xEF);
	return retval;
}

uint32_t size(uint32_t f, uint32_t r)
{
	return (f>r)?(BUFSZ-(r-f+1)) : (r-f+1);
}
void print_addr(void* b){
	uint32_t addr = (intptr_t)(b);
	uint32_t temp = 0;
	while(addr){
		temp = temp*10 + addr%10;
		addr/=10;
	}
	while(temp){
		HC_print8bit((temp%10)+0x30);
		temp/=10;
	}
	HC_print8bit('\n');
}


void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{
	
	/* Write code here */
	int8_t buffer[BUFSZ];
	buf_state b = {.cons_p = -1, .prod_p = -1};

	// if (ioperm(0xEF, 4, 1)) {  // 4 bytes (32-bit port access)
	// 	HC_print8bit('e');
	// 	// return 1;
	// }
	// else{
	// 	HC_print8bit('s');
	// }
	send_base_addr(buffer);
	// HC_print8bit(sizeof(buf_state)+40);
	while(1){
		// generate rand between [0,10]
		uint8_t num_elem = random(0, 10);

		// get cons_p
		// HC_print8bit('x');
		b.cons_p = recv_cons();
		if(b.cons_p == -1){
			b.prod_p = -1;
		}
		// fill buffer with random numbers 
		for(uint8_t i=0; i<num_elem && b.prod_p != (b.cons_p-1); i++){
			b.prod_p = (b.prod_p+1)%BUFSZ;
			buffer[b.prod_p] = random(0,9);
		}

		send_prod(b.prod_p);
	}

	*(long *)0x400 = 42;

	for (;;)
		asm("hlt" : /* empty */ : "a"(42) : "memory");
}
