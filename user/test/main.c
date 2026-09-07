
#include <registry.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    println("Test.");

    malloc(0);

    GRegistry registry;
    if (!poll_registry(&registry)) {
        println("Something went wrong while polling registry.");
        return -1;
    }

    println("Interface count: %ld", registry.item_count);

    while (1);

    return 0;
}
