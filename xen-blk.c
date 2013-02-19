#include "util.h" // dprintf
#include "pci.h" // foreachpci
#include "config.h" // CONFIG_*
#include "biosvar.h" // GET_GLOBAL
#include "pci_ids.h" // PCI_DEVICE_ID_XEN PCI_VENDOR_ID
#include "boot.h" // boot_add_hd
#include "xen.h" // hypercalls
#include "disk.h" //drive structs
#include "xen-xs.h" //drive structs
#include "xen-blk.h" //drive structs
#include "memmap.h" //PAGE_SHIFT


/*
 * ---------------------------------------------
 * 	TODO LIST:
 *  retrieve parameters from PCI device
	allocate free page
	initiate ring struct on freed page
	initiate private data
	share page with grant table
	enter grant reference in xenstore
	create event channel
	pass channel to backend via xenstore
	update xenbus status
	read disk geometry and create geometry struct
-------------------------------------------------------
*/

int xenstore_share_disk(char * vbd, struct xendrive_s *xd);
int xenstore_share_disk2(char * device, struct xendrive_s *xd,char * vbd_backend_path_status,char * device_state,char * bckend_path);
int xenstore_share_disk3(char * device, struct xendrive_s *xd,char * vbd_backend_path_status,char * bckend_path,char * state,char *device_state);
int xenstore_share_disk4(char * device, struct xendrive_s *xd,char * vbd_backend_path_status,char * bckend_path,char * state,char *device_state);
int xenstore_share_disk5(char * device, struct xendrive_s *xd,char * vbd_backend_path_status,char * bckend_path,char * state,char *device_state);
int xenstore_share_disk6(char * device, struct xendrive_s *xd,char * vbd_backend_path_status,char * bckend_path,char * state,char *device_state);
int xenstore_share_disk7(char * device, struct xendrive_s *xd,char * bckend_path,char * state,int dom0_state,char * device_state);



/*
 * Grant table struct
 */
#define MAX_GRANT_ENTRIES 30
struct grant_entry_v1 * grant_entries;
/*
 * this counts the number of grants in the pagetable grants used. starts at 0.
 */
int count_grants = 5 ;



u16 drives;


/*
 * The functions starts by getting a page for the page table
 * do XENMEM_add_to_physmap with space = XENMAPSPACE_grant_table on a page maps the granttable page in hypervisor
 * do a GNTTABOP_setup_table hypercall get granttable
 * For details look at hypercall wrappers
 * grant_table has the pointer to the page table afterwards
 */
void
init_grant_tables(void){
	int res;
	struct gnttab_setup_table gst;
	struct grant_entry_v1 *grant_entries_local = NULL;
	struct xen_add_to_physmap xatp;

	xatp.domid = DOMID_SELF;
	xatp.space = XENMAPSPACE_grant_table;
	xatp.idx   = 0;
	grant_entries_local = (struct grant_entry_v1 *) memalign_high(PAGE_SIZE, PAGE_SIZE);
	memset(grant_entries_local, 0, PAGE_SIZE);
	xatp.gpfn  = ((unsigned long)grant_entries_local >> PAGE_SHIFT);
	dprintf(1, "allocated grant_entries at %d bytes at %p, gpfn 0x%lx\n",sizeof(*grant_entries_local), grant_entries_local, xatp.gpfn);
	if (hypercall_memory_op(XENMEM_add_to_physmap, &xatp) != 0)
		panic("MAP grant_entries info page fail");
	SET_GLOBAL(grant_entries,grant_entries_local);
	SET_GLOBAL(count_grants,1);

	gst.dom = DOMID_SELF;
	gst.nr_frames = 1;
	res = hypercall_grant_table_op(GNTTABOP_setup_table, &gst, 1);
	if(res!=0){
		dprintf(1,"Error Mapping Grant Table... Abort...\n");
		panic("Map Grant Table Failed\n");
	}
	dprintf(1,"GNTTABOP_setup_table return %d status:%d\n",res,gst.status);
	/*
	 * The grant ref is just a number assiged as an offset in the grant table.
	 */
	//grant_entries[ref].domid = 0;
	//grant_entries[ref].frame = (u32)sring >> PAGE_SHIFT;
	//grant_entries[ref].flags = GTF_permit_access;
}

/*
 * We only issue share grants the params are mandatory
 * register with hypervisor the grant
 * count_grants is the index the last used grant. we need the next one
 * perms 0 for GTF_permit_access
 * perms 1 for
 * Return the allocated grant and advances the grant counter.
 */
