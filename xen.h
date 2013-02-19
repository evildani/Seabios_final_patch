#ifndef __XEN_H
#define __XEN_H

#include "util.h"
#include "config.h"

extern u32 xen_cpuid_base;

void xen_probe(void);
void xen_setup(void);
void xen_init_hypercalls(void);
void xen_copy_biostables(void);


static inline int usingXen(void) {
    if (!CONFIG_XEN)
	return 0;
    return (xen_cpuid_base != 0);
}

unsigned long xen_hypercall_page;
typedef unsigned long xen_ulong_t;
typedef unsigned long xen_pfn_t;

#define _hypercall0(type, name)                                         \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res;                                                         \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res)                                                  \
        : "0" (__hentry)                                                \
        : "memory" );                                                   \
    (type)__res;                                                        \
})

#define _hypercall1(type, name, a1)                                     \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res, __ign1;                                                 \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res), "=b" (__ign1)                                   \
        : "0" (__hentry), "1" ((long)(a1))                              \
        : "memory" );                                                   \
    (type)__res;                                                        \
})

#define _hypercall2(type, name, a1, a2)                                 \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res, __ign1, __ign2;                                         \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res), "=b" (__ign1), "=c" (__ign2)                    \
        : "0" (__hentry), "1" ((long)(a1)), "2" ((long)(a2))            \
        : "memory" );                                                   \
    (type)__res;                                                        \
})

#define _hypercall3(type, name, a1, a2, a3)                             \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res, __ign1, __ign2, __ign3;                                 \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res), "=b" (__ign1), "=c" (__ign2),                   \
          "=d" (__ign3)                                                 \
        : "0" (__hentry), "1" ((long)(a1)), "2" ((long)(a2)),           \
          "3" ((long)(a3))                                              \
        : "memory" );                                                   \
    (type)__res;                                                        \
})

#define _hypercall4(type, name, a1, a2, a3, a4)                         \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res, __ign1, __ign2, __ign3, __ign4;                         \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res), "=b" (__ign1), "=c" (__ign2),                   \
          "=d" (__ign3), "=S" (__ign4)                                  \
        : "0" (__hentry), "1" ((long)(a1)), "2" ((long)(a2)),           \
          "3" ((long)(a3)), "4" ((long)(a4))                            \
        : "memory" );                                                   \
    (type)__res;                                                        \
})

#define _hypercall5(type, name, a1, a2, a3, a4, a5)                     \
({                                                                      \
    unsigned long __hentry = xen_hypercall_page+__HYPERVISOR_##name*32; \
    long __res, __ign1, __ign2, __ign3, __ign4, __ign5;                 \
    asm volatile (                                                      \
        "call *%%eax"                                                   \
        : "=a" (__res), "=b" (__ign1), "=c" (__ign2),                   \
          "=d" (__ign3), "=S" (__ign4), "=D" (__ign5)                   \
        : "0" (__hentry), "1" ((long)(a1)), "2" ((long)(a2)),           \
          "3" ((long)(a3)), "4" ((long)(a4)),                           \
          "5" ((long)(a5))                                              \
        : "memory" );                                                   \
    (type)__res;                                                        \
})


/******************************************************************************
 *
 * The following interface definitions are taken from Xen and have the
 * following license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/******************************************************************************
 * arch-x86/xen.h
 *
 * Guest OS interface to x86 Xen.
 *
 * Copyright (c) 2004-2006, K A Fraser
 */


/* Structural guest handles introduced in 0x00030201. */
#define ___DEFINE_XEN_GUEST_HANDLE(name, type) \
    typedef struct { type *p; } __guest_handle_ ## name

