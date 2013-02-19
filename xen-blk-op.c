#include "util.h" // dprintf
#include "pci.h" // foreachpci
#include "config.h" // CONFIG_*
#include "biosvar.h" // GET_GLOBAL
#include "pci_ids.h" // PCI_DEVICE_ID_XEN PCI_VENDOR_ID
#include "boot.h" // boot_add_hd
#include "xen.h" // hypercalls
#include "disk.h" //drive structs
#include "xen-blk.h" //drive structs
#include "memmap.h" //PAGE_SHIFT


/*
 * operation on blk
 * here the actual read or write is performed the xen_ring is the source or destini
 * depending on write or read accordingly.
 * TODO read and write
 */
int xen_blk_op(struct disk_op_s *op, int operation){
	dprintf(1,"Operation 16bit drive at@%p\n",op->drive_g);
	void *flatptr = MAKE_FLATPTR(GET_SEG(SS), op);
	if(operation==0){ //READ
		extern void _cfunc32flat_xen_blk_op_read(struct disk_op_s *);
		return call32(_cfunc32flat_xen_blk_op_read,(u32)flatptr,DISK_RET_EPARAM);

	}
	if(operation==1){
		extern void _cfunc32flat_xen_blk_op_write(struct disk_op_s *);
		return call32(_cfunc32flat_xen_blk_op_write,(u32)flatptr,DISK_RET_EPARAM);
	}
	return DISK_RET_EPARAM;
}


/*
 * Fuction to present the operation permited by blk
 * Change to use rings negotiated from xenstore
 */
int process_xen_op(struct disk_op_s *op)
{
	ASSERT16();
    if (! CONFIG_XEN || CONFIG_COREBOOT)
        return 0;
    //start to fill request struct, xendrive_g has a pointer called info to blkfront_info
    //end filling of request struct
    switch (op->command) { //extract command from struct
    case CMD_READ:
        return xen_blk_op(op, 0);
    case CMD_WRITE:
        return xen_blk_op(op, 1);
    case CMD_FORMAT:
    case CMD_RESET:
    case CMD_ISREADY:
    case CMD_VERIFY:
    case CMD_SEEK:
        return DISK_RET_SUCCESS;
    default:
        op->count = 0;
        return DISK_RET_EPARAM;
    }
}

/*
 *
 */

/*
 * operation on blk
 * here the actual read or write is performed the xen_ring is the source or destini
 * depending on write or read accordingly.
 * TODO read and write
 */
int xen_blk_op(struct disk_op_s *op, int operation){
	dprintf(1,"Operation 16bit drive at@%p\n",op->drive_g);
	void *flatptr = MAKE_FLATPTR(GET_SEG(SS), op);
	if(operation==0){ //READ
		extern void _cfunc32flat_xen_blk_op_read(struct disk_op_s *);
		return call32(_cfunc32flat_xen_blk_op_read,(u32)flatptr,DISK_RET_EPARAM);

	}
	if(operation==1){
		extern void _cfunc32flat_xen_blk_op_write(struct disk_op_s *);
		return call32(_cfunc32flat_xen_blk_op_write,(u32)flatptr,DISK_RET_EPARAM);
	}
	return DISK_RET_EPARAM;
}

