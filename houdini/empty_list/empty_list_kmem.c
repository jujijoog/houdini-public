#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mach/mach.h>

#include "kmem.h"

/***** mach_vm.h *****/
kern_return_t mach_vm_read(
                           vm_map_t target_task,
                           mach_vm_address_t address,
                           mach_vm_size_t size,
                           vm_offset_t *data,
                           mach_msg_type_number_t *dataCnt);

kern_return_t mach_vm_write(
                            vm_map_t target_task,
                            mach_vm_address_t address,
                            vm_offset_t data,
                            mach_msg_type_number_t dataCnt);

kern_return_t mach_vm_read_overwrite(
                                     vm_map_t target_task,
                                     mach_vm_address_t address,
                                     mach_vm_size_t size,
                                     mach_vm_address_t data,
                                     mach_vm_size_t *outsize);

kern_return_t mach_vm_allocate(
                               vm_map_t target,
                               mach_vm_address_t *address,
                               mach_vm_size_t size,
                               int flags);

kern_return_t mach_vm_deallocate (
                                  vm_map_t target,
                                  mach_vm_address_t address,
                                  mach_vm_size_t size);

kern_return_t mach_vm_protect (
                               vm_map_t target_task,
                               mach_vm_address_t address,
                               mach_vm_size_t size,
                               boolean_t set_maximum,
                               vm_prot_t new_protection);

extern mach_port_t tfp0;
void empty_list_prepare_for_rw_with_fake_tfp0(mach_port_t fake_tfp0) {
  tfp0 = fake_tfp0;
}

void empty_list_wk32(uint64_t kaddr, uint32_t val) {
  if (tfp0 == MACH_PORT_NULL) {
    printf("attempt to write to kernel memory before any kernel memory write primitives available\n");
    sleep(3);
    return;
  }
  
  kern_return_t err;
  err = mach_vm_write(tfp0,
                      (mach_vm_address_t)kaddr,
                      (vm_offset_t)&val,
                      (mach_msg_type_number_t)sizeof(uint32_t));
  
  if (err != KERN_SUCCESS) {
    printf("tfp0 write failed: %s %x\n", mach_error_string(err), err);
    return;
  }
}

void empty_list_wk64(uint64_t kaddr, uint64_t val) {
  uint32_t lower = (uint32_t)(val & 0xffffffff);
  uint32_t higher = (uint32_t)(val >> 32);
  wk32(kaddr, lower);
  wk32(kaddr+4, higher);
}

uint32_t empty_list_rk32(uint64_t kaddr) {
  kern_return_t err;
  uint32_t val = 0;
  mach_vm_size_t outsize = 0;
  err = mach_vm_read_overwrite(tfp0,
                               (mach_vm_address_t)kaddr,
                               (mach_vm_size_t)sizeof(uint32_t),
                               (mach_vm_address_t)&val,
                               &outsize);
  if (err != KERN_SUCCESS){
    printf("tfp0 read failed %s addr: 0x%llx err:%x port:%x\n", mach_error_string(err), kaddr, err, tfp0);
    sleep(3);
    return 0;
  }
  
  if (outsize != sizeof(uint32_t)){
    printf("tfp0 read was short (expected %lx, got %llx\n", sizeof(uint32_t), outsize);
    sleep(3);
    return 0;
  }
  return val;
}

uint64_t empty_list_rk64(uint64_t kaddr) {
  uint64_t lower = rk32(kaddr);
  uint64_t higher = rk32(kaddr+4);
  uint64_t full = ((higher<<32) | lower);
  return full;
}
