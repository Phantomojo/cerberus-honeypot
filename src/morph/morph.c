#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "morph.h"
#include "utils.h"

void morph_device(void) {
    // TODO: Rotate Cowrie banners, swap web HTML themes, log event
    log_event("Device morphing: Rotating device profile");
}

int main() {
    printf("Bio-Adaptive IoT Honeynet Morphing Engine\n");

    // EXAMPLE: Simulate morphing event (replace with actual logic)
    time_t now = time(NULL);
    printf("Morph event: Rotating device profile at %s", ctime(&now));

    morph_device();

    return 0;
}