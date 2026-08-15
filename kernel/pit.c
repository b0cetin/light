
#include "pit.h"
#include "debugging.h"
#include "pic.h"

// TODO: Unfinished.

void pit_init() {
    pic_clear_mask(PIT_IRQ0);

    kprintln("PIT driver initialized.");
}
