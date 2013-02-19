#ifndef _XEN_XS_H
#define _XEN_XS_H

void reverse(char s[]);
int myatoi(const char *string);
void itoa(int n, char s[]);
void itoa2(int n, char *s);
void u32toa(u32 n, char s[]);

char * build_write_query(char * a,char *b);
char * add_string(char * a,char *b);
char * strconcat( char *dest, const char *src);
void xenbus_setup(void);
char * xenstore_read(char *path);
char * xenstore_write(char *path, char *value);
char * xenstore_directory(char *path, u32 *ans_len);
char* xenstore_watch(char *path, char * key);
void test_xenstore(void);
void wait_xenstore(void);
void release(char * string);
int countDigits(int i);
 /*
 * Xenbus protocol details.
 *
 * Copyright (C) 2005 XenSource Ltd.
 */
enum xenbus_state {
    XenbusStateUnknown       = 0,
    XenbusStateInitialising  = 1,
    /*
     * InitWait: Finished early initialisation but waiting for information
     * from the peer or hotplug scripts.
     */
    XenbusStateInitWait      = 2,
    /*
     * Initialised: Waiting for a connection from the peer.
     */
    XenbusStateInitialised   = 3,
    XenbusStateConnected     = 4,
    /*
     * Closing: The device is being closed due to an error or an unplug event.
     */
    XenbusStateClosing       = 5,
    XenbusStateClosed        = 6,
    /*
     * Reconfiguring: The device is being reconfigured.
     */
    XenbusStateReconfiguring = 7,
    XenbusStateReconfigured  = 8
};
typedef enum xenbus_state XenbusState;


#endif
