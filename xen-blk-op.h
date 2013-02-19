#ifndef _XEN_BLK_OP_H
#define _XEN_BLK_OP_H


int process_xen_op(struct disk_op_s *op);
int xen_blk_op(struct disk_op_s *op, int operation);


#endif
