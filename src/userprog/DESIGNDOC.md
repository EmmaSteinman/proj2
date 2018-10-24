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



#### ALGORITHMS

> A2: Briefly describe how you implemented argument parsing.  How do
> you arrange for the elements of argv[] to be in the right order?
> How do you avoid overflowing the stack page?

  * First we get parse the arguments from the parameter into an array. We pass this into our function which then sets up the stack. It pushes each argument onto the stack in the order in which it is in the array. Then we add null bits to ensure word alignment. We then iterate through the argument array once more but pushing the address of the argument instead of the argument itself. Since we are putting pointers to the arguments, the order of the arguments themselves are not important, as long as the addresses and arguments are in the same order.

#### RATIONALE/JUSTIFICATION

> A3: Why does Pintos implement strtok_r() but not strtok()?

> A4: In Pintos, the kernel separates commands into a executable name
> and arguments.  In Unix-like systems, the shell does this
> separation.  Identify at least two advantages of the Unix approach.

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
};
```
  * A struct for a child process, holding pid, parent_pid, exit status, and a semaphore, allowing us to check exit status and use appropriate synchronization

* New `struct list process_list`
  * A list of all processes, where each element is a pointer to a process struct, allowing us to keep track of all processes and their children

* Changes to `struct thread`
  * `struct file *fd_array[SCHAR_MAX];`
    * An array to hold file descriptors to ensure uniqueness
  * `int current_fd;`
    * The current index of fd_array, aka the next fd to be assignede
  * `struct semaphore thread_dying_sema;`
    * A semaphore to notify when the thread is dying
  * `int exit_status;`
    * Holds thread's exit status
  * `struct semaphore load_done_sema;`
    * Semaphore to ensure exec does not return before load is compmlete
  * `bool load_success;`
    * Indicates if load is successful or not, for exec syscall


> B2: Describe how file descriptors are associated with open files.
> Are file descriptors unique within the entire OS or just within a
> single process?

  * File descriptors refer to an instance of an open file. They are only unique to a single process, as a process would only access an instance of a file within itself. If files are opened multiple times, a new fd is assigned at each open, and closed according to that fd. If a file is removed while instances are still open, processes can still write to them until they close the file themselves or the machine shuts down, at which point it no longer exists.

#### ALGORITHMS

> B3: Describe your code for reading and writing user data from the
> kernel.

> B4: Suppose a system call causes a full page (4,096 bytes) of data
> to be copied from user space into the kernel.  What is the least
> and the greatest possible number of inspections of the page table
> (e.g. calls to pagedir_get_page()) that might result?  What about
> for a system call that only copies 2 bytes of data?  Is there room
> for improvement in these numbers, and how much?

> B5: Briefly describe your implementation of the "wait" system call
> and how it interacts with process termination.

  * After checking for valid pid and child pid, wait calls sema_down on the child struct's semaphore, which will wait until the child calls sema_up upon completion in process_exit. It then gets the exit status before freeing its resources and returning the exit value. Its interaction with process termination is almost solely through the semaphore, which should only have a non-zero value after the process is exited.

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

#### SYNCHRONIZATION

> B7: The "exec" system call returns -1 if loading the new executable
> fails, so it cannot return before the new executable has completed
> loading.  How does your code ensure this?  How is the load
> success/failure status passed back to the thread that calls "exec"?

  * The process struct has a load_done_sema semaphore that only has a non-zero value when the executable associated with that process is completely loaded. The struct also has a load_success bool that tells whether the load was successful or not. After calling process_execute for the new file, exec calls sema_down on the new process' load_done_sema, which will wait until the load is finished. Exec will only continue if load_success is true, and will return -1 otherwise.

> B8: Consider parent process P with child process C.  How do you
> ensure proper synchronization and avoid race conditions when P
> calls wait(C) before C exits?  After C exits?  How do you ensure
> that all resources are freed in each case?  How about when P
> terminates without waiting, before C exits?  After C exits?  Are
> there any special cases?

  * If P calls wait(C) before C exists, it will return -1 because there will not be a valid process struct for C.
  * After C exists, C's semaphore will handle race conditions because P will wait until C's semaphore has a positive value which will only happen upon calling process_exit.
  * FREEING RESOURCES???????????

#### RATIONALE

> B9: Why did you choose to implement access to user memory from the
> kernel in the way that you did?

  *

> B10: What advantages or disadvantages can you see to your design
> for file descriptors?

  * Our design for file descriptors has very little chance of repeating file descriptors since it just iterates through an array and increments each time. It is also advantageous because it keeps track of the file structs in the array as well, for easy access. However, it is very likely that there is a lot of wasted space in the array, as it does not return to 0 until the entire array (128 bytes) is filled, so it is very likely that once you get about halfway through the array, many of the files near index 0 are closed, so you could save space by putting new files in the first empty spot instead of solely incrementing each time.

> B11: The default tid_t to pid_t mapping is the identity mapping.
> If you changed it, what advantages are there to your approach?

  * We kept identity mapping for tid_t to pid_t. An advantage to creating a more complex mapping could be having more information about processes and threads and their relationship, based on the relationship between the pid and tid.