int
get_free_grant(u16 ext_domid, int perms, u32 _frame)
{
	ASSERT32FLAT();
	int aval_grant;
	int count_grants_global = count_grants;
	struct grant_entry_v1 *  grant_entries_global  = grant_entries;
	if (count_grants_global < MAX_GRANT_ENTRIES){
		dprintf(1,"DEBUG Get free grant for %d for ext dom %d perms %d\n",_frame >> 12,ext_domid,perms);
		aval_grant = count_grants_global+1; //Next grant
		grant_entries_global[aval_grant].domid = ext_domid;
		grant_entries_global[aval_grant].frame = _frame >> 12;
		barrier();
		if(perms==permit_access){
			grant_entries_global[aval_grant].flags = GTF_permit_access;
		}
		else if(perms==accept_transfer){
			grant_entries_global[aval_grant].flags = GTF_accept_transfer;
		}else{
			dprintf(1,"The specified perms are not supported\n");
			panic("Grant entry Perms not supported\n");
		}
		//only grants one entry err = hypercall_grant_table_op(GNTTABOP_map_grant_ref, grant_entries+(sizeof(grant_entries)*aval_grant),1);
		//count_grants = aval_grant;
		SET_GLOBAL(count_grants,aval_grant);
		return aval_grant;
	}else
	{
		dprintf(1,"Error no more free entried\n");
		return -1;
	}
}



/*
 * First funtion called on POST hw_setup
 * This function walks the PCI devices to locate XenStore PCI device
 * If found calls init_xen_blk
 *
 */
int xen_blk_setup(void){
	dprintf(3, "Start initilize xen-blk\n");
	count_drives = 0 ;
	int drives = 0;
	u16 bdf_id;
	ASSERT32FLAT();
	if (!CONFIG_XEN)
		return 1;
	dprintf(3, "initilize xen-blk\n");
	int do_once=0;
	char * vbds = NULL;
	char * current_vbd = NULL; //this is where I must pass the current vbd string
	char * last_vbd = NULL; //this will contain the complete string at all times
	u32 vbd_count = 0;  //total chars including nulls in string
	int vbd_total = 0; //how many nulls are there, this indicates who many string there are
	int vbd_examined = 0;
	int i;
	struct xendrive_s *temp;
	struct pci_device *pci;
	struct bios_data_area_s *bda = MAKE_FLATPTR(SEG_BDA, 0);
	u8 hdid = bda->hdcount;
	dprintf(1,"Detected %d drives\n",hdid);
	foreachpci(pci) {
		//dprintf(1,"\nDEBUG:: Cheching foreachpci PCI_VENDOR_ID_XEN %x=%x, DTYPE_XEN %x=%x NEXT %x\n",PCI_VENDOR_ID_XEN,pci->vendor,DTYPE_XEN,pci->device,pci->next->device);
		if (pci->vendor == PCI_VENDOR_ID_XEN || pci->device == 1){ //Xenstore pci device
			//dprintf(1,"PCI found device device %x vendor %x\n",pci->device,pci->vendor);
			if(do_once==0){
				/*
				 * here the xenstore is located and rings for xenbus initiated
				 */
				xenbus_setup();
				vbds = list_xenstore_drives(&vbd_count);
				last_vbd = vbds; //start fresh count
				for(i=0;i<vbd_count;i++){
					if(vbds[i]=='\0'){
						//dprintf(1,"NULL");
						vbd_total++; //do once the counting
					}else{
						//dprintf(1,"%c",vbds[i]);
					}
				}
				dprintf(1,"\nDEBUG: found a total of %d nulls in %d chars\n",vbd_total,vbd_count);
				init_grant_tables();
				do_once=1;
			}
			//vbd_count has the total ammount of chars
			//vbds the full response string, same as last_vbd
			//vbd_examined how many nulls I have seen
			//vbd_total how many nulls are in the full string (char * vbds)
			//current_vbd is a local copy of the string I am working on
			int count = 0; //contains the amount of chars processed, in chunks of strlens
			int this = strlen(last_vbd);
			current_vbd  = malloc_high(this);
			memcpy(current_vbd,last_vbd,this+1);
			while(vbd_examined<vbd_total){ //strlen(vbds): size of str vbd_count: how many chars in response
				dprintf(1,"There is more than one drive, %d of %d - %d of %d chars\n",vbd_examined,vbd_total,count,vbd_count);
				//create local copy of current_vbd, consumes the pointer last_vbd
				this = strlen(last_vbd + count); //how far
				release(current_vbd);
				current_vbd = malloc_high(this+1);
				memcpy(current_vbd,last_vbd + count,this+1);
				//dprintf(1,"DEBUG: this:%d current_vbd:%s start:%p end:%p\n",this,current_vbd,last_vbd,last_vbd + count);
				/*for(j=0;j<this;j++){ //copy to next null
    					current_vbd[j] = last_vbd[j];
    					if(last_vbd[j]=='\0'){
    						last_vbd = last_vbd+j+1; //prepare to next one, start of next string
    						j=this; //force the end of for
    					}
    				}*/
				//dprintf(1,"DEBUG Device string %s length %d\n",current_vbd,strlen(current_vbd));
				vbd_examined++;
				//dprintf(1,"Ready Strings current %s last %s\n",current_vbd,last_vbd + count);

				bdf_id=pci->bdf;
				if(!pci){
					dprintf(3, "XEN PCI device NOT found\n");
					panic("Error in xen_blk_setup !bdf \n");
				}
				dprintf(1, "Init XEN Drive id:%d!\n",(u16)myatoi(current_vbd));
				temp = init_xen_blk(drives); //initiate ring and register handler
				//dprintf(1,"DEBUG: count %d current %d - %s\n",count,strlen(current_vbd),current_vbd);
				int res = xenstore_share_vbd(current_vbd,temp); //share stuff to xenstore
				count  = count + strlen(current_vbd)+1;
				//dprintf(1,"DEBUG: count %d current %d - %s\n",count,strlen(current_vbd),current_vbd);
				if(res==0){
					char *desc = znprintf(MAXDESCSIZE,"Xen DISK %d-XS %d\n",drives,myatoi(current_vbd));
					boot_add_hd(&temp->drive,desc,drives);
					drives++;
					dprintf(1, "END initilize disk  id:%d-%d!\n",drives-1,(u16)myatoi(current_vbd));
				}else if(res==1){
					//error 1
				}else if(res==3){
					char *desc = znprintf(MAXDESCSIZE,"Xen CD-ROM %d-XS %d\n",drives,myatoi(current_vbd));
					boot_add_cd(&temp->drive,desc,drives);
					drives++;
					dprintf(1, "End initilize cdrom id:%d-%d!\n",drives-1,(u16)myatoi(current_vbd));
				}else{

					//error 2
				}
			}
		}
	}
	return 0;
}


