# MuThreadPackage
MuThread package provides basic thread and mutex functions.

* Thread 
    * with thread local storage(TLS)
* Mutex
    * type
        1. normal(default)
        2. errorCheck
        3. recursive
    * protocol
        1. priority inheritance
        2. priority protection
## Running auto-test
Check the correctness of your code, i.e. auto-test:
```shell
$ make test
```
Check the example usage of thread package:
```shell
$ make check
```
Compile specific file with thread package
```shell
make FILE=`$FILENAME` NUM=`$NUMBER`
```
* number is unnecessary, this argument can generate different file name

Switch muthread to pthread

* add `PTHREAD=1` in the end of build command.

For example:
```shell
make FILE=`$FILENAME` NUM=`$NUMBER` PTHREAD=1
```
## MuThread Layout
* Thread data
    * thread memory address
    * thread id
* Stack(TLS) with guard page in the end to prevent overflow
* Task
    * Funciton and argument
* Scheduling attributes
    * ploicy
    * parameter (priority)
* wait_list : a singly linked-list records waiting rescores
* priority_protection_data : an array records proirity change pattern

![](https://i.imgur.com/JOKuWOA.png)
### Muthread attr
```c
typedef struct {
    uint32_t stack_size; /* thread local storage size */
    uint16_t policy; /* schedule policy */
    uint16_t flags; 
    /* TBTHREAD_INHERIT_SCHED (the scheduling attributes in attr would be ignored)
     * TBTHREAD_EXPLICIT_SCHED (the scheduling attributes in attr would be set)
     */
    struct sched_param *param; /* schedule parameter */
} muthread_attr_t;
```
### Mutex attr
```c
typedef struct {
    uint16_t type; /* bit 0 ~ 7: prioceiling, bit 8 ~ 11: protocol, bit 12 ~ 15: mutex type */
} muthread_mutexattr_t;
```
## Muthread API
### Init and Set muthread attributes
* void muthread_attr_init(muthread_attr_t *attr);
* int muthread_attr_setschedpolicy(muthread_attr_t *attr, uint16_t policy);
* int muthread_attr_setschedparam(muthread_attr_t *attr, struct sched_param *param);
* int muthread_attr_setinheritsched(muthread_attr_t *attr, int inherit);
### Muthread
* int muthread_create(muthread_t *thread,
                    const muthread_attr_t *attrs,
                    void *(*f)(void *),
                    void *arg);
* void muthread_join(muthread_t thread, void **thread_return);
* muthread_t muthread_self()
### Init and Set mutex attributes
* int muthread_mutexattr_init(muthread_mutexattr_t *attr);
* int muthread_mutexattr_settype(muthread_mutexattr_t *attr, int type);
* int muthread_mutexattr_setprotocol(muthread_mutexattr_t *attr, int protocol);
* int muthread_mutexattr_setprioceiling(muthread_mutexattr_t *attr, int prioceiling)
### Mutex
* int muthread_mutex_init(muthread_mutex_t *mutex,
                        const muthread_mutexattr_t *attr);
* int muthread_mutex_lock(muthread_mutex_t *mutex);
* int muthread_mutex_trylock(muthread_mutex_t *mutex);
* int muthread_mutex_unlock(muthread_mutex_t *mutex);
### Utils for muthread
* void muprint(const char *format, ...);
* void musleep(int secs);