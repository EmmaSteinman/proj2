# Project 2 Design Document

> Denison cs372  
> Fall 2018

## GROUP

> Fill in the name and email addresses of your group members.

- Quan Tran <tran_q1@denison.edu>
- Brett Weicht <weicht_b1@denison.edu>
- Emma Steinman <steinm_e1@denison.edu>

## PRELIMINARIES

> If you have any preliminary comments on your submission, notes for the
> Instructor, or extra credit, please give them here.

> Please cite any offline or online sources you consulted while
> preparing your submission, other than the Pintos documentation, course
> text, lecture notes, and course staff.

## PROJECT PARTS

> In the documentation that you provide in the following, you need to provide clear, organized, and coherent documentation, not just short, incomplete, or vague "answers to the questions".  The questions are provided to help you structure your thinking around the information you need to convey.

### A. ARGUMENT PASSING  

#### DATA STRUCTURES

> A1: Copy here the declaration of each new or changed `struct` or
> `struct` member, `global` or `static` variable, `typedef`, or
> enumeration.  Document the purpose of each in 25 words or less.

  * We did not declare any new variables or structs for argument passing.

#### ALGORITHMS

> A2: Briefly describe how you implemented argument parsing.  How do
> you arrange for the elements of argv[] to be in the right order?
> How do you avoid overflowing the stack page?

  * First we parse the arguments from the parameter into an array. We pass this into our function which then sets up the stack. It pushes each argument onto the stack in the order in which it is in the array. Then we add null bits to ensure word alignment. We then iterate through the argument array once more but pushing the address of the argument instead of the argument itself. Since we are putting pointers to the arguments, the order of the arguments themselves are not important, as long as the addresses and arguments are in the same order. We avoid overflowing the stack by limiting the number of arguments to what fits into a single page. This is achieved by initializing the size of our array to the size of a page divided by the size of a pointer.

#### RATIONALE/JUSTIFICATION

> A3: Why does Pintos implement strtok_r() but not strtok()?

  * `strtok_r()` takes an extra parameter that allows you to save your position in the string, allowing it to be called on the same string by two threads or processes.

> A4: In Pintos, the kernel separates commands into a executable name
> and arguments.  In Unix-like systems, the shell does this
> separation.  Identify at least two advantages of the Unix approach.

  * One advantage to the Unix approach is that the shell is part of user mode, which is not privileged. Thus, there is a smaller chance the executable or arguments could overwrite something important or access invalid memory. Since it is not privileged, in the case an invalid argument is passed in, the shell will catch it before going into the kernel, eliminating some error handling in the kernel.
  * Another advantage is that it creates an abstraction for the user, allowing them to make system calls and access the file system without explicitly knowing the code or how it is working.  

### SYSTEM CALLS

#### DATA STRUCTURES

> B1: Copy here the declaration of each new or changed `struct` or
> `struct` member, `global` or `static` variable, `typedef`, or
> enumeration.  Document the purpose of each in 25 words or less.

* New `struct process`
```
struct process {
  pid_t pid;
  pid_t parent_pid;
  struct list child_list;
  struct list_elem procelem;
  struct lock child_lock;
};
```
   * Holds a process' pid, parent's pid, list of all children, allowing us to maintain relationships between parent and child

* New `struct child`
```
struct child {
  pid_t pid;
  pid_t parent_pid;
  int exit_status;
  struct semaphore child_sema;
  struct list_elem childelem;
  struct semaphore load_done_sema;
  bool load_success;
};
```
   * A struct for a child process, holding pid, parent_pid, exit status, and two semaphores, allowing us to check exit status and load status and use appropriate synchronization

* New `struct list process_list`
  * A list of all processes, where each element is a pointer to a process struct, allowing us to keep track of all processes and their children

* New `struct lock process_list_lock`
  * A lock to control data races on the process list