#define __DEFINE_XEN_GUEST_HANDLE(name, type) \
    ___DEFINE_XEN_GUEST_HANDLE(name, type);   \
    ___DEFINE_XEN_GUEST_HANDLE(const_##name, const type)
#define DEFINE_XEN_GUEST_HANDLE(name)   __DEFINE_XEN_GUEST_HANDLE(name, name)
#define __XEN_GUEST_HANDLE(name)        __guest_handle_ ## name
#define XEN_GUEST_HANDLE(name)          __XEN_GUEST_HANDLE(name)
#define set_xen_guest_handle_raw(hnd, val)  do { (hnd).p = val; } while (0)
#define set_xen_guest_handle(hnd, val) set_xen_guest_handle_raw(hnd, val)

/****
 * This is needed to get the shared info page
 ****/


struct arch_shared_info {
	unsigned long max_pfn;                  /* max pfn that appears in table */
	/* Frame containing list of mfns containing list of mfns containing p2m. */
	xen_pfn_t     pfn_to_mfn_frame_list_list;
	unsigned long nmi_reason;
	u64 pad[32];
};
typedef struct arch_shared_info arch_shared_info_t;

struct arch_vcpu_info {
	unsigned long cr2;
	unsigned long pad[5]; /* sizeof(vcpu_info_t) == 64 */
};
typedef struct arch_vcpu_info arch_vcpu_info_t;

struct vcpu_time_info {
	/*
	 * Updates to the following values are preceded and followed by an
	 * increment of 'version'. The guest can therefore detect updates by
	 * looking for changes to 'version'. If the least-significant bit of
	 * the version number is set then an update is in progress and the guest
	 * must wait to read a consistent set of values.
	 * The correct way to interact with the version number is similar to
	 * Linux's seqlock: see the implementations of read_seqbegin/read_seqretry.
	 */
	u32 version;
	u32 pad0;
	u64 tsc_timestamp;   /* TSC at last update of time vals.  */
	u64 system_time;     /* Time, in nanosecs, since boot.    */
	/*
	 * Current system time:
	 *   system_time +
	 *   ((((tsc - tsc_timestamp) << tsc_shift) * tsc_to_system_mul) >> 32)
	 * CPU frequency (Hz):
	 *   ((10^9 << 32) / tsc_to_system_mul) >> tsc_shift
	 */
	u32 tsc_to_system_mul;
	u8   tsc_shift;
	u8   pad1[3];
}; /* 32 bytes */
typedef struct vcpu_time_info vcpu_time_info_t;


struct vcpu_info {
	u8 evtchn_upcall_pending;
	u8 evtchn_upcall_mask;
	unsigned long evtchn_pending_sel;
	struct arch_vcpu_info arch;
	struct vcpu_time_info time;
};  /*64 bytes (x86)*/

#define XEN_LEGACY_MAX_VCPUS 32

struct shared_info {
	struct vcpu_info vcpu_info[XEN_LEGACY_MAX_VCPUS];
	unsigned long evtchn_pending[sizeof(unsigned long) * 8];
	unsigned long evtchn_mask[sizeof(unsigned long) * 8];
	u32 wc_version;      /* Version counter: see vcpu_time_info_t. */
	u32 wc_sec;          /* Secs  00:00:00 UTC, Jan 1, 1970.  */
	u32 wc_nsec;         /* Nsecs 00:00:00 UTC, Jan 1, 1970.  */

	struct arch_shared_info arch;

};
typedef struct shared_info shared_info_t;


struct shared_info *get_shared_info(void);

/******************************************************************************
 * version.h
 *
 * Xen version, type, and compile information.
 *
 * Copyright (c) 2005, Nguyen Anh Quynh <aquynh@gmail.com>
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */

/* arg == xen_extraversion_t. */
#define XENVER_extraversion 1
typedef char xen_extraversion_t[16];
#define XEN_EXTRAVERSION_LEN (sizeof(xen_extraversion_t))

/******************************************************************************
 * xen.h
 *
 * Guest OS interface to Xen.
 *
 * Copyright (c) 2004, K A Fraser
 */

#define DOMID_SELF  (0x7FF0U)

/* Guest handles for primitive C types. */
DEFINE_XEN_GUEST_HANDLE(char);
__DEFINE_XEN_GUEST_HANDLE(uchar, unsigned char);
DEFINE_XEN_GUEST_HANDLE(int);
__DEFINE_XEN_GUEST_HANDLE(uint,  unsigned int);
DEFINE_XEN_GUEST_HANDLE(long);
__DEFINE_XEN_GUEST_HANDLE(ulong, unsigned long);
DEFINE_XEN_GUEST_HANDLE(void);

DEFINE_XEN_GUEST_HANDLE(u64);
DEFINE_XEN_GUEST_HANDLE(xen_pfn_t);

__DEFINE_XEN_GUEST_HANDLE(u8,  u8);
__DEFINE_XEN_GUEST_HANDLE(u16, u16);
__DEFINE_XEN_GUEST_HANDLE(u32, u32);

#define __HYPERVISOR_memory_op            12
#define __HYPERVISOR_xen_version          17
#define __HYPERVISOR_grant_table_op       20
#define __HYPERVISOR_sched_op             29
#define __HYPERVISOR_event_channel_op     32
#define __HYPERVISOR_hvm_op               34

/*
 *  from: xen/include/public/hvm/hvm_op.h
 */

/*
 *  in an HVM guest you find your xenstore ring and evtchn via hvmparams,
 *  HVM_PARAM_STORE_PFN and HVM_PARAM_STORE_EVTCHN.
 *  it's a hypercall (type) hvmop, subcommand HVMOP_get_param (this is the hypercall actually) with this:
 *  Get/set subcommands: extra argument == pointer to xen_hvm_param struct.
*/
#define HVMOP_set_param  0
#define HVMOP_get_param  1

/*
 * from: include/public/hvm/params.h
 */

#define HVM_PARAM_STORE_PFN    1 //pass as index
#define HVM_PARAM_STORE_EVTCHN 2 //pass as index

struct xen_hvm_param {
    u16 domid;    //IN
    u32 index;    //IN
    u64 value;    //IN/OUT
};


/******************************************************************************
 * event_channel.h
 *
 * Event channels between domains.
 *
 * Copyright (c) 2003-2004, K A Fraser.
 */

typedef u32 evtchn_port_t;
DEFINE_XEN_GUEST_HANDLE(evtchn_port_t);

#define EVTCHNOP_send             4
struct evtchn_send {
    /* IN parameters. */
    evtchn_port_t port;
};
typedef struct evtchn_send evtchn_send_t;

/*
 * EVTCHNOP_status: Get the current status of the communication channel which
 * has an endpoint at <dom, port>.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may obtain the status of an event
 *     channel for which <dom> is not DOMID_SELF.
 */
#define EVTCHNOP_status           5
struct evtchn_status {
    /* IN parameters */
    u16  dom;
    evtchn_port_t port;
    /* OUT parameters */
#define EVTCHNSTAT_closed       0  /* Channel is not in use.                 */
#define EVTCHNSTAT_unbound      1  /* Channel is waiting interdom connection.*/
#define EVTCHNSTAT_interdomain  2  /* Channel is connected to remote domain. */
#define EVTCHNSTAT_pirq         3  /* Channel is bound to a phys IRQ line.   */
#define EVTCHNSTAT_virq         4  /* Channel is bound to a virtual IRQ line */
#define EVTCHNSTAT_ipi          5  /* Channel is bound to a virtual IPI line */
    u32 status;
    u32 vcpu;                 /* VCPU to which this channel is bound.   */
    union {
        struct {
        	u16 dom;
        } unbound; /* EVTCHNSTAT_unbound */
        struct {
        	u16 dom;
            evtchn_port_t port;
        } interdomain; /* EVTCHNSTAT_interdomain */
        u32 pirq;      /* EVTCHNSTAT_pirq        */
        u32 virq;      /* EVTCHNSTAT_virq        */
    } u;
};
typedef struct evtchn_status evtchn_status_t;

/*
 * EVTCHNOP_alloc_unbound: Allocate a port in domain <dom> and mark as
 * accepting interdomain bindings from domain <remote_dom>. A fresh port
 * is allocated in <dom> and returned as <port>.
 * NOTES:
 *  1. If the caller is unprivileged then <dom> must be DOMID_SELF.
 *  2. <rdom> may be DOMID_SELF, allowing loopback connections.
 */
#define EVTCHNOP_alloc_unbound    6
struct evtchn_alloc_unbound {
    /* IN parameters */
    u16 dom, remote_dom;
    /* OUT parameters */
    evtchn_port_t port;
};
typedef struct evtchn_alloc_unbound evtchn_alloc_unbound_t;

/*
 * EVTCHNOP_bind_interdomain: Construct an interdomain event channel between
 * the calling domain and <remote_dom>. <remote_dom,remote_port> must identify
 * a port that is unbound and marked as accepting bindings from the calling
 * domain. A fresh port is allocated in the calling domain and returned as
 * <local_port>.
 * NOTES:
 *  2. <remote_dom> may be DOMID_SELF, allowing loopback connections.
 */
#define EVTCHNOP_bind_interdomain 0
struct evtchn_bind_interdomain {
    /* IN parameters. */
    u16 remote_dom;
    evtchn_port_t remote_port;
    /* OUT parameters. */
    evtchn_port_t local_port;
};
typedef struct evtchn_bind_interdomain evtchn_bind_interdomain_t;



/******************************************************************************
 * sched.h
 *
 * Scheduler state interactions
 *
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */

/*
 * Voluntarily yield the CPU.
 * @arg == NULL.
 */
#define SCHEDOP_yield       0

/*
 * Block execution of this VCPU until an event is received for processing.
 * If called with event upcalls masked, this operation will atomically
 * reenable event delivery and check for pending events before blocking the
 * VCPU. This avoids a "wakeup waiting" race.
 * @arg == NULL.
 */
#define SCHEDOP_block       1


/*
 * Poll a set of event-channel ports. Return when one or more are pending. An
 * optional timeout may be specified.
 * @arg == pointer to sched_poll structure.
 */
#define SCHEDOP_poll        3
struct sched_poll {
    XEN_GUEST_HANDLE(evtchn_port_t) ports;
    unsigned int nr_ports;
    u64 timeout;
};
typedef struct sched_poll sched_poll_t;
DEFINE_XEN_GUEST_HANDLE(sched_poll_t);

/*
 * Error base
 */
#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	EIO		 5	/* I/O error */
#define	EACCES		13	/* Permission denied */
#define	EINVAL		22	/* Invalid argument */
#define	ENOSYS		38	/* Function not implemented */
#define	EISCONN		106	/* Transport endpoint is already connected */

/*
 * xs_wire.h
 * Details of the "wire" protocol between Xen Store Daemon and client
 * library or guest kernel.
 *
 * Copyright (C) 2005 Rusty Russell IBM Corporation
 */
#define XENSTORE_PAYLOAD_MAX 4096

enum xsd_sockmsg_type
{
    XS_DEBUG,
    XS_DIRECTORY,
    XS_READ,
    XS_GET_PERMS,
    XS_WATCH,
    XS_UNWATCH,
    XS_TRANSACTION_START,
    XS_TRANSACTION_END,
    XS_INTRODUCE,
    XS_RELEASE,
    XS_GET_DOMAIN_PATH,
    XS_WRITE,
    XS_MKDIR,
    XS_RM,
    XS_SET_PERMS,
    XS_WATCH_EVENT,
    XS_ERROR,
    XS_IS_DOMAIN_INTRODUCED,
    XS_RESUME,
    XS_SET_TARGET,
    XS_RESTRICT
};

#define XS_WRITE_NONE "NONE"
#define XS_WRITE_CREATE "CREATE"
#define XS_WRITE_CREATE_EXCL "CREATE|EXCL"

/* We hand errors as strings, for portability. */
struct xsd_errors
{
    int errnum;
    const char *errstring;
};

struct xsd_sockmsg
{
    u32 type;  /* XS_??? */
    u32 req_id;/* Request identifier, echoed in daemon's response.  */
    u32 tx_id; /* Transaction id (0 if not related to a transaction). */
    u32 len;   /* Length of data following this. */

    /* Generally followed by nul-terminated string(s). */
};

enum xs_watch_type
{
    XS_WATCH_PATH = 0,
    XS_WATCH_TOKEN
};

/* Inter-domain shared memory communications. */
#define XENSTORE_RING_SIZE 1024
typedef u32 XENSTORE_RING_IDX;
#define MASK_XENSTORE_IDX(idx) ((idx) & (XENSTORE_RING_SIZE-1))
struct xenstore_domain_interface {
    char req[XENSTORE_RING_SIZE]; /* Requests to xenstore daemon. */
    char rsp[XENSTORE_RING_SIZE]; /* Replies and async watch events. */
    XENSTORE_RING_IDX req_cons, req_prod;
    XENSTORE_RING_IDX rsp_cons, rsp_prod;
};

#define XSD_ERROR(x) { x, #x }
static struct xsd_errors __attribute__ ((unused)) xsd_errors[]
    = {
    XSD_ERROR(EINVAL),
    XSD_ERROR(EACCES),
    XSD_ERROR(EIO),
    XSD_ERROR(EISCONN)
};

/******************************************************************************
 * memory.h
 *
 * Memory reservation and information.
 *
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */
/*
 * Sets the GPFN at which a particular page appears in the specified guest's
 * pseudophysical address space.
 * arg == addr of xen_add_to_physmap_t.
 */
#define XENMEM_add_to_physmap      7
struct xen_add_to_physmap {
    /* Which domain to change the mapping for. */
    u16 domid;

    /* Source mapping space. */
#define XENMAPSPACE_shared_info 0 /* shared info page */
#define XENMAPSPACE_grant_table 1 /* grant table page */
#define XENMAPSPACE_gmfn        2 /* GMFN */
    unsigned int space;

#define XENMAPIDX_grant_table_status 0x80000000

    /* Index into source mapping space. */
    xen_ulong_t idx;

    /* GPFN where the source mapping page should appear. */
    xen_pfn_t     gpfn;
};
typedef struct xen_add_to_physmap xen_add_to_physmap_t;
DEFINE_XEN_GUEST_HANDLE(xen_add_to_physmap_t);

/* -------------------------------------------------------------------------
 * xen/include/public/io/blkif.h
 * blkif.h
 *
 * Unified block-device I/O interface for Xen guest OSes.
 *
 * Copyright (c) 2003-2004, Keir Fraser
 */

enum blkif_state {
         BLKIF_STATE_DISCONNECTED,
         BLKIF_STATE_CONNECTED,
        BLKIF_STATE_SUSPENDED,
};

/*
 * REQUEST CODES.
 */
#define BLKIF_OP_READ              0
#define BLKIF_OP_WRITE             1
#define BLKIF_MAX_SEGMENTS_PER_REQUEST 8
/*
 * NB. first_sect and last_sect in blkif_request_segment, as well as
 * sector_number in blkif_request, are always expressed in 512-byte units.
 * However they must be properly aligned to the real sector size of the
 * physical disk, which is reported in the "sector-size" node in the backend
 * xenbus info. Also the xenbus "sectors" node is expressed in 512-byte units.
 */
struct blkif_request_segment {
    u32 gref;        /* reference to I/O buffer frame        */
    /* @first_sect: first sector in frame to transfer (inclusive).   */
    /* @last_sect: last sector in frame to transfer (inclusive).     */
    u8     first_sect, last_sect;
};



struct blkif_request {
    u8        operation;    /* BLKIF_OP_???                         */
    u8        nr_segments;  /* number of segments                   */
    u16   handle;       /* only for read/write requests    ????     */
    u64       id;           /* private guest value, echoed in resp  */
    u64 sector_number;/* start sector idx on disk (r/w only)  */
    struct blkif_request_segment seg[BLKIF_MAX_SEGMENTS_PER_REQUEST];
};
typedef struct blkif_request blkif_request_t;


struct blkif_response {
    u64        id;              /* copied from request */
    u8         operation;       /* copied from request */
    u16         status;          /* BLKIF_RSP_???       */
};
typedef struct blkif_response blkif_response_t;

/*
 * STATUS RETURN CODES.
 */
 /* Operation not supported (only happens on barrier writes). */
#define BLKIF_RSP_EOPNOTSUPP  -2
 /* Operation failed for some unspecified reason (-EIO). */
#define BLKIF_RSP_ERROR       -1
 /* Operation completed successfully. */
#define BLKIF_RSP_OKAY         0

/******************************************************************************
 * grant_table.h
 *
 * Interface for granting foreign access to page frames, and receiving
 * page-ownership transfers.
 *
 * Copyright (c) 2004, K A Fraser
 */

/*
 * Type of grant entry.
 *  GTF_invalid: This grant entry grants no privileges.
 *  GTF_permit_access: Allow @domid to map/access @frame.
 *  GTF_accept_transfer: Allow @domid to transfer ownership of one page frame
 *                       to this guest. Xen writes the page number to @frame.
 *  GTF_transitive: Allow @domid to transitively access a subrange of
 *                  @trans_grant in @trans_domid.  No mappings are allowed.
 */
#define GTF_invalid         (0U<<0)
#define GTF_permit_access   (1U<<0)
#define GTF_accept_transfer (2U<<0)
#define GTF_transitive      (3U<<0)
#define GTF_type_mask       (3U<<0)

/*
 * Subflags for GTF_permit_access.
 *  GTF_readonly: Restrict @domid to read-only mappings and accesses. [GST]
 *  GTF_reading: Grant entry is currently mapped for reading by @domid. [XEN]
 *  GTF_writing: Grant entry is currently mapped for writing by @domid. [XEN]
 *  GTF_PAT, GTF_PWT, GTF_PCD: (x86) cache attribute flags for the grant [GST]
 *  GTF_sub_page: Grant access to only a subrange of the page.  @domid
 *                will only be allowed to copy from the grant, and not
 *                map it. [GST]
 */
#define _GTF_readonly       (2)
#define GTF_readonly        (1U<<_GTF_readonly)
#define _GTF_reading        (3)
#define GTF_reading         (1U<<_GTF_reading)
#define _GTF_writing        (4)
#define GTF_writing         (1U<<_GTF_writing)
#define _GTF_PWT            (5)
#define GTF_PWT             (1U<<_GTF_PWT)
#define _GTF_PCD            (6)
#define GTF_PCD             (1U<<_GTF_PCD)
#define _GTF_PAT            (7)
#define GTF_PAT             (1U<<_GTF_PAT)
#define _GTF_sub_page       (8)
#define GTF_sub_page        (1U<<_GTF_sub_page)

struct grant_entry_header {
    u16 flags;
    u32  domid;
};
typedef struct grant_entry_header grant_entry_header_t;

/*
 * Version 2 of the grant entry structure.
 */
//#define grant_entry_v1 grant_entry
#define grant_entry_v1_t grant_entry_t

struct grant_entry_v1 {
    /* GTF_xxx: various type and flag information.  [XEN,GST] */
    u16 flags;
    /* The domain being granted foreign privileges. [GST] */
    u16  domid;
    /*
     * GTF_permit_access: Frame that @domid is allowed to map and access. [GST]
     * GTF_accept_transfer: Frame whose ownership transferred by @domid. [XEN]
     */
    u32 frame;
};
typedef struct grant_entry_v1 grant_entry_v1_t;

struct grant_entry_v1 *get_grant_table(void);

typedef u16 grant_status_t;

/*
 * Reference to a grant entry in a specified domains grant table.
 */
typedef u32 grant_ref_t;

/*
 * Handle to track a mapping created via a grant reference.
 */
typedef u32 grant_handle_t;

/*
 * GNTTABOP_map_grant_ref: Map the grant entry (<dom>,<ref>) for access
 * by devices and/or host CPUs. If successful, <handle> is a tracking number
 * that must be presented later to destroy the mapping(s). On error, <handle>
 * is a negative status code.
 * NOTES:
 *  1. If GNTMAP_device_map is specified then <dev_bus_addr> is the address
 *     via which I/O devices may access the granted frame.
 *  2. If GNTMAP_host_map is specified then a mapping will be added at
 *     either a host virtual address in the current address space, or at
 *     a PTE at the specified machine address.  The type of mapping to
 *     perform is selected through the GNTMAP_contains_pte flag, and the
 *     address is specified in <host_addr>.
 *  3. Mappings should only be destroyed via GNTTABOP_unmap_grant_ref. If a
 *     host mapping is destroyed by other means then it is *NOT* guaranteed
 *     to be accounted to the correct grant reference!
 */
#define GNTTABOP_map_grant_ref        0
struct gnttab_map_grant_ref {
    /* IN parameters. */
    u64 host_addr;
    u32 flags;               /* GNTMAP_* */
    u32 ref;
    u32  dom;
    /* OUT parameters. */
    u16  status;              /* GNTST_* */
    grant_handle_t handle;
    u64 dev_bus_addr;
};
typedef struct gnttab_map_grant_ref gnttab_map_grant_ref_t;

/*
 * GNTTABOP_unmap_grant_ref: Destroy one or more grant-reference mappings
 * tracked by <handle>. If <host_addr> or <dev_bus_addr> is zero, that
 * field is ignored. If non-zero, they must refer to a device/host mapping
 * that is tracked by <handle>
 * NOTES:
 *  1. The call may fail in an undefined manner if either mapping is not
 *     tracked by <handle>.
 *  3. After executing a batch of unmaps, it is guaranteed that no stale
 *     mappings will remain in the device or host TLBs.
 */
#define GNTTABOP_unmap_grant_ref      1
struct gnttab_unmap_grant_ref {
    /* IN parameters. */
    u64 host_addr;
    u64 dev_bus_addr;
    grant_handle_t handle;
    /* OUT parameters. */
    u16  status;              /* GNTST_* */
};
typedef struct gnttab_unmap_grant_ref gnttab_unmap_grant_ref_t;

/*
 * GNTTABOP_setup_table: Set up a grant table for <dom> comprising at least
 * <nr_frames> pages. The frame addresses are written to the <frame_list>.
 * Only <nr_frames> addresses are written, even if the table is larger.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may specify <dom> != DOMID_SELF.
 *  3. Xen may not support more than a single grant-table page per domain.
 */
#define GNTTABOP_setup_table          2
struct gnttab_setup_table {
    /* IN parameters. */
    u32  dom;
    u32 nr_frames;
    /* OUT parameters. */
    u16  status;              /* GNTST_* */
    XEN_GUEST_HANDLE(ulong) frame_list;
};
typedef struct gnttab_setup_table gnttab_setup_table_t;

/******************************************************************************
 * ring.h
 *
 * Shared producer-consumer ring macros.
 *
 * Tim Deegan and Andrew Warfield November 2004.
 */



#define xen_mb()  barrier()
#define xen_rmb() barrier()
#define xen_wmb() barrier()


typedef unsigned int RING_IDX;

/* Round a 32-bit unsigned constant down to the nearest power of two. */
#define __RD2(_x)  (((_x) & 0x00000002) ? 0x2                  : ((_x) & 0x1))
#define __RD4(_x)  (((_x) & 0x0000000c) ? __RD2((_x)>>2)<<2    : __RD2(_x))
#define __RD8(_x)  (((_x) & 0x000000f0) ? __RD4((_x)>>4)<<4    : __RD4(_x))
#define __RD16(_x) (((_x) & 0x0000ff00) ? __RD8((_x)>>8)<<8    : __RD8(_x))
#define __RD32(_x) (((_x) & 0xffff0000) ? __RD16((_x)>>16)<<16 : __RD16(_x))

/*
 * Calculate size of a shared ring, given the total available space for the
 * ring and indexes (_sz), and the name tag of the request/response structure.
 * A ring contains as many entries as will fit, rounded down to the nearest
 * power of two (so we can mask with (size-1) to loop around).
 */
#define __CONST_RING_SIZE(_s, _sz) \
    (__RD32(((_sz) - offsetof(struct _s##_sring, ring)) / \
	    sizeof(((struct _s##_sring *)0)->ring[0])))
/*
 * The same for passing in an actual pointer instead of a name tag.
 */
#define __RING_SIZE(_s, _sz) \
    (__RD32(((_sz) - (long)(_s)->ring + (long)(_s)) / sizeof((_s)->ring[0])))

/*
 * Macros to make the correct C datatypes for a new kind of ring.
 *
 * To make a new ring datatype, you need to have two message structures,
 * let's say request_t, and response_t already defined.
 *
 * In a header where you want the ring datatype declared, you then do:
 *
 *     DEFINE_RING_TYPES(mytag, request_t, response_t);
 *
 * These expand out to give you a set of types, as you can see below.
 * The most important of these are:
 *
 *     mytag_sring_t      - The shared ring.
 *     mytag_front_ring_t - The 'front' half of the ring.
 *     mytag_back_ring_t  - The 'back' half of the ring.
 *
 * To initialize a ring in your code you need to know the location and size
 * of the shared memory area (PAGE_SIZE, for instance). To initialise
 * the front half:
 *
 *     mytag_front_ring_t front_ring;
 *     SHARED_RING_INIT((mytag_sring_t *)shared_page);
 *     FRONT_RING_INIT(&front_ring, (mytag_sring_t *)shared_page, PAGE_SIZE);
 *
 * Initializing the back follows similarly (note that only the front
 * initializes the shared ring):
 *
 *     mytag_back_ring_t back_ring;
 *     BACK_RING_INIT(&back_ring, (mytag_sring_t *)shared_page, PAGE_SIZE);
 */

#define DEFINE_RING_TYPES(__name, __req_t, __rsp_t)                     \
                                                                        \
/* Shared ring entry */                                                 \
union __name##_sring_entry {                                            \
    __req_t req;                                                        \
    __rsp_t rsp;                                                        \
};                                                                      \
                                                                        \
/* Shared ring page */                                                  \
struct __name##_sring {                                                 \
    RING_IDX req_prod, req_event;                                       \
    RING_IDX rsp_prod, rsp_event;                                       \
    union {                                                             \
        struct {                                                        \
            u8 smartpoll_active;                                   \
        } netif;                                                        \
        struct {                                                        \
            u8 msg;                                                \
        } tapif_user;                                                   \
        u8 pvt_pad[4];                                             \
    } private;                                                          \
    u8 __pad[44];                                                  \
    union __name##_sring_entry ring[1]; /* variable-length */           \
};                                                                      \
                                                                        \
/* "Front" end's private variables */                                   \
struct __name##_front_ring {                                            \
    RING_IDX req_prod_pvt;                                              \
    RING_IDX rsp_cons;                                                  \
    unsigned int nr_ents;                                               \
    struct __name##_sring *sring;                                       \
};                                                                      \
                                                                        \
/* "Back" end's private variables */                                    \
struct __name##_back_ring {                                             \
    RING_IDX rsp_prod_pvt;                                              \
    RING_IDX req_cons;                                                  \
    unsigned int nr_ents;                                               \
    struct __name##_sring *sring;                                       \
};                                                                      \
                                                                        \
/* Syntactic sugar */                                                   \
typedef struct __name##_sring __name##_sring_t;                         \
typedef struct __name##_front_ring __name##_front_ring_t;               \
typedef struct __name##_back_ring __name##_back_ring_t

/*
 * Macros for manipulating rings.
 *
 * FRONT_RING_whatever works on the "front end" of a ring: here
 * requests are pushed on to the ring and responses taken off it.
 *
 * BACK_RING_whatever works on the "back end" of a ring: here
 * requests are taken off the ring and responses put on.
 *
 * N.B. these macros do NO INTERLOCKS OR FLOW CONTROL.
 * This is OK in 1-for-1 request-response situations where the
 * requestor (front end) never has more than RING_SIZE()-1
 * outstanding requests.
 */

/* Initialising empty rings */
#define SHARED_RING_INIT(_s) do {                                       \
    (_s)->req_prod  = (_s)->rsp_prod  = 0;                              \
    (_s)->req_event = (_s)->rsp_event = 1;                              \
    (void)memset((_s)->private.pvt_pad, 0, sizeof((_s)->private.pvt_pad)); \
    (void)memset((_s)->__pad, 0, sizeof((_s)->__pad));                  \
} while(0)

#define FRONT_RING_INIT(_r, _s, __size) do {                            \
    (_r)->req_prod_pvt = 0;                                             \
    (_r)->rsp_cons = 0;                                                 \
    (_r)->nr_ents = __RING_SIZE(_s, __size);                            \
    (_r)->sring = (_s);                                                 \
} while (0)

#define BACK_RING_INIT(_r, _s, __size) do {                             \
    (_r)->rsp_prod_pvt = 0;                                             \
    (_r)->req_cons = 0;                                                 \
    (_r)->nr_ents = __RING_SIZE(_s, __size);                            \
    (_r)->sring = (_s);                                                 \
} while (0)

/* Initialize to existing shared indexes -- for recovery */
#define FRONT_RING_ATTACH(_r, _s, __size) do {                          \
    (_r)->sring = (_s);                                                 \
    (_r)->req_prod_pvt = (_s)->req_prod;                                \
    (_r)->rsp_cons = (_s)->rsp_prod;                                    \
    (_r)->nr_ents = __RING_SIZE(_s, __size);                            \
} while (0)

