#include "unode.h"
#include "glo.h"

FORWARD _PROTOTYPE( int ulist_init, (void) );
FORWARD _PROTOTYPE( struct unode* unode_init, (uid_t uid) );
FORWARD _PROTOTYPE( struct unode* unode_get_always, (uid_t uid) );



PUBLIC int ulist_init()
{
    /* Initialize to NULL because we don't have any users yet */
    nodes = NULL;

    return 0;
}

PUBLIC struct unode* unode_init( uid_t uid )
{
    struct unode* node = (struct unode*) malloc(sizeof(struct unode));
    if(unode == NO_MEM)
        return NULL;
    node->uid = uid;
    node->plim.rlim_cur = RLIM_NPROC_DEFAULT;
    node->plim.rlim_max = RLIM_NPROC_DEFAULT;
    node->nbproc = 0;
    node->next = NULL;

}

PUBLIC struct unode* unode_get_always(uid_t uid)
{
    struct unode *tmp = nodes;
    /* Search for node and return it */
    while(tmp != NULL)
    {
        if(tmp->uid = uid)
            return tmp;
        if(tmp->next == NULL)
            break;
        tmp = tmp->next;
    }

    /* Node does not exist, create and return it */
    struct unode *new = unode_init(uid);
    /* Add the new node to the list */
    tmp->next = new;

    return new;
}
