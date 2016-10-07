#include <stdlib.h>
#include <stdint.h>

#include IMPL

entry *init_entry(uint32_t structureSize, uint32_t num)
{
    entry *ptr = (entry *) malloc(structureSize * num);
    ptr->pNext = NULL;
    return ptr;
}

void free_entry(entry *head)
{
    entry *e;
    if(head == NULL)
        return;
    while (head != NULL) {
        e = head;
        head = head->pNext;
        free(e);
    }
}