/*
 * _bdf is the address for the xen PCI device
	Xen's PC vendor ID is 0x5853 (XS in ASCII)
    The platform device is dev-id 0x0001
    So the sequence of events on boot:
    for any PVonHVM kernel (which includes SeaBIOS) is:
    scan PCI BUS (setup_blk_setup) -> discover Xen platform device -> setup/register xenbus
    and then scan xenbus -> discover Xen PV devices -> setup/register devices
    return 0 on sucess
 */

struct xendrive_s*
init_xen_blk(u16 _bdf)
{
	//dprintf(3, "Found BLK init start\n");
	struct xendrive_s *xd = malloc_fseg(sizeof(struct xendrive_s*));
	SET_FLATPTR(xendrives_ptrs[count_drives],xd);
	SET_FLATPTR(drive_ptrs[count_drives],&xd->drive);
	count_drives++;
	dprintf(1,"xendrive at %p\n",xd);
	if (xd == NULL) {
		dprintf(1, "xendrive init fail\n");
		warn_noalloc();
		dprintf(2, "fail to init front ring xen-blk\n");
		free(xd);
		return NULL;
	}
	memset(xd, 0, sizeof(*xd));
	xd->info.buffer = memalign_low(4096, 4096); //the buffer has the same size as the ring
	if(!xd->info.buffer)
		panic("Error no free mem");
	memset(xd->info.buffer,0,4096);
    xd->drive.type = DTYPE_XEN;
    /*
     * Unique id for a given driver type. Using the drives global variable
     */
    xd->drive.cntl_id = drives;
	/*
	 * here the rings for block device have to be initiated.
	 */
	xd->info.ring_ref = 0; //start invalid
	/*
	 * One is shared data area the other is private data area
	 */
	xd->info.private = (struct blkif_front_ring *)memalign_low(4096, 4096);
	xd->info.shared = (struct blkif_sring *)memalign_low(4096, 4096);
	dprintf(1,"MEM INFO: buffer at %p. shared ring at %p. private ring at%p\n",xd->info.buffer,xd->info.shared,xd->info.private);
	if (!xd->info.shared || !xd->info.private) {
		warn_noalloc();
		dprintf(1,"Error could not alloc memory for rings in low segment\n");
		return NULL; //allocation failed
	}
	memset(xd->info.private,0,4096);
	memset(xd->info.shared,0,4096);
	SHARED_RING_INIT(xd->info.shared);
	FRONT_RING_INIT(xd->info.private, xd->info.shared, PAGE_SIZE); //private r, shared d
	//Hard code maximum of 10 segments
	//gref for the ring, will be used in xenstore_setup
	xd->info.ring_ref = get_free_grant((u16)0,permit_access,(u32)xd->info.shared);
	//gref for the buffer will be used in process_xen_op
	xd->info.buffer_gref = get_free_grant((u16)0,permit_access,(u32)xd->info.buffer);
	return xd;
}

