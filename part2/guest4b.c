#include <stddef.h>
#include <stdint.h>


#define BUFSZ 20


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
	__asm__ __volatile__("cld; rep insl"
		: "=D"(addr), "=c"(count)
		: "d"(port), "0" (addr), "1"(count)
		: "memory");
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
void HC_print8bit(uint32_t val)
{
	outl(0xE9, val);
}
uint8_t random(uint8_t lb, uint8_t ub)
{
	uint8_t rand = rdtsc() % (ub - lb + 1);
	return rand;
}

void send_base_addr(int8_t* base_addr)
{
	outl(0xEB, (uint32_t)(intptr_t)base_addr);
}


void send_buff_state(buf_state* b)
{
	outsl(0xED, b, sizeof(buf_state)/4);
}

void get_buff_state(void* addr)
{
	insl(0xEF, addr, sizeof(buf_state)/4);
}

uint32_t size(int32_t f, int32_t r)
{
	if(f==r && f==-1){
		return 0;
	}
	return (f>=r)?(BUFSZ-(f-r)+1) : (r-f+1);
}

void send_cons(uint32_t val){
	outl(0xEF, val);
}

int32_t recv_prod()
{
	int32_t retval;
	retval = (int32_t)(intptr_t)in(0xED);
	return retval;
}

void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{

	/* Write code here */
	int8_t buffer[BUFSZ];
	buf_state b = {.cons_p = -1, .prod_p = -1};
	send_base_addr(buffer);

	while(1){
		// generate rand between [0,10]
		uint8_t num_elem = random(0, 10);
		// HC_print8bit(num_elem);
		// HC_print8bit('\n');
		// get prod_p
		int32_t prod_p = recv_prod();
		if(b.prod_p == -1 && prod_p == -1){
			send_cons(b.cons_p);
			continue;
		}
		else if(b.prod_p == -1){
			b.cons_p = 0;
		}
		b.prod_p = prod_p;

		
		// b.cons_p = recv->cons_p;
		// b.prod_p = recv->prod_p;
		// fill buffer with random numbers

		// HC_print8bit((short int)b.prod_p+0x31);
		// HC_print8bit(' ');
		// HC_print8bit((short int)b.cons_p+0x31);
		// HC_print8bit('\n');
		// uint8_t flag = 0;
		if(b.cons_p == b.prod_p+1){
			for(uint8_t i=0; i<num_elem; i++){
				buffer[b.cons_p] = -1;

				b.cons_p = (b.cons_p+1)%BUFSZ;
			}
		}
		else{
			for(uint8_t i=0; i<num_elem && b.cons_p != (b.prod_p + 1); i++){
				// HC_print8bit(buffer[cons_p]+0x30);
				buffer[b.cons_p] = -1;
	
				b.cons_p = (b.cons_p+1)%BUFSZ;
			}
			if(b.cons_p == b.prod_p+1){
				b.cons_p = -1;
				b.prod_p = -1;
			}
			// HC_print8bit(b.prod_p);
			// HC_print8bit(b.cons_p);
		}
		
		
		
		send_cons(b.cons_p);
	}

	*(long *)0x400 = 42;

	for (;;)
		asm("hlt" : /* empty */ : "a"(42) : "memory");
}
