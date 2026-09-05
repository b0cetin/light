
#include "shared_memory.h"
#include "debugging.h"
#include "types.h"
#include "utils/hashtables/u64toaddr_hashtable.h"
#include "vas.h"
#include <stdint.h>

Hashtable *table;

void smem_init() {
    table = ht_create();
}

SharedMemoryID next_id = SMEM_ID_NULL + 1; // TODO: A random number generator needed

SharedMemoryID smem_add(VirtualMemoryObject *object) {
    if (object == null) PANIC("smem_add: object is null");
    if (smem_is_shared(object)) PANIC("smem_add: object is already shared");

    SharedMemoryID id = next_id++;

    if (!ht_add(table, id, (uintptr_t) object))
        PANIC("smem_add: ht_add failed");

    object->smem_id = id;

    kprintln("SMEM: Object shared with ID: %ld", next_id);

    return id;
}

VirtualMemoryObject *smem_get(SharedMemoryID id) {
    VirtualMemoryObject *object = null;
    ht_lookup(table, id, (uintptr_t*) &object);
    return object;
}

void smem_remove(VirtualMemoryObject *object) {
    if (!smem_is_shared(object)) PANIC("smem_remove: object is not shared");

    SharedMemoryID id = object->smem_id;

    ht_remove(table, id);

    object->smem_id = SMEM_ID_NULL;

    kprintln("SMEM: Object stopped being shared with ID: %ld", next_id);
}