/*
 * This fuctions calls the xenstore and list all the xen ids of all vbd presented
 * in xenstore, this is needed later on. for drive geometry purposes
 */
char * list_xenstore_drives(u32 *num){
		char * ret_string = NULL;
		char * vbd_id = xenstore_directory("device/vbd",num); //the device id for this device
		u32 temp = (u32)&num;
		ret_string = malloc_high(temp+1);
		memcpy(ret_string,vbd_id,temp+1);
		//dprintf(1,"DEBUG devices xenstore-ls %s length %d\n",ret_string,*num);
		return ret_string;
}



int xenstore_share_vbd(char * vbd, struct xendrive_s *xd){
		if(!xd){
			dprintf(1,"Error in share vbd\n");
		}
		//dprintf(1,"Doing xenstore check for DISK for device %s lenght %d\n",vbd,strlen(vbd));
		char * device = strconcat("device/vbd/",vbd);
		char * device_type = strconcat(device,"/device-type");
		release(device);
		//dprintf(1,"DEBUG before read device path is: %s length %d\n",device_type,strlen(device_type));
		char * res = xenstore_read(device_type);
		//dprintf(1,"DEBUG after read Device %s is of type %s lenght %d \n",vbd,res,strlen(res));
		if(strcmp(res,"disk")==0){
			dprintf(1,"Device %s is a DISK proceed to register on xenstore\n",vbd);
			release(device_type);
			release(res);
			return xenstore_share_disk(vbd,xd);
		}else if(strcmp(res,"cdrom")==0){
			dprintf(1,"Device %s is a CDROM proceed to register on Seabios skip xenstore\n",vbd);
			release(device_type);
			release(res);
			return 3;
		}
		else{
			dprintf(1,"Device %s is %s SKIP...\n",vbd,res);
			release(device_type);
			release(res);
			return 1;
		}
		return 1;
}


