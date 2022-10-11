#ifndef POWER_UTIL_H
#define POWER_UTIL_H

#define DEFAULT_SLEEP_TIME 20
#define DEFAULT_SLEEP_DELAY 2

void deep_sleep_for_n_seconds(int seconds);

void start_deep_sleep_timer(int delay_seconds, int sleep_time_seconds);
void cancel_deep_sleep_timer();

#endif //POWER_UTIL_H