#define BACK_RING_ATTACH(_r, _s, __size) do {                           \
    (_r)->sring = (_s);                                                 \
    (_r)->rsp_prod_pvt = (_s)->rsp_prod;                                \
    (_r)->req_cons = (_s)->req_prod;                                    \
    (_r)->nr_ents = __RING_SIZE(_s, __size);                            \
} while (0)

/* How big is this ring? */
#define RING_SIZE(_r)                                                   \
    ((_r)->nr_ents)

#define RING_SIZE_16BIT(_r)                                                   \
		((_r)->nr_ents)

/* Number of free requests (for use on front side only). */
#define RING_FREE_REQUESTS(_r)                                          \
    (RING_SIZE(_r) - ((_r)->req_prod_pvt - (_r)->rsp_cons))

/* Test if there is an empty slot available on the front ring.
 * (This is only meaningful from the front. )
 */
#define RING_FULL(_r)                                                   \
    (RING_FREE_REQUESTS(_r) == 0)

/* Test if there are outstanding messages to be processed on a ring. */
#define RING_HAS_UNCONSUMED_RESPONSES(_r)                               \
    ((_r)->sring->rsp_prod - (_r)->rsp_cons)

#ifdef __GNUC__
#define RING_HAS_UNCONSUMED_REQUESTS(_r) ({                             \
    unsigned int req = (_r)->sring->req_prod - (_r)->req_cons;          \
    unsigned int rsp = RING_SIZE(_r) -                                  \
        ((_r)->req_cons - (_r)->rsp_prod_pvt);                          \
    req < rsp ? req : rsp;                                              \
})
#else
/* Same as above, but without the nice GCC ({ ... }) syntax. */
#define RING_HAS_UNCONSUMED_REQUESTS(_r)                                \
    ((((_r)->sring->req_prod - (_r)->req_cons) <                        \
      (RING_SIZE(_r) - ((_r)->req_cons - (_r)->rsp_prod_pvt))) ?        \
     ((_r)->sring->req_prod - (_r)->req_cons) :                         \
     (RING_SIZE(_r) - ((_r)->req_cons - (_r)->rsp_prod_pvt)))