int xenstore_share_disk(char * vbd, struct xendrive_s *xd){
	//dprintf(1,"Doing xenstore share for device %s\n",vbd);

	char * device = strconcat("device/vbd/",vbd); //the relative path for this device like: device/vbd/XXX
	//dprintf(1,"device path is %s\n",device);

	char * device_protocol = strconcat(device,"/protocol");
	char * proto_res = xenstore_write(device_protocol,"x86_32-abi");
	dprintf(1,"DEBUG response from protocol write is %s",proto_res);

	char * device_state = strconcat(device,"/state");
	//dprintf(1,"state path is %s\n",device_state);
	char * vbd_backend_path = strconcat(device,"/backend"); //DOM_ID for this type of device
	//with this result I should call get_free_gref
	//dprintf(1,"DEBUG xenstore read %s\n",vbd_backend_path);
	char * vbd_backend_0_path = xenstore_read(vbd_backend_path);
	//dprintf(1,"DEBUG xnestore read for %s response:%s\n",vbd_backend_path,vbd_backend_0_path);
	if(strlen(vbd_backend_0_path)<1){
		dprintf(1,"DEBUG xnestore read FAIL %s response:%s\n",vbd_backend_path,vbd_backend_0_path);
		return 1;
	}
	//release(vbd_backend_path);
	char * bckend_path = malloc_high(strlen(vbd_backend_0_path)+1); //use bckend_path
	memcpy(bckend_path,vbd_backend_0_path,strlen(vbd_backend_0_path)+1);
	char * vbd_backend_path_status = strconcat(bckend_path,"/state"); //full path for state of backend vbd_backend_path_status
	release(vbd_backend_0_path);
	//dprintf(1,"DEBUG device:%s vbd_backend_path_status:%s device_state:%s bckend_path:%s\n",device,vbd_backend_path_status,device_state,bckend_path);
	return xenstore_share_disk2(device,xd,vbd_backend_path_status,device_state,bckend_path);
}
int xenstore_share_disk2(char * device, struct xendrive_s *xd,char * vbd_backend_path_status,char * device_state,char * bckend_path){
	//++++++
	//dprintf(1,"DEBUG device %s backend path status %s share2\n",device,vbd_backend_path_status);
	char * state = malloc_high(6);
	//char * vbd_backend_0_path_status_value = xenstore_read(vbd_backend_path_status);
	//dprintf(1,"Back-end status:%s value:%s \n",vbd_backend_path_status,vbd_backend_0_path_status_value);
	itoa(XenbusStateInitialising,state);
	usleep(5000);
	char * xw_res = xenstore_write(device_state,state);
	dprintf(1,"Change state path %s value %s write response %s\n",device_state,state, xw_res);
	//move from ring to memory the back-end full path to state so we can verify it many times.
	//++++++
/*	usleep(5000);
	release(vbd_backend_0_path_status_value);
	vbd_backend_0_path_status_value = xenstore_read(vbd_backend_path_status);
	dprintf(1,"Back-end %s %s \n",vbd_backend_path_status,vbd_backend_0_path_status_value);
	release(vbd_backend_0_path_status_value);*/
	return xenstore_share_disk3(device,xd,vbd_backend_path_status,bckend_path, state,device_state);
}
int xenstore_share_disk3(char * device, struct xendrive_s *xd,char * vbd_backend_path_status,char * bckend_path,char * state,char *device_state){
	//get a new port for the each device
	struct evtchn_alloc_unbound * newport;
	newport = malloc_high(sizeof(struct evtchn_alloc_unbound));
	newport->dom = DOMID_SELF;
	newport->remote_dom = 0;
	int hret = hypercall_event_channel_op(EVTCHNOP_alloc_unbound,newport);
	xd->info.port = newport->port;
	// the new port is in the struct xendrive->info.port

	dprintf(1,"hypercall ret %d new port is %d\n",hret,newport->port);
	int temp_int = (int)newport->port;
	//dprintf(1,"Port has: %d digits\n",countDigits(temp_int));
	char * ring_port;
	ring_port = malloc_high(countDigits(temp_int)+1);
	//memset(ring_port,0,countDigits(temp_int)+1);

	//convert the new port number to a string
	itoa(temp_int,ring_port);
	//dprintf(1,"ring port string: %s. Device %p\n",ring_port,device);
	char * device_port = strconcat(device,"/port");
	//dprintf(1,"ring port: %s length: %d\n",ring_port, strlen(ring_port));
	//event-channel
	char * port_result_port = xenstore_write(device_port,ring_port);
	release(device_port);
	char * device_event_channel = strconcat(device,"/event-channel");

	//dprintf(1,"Port result %s .",port_result_port);

	release(port_result_port);
	char * port_result_channel = xenstore_write(device_event_channel,ring_port);
	release(device_event_channel);
	//dprintf(1,"Event-Channel result is %s \n",port_result_channel);
	release(ring_port);
	release(port_result_channel);
	return xenstore_share_disk4(device,xd,vbd_backend_path_status,bckend_path,state,device_state);
}
int xenstore_share_disk4(char * device, struct xendrive_s *xd,char * vbd_backend_path_status,char * bckend_path,char * state,char * device_state){
	//++++++
	//release(vbd_backend_0_path_status_value);
	//dprintf(1,"DEBUG xenstore read %s\n",vbd_backend_path_status);
	char * vbd_backend_0_path_status_value2 = xenstore_read(vbd_backend_path_status);
	dprintf(1,"Back-end %s returned %s \n",vbd_backend_path_status,vbd_backend_0_path_status_value2);


	char * ring_ref_path = strconcat(device,"/ring-ref");
	release(device);
	char *counts = malloc_high(6);
	itoa(xd->info.ring_ref,counts);
	//dprintf(1,"gref offset at this point %s original value %d\n",counts,xd->info.ring_ref);
	char * ring_ref_w_res = xenstore_write(ring_ref_path,counts);
	//dprintf(1,"ring ref path is %s result is %s\n",ring_ref_path,ring_ref_w_res);
	release(ring_ref_path);
	release(counts);
	release(ring_ref_w_res);
	return xenstore_share_disk5(device,xd,vbd_backend_path_status,bckend_path,state,device_state);
	//++++++
}
int xenstore_share_disk5(char * device, struct xendrive_s *xd,char * vbd_backend_path_status,char * bckend_path,char * state,char *device_state){
	dprintf(1,"DEBUG xenstore read %s\n",vbd_backend_path_status);
	char * vbd_backend_0_path_status_value3 = xenstore_read(vbd_backend_path_status);
	//dprintf(1,"DEBUG xenstore read %s response:%s\n",vbd_backend_path_status,vbd_backend_0_path_status_value3);
	//dprintf(1,"Back-end %s %s \n",vbd_backend_path_status,vbd_backend_0_path_status_value3);
	release(vbd_backend_0_path_status_value3);
	//must set state to XenbusStateInitialised
	//dprintf(1,"back end path is %s\n",bckend_path);
	itoa(XenbusStateInitialised,state);
	char * xw_res = xenstore_write(device_state,state);
	release(xw_res);
	return xenstore_share_disk6(device,xd,vbd_backend_path_status,bckend_path,state,device_state);

}
int xenstore_share_disk6(char * device, struct xendrive_s *xd,char * vbd_backend_path_status,char * bckend_path,char * state,char *device_state){
	usleep(150000);
	//++++++
	//dprintf(1,"DEBUG xenstore read %s\n",vbd_backend_path_status);
	char * vbd_backend_0_path_status_value4 = xenstore_read(vbd_backend_path_status);
	//dprintf(1,"DEBUG xenstore read %s response:%s\n",vbd_backend_path_status,vbd_backend_0_path_status_value4);
	int dom0_state = myatoi(vbd_backend_0_path_status_value4);
	release(vbd_backend_0_path_status_value4);
	char * vbd_backend_0_path_status_value5 = NULL;
	int i = 0;
	while(dom0_state!=4){
		usleep(1500000);
		//dprintf(1,"DEBUG xenstore read %s\n",vbd_backend_path_status);
		vbd_backend_0_path_status_value5 = xenstore_read(vbd_backend_path_status);
		//dprintf(1,"DEBUG xenstore read %s response:%s\n",vbd_backend_path_status,vbd_backend_0_path_status_value5);
		dom0_state = myatoi(vbd_backend_0_path_status_value5);
		release(vbd_backend_0_path_status_value5);
		dprintf(1,"Try %d for state change %d\n",i,dom0_state);
		i++;
	}
	release(vbd_backend_path_status);
	return xenstore_share_disk7(device,xd,bckend_path,state,dom0_state,device_state);
}