* Changes to `struct thread
  * `struct file *fd_array[SCHAR_MAX];`
    * An array to hold file descriptors to ensure uniqueness
  * `int current_fd;`
    * The current index of fd_array, aka the next fd to be assignede
  * `struct semaphore thread_dying_sema;`
    * A semaphore to notify when the thread is dying
  * `int exit_status;`
    * Holds thread's exit status



> B2: Describe how file descriptors are associated with open files.
> Are file descriptors unique within the entire OS or just within a
> single process?

  * File descriptors refer to an instance of an open file. They are only unique to a single process, as a process would only access an instance of a file within itself. If files are opened multiple times, a new fd is assigned at each open, and closed according to that fd. If a file is removed while instances are still open, processes can still write to them until they close the file themselves or the machine shuts down, at which point it no longer exists.

#### ALGORITHMS

> B3: Describe your code for reading and writing user data from the
> kernel.

 * Syscall read checks for a file descriptor and either reads from the command line or gets the file and calls file_read which will fill the buffer and return the number of bytes read. Similarly, syscall write checks for a file descriptor and either writes to stdout or gets the file and calls file_write which writes to the buffer and returns the number of bytes written. To take care of synchronization when start_process() opens a file, we call file_deny_write() which will not allow any other processes to write to the file until file_allow_write() is called in process_exit(). This will make sure that there is no data race on reading and writing to files simultaneously.

> B4: Suppose a system call causes a full page (4,096 bytes) of data
> to be copied from user space into the kernel.  What is the least
> and the greatest possible number of inspections of the page table
> (e.g. calls to pagedir_get_page()) that might result?  What about
> for a system call that only copies 2 bytes of data?  Is there room
> for improvement in these numbers, and how much?

  * If the data is copied in one page then there will only be one call to `pagedir_get_page()` necessary. In the worst case, if all 4096 bytes are copied into separate pages, then there could be 4096 calls necessary. For a system call that only copies 2 bytes of data, they could either be copied to the same page or separate pages, so either one or two calls are necessary. The maximum number of calls necessary could be reduced if the kernel makes sure to fill one page before allocating a new page. This will minimize the number of bytes that are copied onto separate pages. This could decrease the maximum to two system calls. If the data is copied at the beginning of a page then it will all be copied onto one page. Alternatively, if the page already has some data on it before the system call begins to copy new data, then it will take up two pages; the rest of the first page and part of a new page.

> B5: Briefly describe your implementation of the "wait" system call
> and how it interacts with process termination.

  * After checking for valid pid and child pid, wait calls `sema_down()` on the child struct's semaphore, which will wait until the child calls `sema_up()` upon completion in process_exit. It then gets the exit status before freeing its resources and returning the exit value. Its interaction with process termination is almost solely through the semaphore, which should only have a non-zero value after the process has exited.

> B6: Any access to user program memory at a user-specified address
> can fail due to a bad pointer value.  Such accesses must cause the
> process to be terminated.  System calls are fraught with such
> accesses, e.g. a "write" system call requires reading the system
> call number from the user stack, then each of the call's three
> arguments, then an arbitrary amount of user memory, and any of
> these can fail at any point.  This poses a design and
> error-handling problem: how do you best avoid obscuring the primary
> function of code in a morass of error-handling?  Furthermore, when
> an error is detected, how do you ensure that all temporarily
> allocated resources (locks, buffers, etc.) are freed?  In a few
> paragraphs, describe the strategy or strategies you adopted for
> managing these issues.  Give an example.

 * In our syscall handler, we use a helper function `sc_get_args` to get the arguments from the stack and check their validity with `get_vaddr` as well. Having a separate function handle this avoids clouding the primary function of the code with error handling.
 * When an error is detected we assess this with an if statement and return an error code (-1) or calling exit(). Thus in order to ensure all temporarily allocated resources are freed this is done prior to the return or call of exit(). 
 * For example, if `process_execute()` is called, it will pass the file name and arguments into `start_process()`. `start_process()` then parses the arguments into an array, which is passed into our `setup_arguments()` function. This function takes care of pushing the elements and their respective addresses onto the stack. Then, if this process makes a system call, it will use these arguments. Our syscall handler uses `get_vaddr()` to get the virtual address of data from user memory, which will be the system call number. We check that the first and last byte are valid (thus the entire address is valid) before continuing, which will prevent errors when trying to make the system call. Then in the switch statement the correct system call is found and `sc_get_arg()` gets the ith argument from the stack. `sc_get_arg()` checks that the entire 4 bytes are valid before returning, which avoids clouding the syscall handler with error handling. If the syscall is getting a character argument, `sc_get_char_arg()` makes sure to check that the contents of the address are valid as well, not just the address. For example suppose the system call is read. After getting the arguments, it will call `read()` and immediately check if the buffer is null. This will prevent trying to access null data, preventing more errors down the road. Then using the file descriptor, it gets the file or a null value. If the file is null, that means the file descriptor was invalid and exits with a code of -1. Then if it has passed all of our error handling, it will call and return `file_read()`. The other system calls are handled very similarly, by checking for basic error conditions inside the function but in a simple manner that simply exits or returns a value promptly so that the main portion of the code is clear.

#### SYNCHRONIZATION

> B7: The "exec" system call returns -1 if loading the new executable
> fails, so it cannot return before the new executable has completed
> loading.  How does your code ensure this?  How is the load
> success/failure status passed back to the thread that calls "exec"?

  * The process struct has a `load_done_sema` semaphore that only has a non-zero value when the executable associated with that process is completely loaded. The struct also has a `load_success` bool that tells whether the load was successful or not. After calling `process_execute()` for the new file, exec calls `sema_down()` on the new process' `load_done_sema`, which will wait until the load is finished. Exec will only continue if `load_success()` is true, and will return -1 otherwise.

> B8: Consider parent process P with child process C.  How do you
> ensure proper synchronization and avoid race conditions when P
> calls wait(C) before C exits?  After C exits?  How do you ensure
> that all resources are freed in each case?  How about when P
> terminates without waiting, before C exits?  After C exits?  Are
> there any special cases?

  * If P calls `wait(C)` before C exists, it will return -1 because there will not be a valid process struct for C.
  * After C exists, C's semaphore will handle race conditions because P will wait until C's semaphore has a positive value which will only happen upon calling `process_exit()`.
  * If the child exits first, the parent may still need to access it's exit status so we cannot free it's resources yet. However, if a parent exits first, the child will not need to access it's resources, so we can free them, even if the child still exists. So in either case, the parent will deallocate the process' resources.
  * If the child process fails to load, `exec()` will free it's resources.
  * If the parent calls wait, it will deallocate the child after it exits because the process has terminated and it will not wait for it again.
  * If the parent never calls wait for a child and terminates, it will go through it's child list and free any resources before finishing.

#### RATIONALE

> B9: Why did you choose to implement access to user memory from the
> kernel in the way that you did?

  * We chose to use the method of verifying the validity of the pointer provided by the user and then dereferencing it using `get_vaddr` from `userprog/pagedir.c`. Since this was recommended as the simplest way to implement user memory access, we thought it would be the most efficient and safest way to deal with accessing the memory. Since this method uses outside functions, it will also increase the readability of our code.

> B10: What advantages or disadvantages can you see to your design
> for file descriptors?

  * Our design for file descriptors has very little chance of repeating file descriptors since it just iterates through an array and increments each time. It is also advantageous because it keeps track of the file structs in the array as well, for easy access. However, it is very likely that there is a lot of wasted space in the array, as it does not return to 0 until the entire array (128 bytes) is filled, so it is very likely that once you get about halfway through the array, many of the files near index 0 are closed, so you could save space by putting new files in the first empty spot instead of solely incrementing each time.

> B11: The default tid_t to pid_t mapping is the identity mapping.
> If you changed it, what advantages are there to your approach?

  * We kept identity mapping for `tid_t` to `pid_t`. An advantage to creating a more complex mapping could be having more information about processes and threads and their relationship, based on the relationship between the pid and tid.
