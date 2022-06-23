#include "mu.h"

#include <asm-generic/param.h>
#include <linux/futex.h>
#include <linux/time.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/mman.h>

/* A futex for serializing the writes */
static void futex_lock(_Atomic int *futex)
{
    while (1) {
        _Atomic int val = *futex;
        if (val == 0 && atomic_bool_cmpxchg(futex, val, 1))
            return;
        SYSCALL3(__NR_futex, futex, FUTEX_WAIT, 1);
    }
}

static void futex_unlock(_Atomic int *futex)
{
    atomic_bool_cmpxchg(futex, 1, 0);
    SYSCALL3(__NR_futex, futex, FUTEX_WAKE, 1);
}

static inline int muwrite(int fd, const char *buffer, unsigned long len)
{
    return write(fd, buffer, len);
}

/* Print unsigned int to a string */
static void printNum(uint64_t num, int base)
{
    if (base <= 0 || base > 16)
        return;
    if (num == 0) {
        muwrite(1, "0", 1);
        return;
    }
    uint64_t n = num;
    char str[32];
    str[31] = 0;
    char *cursor = str + 30;
    char digits[] = "0123456789abcdef";
    while (n && cursor != str) {
        int rem = n % base;
        *cursor = digits[rem];
        n /= base;
        --cursor;
    }
    ++cursor;
    muwrite(1, cursor, 32 - (cursor - str));
}

/* Print signed int to a string */
static void printNumS(int64_t num)
{
    if (num == 0) {
        muwrite(1, "0", 1);
        return;
    }
    uint64_t n = num;
    char str[32];
    str[31] = 0;
    char *cursor = str + 30;
    char digits[] = "0123456789";
    while (n && cursor != str) {
        int rem = n % 10;
        *cursor = digits[rem];
        n /= 10;
        --cursor;
    }
    ++cursor;
    muwrite(1, cursor, 32 - (cursor - str));
}

/* print to stdout */
_Atomic static int print_lock;
void muprint(const char *format, ...)
{
    futex_lock(&print_lock);
    va_list ap;
    int length = 0;
    int sz = 0;
    int base = 0;
    int sgn = 0;
    const char *cursor = format, *start = format;

    va_start(ap, format);
    while (*cursor) {
        if (*cursor == '%') {
            muwrite(1, start, length);
            ++cursor;
            if (*cursor == 0)
                break;

            if (*cursor == 's') {
                const char *str = va_arg(ap, const char *);
                muwrite(1, str, strlen(str));
            }

            else {
                while (*cursor == 'l') {
                    ++sz;
                    ++cursor;
                }
                if (sz > 2)
                    sz = 2;

                if (*cursor == 'x')
                    base = 16;
                else if (*cursor == 'u')
                    base = 10;
                else if (*cursor == 'o')
                    base = 8;
                else if (*cursor == 'd')
                    sgn = 1;

                if (!sgn) {
                    uint64_t num;
                    if (sz == 0)
                        num = va_arg(ap, unsigned);
                    else if (sz == 1)
                        num = va_arg(ap, unsigned long);
                    else
                        num = va_arg(ap, unsigned long long);
                    printNum(num, base);
                } else {
                    int64_t num;
                    if (sz == 0)
                        num = va_arg(ap, int);
                    else if (sz == 1)
                        num = va_arg(ap, long);
                    else
                        num = va_arg(ap, long long);
                    printNumS(num);
                }
                sz = 0;
                base = 0;
                sgn = 0;
            }
            ++cursor;
            start = cursor;
            length = 0;
            continue;
        }
        ++length;
        ++cursor;
    }
    if (length)
        muwrite(1, start, length);
    va_end(ap);
    futex_unlock(&print_lock);
}

/* sleep */
void musleep(int secs)
{
    struct timespec ts = {.tv_sec = secs, .tv_nsec = 0},
                    rem = {.tv_sec = 0, .tv_nsec = 0};
    /* Sleep would be interrupted by signal handler, so we should 
     * record and resume it 
     */
    while (nanosleep(&ts, &rem) == -EINTR) {
        ts.tv_sec = rem.tv_sec;
        ts.tv_nsec = rem.tv_nsec;
    }
}

/* mmap */
void *mummap(void *addr,
             unsigned long length,
             int prot,
             int flags,
             int fd,
             unsigned long offset)
{
    return (void *) mmap(addr, length, prot, flags, fd, offset);
}

/* munmap */
int mumunmap(void *addr, unsigned long length)
{
    return munmap(addr, length);
}

static inline void *mubrk(void *addr)
{
    return (void *) SYSCALL1(__NR_brk, addr);
}

/* malloc helpers */
typedef struct memchunk {
    struct memchunk *next;
    uint64_t size;
} memchunk_t;

static memchunk_t head;
static void *heap_limit;
#define MEMCHUNK_USED 0x4000000000000000