int xenstore_share_disk7(char * device, struct xendrive_s *xd,char * bckend_path,char * state,int dom0_state,char * device_state){
	if(dom0_state==4){
		itoa(XenbusStateConnected,state);
		char * last_res = xenstore_write(device_state,state);
		//dprintf(1,"Connected %s\n",last_res);
		//dprintf(1,"back end path is %s\n",bckend_path);
		char * backend_sectors = strconcat(bckend_path,"/sectors");
		//dprintf(1,"DEBUG xenstore read %s\n",backend_sectors);
		char * response = xenstore_read(backend_sectors);
		//dprintf(1,"DEBUG xenstore read %s response:%s\n",backend_sectors,response);
		release(backend_sectors);
		dprintf(1,"sectors %s\n",response);
		int tempa = myatoi(response);
		xd->drive.sectors = (u64) tempa;


		char * backend_sector_size = strconcat(bckend_path,"/sector-size");
		//dprintf(1,"DEBUG xenstore read %s\n",backend_sector_size);
		char * response2 = xenstore_read(backend_sector_size);
		//dprintf(1,"DEBUG xenstore read %s: response:%s\n",backend_sector_size,response2);
		dprintf(1,"sector-size %s\n",response2);
		tempa = myatoi(response2);
		xd->drive.blksize = (u16) tempa;

		xd->drive.lchs.cylinders = 255;
		xd->drive.lchs.heads = 255;
		xd->drive.lchs.spt = 63;
		release(response);
		release(response2);
		release(state);
		release(device_state);
		release(bckend_path);
		release(backend_sector_size);
		return 0;
	}else{
		itoa(XenbusStateInitialising,state);
		char * xw_res = xenstore_write(device_state,state);
		dprintf(1,"FAIL change state path %s value %s\n",device_state,state);
		dprintf(1,"XenbusStateInitialising %s\n",xw_res);
		release(state);
		release(xw_res);
		release(device_state);
		release(bckend_path);
		return 1;
	}
	return 2;
}

struct blkif_request * RING_GET_REQUEST_16BIT(struct blkif_front_ring * r,RING_IDX idx){
	struct blkif_request * ret;
	int i = (idx) & (RING_SIZE_16BIT(r) - 1);
	struct blkif_sring * temp = (struct blkif_sring *)GET_GLOBALFLAT((r)->sring);
	dprintf(1,"DEBUG inside get request sring at %p %d\n",temp,idx);
	union blkif_sring_entry  * temp2 = (temp->ring);
	dprintf(1,"DEBUG inside get request sring_entry at %p\n",temp2);
	ret = (&temp2[i].req);
	dprintf(1,"DEBUG ret inside get request sring_entry at %p\n",ret);
	return ret;
}

