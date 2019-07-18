/**
 * @file vmaccess.c
 * @author Prof. Dr. Wolfgang Fohl, HAW Hamburg
 * @date 2010
 * @brief The access functions to virtual memory.
 */

#include "vmaccess.h"
#include <sys/ipc.h>
#include <sys/shm.h>

#include "syncdataexchange.h"
#include "vmem.h"
#include "debug.h"

/*
 * static variables
 */

static struct vmem_struct *vmem = NULL; //!< Reference to virtual memory

/**
 * The progression of time is simulated by the counter g_count, which is incremented by 
 * vmaccess on each memory access. The memory manager will be informed by a command, whenever 
 * a fixed period of time has passed. Hence the memory manager must be informed, whenever 
 * g_count % TIME_WINDOW == 0. 
 * Based on this information, memory manager will update aging information
 */

static int g_count = 0;    //!< global acces counter as quasi-timestamp - will be increment by each memory access
#define TIME_WINDOW   20

/**
 *****************************************************************************************
 *  @brief      This function setup the connection to virtual memory.
 *              The virtual memory has to be created by mmanage.c module.
 *
 *  @return     void
 ****************************************************************************************/
static void vmem_init(void) {
    const int shm_access = 0664; // access-bits user: read/write, group: read/write, other: read
    const int shmflg = 0; // unused

    g_count = 0; // initialize the access counter pseudo-timestamp

    /* Create System V shared memory */
    key_t sh_key = ftok(SHMKEY, SHMPROCID); // generate shared memory key with ftok function
    /* We are only using the shm, don't set the IPC_CREAT flag */
    int shm = shmget(sh_key, SHMSIZE, shm_access); // get the shm via the sh_key and the specifized size of our virtual memory
    TEST_AND_EXIT_ERRNO(shm == -1, "Getting SHM failed.")
    /* attach shared memory to vmem */
    vmem = shmat(shm, NULL, shmflg); // attach SHM to vmem without specfying adress of shm, which is why shmflg is unused
    TEST_AND_EXIT_ERRNO(vmem == NULL, "Failed to attach SHM to VMEM.")
}

/**
 *****************************************************************************************
 *  @brief      This function puts a page into memory (if required). Ref Bit of page table
 *              entry will be updated.
 *              If the time window handle by g_count has reached, the window window message
 *              will be send to the memory manager. 
 *              To keep conform with this log files, g_count must be increased before 
 *              the time window will be checked.
 *              vmem_read and vmem_write call this function.
 *
 *  @param      address The page that stores the contents of this address will be 
 *              put in (if required).
 * 
 *  @return     void
 ****************************************************************************************/
static void vmem_put_page_into_mem(int address) {
    int page_id = address / VMEM_PAGESIZE; // calculate the id of the page
    TEST_AND_EXIT_ERRNO((page_id < 0 || VMEM_NPAGES < page_id), "Page index is out of bounds.")

    struct msg message;
    message.cmd = CMD_PAGEFAULT; // command, that says to store the page with the id of the value
    message.value = page_id; // page id
    message.g_count = g_count; // save current g_count

    sendMsgToMmanager(message);
}

int vmem_read(int address) {
    // initialize vmem if it is not initialized
    if(vmem == NULL) {
        vmem_init();
    }

    int page_id = address / VMEM_PAGESIZE; // calculate the id of the page
    // write the page into memory if it is not in the pagefile
    if(!(vmem->pt[page_id].flags & PTF_PRESENT)) {
        vmem_put_page_into_mem(address);
    }

    g_count++;
    vmem->pt[page_id].flags |= PTF_REF; // set referenced bit

    // calculate the physical address of the data
    int pageframe_address = (vmem->pt[page_id].frame * VMEM_PAGESIZE);
    int offset = address % VMEM_PAGESIZE;
    int phy_addr = pageframe_address + offset;

    // create and send message that timer interval expired
    if(g_count % TIME_WINDOW == 0) {
        struct msg message;
        message.cmd = CMD_TIME_INTER_VAL;
        message.g_count = g_count;
        sendMsgToMmanager(message);
    }
    return vmem->mainMemory[phy_addr];
}

void vmem_write(int address, int data) {
    // initialize vmem if it is not initialized
    if(vmem == NULL) {
        vmem_init();
    }

    int page_id = address / VMEM_PAGESIZE; // calculate the id of the page
    // write the page into memory if it is not in the pagefile
    if(!(vmem->pt[page_id].flags & PTF_PRESENT)) {
        vmem_put_page_into_mem(address);
    }

    g_count++;
    vmem->pt[page_id].flags |= (PTF_REF | PTF_DIRTY); // set referenced and dirty bit

    // calculate the physical address of the data
    int pageframe_address = (vmem->pt[page_id].frame * VMEM_PAGESIZE);
    int offset = address % VMEM_PAGESIZE;
    int phy_addr = pageframe_address + offset;

    // create and send message that timer interval expired
    if(g_count % TIME_WINDOW == 0) {
        struct msg message;
        message.cmd = CMD_TIME_INTER_VAL;
        message.g_count = g_count;
        sendMsgToMmanager(message);
    }

    vmem->mainMemory[phy_addr] = data; // write data to memory
}
// EOF