/* Malloc */
_Atomic static int memory_lock = 0;
void *malloc(size_t size)
{
    futex_lock(&memory_lock);

    /* Allocating anything less than 16 bytes is kind of pointless, the
     * book-keeping overhead is too big. We will also align to 8 bytes.
     */
    size_t alloc_size = (((size - 1) >> 3) << 3) + 8;
    if (alloc_size < 16)
        alloc_size = 16;

    /* Try to find a suitable chunk that is unused */
    memchunk_t *cursor = &head;
    memchunk_t *chunk = 0;
    while (cursor->next) {
        chunk = cursor->next;
        if (!(chunk->size & MEMCHUNK_USED) && chunk->size >= alloc_size)
            break;
        cursor = cursor->next;
    }

    /* No chunk found, ask Linux for more memory */
    if (!cursor->next) {
        /* We have been called for the first time and don't know the heap limit
         * yet. On Linux, the brk syscall will return the previous heap limit on
         * error. We try to set the heap limit at 0, which is obviously wrong,
         * so that we could figure out what the current heap limit is.
         */
        if (!heap_limit)
            heap_limit = mubrk(0);

        /* We will allocate at least one page at a time */
        size_t chunk_size = (size + sizeof(memchunk_t) - 1) / EXEC_PAGESIZE;
        chunk_size *= EXEC_PAGESIZE;
        chunk_size += EXEC_PAGESIZE;

        void *new_heap_limit = mubrk((char *) heap_limit + chunk_size);
        uint64_t new_chunk_size = (char *) new_heap_limit - (char *) heap_limit;

        if (heap_limit == new_heap_limit) {
            futex_unlock(&memory_lock);
            return 0;
        }

        cursor->next = heap_limit;
        chunk = cursor->next;
        chunk->size = new_chunk_size - sizeof(memchunk_t);
        chunk->next = 0;
        heap_limit = new_heap_limit;
    }

    /* Split the chunk if it's big enough to contain one more header and at
     * least 16 more bytes
     */
    if (chunk->size > alloc_size + sizeof(memchunk_t) + 16) {
        memchunk_t *new_chunk =
            (memchunk_t *) ((char *) chunk + sizeof(memchunk_t) + alloc_size);
        new_chunk->size = chunk->size - alloc_size - sizeof(memchunk_t);
        new_chunk->next = chunk->next;
        chunk->next = new_chunk;
        chunk->size = alloc_size;
    }

    /* Mark the chunk as used and return the memory */
    chunk->size |= MEMCHUNK_USED;
    futex_unlock(&memory_lock);
    return (char *) chunk + sizeof(memchunk_t);
}

/* Free */
void free(void *ptr)
{
    if (!ptr)
        return;

    futex_lock(&memory_lock);
    memchunk_t *chunk = (memchunk_t *) ((char *) ptr - sizeof(memchunk_t));
    chunk->size &= ~MEMCHUNK_USED;
    futex_unlock(&memory_lock);
}

/* Get thread current priority and change thread priority */
int change_muthread_priority(muthread_t target, uint32_t priority, int is_inherit) 
{
    uint16_t prio_max = 99, prio_min = 1;
    int priomax = target->tpp.priomax;
    int newpriomax = priomax;
    /* get lock and raise priority */
    if (priority != -1) {
        if (priority > prio_max || priority < prio_min)
            return -EINVAL;
        if (target->tpp.priomap[priority - prio_min] + 1 == 0)
            return -EAGAIN;
        futex_lock(&target->priority_lock);
        ++target->tpp.priomap[priority - prio_min];
        futex_unlock(&target->priority_lock);
        if(is_inherit) {
            if (priority > priomax) {
                futex_lock(&target->priority_lock);
                --target->tpp.priomap[priomax - prio_min];
                futex_unlock(&target->priority_lock);
            } else {
                futex_lock(&target->priority_lock);
                --target->tpp.priomap[priority - prio_min];
                futex_unlock(&target->priority_lock);
            }
        }
        if (priority > priomax) {
            newpriomax = priority;
        } 
    } else {
        /* release lock and drop priority */
        futex_lock(&target->priority_lock);
        if (priomax && --target->tpp.priomap[priomax - prio_min] == 0) {
            int i;
            for (i = priomax - 1; i >= prio_min; --i) {
                if (target->tpp.priomap[i - prio_min])
                    break;    
            }
            newpriomax = i;
        }
        futex_unlock(&target->priority_lock);
    }
    if (newpriomax == priomax)
        return 0;
    int status = 0;
    struct sched_param param;
    futex_lock(&target->priority_lock);
    target->tpp.priomax = newpriomax;
    if (priomax < newpriomax) {
        param.sched_priority = newpriomax;
        status = sched_setparam(target->tid, &param);
    }
    if (newpriomax == 0) {
        status = sched_setparam(target->tid, &target->param);
    }
    futex_unlock(&target->priority_lock);
    return status;
}

/* Maintain wait_list */
void wait_list_add(muthread_mutex_t *resource) 
{
    muthread_t self = muthread_self();
    wait_list_t *node = malloc(sizeof(wait_list_t));
    node->mutex = resource;
    futex_lock(&self->wait_list_lock);
    node->next = self->list;
    self->list = node;
    futex_unlock(&self->wait_list_lock);
}

void wait_list_delete(muthread_mutex_t *target) 
{
    muthread_t self = muthread_self();
    futex_lock(&self->wait_list_lock);
    wait_list_t *cur = self->list, *prev = self->list;
    futex_unlock(&self->wait_list_lock);
    while (cur) {
        if (cur->mutex == target)
            break;
        prev = cur;
        cur = cur->next;
    }
    if (cur) {
        wait_list_t *tmp = cur;
        futex_lock(&(self->wait_list_lock));
        if (prev == cur)
            self->list = self->list->next;
        else
            prev->next = cur->next;
        futex_unlock(&self->wait_list_lock);
        free(tmp);
    }
}

int inherit_priority_chaining(muthread_t list_owner, uint32_t priority) 
{
    if(!list_owner)
        return 0;
    futex_lock(&list_owner->wait_list_lock);
    wait_list_t *cur = list_owner->list;
    int status = 0;
    while(cur) {
        status = change_muthread_priority(cur->mutex->owner, priority, 1);
        if (status < 0) {
            futex_unlock(&list_owner->wait_list_lock);
            return -1;
        }
            
        status = inherit_priority_chaining(cur->mutex->owner, priority);
        if (status < 0) {
            futex_unlock(&list_owner->wait_list_lock);
            return -1;
        }
        cur = cur->next;
    }
    futex_unlock(&list_owner->wait_list_lock);
    return status;
}