/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <pagemem.h>
#include <cr.h>

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
		pg_set_entry(&pgd_task_1_ptb_1[i], PG_USR|PG_RW, 1024 + i); // de 0x400000 a 0x800000
	 	pg_set_entry(&pgd_task_2_ptb_1[i], PG_USR|PG_RW, 2* 1024 + i); // de 0x800000 a 0xc00000
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
	pg_set_entry(&pgd_task_2[1], PG_USR|PG_RW, page_get_nr(pgd_task_2_ptb_2)); // 0x400000 pour tache 2
	pg_set_entry(&pgd_task_1[2], PG_USR|PG_RW, page_get_nr(pgd_task_1_ptb_2)); // 0x800000 pour tache 1


	set_cr3((uint32_t)pgd);

	uint32_t cr0 = get_cr0();
	set_cr0(cr0|CR0_PG);

	debug("Pagination on\n");
}


// Taches

void tache1() {
	uint32_t *compteur = 0xFFFFFFFF;

}

void tp() {
	// TODO
	config_seg();
	config_pgd();
}
