#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <stdbool.h>
#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/moduleparam.h>
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function
#include <linux/slab.h>

#define DEVICE_NAME_0 "trans0" ///< The device will appear at /dev/trans0 using this value
#define DEVICE_NAME_1 "trans1" ///< The device will appear at /dev/trans1 using this value
#define CLASS_NAME_0 "tran0" ///< The device class -- this is a character device driver
#define CLASS_NAME_1 "tran1" ///< The device class -- this is a character device driver
#define ALPHABET_LENGTH 53

MODULE_LICENSE("GPL"); ///< The license type -- this affects available functionality
MODULE_AUTHOR("Tobians Ranft, Philipp Schwarz"); ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION(""); ///< The description -- see modinfo
MODULE_VERSION("0.1"); ///< A version number to inform users
static int TRANSLATE_BUFSIZE = 40;
module_param(TRANSLATE_BUFSIZE, int, 0660);
static int TRANSLATE_SHIFT = 3;
module_param(TRANSLATE_SHIFT, int, 0660);
static DEFINE_MUTEX(trans0_mutex);
static DEFINE_MUTEX(trans1_mutex);
static int majorNumber; ///< Stores the device number -- determined automatically
static char *message0; ///< Memory for the string that is passed from userspace
static char *message1;
static int readPtr0;
static int readPtr1;
static int writePtr0;
static int writePtr1;
static int write0Count;
static int read0Count;
static int write1Count;
static int read1Count;

wait_queue_head_t queue_write1;
wait_queue_head_t queue_read1;
wait_queue_head_t queue_write0;
wait_queue_head_t queue_read0;
 
static short size_of_message0;
static short size_of_message1; ///< Used to remember the size of the string stored
static struct class * trans0Class = NULL; ///< The device-driver class struct pointer
static struct device * trans0Device = NULL; ///< The device-driver device struct pointer
static struct class * trans1Class = NULL; ///< The device-driver class struct pointer
static struct device * trans1Device = NULL; ///< The device-driver device struct pointer
static char * alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";
// The prototype functions for the character driver -- must come before the struct definition
static int dev_open_trans(struct inode * , struct file * );
static int dev_release_trans(struct inode * , struct file * );
static ssize_t dev_read_trans(struct file * , char * , size_t, loff_t * );
static ssize_t dev_write_trans(struct file * ,
const char * , size_t, loff_t * );
void encodeMsg(char * msg, int position);
void decodeMsg(char * msg, int position);
int k = 0;