#endif

/* Direct access to individual ring elements, by index. */
#define RING_GET_REQUEST(_r, _idx)                                      \
    (&((_r)->sring->ring[((_idx) & (RING_SIZE(_r) - 1))].req))


#define RING_GET_RESPONSE(_r, _idx)                                     \
    (&((_r)->sring->ring[((_idx) & (RING_SIZE(_r) - 1))].rsp))

/* Loop termination condition: Would the specified index overflow the ring? */
#define RING_REQUEST_CONS_OVERFLOW(_r, _cons)                           \
    (((_cons) - (_r)->rsp_prod_pvt) >= RING_SIZE(_r))

#define RING_PUSH_REQUESTS(_r) do {                                     \
    xen_wmb(); /* back sees requests /before/ updated producer index */ \
    (_r)->sring->req_prod = (_r)->req_prod_pvt;                         \
} while (0)

#define RING_PUSH_RESPONSES(_r) do {                                    \
    xen_wmb(); /* front sees resps /before/ updated producer index */   \
    (_r)->sring->rsp_prod = (_r)->rsp_prod_pvt;                         \
} while (0)

/*
 * Notification hold-off (req_event and rsp_event):
 *
 * When queueing requests or responses on a shared ring, it may not always be
 * necessary to notify the remote end. For example, if requests are in flight
 * in a backend, the front may be able to queue further requests without
 * notifying the back (if the back checks for new requests when it queues
 * responses).
 *
 * When enqueuing requests or responses:
 *
 *  Use RING_PUSH_{REQUESTS,RESPONSES}_AND_CHECK_NOTIFY(). The second argument
 *  is a boolean return value. True indicates that the receiver requires an
 *  asynchronous notification.
 *
 * After dequeuing requests or responses (before sleeping the connection):
 *
 *  Use RING_FINAL_CHECK_FOR_REQUESTS() or RING_FINAL_CHECK_FOR_RESPONSES().
 *  The second argument is a boolean return value. True indicates that there
 *  are pending messages on the ring (i.e., the connection should not be put
 *  to sleep).
 *
 *  These macros will set the req_event/rsp_event field to trigger a
 *  notification on the very next message that is enqueued. If you want to
 *  create batches of work (i.e., only receive a notification after several
 *  messages have been enqueued) then you will need to create a customised
 *  version of the FINAL_CHECK macro in your own code, which sets the event
 *  field appropriately.
 */

