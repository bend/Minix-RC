#include "pm.h"
#include "unode.h"

FORWARD _PROTOTYPE( struct unode* unode_init, (uid_t uid) );



PUBLIC int ulist_init()
{
    /* Initialize to NULL because we don't have any users yet */
    nodes = NULL; 
    printf("initializing ulist\n");

    return 0;
}

PRIVATE struct unode* unode_init( uid_t uid )
{
    struct unode* node = (struct unode*) malloc(sizeof(struct unode));
    if(node == NULL)
        return NULL;
    node->uid = uid;
    node->plim.rlim_cur = RLIM_NPROC_DEFAULT;
    node->plim.rlim_max = RLIM_NPROC_DEFAULT;
    node->nb_proc = 0;
    node->next = NULL;
    printf("initializing unode\n");
    return node;
}

PUBLIC struct unode* unode_get_always(uid_t uid)
{
    struct unode *tmp  = nodes;
    struct unode *new;
    /* Search for node and return it */
    printf("get always\n");
    while(tmp != NULL)
    {
        printf("uid of tmp is %d and uid is %d\n", tmp->uid, uid);
        if(tmp->uid == uid)
        {

            printf("unode found\n");
            return tmp;
        }
        if(tmp->next == NULL)
            break;
        tmp = tmp->next;
    }

    /* Node does not exist, create and return it */
    new  = unode_init(uid);
    /* Add the new node to the list */
    if(tmp == NULL)
        nodes = new;
    else
        tmp->next = new;

    return new;
}