/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops_0 = {
    .open = dev_open_trans,
    .read = dev_read_trans,
    .write = dev_write_trans,
    .release = dev_release_trans,
};

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init trans_init(void) {
    int clearIndex;
    write0Count = 0;
    read0Count = 0;
    write1Count = 0;
    read1Count = 0;
    readPtr0 = 0;
    writePtr0 = 0;
    readPtr1 = 0;
    writePtr1 = 0;
    size_of_message0 = 0;
    size_of_message1 = 0;
    
    printk(KERN_INFO "init called");
    printk(KERN_INFO "TRANSLATE_BUFSIZE = %d, TRANSLATE_SHIFT = %d", TRANSLATE_BUFSIZE, TRANSLATE_SHIFT);
    printk(KERN_INFO "trans0: Initializing the trans0 LKM\n");
    // Try to dynamically allocate a major number for the device -- more difficult but worth it
    majorNumber = register_chrdev(0, DEVICE_NAME_0, & fops_0);
    if (majorNumber < 0) {
        printk(KERN_ALERT "trans0 failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "trans0: registered correctly with major number %d\n", majorNumber);

    // Register the device class
    trans0Class = class_create(THIS_MODULE, CLASS_NAME_0);
    if (IS_ERR(trans0Class)) { // Check for error and clean up if there is
        unregister_chrdev(majorNumber, DEVICE_NAME_0);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(trans0Class); // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "trans0: device class registered correctly\n");

    // Register the device driver
    trans0Device = device_create(trans0Class, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME_0);
    message0 = (char * ) kmalloc(TRANSLATE_BUFSIZE * sizeof(char), GFP_KERNEL);
    init_waitqueue_head(&queue_read0);
    init_waitqueue_head(&queue_write0);
    if (IS_ERR(trans0Device)) { // Clean up if there is an error
        class_destroy(trans0Class); // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, DEVICE_NAME_0);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(trans0Device);
    }
    printk(KERN_INFO "trans0: device class created correctly\n"); // Made it! device was initialized
    mutex_init( & trans0_mutex);
    ////////////// trans1 /////////////

    printk(KERN_INFO "trans1: Initializing the trans0 LKM\n");

    // Register the device class
    trans1Class = class_create(THIS_MODULE, CLASS_NAME_1);
    if (IS_ERR(trans1Class)) { // Check for error and clean up if there is
        unregister_chrdev(majorNumber, DEVICE_NAME_1);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(trans1Class); // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "trans1: device class registered correctly\n");

    // Register the device driver
    trans1Device = device_create(trans1Class, NULL, MKDEV(majorNumber, 1), NULL, DEVICE_NAME_1);
    message1 = (char * ) kmalloc(TRANSLATE_BUFSIZE * sizeof(char), GFP_KERNEL);
    init_waitqueue_head(&queue_read1);
    init_waitqueue_head(&queue_write1);
    for (clearIndex = 0; clearIndex < TRANSLATE_BUFSIZE; clearIndex++) {
        message0[clearIndex] = '\0';
        message1[clearIndex] = '\0';
    }
    if (IS_ERR(trans1Device)) { // Clean up if there is an error
        class_destroy(trans1Class); // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, DEVICE_NAME_1);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(trans1Device);
    }
    printk(KERN_INFO "trans1: device class created correctly\n"); // Made it! device was initialized
    mutex_init( & trans1_mutex);
    return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit trans_exit(void) {
    printk(KERN_INFO "exit called");
    mutex_destroy( & trans0_mutex); /// destroy the dynamically-allocated mutex
    device_destroy(trans0Class, MKDEV(majorNumber, 0)); // remove the device
    class_unregister(trans0Class); // unregister the device class
    class_destroy(trans0Class); // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME_0); // unregister the major number
    kfree(message0);
    printk(KERN_INFO "trans0: Goodbye from the LKM!\n");

    mutex_destroy( & trans1_mutex); /// destroy the dynamically-allocated mutex
    device_destroy(trans1Class, MKDEV(majorNumber, 1)); // remove the device
    class_unregister(trans1Class); // unregister the device class
    class_destroy(trans1Class); // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME_1); // unregister the major number
    kfree(message1);
    printk(KERN_INFO "trans1: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open_trans(struct inode * inodep, struct file * filep) {
    int minorAct;
    dev_t device = inodep -> i_rdev;
    minorAct = MINOR(device);
    printk(KERN_INFO "open called, flag is %x", filep->f_flags);
    switch (minorAct) {
        case 0:
            printk(KERN_INFO "case was 0");
            if((filep->f_flags & O_ACCMODE) == O_RDONLY) {
                printk(KERN_INFO "RONLY flag true");
                if(read0Count == 0){
                    //read allowed
                    read0Count++;
                    printk(KERN_INFO "trans0: Device opend, read0Count = %d\n", read0Count);
                    return 0;
                }else{
                    //read not allowed
                    printk(KERN_ALERT "Device is already in reading mode");
                    return -EBUSY;
                }
            }
            if((filep->f_flags & O_ACCMODE) == O_WRONLY) {
                printk(KERN_INFO "WRONLY flag true");
                if(write0Count == 0){
                    //write allowed
                    write0Count++;
                    printk(KERN_INFO "trans0: Device opend, write0Count = %d\n", write0Count);
                    return 0;
                }else{
                    //write not allowed
                    printk(KERN_ALERT "Device is already in writing mode");
                    return -EBUSY;
                }
            }
            break;
    case 1:
        printk(KERN_INFO "case was 1");
        if((filep->f_flags & O_ACCMODE) == O_RDONLY) {
            printk(KERN_INFO "RONLY flag true");
            if(read1Count == 0){
                //read allowed
                read1Count++;
                printk(KERN_INFO "trans0: Device opend, read1Count = %d\n", read1Count);
                return 0;
            }else{
                //read not allowed
                printk(KERN_ALERT "Device is already in reading mode");
                return -EBUSY;
            }
        }
        if((filep->f_flags & O_ACCMODE) == O_WRONLY) {
            printk(KERN_INFO "WRONLY flag true");
            if(write1Count == 0){
                //write allowed
                write1Count++;
                printk(KERN_INFO "trans0: Device opend, write1Count = %d\n", write1Count);
                return 0;
            }else{
                //write not allowed
                printk(KERN_ALERT "Device is already in writing mode");
                return -EBUSY;
            }
        }
        break;
    default:
        printk(KERN_INFO "couldnt address the minor number of a device it was %d in default case!!", minorAct);
        return -1;
    }
    printk(KERN_INFO "didnt went in any case of switch statement of read");
    return -1;

}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read_trans(struct file * filep, char * buffer, size_t len, loff_t * offset) {
    int error_trans0;
    int error_trans1;
    int index_trans0;
    int index_trans1;
    int retVal0;
    int retVal1;
    int clearIndex;
    dev_t device;
    int minorAct;

    int sizeToSend0;
    int sizeToSend1;
    char * toSend0;
    char * toSend1;
    
    printk(KERN_INFO "read called");

    device = filep -> f_inode -> i_rdev;
    minorAct = MINOR(device);
    index_trans0 = 0;
    index_trans1 = 0;

    sizeToSend1 = 0;
    sizeToSend0 = 0;
    switch (minorAct) {
    case 0:
        toSend0 = (char * ) kmalloc(TRANSLATE_BUFSIZE * sizeof(char), GFP_USER);
        for (clearIndex = 0; clearIndex < TRANSLATE_BUFSIZE; clearIndex++) {
            toSend0[clearIndex] = '\0';
        }
        error_trans0 = 0;
        printk(KERN_INFO "read trans0 called!");
        // copy_to_user has the format ( * to, *from, size) and returns 0 on success
        wait_event_interruptible(queue_read0, writePtr0 != readPtr0);
        sizeToSend0 = 0;
        while (readPtr0 != writePtr0) {
            printk(KERN_INFO "aktueller writePtr = %d, aktueller ReadPtr = %d indexToCopyIn is %d, size of Copy is %d", writePtr0, readPtr0, index_trans0, sizeToSend0);
            if (message0[readPtr0] != '\n') {
                toSend0[index_trans0] = message0[readPtr0];
                printk(KERN_INFO "put character %c from buffer to toSend \n index of toSend is %d, index of buffer is %d", message0[readPtr0], index_trans0, readPtr0);
                sizeToSend0++;
                index_trans0++;
                printk(KERN_INFO "toSend : %s ", toSend0);

            }
            readPtr0 = (readPtr0 + 1) % TRANSLATE_BUFSIZE;
        }
        if(readPtr0 == writePtr0){
                //Last char to read
            toSend0[index_trans0] = message0[readPtr0];
            sizeToSend0++;
        }
        error_trans0 = copy_to_user(buffer, toSend0, sizeToSend0);
        wake_up_interruptible(&queue_write0);
        retVal0 = size_of_message0;
        size_of_message0 = 0;
        kfree(toSend0);
        return (sizeToSend0); // clear the position to the start and return 0
    case 1:
        toSend1 = (char * ) kmalloc(TRANSLATE_BUFSIZE * sizeof(char), GFP_USER);
        for (clearIndex = 0; clearIndex < TRANSLATE_BUFSIZE; clearIndex++) {
            toSend1[clearIndex] = '\0';
        }
        error_trans1 = 0;
        printk(KERN_INFO "read trans1 called!");
        // copy_to_user has the format ( * to, *from, size) and returns 0 on success
        wait_event_interruptible(queue_read1, writePtr1 != readPtr1);
        sizeToSend1 = 0;
        while (readPtr1 != writePtr1) {

            printk(KERN_INFO "aktueller writePtr = %d, aktueller ReadPtr = %d indexToCopyIn is %d, size of Copy is %d", writePtr1, readPtr1, index_trans1, sizeToSend1);
            if (message1[readPtr1] != '\n') {
                toSend1[index_trans1] = message1[readPtr1];
                printk(KERN_INFO "put character %c from buffer to toSend \n index of toSend is %d, index of buffer is %d", message1[readPtr1], index_trans1, readPtr1);
                sizeToSend1++;
                index_trans1++;
                printk(KERN_INFO "toSend : %s ", toSend1);
            }
            readPtr1 = (readPtr1 + 1) % TRANSLATE_BUFSIZE;
        }
        if(readPtr1 == writePtr1){
                //Last char to read
            toSend0[index_trans1] = message0[readPtr1];
            sizeToSend1++;
        }
        error_trans1 = copy_to_user(buffer, toSend1, sizeToSend1);
        wake_up_interruptible(&queue_write1);
        retVal1 = size_of_message1;
        size_of_message1 = 0;
        kfree(toSend1);
        return (sizeToSend1); // clear the position to the start and return 0
    default:
        printk(KERN_ALERT "Minor number unknown!");
        return -1;
    }
}

/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write_trans(struct file * filep, const char * buffer, size_t len, loff_t * offset) {
    dev_t device;
    int minorAct;
    int error_decode;
    int error_encode;
    int copyIndex0;
    int copyIndex1;
    printk(KERN_INFO "write called");

    device = filep -> f_inode -> i_rdev;
    minorAct = MINOR(device);
    copyIndex0 = 0;
    copyIndex1 = 0;
    error_decode = 0;
    error_encode = 0;
    switch (minorAct) {
    case 0:
        wait_event_interruptible(queue_write0, ((writePtr0 + 1) % TRANSLATE_BUFSIZE) != readPtr0);
        
        while (((writePtr0 + 1) % TRANSLATE_BUFSIZE != readPtr0)) {
            printk(KERN_INFO "writePtr0 = %d, readPtr0 = %d", writePtr0, readPtr0);
            error_encode = copy_from_user( &message0[writePtr0], &buffer[copyIndex0], 1);
            copyIndex0++;
            encodeMsg(message0, writePtr0);
            writePtr0 = (writePtr0 + 1) % TRANSLATE_BUFSIZE;
            if (error_encode == 0) {
                printk(KERN_INFO "trans0 copied from user");
                wake_up_interruptible(&queue_read0);
            } else {
                printk(KERN_INFO "copy from user by trans0 didnt work!");
            }
        }
        if((writePtr0 + 1) % TRANSLATE_BUFSIZE == readPtr0){
            printk(KERN_INFO "went in last write in if after while");
            error_encode = copy_from_user( &message0[writePtr0], &buffer[copyIndex0], 1);
            if (error_encode == 0) {
                printk(KERN_INFO "trans0 copied from user");
                wake_up_interruptible(&queue_read0);
            } else {
                printk(KERN_INFO "copy from user by trans0 didnt work!");
            }
            encodeMsg(message0, writePtr0);
            wake_up_interruptible(&queue_read0);
        }
       
        size_of_message0 = len; // store the length of the stored message
        printk(KERN_INFO "aktuelle Size of message is: %d", size_of_message0);
        printk(KERN_INFO "trans0: Received %lu zu characters from the user\n", len - 1);
        return len;
    case 1:
        wait_event_interruptible(queue_write1, ((writePtr1 + 1) % TRANSLATE_BUFSIZE) != readPtr1);
        
        while (((writePtr1 + 1) % TRANSLATE_BUFSIZE != readPtr1)) {
            printk(KERN_INFO "writePtr1 = %d, readPtr1 = %d", writePtr1, readPtr1);
            error_decode = copy_from_user( & message1[writePtr1], & buffer[copyIndex1], 1);
            copyIndex1++;
            decodeMsg(message1, writePtr1);
            
            writePtr1 = (writePtr1 + 1) % TRANSLATE_BUFSIZE;
            if (error_encode == 0) {
                printk(KERN_INFO "trans1 copied from user");
                wake_up_interruptible(&queue_read1);
            } else {
                printk(KERN_INFO "copy from user by trans1 didnt work!");
            }
        }
        if((writePtr1 + 1) % TRANSLATE_BUFSIZE == readPtr1){
            printk(KERN_INFO "went in last write in if after while");
            error_encode = copy_from_user( &message1[writePtr1], &buffer[copyIndex1], 1);
        if (error_encode == 1) {
            printk(KERN_INFO "trans1 copied from user");
            wake_up_interruptible(&queue_read0);
        } else {
            printk(KERN_INFO "copy from user by trans1 didnt work!");
        }
        encodeMsg(message1, writePtr1);
        wake_up_interruptible(&queue_read1);
        }
        

        size_of_message1 = len; // store the length of the stored message
        printk(KERN_INFO "aktuelle Size of message is: %d", size_of_message1);
        printk(KERN_INFO "trans0: Received %lu zu characters from the user\n", len - 1);
        return len;
    default:
        printk(KERN_ALERT "Minor number unknown!");
        return -1;
    }
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release_trans(struct inode * inodep, struct file * filep) {
    dev_t device = inodep -> i_rdev;
    int minorAct = MINOR(device);
    printk(KERN_INFO "release called");
    printk(KERN_INFO "Positions -- writePtr0 = %d  --  readPtr0 = %d ", writePtr0, readPtr0);

    switch (minorAct) {
    case 0:
        if((filep->f_flags & O_ACCMODE) == O_RDONLY) {
                read0Count--;
                printk(KERN_INFO "trans0: Device closed, read0Count = %d\n", read0Count);
        }
        if((filep->f_flags & O_ACCMODE) == O_WRONLY) {
                write0Count--;
                printk(KERN_INFO "trans0: Device closed, write0Count = %d\n", write0Count);
        }
        printk(KERN_INFO "trans: Device successfully closed\n");
        
        return 0;
    case 1:
        if((filep->f_flags & O_ACCMODE) == O_RDONLY) {
                read1Count--;
                printk(KERN_INFO "trans0: Device closed, read1Count = %d\n", read1Count);
        }
        if((filep->f_flags & O_ACCMODE) == O_WRONLY) {
                write1Count--;
                printk(KERN_INFO "trans0: Device closed, write1Count = %d\n", write1Count);
        }
        printk(KERN_INFO "trans: Device successfully closed\n");
        
        return 0;
    default:
        printk(KERN_INFO "couldnt adress the minor number of a device");
        return -1;
    }
}

void encodeMsg(char * msg, int position) {
    int j;
    int codeIndex;
    char currentChar = msg[position];
    for (j = 0; j < ALPHABET_LENGTH; j++) {
        if (currentChar == alphabet[j]) {
            codeIndex = j + TRANSLATE_SHIFT;
            if (codeIndex > ALPHABET_LENGTH - 1) {
                codeIndex -= ALPHABET_LENGTH;
            }
            msg[position] = alphabet[codeIndex];
        }
    }
}

void decodeMsg(char * msg, int position) {
    int j;
    char currentChar = msg[position];
    int codeIndex;
    for (j = 0; j < ALPHABET_LENGTH; j++) {
        if (currentChar == alphabet[j]) {
            codeIndex = j - TRANSLATE_SHIFT;
            if (codeIndex < 0) {
                codeIndex += ALPHABET_LENGTH;
            }
            msg[position] = alphabet[codeIndex];
        }
    }
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(trans_init);
module_exit(trans_exit);