#define RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(_r, _notify) do {           \
    RING_IDX __old = (_r)->sring->req_prod;                             \
    RING_IDX __new = (_r)->req_prod_pvt;                                \
    xen_wmb(); /* back sees requests /before/ updated producer index */ \
    (_r)->sring->req_prod = __new;                                      \
    xen_mb(); /* back sees new requests /before/ we check req_event */  \
    (_notify) = ((RING_IDX)(__new - (_r)->sring->req_event) <           \
                 (RING_IDX)(__new - __old));                            \
} while (0)

#define RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(_r, _notify) do {          \
    RING_IDX __old = (_r)->sring->rsp_prod;                             \
    RING_IDX __new = (_r)->rsp_prod_pvt;                                \
    xen_wmb(); /* front sees resps /before/ updated producer index */   \
    (_r)->sring->rsp_prod = __new;                                      \
    xen_mb(); /* front sees new resps /before/ we check rsp_event */    \
    (_notify) = ((RING_IDX)(__new - (_r)->sring->rsp_event) <           \
                 (RING_IDX)(__new - __old));                            \
} while (0)

#define RING_FINAL_CHECK_FOR_REQUESTS(_r, _work_to_do) do {             \
    (_work_to_do) = RING_HAS_UNCONSUMED_REQUESTS(_r);                   \
    if (_work_to_do) break;                                             \
    (_r)->sring->req_event = (_r)->req_cons + 1;                        \
    xen_mb();                                                           \
    (_work_to_do) = RING_HAS_UNCONSUMED_REQUESTS(_r);                   \
} while (0)

