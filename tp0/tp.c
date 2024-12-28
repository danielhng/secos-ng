/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

extern info_t   *info;
extern uint32_t __kernel_start__;
extern uint32_t __kernel_end__;

void tp() {
   debug("kernel mem [0x%p - 0x%p]\n", &__kernel_start__, &__kernel_end__);
   debug("MBI flags 0x%x\n", info->mbi->flags);

   multiboot_memory_map_t* entry = (multiboot_memory_map_t*)info->mbi->mmap_addr;
   while((uint32_t)entry < (info->mbi->mmap_addr + info->mbi->mmap_length)) {
      // TODO print "[start - end] type" for each entry
      
      switch (entry->type)
      {
      case 1:
         debug("[0x%llx-0x%llx] %s \n", entry->addr, ((entry->addr)+(entry->len)-1), "MULTIBOOT_MEMORY_AVAILABLE");
         break;

      case 2:
         debug("[0x%llx-0x%llx] %s \n", entry->addr, ((entry->addr)+(entry->len)-1),"MULTIBOOT_MEMORY_RESERVED");
         break;

      case 3:
         debug("[0x%llx-0x%llx] %s \n", entry->addr, ((entry->addr)+(entry->len)-1), "MULTIBOOT_MEMORY_ACPI_RECLAIMABLE");
         break;

      case 4:
         debug("[0x%llx-0x%llx] %s \n", entry->addr, ((entry->addr)+(entry->len)-1), "MULTIBOOT_MEMORY_NVS");
         break;
      
      default:
         break;
      }
      
      entry++;
   }

   int *ptr_in_available_mem;
   ptr_in_available_mem = (int*)0xf0000;
   debug("Available mem (0xf0000): before: 0x%x ", *ptr_in_available_mem); // read
   *ptr_in_available_mem = 0xaaaaaaaa;                           // write
   debug("after: 0x%x\n", *ptr_in_available_mem);                // check

   int *ptr_in_reserved_mem;
   ptr_in_reserved_mem = (int*)0xf0000;
   debug("Reserved mem (at: 0xf0000):  before: 0x%x ", *ptr_in_reserved_mem); // read
   *ptr_in_reserved_mem = 0xaaaaaaaa;                           // write
   debug("after: 0x%x\n", *ptr_in_reserved_mem);                // check

   int *ptr_out_of_mem;
   ptr_out_of_mem = (int*)0xfffffff;
   debug("Writting mem > 128MB (0x100000): before : 0x%x ", *ptr_out_of_mem);
   *ptr_out_of_mem = 0xaaaaaaaa;
   debug("after: 0x%x\n", *ptr_out_of_mem);

}