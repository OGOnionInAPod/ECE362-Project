#ifndef DISPLAY_H
#define DISPLAY_H

void start_display();
int display_get_bin_count();
void display_set_amplitudes(const uint8_t * amps, size_t offset, size_t num);

#endif