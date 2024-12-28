/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>

void bp_handler() {
	//debug("Exception BP occured, Breakpoint happened:");
	__asm__ volatile ("pusha\n");
	uint32_t val;
	__asm__ volatile ("mov 4(%%ebp), %0":"=r"(val));
	__asm__ volatile ("popa \n");
	__asm__ volatile ("leave\n"
					  "iret\n");
}

void bp_trigger() {
	// TODO
	__asm__("int3");
	debug("on est bien revenus\n");
}

void tp() {
	// TODO print idtr
	idt_reg_t idtr;
	get_idtr(idtr);
	debug("Adresse de l'IDT: 0x%lx\n", idtr.addr);



	debug("Adresse de bp_handler: 0x%x\n", (unsigned int) &bp_handler);

	int_desc_t * bp = idtr.desc + 3;
	short int off_1 = (int) &bp_handler;
	short int off_2 = (int) &bp_handler >> 16;
	debug("valeur de off1: %x, valeur de off 2: %x\n", off_1, off_2);
	bp->offset_1 = off_1;
	bp->offset_2 = off_2;

	bp_trigger();
	// TODO call bp_trigger
   //bp_trigger();
}
