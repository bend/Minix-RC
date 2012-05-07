#include <sys/resource.h>


/* Represents a user node containing the number of process for the user */
struct unode {
    uid_t uid;              /* user's uid */
    unsigned int nb_proc;   /* number of running processes for the user */
    struct rlimit plim;     /* limit for nproc */
    struct unode* next;           /* pointer to next */
};

