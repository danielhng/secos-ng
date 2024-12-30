/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <pagemem.h>
#include <string.h>
#include <cr.h>
#include <intr.h>
#include <info.h>
#include <asm.h>
#include <io.h>

// Segmentation

#define c0_idx  1
#define d0_idx  2
#define c3_idx  3
#define d3_idx  4
#define ts_idx  5

// segments 1 et 2 en ring 0
#define c0_sel  gdt_krn_seg_sel(c0_idx)
#define d0_sel  gdt_krn_seg_sel(d0_idx)

// segments 3 et 4 en ring 3
#define c3_sel  gdt_usr_seg_sel(c3_idx)
#define d3_sel  gdt_usr_seg_sel(d3_idx)

#define ts_sel  gdt_krn_seg_sel(ts_idx)

seg_desc_t GDT[6];
tss_t      TSS;

#define gdt_flat_dsc(_dSc_,_pVl_,_tYp_)                                 \
   ({                                                                   \
      (_dSc_)->raw.raw     = 0;                                             \
      (_dSc_)->limit_1 = 0xffff;                                        \
      (_dSc_)->limit_2 = 0xf;                                           \
      (_dSc_)->type    = _tYp_;                                         \
      (_dSc_)->dpl     = _pVl_;                                         \
      (_dSc_)->d       = 1;                                             \
      (_dSc_)->g       = 1;                                             \
      (_dSc_)->s       = 1;                                             \
      (_dSc_)->p       = 1;                                             \
   })

#define tss_dsc(_dSc_,_tSs_)                                            \
   ({                                                                   \
      raw32_t addr    = {.raw = _tSs_};                                 \
      (_dSc_)->raw.raw    = sizeof(tss_t);                                  \
      (_dSc_)->base_1 = addr.wlow;                                      \
      (_dSc_)->base_2 = addr._whigh.blow;                               \
      (_dSc_)->base_3 = addr._whigh.bhigh;                              \
      (_dSc_)->type   = SEG_DESC_SYS_TSS_AVL_32;                        \
      (_dSc_)->p      = 1;                                              \
   })

#define c0_dsc(_d) gdt_flat_dsc(_d,0,SEG_DESC_CODE_XR)
#define d0_dsc(_d) gdt_flat_dsc(_d,0,SEG_DESC_DATA_RW)
#define c3_dsc(_d) gdt_flat_dsc(_d,3,SEG_DESC_CODE_XR)
#define d3_dsc(_d) gdt_flat_dsc(_d,3,SEG_DESC_DATA_RW)

void config_seg()
{
	gdt_reg_t gdtr;

	GDT[0].raw.raw = 0ULL;

	c0_dsc(&GDT[c0_idx]);
	d0_dsc(&GDT[d0_idx]);
	c3_dsc(&GDT[c3_idx]);
	d3_dsc(&GDT[d3_idx]);

	gdtr.desc  = GDT;
   	gdtr.limit = sizeof(GDT) - 1;
	set_gdtr(gdtr);

	set_cs(gdt_krn_seg_sel(c0_idx));

	set_ss(gdt_krn_seg_sel(d0_idx));
	set_ds(gdt_krn_seg_sel(d0_idx));
	set_es(gdt_krn_seg_sel(d0_idx));
	set_fs(gdt_krn_seg_sel(d0_idx));
	set_gs(gdt_krn_seg_sel(d0_idx));

	TSS.s0.esp = get_esp();
    TSS.s0.ss  = d0_sel;
    tss_dsc(&GDT[ts_idx], (offset_t)&TSS);
    set_tr(ts_sel);

	debug("Segmentation on\n");
}


// Pagination 

pde32_t *pgd = (pde32_t*)0x200000;
pde32_t *ptb = (pde32_t*)0x201000;

pde32_t *pgd_task_1 = (pde32_t*)0x202000;
pde32_t *pgd_task_1_ptb_0 = (pde32_t*)0x203000;
pde32_t *pgd_task_1_ptb_1 = (pde32_t*)0x204000;
pde32_t *pgd_task_1_ptb_2 = (pde32_t*)0x205000;

pde32_t *pgd_task_2 = (pde32_t*)0x206000;
pde32_t *pgd_task_2_ptb_0 = (pde32_t*)0x207000;
pde32_t *pgd_task_2_ptb_1 = (pde32_t*)0x208000;
pde32_t *pgd_task_2_ptb_2 = (pde32_t*)0x209000;