struct blkif_request * RING_GET_RESPONSE_16BIT(struct blkif_front_ring * r,RING_IDX idx){
	struct blkif_request * ret;
	int i = (idx) & (RING_SIZE_16BIT(r) - 1);
	struct blkif_sring * temp = (struct blkif_sring *)((r)->sring);
	dprintf(1,"DEBUG inside get response sring at %p idx %d\n",temp,idx);
	union blkif_sring_entry  * temp2 = (temp->ring);
	dprintf(1,"DEBUG inside get response sring_entry at %p\n",temp2);
	ret = (&temp2[i].rsp);
	dprintf(1,"DEBUG ret inside get response sring_entry at %p\n",ret);
	return ret;
}

void RING_PUSH_REQUESTS_AND_CHECK_NOTIFY_16BIT(struct blkif_front_ring * r, int notify){
		struct blkif_sring * temp = (struct blkif_sring *)((r)->sring);
		dprintf(1,"DEBUG inside push request notify sring at %p\n",temp);
		RING_IDX old = (temp->req_prod);
		RING_IDX new = ((r)->req_prod_pvt);
		dprintf(1,"DEBUG inside push request notify old idx %d new %d\n",old,new);
		barrier(); /* back sees requests /before/ updated producer index */
		temp->req_prod = new;
		barrier(); /* back sees new requests /before/ we check req_event */
		(notify) = ((RING_IDX)(new - (temp->req_event)) < (RING_IDX)(new - old));
		dprintf(1,"Notify result %d\n",notify);
}

int VISIBLE32INIT
xen_blk_op_write(struct disk_op_s *op){
	dprintf(1,"Xen Disk  buffer %x lba %x count %d command %d\n",op->buf_fl,op->lba,op->count,op->command);
	int notify=1;
	int i;
	struct xendrive_s * xendrive = NULL;
	for(i=0;i<MAX_DRIVES;i++){
		dprintf(1,"Looking for drive #%d %p cmp %p\n",i,(u16)GET_GLOBAL(drive_ptrs[i]),op->drive_g);
		if(((u16)GET_GLOBAL(drive_ptrs[i]))==op->drive_g){
			xendrive = (xendrives_ptrs[i]);
		}
	}
	if(xendrive==NULL)
		return -1;
	//struct xendrive_s * xendrive = GLOBALFLAT2GLOBAL(container_of(GET_GLOBAL(op->drive_g), struct xendrive_s, drive)); //the global struct is extracted
	dprintf(1,"Xendrive at:%p\n",xendrive);

	struct blkfront_info * bi = &xendrive->info; //kernel specific info
	//dprintf(1,"blkfront_info:%p shared_ring:%p private_ring:%p\n",bi,GET_GLOBALFLAT(bi->shared),GET_GLOBALFLAT(bi->private));

	/*
	 * BUFFER: disk_op_s->buf_fl
	 * Must encapsulate the disk_op_s->buf_fl in blkif_request.id
	 */
	struct blkif_response *ring_res;
	struct blkif_request *ring_req;

	ring_req = RING_GET_REQUEST((bi->private),(bi->private->req_prod_pvt));
	ring_req->id = (u64)111;
	ring_req->operation = (u8)BLKIF_OP_WRITE;
	ring_req->sector_number = (u64)op->lba;
	ring_req->nr_segments= (u8)op->count;
	//if more than one should iterate to fill all of them
	ring_req->seg[0].gref = bi->buffer_gref; //the gref from the buffer
	ring_req->seg[0].first_sect = (u8)0;//)op->lba;
	ring_req->seg[0].last_sect = (u8)7;//op->lba + op->count;
	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY((bi->private),notify);
	if(notify){
		evtchn_send_t send;
		send.port = bi->port;
		hypercall_event_channel_op(EVTCHNOP_send, &send);
	}
	usleep(5);
	ring_res = RING_GET_RESPONSE((bi->private),(bi->shared->rsp_prod++));
	if(notify){
		evtchn_send_t send;
		send.port = bi->port;
		hypercall_event_channel_op(EVTCHNOP_send, &send);
	}
	ring_res = RING_GET_RESPONSE((bi->private),(bi->private->sring->rsp_prod++));
	if(ring_res->id==111 && ring_res->operation ==BLKIF_OP_WRITE){
		memcpy(bi->buffer,op->buf_fl,op->count*(BYTES_PER_SECTOR));
	}

	if(ring_res->status==BLKIF_RSP_OKAY)
		return 0;
	else
		return -1;
}


