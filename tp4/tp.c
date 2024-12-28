/* GPLv2 (c) Airbus */
#include <debug.h>
#include <cr.h>
#include <pagemem.h>

void tp() {
	// TODO
	cr3_reg_t cr3 = get_cr3();
	debug("CR3 = 0x%x\n", (raw32_t) cr3.raw);
	

	pde32_t *pgd = (pde32_t*)0x600000; // 00 0000 0001   -- 10 0000 0000   -- 0000 0000 0000
	set_cr3( (uint32_t)pgd);

	debug("CR3 = 0x%x\n", (unsigned int) cr3.raw);


	//uint32_t cr0 = get_cr0();
	//set_cr0(cr0|CR0_PG);


	pde32_t *ptb = (pde32_t*)0x601000;

	// Q5
	for(int i=0;i<1024;i++) {
	 	pg_set_entry(&ptb[i], PG_KRN|PG_RW, i);
	}
	memset((void*)pgd, 0, PAGE_SIZE);
	pg_set_entry(&pgd[0], PG_KRN|PG_RW, page_get_nr(ptb));
 	//uint32_t cr0 = get_cr0(); // enable paging
	//set_cr0(cr0|CR0_PG);
	// end Q5

	// Q6

	//debug("PTB[1] = %x\n", ptb[1].raw);

	pte32_t *ptb2 = (pte32_t*)0x602000;
	for(int i=0;i<1024;i++) {
		pg_set_entry(&ptb2[i], PG_KRN|PG_RW, i+1024);
	}
	pg_set_entry(&pgd[1], PG_KRN|PG_RW, page_get_nr(ptb2));

	uint32_t cr0 = get_cr0(); // enable paging
	set_cr0(cr0|CR0_PG);
	debug("PTB[1] = %x\n", ptb[2].raw);
	// end Q6

	// Q7
	pte32_t  *ptb3    = (pte32_t*)0x603000;
	uint32_t *target  = (uint32_t*)0xc0000000;
	int      pgd_idx = pd32_get_idx(target);
	int      ptb_idx = pt32_get_idx(target);
	debug("%d %d\n", pgd_idx, ptb_idx);
	/**/
	memset((void*)ptb3, 0, PAGE_SIZE);
	pg_set_entry(&ptb3[ptb_idx], PG_KRN|PG_RW, page_get_nr(pgd));
	pg_set_entry(&pgd[pgd_idx], PG_KRN|PG_RW, page_get_nr(ptb3));
	/**/
	debug("PGD[0] = 0x%x | target = 0x%x\n", (unsigned int) pgd[0].raw, (unsigned int) *target);

	/*uint32_t cr0 = get_cr0(); // enable paging
	set_cr0(cr0|CR0_PG);*/
	// end Q7

	// Q8
	char *v1 = (char*)0x700000; // 7 memoire partagee
	char *v2 = (char*)0x7ff000;
	ptb_idx = pt32_get_idx(v1);
	pg_set_entry(&ptb2[ptb_idx], PG_KRN|PG_RW, 2);
	ptb_idx = pt32_get_idx(v2);
	pg_set_entry(&ptb2[ptb_idx], PG_KRN|PG_RW, 2);
	debug("%p = %s | %p = %s\n", v1, v1, v2, v2);
	// uint32_t cr0 = get_cr0(); // enable paging
	// set_cr0(cr0|CR0_PG);
	// end Q8

	// Q9
    *target = 0; 
    //invalidate(target); // vidage des caches 
	// end Q9
}
