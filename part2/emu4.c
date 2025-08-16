#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>
#include <sys/io.h>
#include <stdbool.h>
/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
#define CR0_PG (1U << 31)

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1U << 1)
#define CR4_TSD (1U << 2)
#define CR4_DE (1U << 3)
#define CR4_PSE (1U << 4)
#define CR4_PAE (1U << 5)
#define CR4_MCE (1U << 6)
#define CR4_PGE (1U << 7)
#define CR4_PCE (1U << 8)
#define CR4_OSFXSR (1U << 8)
#define CR4_OSXMMEXCPT (1U << 10)
#define CR4_UMIP (1U << 11)
#define CR4_VMXE (1U << 13)
#define CR4_SMXE (1U << 14)
#define CR4_FSGSBASE (1U << 16)
#define CR4_PCIDE (1U << 17)
#define CR4_OSXSAVE (1U << 18)
#define CR4_SMEP (1U << 20)
#define CR4_SMAP (1U << 21)

#define EFER_SCE 1
#define EFER_LME (1U << 8)
#define EFER_LMA (1U << 10)
#define EFER_NXE (1U << 11)

/* 32-bit page directory entry bits */
#define PDE32_PRESENT 1
#define PDE32_RW (1U << 1)
#define PDE32_USER (1U << 2)
#define PDE32_PS (1U << 7)

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)

#define BUFSZ 20
struct vm
{
	int dev_fd;
	int vm_fd;
	char *mem;
};

struct vcpu
{
	int vcpu_fd;
	struct kvm_run *kvm_run;
};

/* Data from sched.txt */
char sched_order[100];