void config_pgd()
{
	memset((void*)pgd, 0, PAGE_SIZE);
	memset((void*)ptb, 0, PAGE_SIZE);

	memset((void*)pgd_task_1, 0, PAGE_SIZE);
	memset((void*)pgd_task_1_ptb_0, 0, PAGE_SIZE);
	memset((void*)pgd_task_1_ptb_1, 0, PAGE_SIZE);
	memset((void*)pgd_task_1_ptb_2, 0, PAGE_SIZE);

	memset((void*)pgd_task_2, 0, PAGE_SIZE);
	memset((void*)pgd_task_2_ptb_0, 0, PAGE_SIZE);
	memset((void*)pgd_task_2_ptb_1, 0, PAGE_SIZE);
	memset((void*)pgd_task_2_ptb_2, 0, PAGE_SIZE);

	for(int i=0;i<1024;i++) // 0x0 => 0x400
	{
		// mapper le kernel dans chaque ptb (kernel, tache 1 et tache 2)
	 	pg_set_entry(&ptb[i], PG_KRN|PG_RW, i);
	 	pg_set_entry(&pgd_task_1_ptb_0[i], PG_KRN|PG_RW, i);
	 	pg_set_entry(&pgd_task_2_ptb_0[i], PG_KRN|PG_RW, i);

		// identity map de chaque premiere page des taches
		pg_set_entry(&pgd_task_1_ptb_1[i], PG_USR|PG_RW, 1024 + i); // de 0x400 a 0x800
	 	pg_set_entry(&pgd_task_2_ptb_1[i], PG_USR|PG_RW, 2* 1024 + i); // de 0x800 a 0xc00
	}


	// page partagee 
	pg_set_entry(&pgd_task_1_ptb_2[0], PG_USR|PG_RW, 0xa000 >> 12);
	pg_set_entry(&pgd_task_2_ptb_2[0], PG_USR|PG_RW, 0xa000 >> 12);

	// ptb kernel a l'index 0 de la pgd kernel
	pg_set_entry(&pgd[0], PG_KRN|PG_RW, page_get_nr(ptb));

	// ptb kernel des taches aux index 0 de leur pgd respectives
	pg_set_entry(&pgd_task_1[0], PG_KRN|PG_RW, page_get_nr(pgd_task_1_ptb_0));
	pg_set_entry(&pgd_task_2[0], PG_KRN|PG_RW, page_get_nr(pgd_task_2_ptb_0));

	// pour identity mapper, ptb de la tache 2 user a l'entree 2 de la pgd
	pg_set_entry(&pgd_task_1[1], PG_USR|PG_RW, page_get_nr(pgd_task_1_ptb_1)); // set page table at [1] in order to identity map task space
	pg_set_entry(&pgd_task_2[2], PG_USR|PG_RW, page_get_nr(pgd_task_2_ptb_1)); // set page table at [2] in order to identity map task space

	// entree de la page partagee dans les pgd 
	pg_set_entry(&pgd_task_2[1], PG_USR|PG_RW, page_get_nr(pgd_task_2_ptb_2)); // 0x400 pour tache 2
	pg_set_entry(&pgd_task_1[2], PG_USR|PG_RW, page_get_nr(pgd_task_1_ptb_2)); // 0x800 pour tache 1


	set_cr3((uint32_t)pgd);

	uint32_t cr0 = get_cr0();
	set_cr0(cr0|CR0_PG);

	debug("Pagination on\n");
}


// Taches

struct tache {
    uint32_t stack;      
    pde32_t *pgd;         
	void *function;
	int_ctx_t context;
	bool_t saved_ctx;
	uint32_t int_stack;
}; 
struct tache tasks[2];

#define STACK_SIZE 4096
uint32_t t1_stack[STACK_SIZE] __attribute__((aligned(4), section(".t1_stack")));
uint32_t t2_stack[STACK_SIZE] __attribute__((aligned(4), section(".t2_stack")));

uint32_t  t1_stack_base = (uint32_t)&t1_stack[STACK_SIZE-1];
uint32_t  t2_stack_base = (uint32_t)&t2_stack[STACK_SIZE-1];

__attribute__((section(".user1")))
void tache1() {
	uint32_t *compteur = (uint32_t*)0x800;
	*compteur = 0;
		*compteur += 1;
		debug("tache 1 oui \n");
}

__attribute__((section(".user2")))
void sys_counter(uint32_t *counter) {
    while (true)
	{
		asm volatile (
        "movl %0, %%eax\n" // Numéro d'interruption pour l'appel système
        "int $0x80\n"      // Appel de l'interruption
        :
        : "r"(counter)
        : "eax"
    );
	debug("sys counter oui\n");
	}
	
}

__attribute__((section(".user2")))
void tache2() {
	while (true)
	{
		uint32_t *shared_counter = (uint32_t*)0x400;
		sys_counter(shared_counter);
		debug("tache 2 oui \n");
	}
}

void syscall_isr() {
    asm volatile(
        "cli\n"
        "leave\n"
        "pusha\n"
        "mov %esp, %eax\n"
        "call syscall_handler\n"
        "popa\n"
        "sti\n"
        "iret\n"
    );
	debug("syscall isr oui\n");
}

void syscall_handler(int_ctx_t *ctx) {
    uint32_t eax = ctx->gpr.eax.raw;
    uint32_t *counter = (uint32_t*)eax;
    debug("Counter value: %u\n", *counter);
}