int VISIBLE32INIT
xen_blk_op_read(struct disk_op_s *op){
	int notify;
	struct xendrive_s * xendrive;
	int i;
	for(i=0;i<4;i++){
		dprintf(1,"%d %p\n",i,drive_ptrs[i]);
		if((u16)drive_ptrs[i]==op->drive_g){
			xendrive = xendrives_ptrs[i];
		}
	}
	struct blkfront_info * bi = &xendrive->info; //kernel specific info
	struct blkif_response *ring_res;
	struct blkif_request *ring_req;
	struct  blkif_front_ring * priv = (bi->private);
	ring_req = RING_GET_REQUEST((priv),(priv->req_prod_pvt));
	ring_req->id = 8;

	//ring_req->nr_segments=op->count;
	ring_req->nr_segments=1;
	ring_req->operation = BLKIF_OP_READ;
	ring_req->sector_number = (u64)op->lba; //sector to be read
	ring_req->seg[0].gref = (bi->buffer_gref); //this should be get_free_gref();
	ring_req->seg[0].first_sect = 0;//op->lba;
	ring_req->seg[0].last_sect = 7;//op->lba + op->count;
	dprintf(1,"SIZEOF ring_req:%d handle:%d id:%d nr_segments:%d operation:%d sector_number:%d seg:%d\n",\
			sizeof(struct blkif_request),sizeof(ring_req->handle),sizeof(ring_req->id),sizeof(ring_req->nr_segments),\
			sizeof(ring_req->operation),sizeof(ring_req->sector_number),sizeof(ring_req->seg));
	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY((priv),notify);
	if(notify){
		evtchn_send_t send;
		send.port = (bi->port);
		hypercall_event_channel_op(EVTCHNOP_send, &send);
	}
	ring_res = RING_GET_RESPONSE((bi->private),(bi->shared->rsp_prod++));
	dprintf(1,"SIZEOF ring_res:%d id:%d operation:%d status:%d\n",sizeof(struct blkif_response),sizeof(ring_res->id),sizeof(ring_res->operation),sizeof(ring_res->status));
	dprintf(1,"After RING RESPONSE %p\n",ring_res);
	dprintf(1,"status:%d\n",ring_res->status);
	dprintf(1,"operation %d\n",ring_res->operation);
	dprintf(1,"id:%d\n",ring_res->id);
	if(ring_res->id==(u8)8){
		dprintf(1,"Same id in req response, proceed to copy resp %d\n",memcpy(op->buf_fl,bi->buffer,op->count*(BYTES_PER_SECTOR)));
	}else{
		dprintf(1,"FAIL RING RESPONSE %p id:%d status:%d operation %d\n", ring_res,ring_res->id,ring_res->status,ring_res->operation);
	}

	if(ring_res->status == BLKIF_RSP_EOPNOTSUPP){
		dprintf(1,"After RING RESPONSE BLKIF_RSP_EOPNOTSUPP %p id:%d status:%d operation %d\n", ring_res,ring_res->id,ring_res->status,ring_res->operation);

	}
	else if(ring_res->status == BLKIF_RSP_ERROR){
		dprintf(1,"After RING RESPONSE BLKIF_RSP_ERROR %p id:%d status:%d operation %d\n", ring_res,ring_res->id,ring_res->status,ring_res->operation);

	}
	else if(ring_res->status==BLKIF_RSP_OKAY){
		dprintf(1,"After RING RESPONSE BLKIF_RSP_OKAY %p id:%d status:%d operation %d\n", ring_res,ring_res->id,ring_res->status,ring_res->operation);
		dprintf(1,"Read operation Succeed id:%d op:%d status:%d\n",ring_res->id,ring_res->operation,ring_res->status);

		dprintf(1,"Read operation %p id:%u op:%u status:%u op:%d st:%d\n",ring_res,ring_res->id,ring_res->operation,ring_res->status,ring_res->operation,ring_res->status);

		dprintf(1,"After RING RESPONSE %p\n",ring_res);
		dprintf(1,"%p status:%d\n",&(ring_res->status),ring_res->status);
		dprintf(1,"%p operation %d\n",&(ring_res->operation),ring_res->operation);
		dprintf(1,"%p id:%d\n",&(ring_res->id),ring_res->id);
		return 0;

	}
	else{
		dprintf(1,"Read operation failed id:%d op:%d status:%d\n",ring_res->id,ring_res->operation,ring_res->status);
		return -1;
	}
	return -1;
}

