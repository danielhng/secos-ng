/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <string.h>


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

void userland() {
   asm volatile ("mov %eax, %cr0");
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


void tp() {
	//Q1
    gdt_reg_t gdt_reg;
    get_gdtr(gdt_reg);
    debug("GDT addr: 0x%x ", (unsigned int) gdt_reg.addr);
    debug("limit: %d\n", gdt_reg.limit);
    //GDT addr: 0x8f8c limit: 39


    //Q2
    print_gdt_content(gdt_reg);
    //res Q2
    // 0 [0x0 - 0xfff0] seg_t: 0x0 desc_t: 0 priv: 0 present: 0 avl: 0 longmode: 0 default: 0 gran: 0 
    // 1 [0x0 - 0xffffffff] seg_t: 0xa desc_t: 1 priv: 0 present: 1 avl: 0 longmode: 0 default: 1 gran: 1 
    // 2 [0x0 - 0xffffffff] seg_t: 0x3 desc_t: 1 priv: 0 present: 1 avl: 0 longmode: 0 default: 1 gran: 1 
    // 3 [0x0 - 0xffff] seg_t: 0xe desc_t: 1 priv: 0 present: 1 avl: 0 longmode: 0 default: 0 gran: 0 
    // 4 [0x0 - 0xffff] seg_t: 0x3 desc_t: 1 priv: 0 present: 1 avl: 0 longmode: 0 default: 0 gran: 0 

    //Q3
    printf("cs: %u, ds: %u, ss: %u, es: %u, fs: %u, gs: %u\n",get_cs(),get_ds(), get_ss(), get_es(), get_fs(), get_gs());
    //res Q3
    //cs: 8, ds: 16, ss: 16, es: 16, fs: 16, gs: 16


    //addresse pour la nouvelle gdt: 0xa000
    seg_desc_t my_gdt[7]; 
    
    my_gdt[0].limit_1 = 0;
    my_gdt[0].base_1 = 0;
    my_gdt[0].base_2 = 0;
    my_gdt[0].type = 0;
    my_gdt[0].s = 0;
    my_gdt[0].dpl = 0;
    my_gdt[0].p = 0;
    my_gdt[0].limit_2 = 0;
    my_gdt[0].avl = 0;


    my_gdt[1].limit_1 = 0xFFFF;
    my_gdt[1].base_1 = 0x0000;
    my_gdt[1].base_2 = 0x00;
    my_gdt[1].type = 11; //RX
    my_gdt[1].s = 1;
    my_gdt[1].dpl = 0;
    my_gdt[1].p = 1;
    my_gdt[1].limit_2 = 0xF;
    my_gdt[1].avl = 1;
    my_gdt[1].l = 0;
    my_gdt[1].d = 1;
    my_gdt[1].g = 1;
    my_gdt[1].base_3 = 0x00;

    my_gdt[2].limit_1 = 0xFFFF;
    my_gdt[2].base_1 = 0x0000;
    my_gdt[2].base_2 = 0x00;
    my_gdt[2].type = 3;
    my_gdt[2].s = 1;
    my_gdt[2].dpl = 0;
    my_gdt[2].p = 1;
    my_gdt[2].limit_2 = 0xF;
    my_gdt[2].avl = 1;
    my_gdt[2].l = 0;
    my_gdt[2].d = 1;
    my_gdt[2].g = 1;
    my_gdt[2].base_3 = 0x00;

     // Q6
    gdt_reg_t my_gdtr;
    my_gdtr.addr = (long unsigned int)my_gdt;
    my_gdtr.limit = sizeof(my_gdt) - 1;
    set_gdtr(my_gdtr);
    set_ds(gdt_krn_seg_sel(2));
    set_cs(gdt_krn_seg_sel(1));
    set_es(gdt_krn_seg_sel(2));
    // end Q6

    // Q7
    get_gdtr(my_gdtr);
    debug("\n");
    debug("GDT addr:  0x%x \n", (unsigned int) my_gdtr.addr);
    debug("limit: %d\n", my_gdtr.limit);
    print_gdt_content(my_gdtr);
    // end Q7

    // Q9
    char  src[64];
    char *dst = NULL;
    memset(src, 0xff, 64);
    _memcpy8(dst, src, 32);

    my_gdt[3].limit_1 = 0x20;   //:16;     /* bits 00-15 of the segment limit */
    my_gdt[3].base_1 = 0x0000;    //:16;     /* bits 00-15 of the base address */
    my_gdt[3].base_2 = 0x60;      //:8;      /* bits 16-23 of the base address */
    my_gdt[3].type = 3; //data,RW //:4;      /* segment type */
    my_gdt[3].s = 1;              //:1;      /* descriptor type */
    my_gdt[3].dpl = 0; //ring0    //:2;      /* descriptor privilege level */
    my_gdt[3].p = 1;              //:1;      /* segment present flag */
    my_gdt[3].limit_2 = 0x0;      //:4;      /* bits 16-19 of the segment limit */
    my_gdt[3].avl = 1;            //:1;      /* available for fun and profit */
    my_gdt[3].l = 0; // 32 bits   //:1;      /* longmode */
    my_gdt[3].d = 1;              //:1;      /* default length, depend on seg type */
    my_gdt[3].g = 0;              //:1;      /* granularity */
    my_gdt[3].base_3 = 0x00;      //:8;      /* bits 24-31 of the base address */
    debug("\n");
    debug("\n");
    debug("\n");
    print_gdt_content(my_gdtr);
    // end Q9

    // Q10
    seg_sel_t my_es;
    my_es.index = 3;
    my_es.ti = 0;
    my_es.rpl = 0;
    set_es(my_es);
    _memcpy8(dst, src, 32);

    
    // end Q10

    // Q11
    // _memcpy8(dst, src, 64);
    // end Q11

    // Q12
    my_gdt[4].limit_1 = 0xffff;   //:16;     /* bits 00-15 of the segment limit */
    my_gdt[4].base_1 = 0x0000;    //:16;     /* bits 00-15 of the base address */
    my_gdt[4].base_2 = 0x00;      //:8;      /* bits 16-23 of the base address */
    my_gdt[4].type = 11;//Code,RX //:4;      /* segment type */
    my_gdt[4].s = 1;              //:1;      /* descriptor type */
    my_gdt[4].dpl = 3; //ring3    //:2;      /* descriptor privilege level */
    my_gdt[4].p = 1;              //:1;      /* segment present flag */
    my_gdt[4].limit_2 = 0xf;      //:4;      /* bits 16-19 of the segment limit */
    my_gdt[4].avl = 1;            //:1;      /* available for fun and profit */
    my_gdt[4].l = 0; //32bits     //:1;      /* longmode */
    my_gdt[4].d = 1;              //:1;      /* default length, depend on seg type */
    my_gdt[4].g = 1;              //:1;      /* granularity */
    my_gdt[4].base_3 = 0x00;      //:8;      /* bits 24-31 of the base address */

    my_gdt[5].limit_1 = 0xffff;   //:16;     /* bits 00-15 of the segment limit */
    my_gdt[5].base_1 = 0x0000;    //:16;     /* bits 00-15 of the base address */
    my_gdt[5].base_2 = 0x00;      //:8;      /* bits 16-23 of the base address */
    my_gdt[5].type = 3; //data,RW //:4;      /* segment type */
    my_gdt[5].s = 1;              //:1;      /* descriptor type */
    my_gdt[5].dpl = 3; //ring3    //:2;      /* descriptor privilege level */
    my_gdt[5].p = 1;              //:1;      /* segment present flag */
    my_gdt[5].limit_2 = 0xf;      //:4;      /* bits 16-19 of the segment limit */
    my_gdt[5].avl = 1;            //:1;      /* available for fun and profit */
    my_gdt[5].l = 0; // 32 bits   //:1;      /* longmode */
    my_gdt[5].d = 1;              //:1;      /* default length, depend on seg type */
    my_gdt[5].g = 1;              //:1;      /* granularity */
    my_gdt[5].base_3 = 0x00;      //:8;      /* bits 24-31 of the base address */
    // end Q12

    // Q13
    // DS/ES/FS/GS
    set_ds(gdt_usr_seg_sel(5));
    set_es(gdt_usr_seg_sel(5));
    set_fs(gdt_usr_seg_sel(5));
    set_gs(gdt_usr_seg_sel(5));
    
    // SS
    // set_ss(gdt_usr_seg_sel(5)); // plante, #GP
    tss_t TSS;
    TSS.s0.esp = get_ebp();
    TSS.s0.ss  = gdt_krn_seg_sel(2);
    tss_dsc(&my_gdt[6], (offset_t)&TSS);
    set_tr(gdt_krn_seg_sel(6));
    
    debug("jusqu'ici tout va bien");
    
    // CS via farjump
    //fptr32_t fptr = {.segment = gdt_usr_seg_sel(5), .offset = (uint32_t)userland}; 
    //farjump(fptr);  // plante, #GP
    // interdit, un moyen de démarrer une tâche ring 3 depuis le ring 0 est 
    // de détourner l'usage principal de iret pour profiter du changement 
    // de contexte que le CPU sait effectuer à ce moment-là... cf. TP3 pour l'implem.
    // end Q13

}