#define RING_FINAL_CHECK_FOR_RESPONSES(_r, _work_to_do) do {            \
    (_work_to_do) = RING_HAS_UNCONSUMED_RESPONSES(_r);                  \
    if (_work_to_do) break;                                             \
    (_r)->sring->rsp_event = (_r)->rsp_cons + 1;                        \
    xen_mb();                                                           \
    (_work_to_do) = RING_HAS_UNCONSUMED_RESPONSES(_r);                  \
} while (0)

/*
 * Wrappers for hypercalls
 */
static inline int hypercall_xen_version( int cmd, void *arg)
{
	return _hypercall2(int, xen_version, cmd, arg);
}

static inline int hypercall_hvm_op(int cmd, void *arg)
{
	return _hypercall2(int, hvm_op, cmd, arg);
}

static inline int hypercall_event_channel_op(int cmd, void *arg)
{
	return _hypercall2(int, event_channel_op, cmd, arg);
}

/*
 * called in xen-blk-op 16bit mode to 32bit mode
 */
static inline int hypercall_event_channel_op_evnt_send(void *arg)
{
	dprintf(1,"HYPERCALL %p\n",arg);
	int i = _hypercall2(int,event_channel_op,EVTCHNOP_send,arg);
	dprintf(1,"HYPERCALL RETURN %d\n",i);
	return i;
}

static inline int hypercall_memory_op(int cmd ,void *arg)
{
	return _hypercall2(int, memory_op, cmd ,arg);
}

static inline int hypercall_sched_op(int cmd, void *arg)
{
	return _hypercall2(int, sched_op, cmd, arg);
}

static inline int hypercall_grant_table_op(int cmd, void *arg, unsigned int count)
{
	return _hypercall3(int, grant_table_op, cmd, arg, count);
}


#endif