void irq_generic_handler() {
    outb(0x20, 0x20); 
    asm volatile (
	  	"cli                \n"
      	"leave ; pusha        \n"
      	"mov %esp, %eax      \n"
      	"call clock \n"
      	"popa ;    \n"
		"sti ;  \n"
		"iret"
    );
}

uint32_t curr_task = 0;

void __regparm__(1) clock(int_ctx_t *context) {
	
	debug("entre dans clock\n");
	if((int)curr_task!=-1)   // si on  vient  du noyau, on ne sauvegarde pas le contexte
	{
		memcpy(&tasks[curr_task].context,context,sizeof(int_ctx_t));
		tasks[curr_task].saved_ctx=true;
	}

 	curr_task = (curr_task + 1) % 2;

	

	set_ds(d3_sel);
	set_es(d3_sel);
	set_fs(d3_sel);
	set_gs(d3_sel);
	TSS.s0.esp = tasks[curr_task].int_stack;
    set_cr3(tasks[curr_task].pgd);

	curr_task = curr_task;

   if (tasks[curr_task].saved_ctx)
   {
		memcpy(context, &tasks[curr_task].context, sizeof(int_ctx_t));
   }
   else
   {
		asm volatile(
			//"sti \n"
			"push %0 \n" // esp
			"push %1 \n" // ss
			"pushf   \n" // eflags
			"push %2 \n" // cs
			"push %3 \n" // eip
			"sti \n"
			"iret\n"
			::"i"(d3_sel),
			"m"(tasks[curr_task].stack),
			"i"(c3_sel),
			"r"(tasks[curr_task].function));    //(tasks[curr_task].function)));
   }

	if (curr_task == 0) {
		debug("!!Adding 1 to counter!!\n");
	} else if (curr_task == 1)	{
		debug("!!Reading counter!!\n");
	}
}



void config_idt() {
	idt_reg_t idtr;
	get_idtr(idtr);
	debug("Adresse de l'IDT: 0x%lx\n", idtr.addr);

	debug("Adresse de syscall isr: 0x%x\n", (unsigned int) &syscall_isr);

	int_desc_t *desc;
	desc = &idtr.desc[0x80];
	short int off_1 = (int) &syscall_isr;
	short int off_2 = (int) &syscall_isr >> 16;
	debug("valeur de off1: %x, valeur de off 2: %x\n", off_1, off_2);
	desc->offset_1 = off_1;
	desc->offset_2 = off_2;
	desc->dpl = 3;

	desc = &idtr.desc[32];
	off_1 = (int) &irq_generic_handler;
	off_2 = (int) &irq_generic_handler >> 16;
	desc->offset_1 = off_1;
	desc->offset_2 = off_2;
	desc->dpl = 0;

}

void print_gdt_content(gdt_reg_t gdtr_ptr) {
    seg_desc_t* gdt_ptr;
    gdt_ptr = (seg_desc_t*)(gdtr_ptr.addr);
    int i=0;
    while ((uint32_t)gdt_ptr < ((gdtr_ptr.addr) + gdtr_ptr.limit)) {
        uint32_t start = gdt_ptr->base_3<<24 | gdt_ptr->base_2<<16 | gdt_ptr->base_1;
        uint32_t end;
        if (gdt_ptr->g) {
            end = start + ( (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1) <<12) + 4095;
        } else {
            end = start + (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1);
        }
        debug("%d ", i);
        debug("[0x%x ", start);
        debug("- 0x%x] ", end);
        debug("seg_t: 0x%x ", gdt_ptr->type);
        debug("desc_t: %d ", gdt_ptr->s);
        debug("priv: %d ", gdt_ptr->dpl);
        debug("present: %d ", gdt_ptr->p);
        debug("avl: %d ", gdt_ptr->avl);
        debug("longmode: %d ", gdt_ptr->l);
        debug("default: %d ", gdt_ptr->d);
        debug("gran: %d ", gdt_ptr->g);
        debug("\n");
        gdt_ptr++;
        i++;
    }
}

void config_task(struct tache *task, void *function, uint32_t stack,uint32_t int_stack, pde32_t *pgd) {
    task->function = function;
    task->stack = stack;
    task->pgd = pgd;
	task->int_stack=int_stack;
    memset(&task->context, 0, sizeof(int_ctx_t));
	task->saved_ctx = false;
}

void tp() {
	// TODO
	config_seg();
	config_pgd();
	config_idt();
	gdt_reg_t gdt;
	get_gdtr(gdt);
	print_gdt_content(gdt);


	config_task(&tasks[0], tache1, t1_stack_base,0x303000+STACK_SIZE ,pgd_task_1);
    config_task(&tasks[1], tache2, t2_stack_base,0x304000+STACK_SIZE ,pgd_task_2);


	set_ds(d3_sel);
    set_es(d3_sel);
    set_fs(d3_sel);
    set_gs(d3_sel);
	TSS.s0.esp = tasks[0].int_stack;
	set_cr3((uint32_t)tasks[0].pgd);

	force_interrupts_on();
	
	while(1);
}

