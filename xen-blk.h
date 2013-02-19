#ifndef _XEN_BLK_H
#define _XEN_BLK_H


/*
 * Prototypes
 */
void init_grant_tables(void);
int xen_blk_setup(void);
struct xendrive_s* init_xen_blk(u16 _bdf);
int xenstore_share_vbd(char * vbd, struct xendrive_s *xd);
char * list_xenstore_drives(u32 *num);


/*
 * Via ring.h the ring is created
 * This will create the way to call functions on ring.h such as RING_GET_REQUEST and RING_GET_RESPONSE
 */
#define BLK_RING_SIZE __CONST_RING_SIZE(blkif, 512)
#define BLKIF_REQUEST 10 //8 sectors can fit in a page
#define BYTES_PER_SECTOR 4096

/*
 * grant table perms for this code
 */
#define permit_access 0
#define accept_transfer 1

/*
 * This structure represents the
 */
#define BLKIF_FREE_ID 4234 //a simple mechanism to keep track of requests and responses, increases
DEFINE_RING_TYPES(blkif, struct blkif_request, struct blkif_response);
struct blkfront_info
{
	int free;
	enum blkif_state connected;
	int ring_ref; //gref for the rings
	evtchn_port_t port; //ring event channel
    struct blkif_front_ring * private ;
    struct blkif_sring *shared ;
	u64 segments[BLKIF_MAX_SEGMENTS_PER_REQUEST];
	int is_ready;
    void * buffer; //private buffer
	int buffer_gref; //gref for the private buffer
	int vbd_id;
};
typedef struct blkfront_info blkfront_info_t;

/*
 * Wrapper for xen device has geometry and location on memory
 */
struct xendrive_s {
   struct drive_s drive ; //contains the drive geometry
   struct blkfront_info info ; //memory location
};
typedef struct xendrive_s xendrive_s_t;

#define MAX_DRIVES 4
struct xendrive_s* xendrives_ptrs[MAX_DRIVES] ;
int count_drives ;
struct drive_s* drive_ptrs[MAX_DRIVES] ;


#endif