void vm_init(struct vm *vm, size_t mem_size)
{
	int kvm_version;
	struct kvm_userspace_memory_region memreg;

	vm->dev_fd = open("/dev/kvm", O_RDWR);
	if (vm->dev_fd < 0)
	{
		perror("open /dev/kvm");
		exit(1);
	}

	kvm_version = ioctl(vm->dev_fd, KVM_GET_API_VERSION, 0);
	if (kvm_version < 0)
	{
		perror("KVM_GET_API_VERSION");
		exit(1);
	}

	if (kvm_version != KVM_API_VERSION)
	{
		fprintf(stderr, "Got KVM api version %d, expected %d\n",
				kvm_version, KVM_API_VERSION);
		exit(1);
	}

	vm->vm_fd = ioctl(vm->dev_fd, KVM_CREATE_VM, 0);
	if (vm->vm_fd < 0)
	{
		perror("KVM_CREATE_VM");
		exit(1);
	}

	if (ioctl(vm->vm_fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0)
	{
		perror("KVM_SET_TSS_ADDR");
		exit(1);
	}

	vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (vm->mem == MAP_FAILED)
	{
		perror("mmap mem");
		exit(1);
	}

	madvise(vm->mem, mem_size, MADV_MERGEABLE);

	memreg.slot = 0;
	memreg.flags = 0;
	memreg.guest_phys_addr = 0;
	memreg.memory_size = mem_size;
	memreg.userspace_addr = (unsigned long)vm->mem;
	if (ioctl(vm->vm_fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0)
	{
		perror("KVM_SET_USER_MEMORY_REGION");
		exit(1);
	}
}

void vcpu_init(struct vm *vm, struct vcpu *vcpu)
{
	int vcpu_mmap_size;

	vcpu->vcpu_fd = ioctl(vm->vm_fd, KVM_CREATE_VCPU, 0);
	if (vcpu->vcpu_fd < 0)
	{
		perror("KVM_CREATE_VCPU");
		exit(1);
	}

	vcpu_mmap_size = ioctl(vm->dev_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
	if (vcpu_mmap_size <= 0)
	{
		perror("KVM_GET_VCPU_MMAP_SIZE");
		exit(1);
	}

	vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
						 MAP_SHARED, vcpu->vcpu_fd, 0);
	if (vcpu->kvm_run == MAP_FAILED)
	{
		perror("mmap kvm_run");
		exit(1);
	}
}

/*	Modify this function to complete part 2.4 */
/*	sched_order holds the scheduling order for the VMs
	each VM when "scheduled" should read producer-consumer
	buffer state, act on it and convey the new state back
	to the hypervisor.
*/
uint32_t size(int32_t f, int32_t r)
{
	if(f==r && f==-1){
		return 0;
	}
	uint32_t num = 0;
	while(f!=((r+1)%BUFSZ)){
		f = (f+1)%BUFSZ;
		num++;
	}
	return num;
}

uint32_t num_elem(int32_t new, int32_t prev)
{
	
	uint32_t num = 0;
	while(prev!=new){
		prev = (prev+1)%BUFSZ;
		num++;
	}
	return num;
}
/* Modify this function to complete part 2.4 */
typedef struct {
	int32_t prod_p;
	int32_t cons_p;
}buf_state;

int run_vm(struct vm *vm1, struct vm *vm2, struct vcpu *vcpu1, struct vcpu *vcpu2, size_t sz)
{
	// printf("buf-state size %ld", sizeof(buf_state));
	struct kvm_regs regs;
	struct vm *vm = NULL;
	struct vcpu *vcpu = NULL;
	uint64_t memval = 0;
	// int fd = open("./sched2.txt", O_RDONLY);
	char vm_num = 'a';
	bool vm1_flag = false;
	bool vm2_flag = false;
	bool rd = true;
	int8_t* buff_prod = NULL;
	int8_t* buff_cons = NULL;
	buf_state b = {.cons_p=-1, .prod_p=-1};
	// uint8_t state = 0;
	bool vm1_ran = true;
	bool vm2_ran = true;
	// bool hv_flag = false;
	// if (ioperm(0xEF, 4, 1)) {  // 4 bytes (32-bit port access)
	// 	perror("ioperm failed");
	// 	exit(0);
	// }
	int i=0;
	for (;;)
	{	
		if(!vm1_flag){
			vm = vm1;
			vcpu = vcpu1;
			// vm1_flag = true;
		}
		else if(!vm2_flag){
			vm = vm2;
			vcpu = vcpu2;
			// vm2_flag = true;
		}
		else if (vm1_ran && vm2_ran){
			// printf("HYPVSR: [");
			// if(hv_flag){
			// 	printf("buffer prod: ");
			// 	for(int i=0; i<BUFSZ; i++){
			// 		printf("%d, ", buff_prod[i]);
			// 	}
			// 	printf("\n");
			// 	printf("buffer cons: ");
			// 	for(int i=0; i<BUFSZ; i++){
			// 		printf("%d, ", buff_cons[i]);
			// 	}
			// 	printf("\n");
			// }
			int32_t sz = size(b.cons_p, b.prod_p);
			printf("HYPVSR:[");
			int32_t i = b.cons_p;
			
			bool flag = false;
			fflush(stdout);
			// sleep(2);
			if(sz != 0){
				flag = true;
				while(i!=((b.prod_p+1)%BUFSZ)){
					printf("%d ", buff_cons[i]);
					i = (i+1)%BUFSZ;
				}
			}
			if(flag)
				printf("\b]\n");
			else
				printf("]\n");
			// printf("HYPVSR :prod = %d, cons = %d\n", b.prod_p, b.cons_p);
			// fflush(stdout);
			// state = 1;
			vm1_ran = false;
			vm2_ran = false;
			// hv_flag = true;
			continue;
		}
		else if(vm1_flag && vm2_flag){
			if (rd){
				vm_num = sched_order[i++];
				// int val = read(fd, &vm_num, sizeof(char));
				// if(val<0){
				// 	perror("[ERROR]: can't read file\n");
				// 	exit(1);
				// }
				// else if(val==0){
				// 	// printf("Done\n");
				// 	exit(0);
				// }
				rd = false;
			}
			// printf("vm_num %c\n", vm_num);
			if(vm_num == '1'){
				vm = vm1;
				vcpu = vcpu1;
			}
			else if(vm_num == '2'){
				vm = vm2;
				vcpu = vcpu2;
			}
			else{
				exit(0);
			}
		}
		
		if (ioctl(vcpu->vcpu_fd, KVM_RUN, 0) < 0)
		{
			perror("KVM_RUN");
			exit(1);
		}
		sleep(1);
		// printf("EXIT %d, PORT %x\n", vcpu->kvm_run->exit_reason, vcpu->kvm_run->io.port);
		
		fflush(stdout);
		switch (vcpu->kvm_run->exit_reason)
		{
		case KVM_EXIT_HLT:
			goto check;

		case KVM_EXIT_IO:
			// if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xE9)
			// { // HC_print8bit
			// 	char *p = (char *)vcpu->kvm_run;
			// 	fwrite(p + vcpu->kvm_run->io.data_offset,
			// 		   vcpu->kvm_run->io.size, 1, stdout);
			// 	// printf("\n");
			// 	fflush(stdout);
			// 	continue;
			// }
			// else
			 if(vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xE9)
			{
				// exit_count_out++;
				char* p = (char*)vcpu->kvm_run;
				// printf("was here\n");
				// printf("addr : %p, size : %u\n", p+vcpu->kvm_run->io.data_offset, vcpu->kvm_run->io.size);
				fprintf(stdout, "%u\n", *((uint32_t*)(p+vcpu->kvm_run->io.data_offset)));
				fflush(stdout);
				continue;
			}
			// recieve base
			else if(vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xEB)
			{
				char *p = (char*)vcpu->kvm_run;
				uint32_t gva, hva;
				gva = *(uint32_t*)(p + vcpu->kvm_run->io.data_offset);
				struct kvm_translation t;
				memset(&t, -1, sizeof(t));
				t.linear_address = gva;
				ioctl(vcpu->vcpu_fd, KVM_TRANSLATE, &t);
				hva = t.physical_address;
				int8_t* buffer = (int8_t*)(vm->mem + hva);
				
				// printf("gva: %u, hva: %u\n", gva, (uint32_t)(intptr_t)buffer);
				// printf("\n");
				if (!vm1_flag){
					buff_prod = buffer;
					vm1_flag  = true;
				}
				else{
					buff_cons = buffer;
					vm2_flag = true;
				}
				
				// printf("buff_prod: %p, buff_cons: %p\n", buff_prod, buff_cons);
				// fflush(stdout);
				continue;
			}

			// Buffer state
			// receive buf state from producer
			else if(vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xED && vm_num=='1')
			{
				char *p = (char*)vcpu->kvm_run;
				int32_t prod_p = *(uint32_t*)(p+vcpu->kvm_run->io.data_offset);
				if(b.prod_p == -1 && prod_p!=-1){
					b.cons_p = 0;
				}
				
				printf("VMFD: %u Produced %d values:", vm->vm_fd, 
					num_elem(prod_p, b.prod_p));
				fflush(stdout);
				// sleep(2);
				while(b.prod_p != prod_p){
					b.prod_p = (b.prod_p + 1)%BUFSZ;
					// printf(" [%d] ", b.prod_p);
					// fflush(stdout);
					buff_cons[b.prod_p] = buff_prod[b.prod_p];
					printf(" %u", buff_prod[b.prod_p]);
					fflush(stdout);
				}
				printf("\n");
				// b.cons_p = recv.cons_p;
				// printf("prod_p : %d, cons_p: %d\n",b.prod_p, b.cons_p);
				// fflush(stdout);
				vm1_ran = true;
				rd = true;
				continue;
			}
			
			
			// receive buff state from consumer
			else if(vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xEF && vm_num=='2')
			{
				char *p = (char*)vcpu->kvm_run;
				int32_t cons_p = *(uint32_t*)(p + vcpu->kvm_run->io.data_offset);
				
				
				// printf("[recv]: prod_p %d, cons_p %d\n", b.prod_p, cons_p);
				// fflush(stdout);
				
				if(b.cons_p == -1 && cons_p == -1){
					printf("VMFD: %u Consumed %u values:", 
						vm->vm_fd, 
						0);
						fflush(stdout);
					b.cons_p = -1;
					b.prod_p = -1;
				}
				else if(cons_p == -1){
					printf("VMFD: %u Consumed %u values:", 
						vm->vm_fd, 
						num_elem(b.prod_p+1, b.cons_p));
						fflush(stdout);
					while(b.cons_p != b.prod_p+1){
						printf(" %u", buff_prod[b.cons_p]);
						buff_prod[b.cons_p] = -1;
						b.cons_p = (b.cons_p+1)%BUFSZ;
					}
					b.prod_p = -1;
					b.cons_p = -1;
				}
				else{
					printf("VMFD: %u Consumed %u values:", 
						vm->vm_fd, 
						num_elem(cons_p, b.cons_p));
						fflush(stdout);
					while(b.cons_p != cons_p){
						printf(" %u", buff_prod[b.cons_p]);
						buff_prod[b.cons_p] = -1;
						b.cons_p = (b.cons_p+1)%BUFSZ;
					}
				}
				
				
				printf("\n");
				fflush(stdout);
				// state = 0;
				// b.prod_p = recv.prod_p;
				vm2_ran = true;
				rd = true;
				continue;
			}

			// send prod_p state
			else if(vcpu->kvm_run->io.direction == KVM_EXIT_IO_IN && vcpu->kvm_run->io.port == 0xED)
			{
				char *p = (char*)vcpu->kvm_run;
				// printf("VMFD: %u buf_state recv\n",vcpu->vcpu_fd );
				// fflush(stdout);
				*(uint32_t*)(p+vcpu->kvm_run->io.data_offset) = b.prod_p;
				// printf("[send prod]: prod %d, cons %d\n",prod_p, cons_p);
				
				continue;
			}

			// send cons_p 
			else if(vcpu->kvm_run->io.direction == KVM_EXIT_IO_IN && vcpu->kvm_run->io.port == 0xEF)
			{
				char *p = (char*)vcpu->kvm_run;
				// printf("VMFD: %u buf_state recv\n",vcpu->vcpu_fd );
				// fflush(stdout);
				*(uint32_t*)(p+vcpu->kvm_run->io.data_offset) = b.cons_p;
				// printf("[send prod]: prod %d, cons %d\n",prod_p, cons_p);
				
				continue;
			}

			

			/* fall through */
		default:
			fprintf(stderr, "Got exit_reason %d,"
							" expected KVM_EXIT_HLT (%d)\n",
					vcpu->kvm_run->exit_reason, KVM_EXIT_HLT);
			exit(1);
		}
	}

check:
	if (ioctl(vcpu->vcpu_fd, KVM_GET_REGS, &regs) < 0)
	{
		perror("KVM_GET_REGS");
		exit(1);
	}

	if (regs.rax != 42)
	{
		printf("Wrong result: {E,R,}AX is %lld\n", regs.rax);
		return 0;
	}

	memcpy(&memval, &vm->mem[0x400], sz);
	if (memval != 42)
	{
		printf("Wrong result: memory at 0x400 is %lld\n",
			   (unsigned long long)memval);
		return 0;
	}

	return 1;
}

static void setup_protected_mode(struct kvm_sregs *sregs)
{
	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.selector = 1 << 3,
		.present = 1,
		.type = 11, /* Code: execute, read, accessed */
		.dpl = 0,
		.db = 1,
		.s = 1, /* Code/data */
		.l = 0,
		.g = 1, /* 4KB granularity */
	};

	sregs->cr0 |= CR0_PE; /* enter protected mode */

	sregs->cs = seg;

	seg.type = 3; /* Data: read/write, accessed */
	seg.selector = 2 << 3;
	sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

extern const unsigned char guest4a[], guest4a_end[];
extern const unsigned char guest4b[], guest4b_end[];

int run_protected_mode1(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
	{
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_protected_mode(&sregs);

	if (ioctl(vcpu->vcpu_fd, KVM_SET_SREGS, &sregs) < 0)
	{
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	regs.rsp = 2 << 20;

	if (ioctl(vcpu->vcpu_fd, KVM_SET_REGS, &regs) < 0)
	{
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest4a, guest4a_end - guest4a);
	printf("VMFD: %d Loaded Program with size: %ld\n", vm->vm_fd, guest4a_end - guest4a);
	return 0;
}

int run_protected_mode2(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
	{
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_protected_mode(&sregs);

	if (ioctl(vcpu->vcpu_fd, KVM_SET_SREGS, &sregs) < 0)
	{
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	regs.rsp = 2 << 20;

	if (ioctl(vcpu->vcpu_fd, KVM_SET_REGS, &regs) < 0)
	{
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest4b, guest4b_end - guest4b);
	printf("VMFD: %d Loaded Program with size: %ld\n", vm->vm_fd, guest4b_end - guest4b);
	return 0;
}

void read_sched_file(char *filename)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "Error in opening file");
		exit(EXIT_FAILURE);
	}
	int rc = read(fd, sched_order, 100);
	if (rc < 0)
	{
		fprintf(stderr, "Error in opening file");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	struct vm vm1, vm2;
	struct vcpu vcpu1, vcpu2;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s filename\n",
				argv[0]);
		return 1;
	}
	read_sched_file(argv[1]);

	vm_init(&vm1, 0x200000);
	vm_init(&vm2, 0x200000);
	vcpu_init(&vm1, &vcpu1);
	vcpu_init(&vm2, &vcpu2);
	run_protected_mode1(&vm1, &vcpu1);
	run_protected_mode2(&vm2, &vcpu2);
	return run_vm(&vm1, &vm2, &vcpu1, &vcpu2, 4);
}
