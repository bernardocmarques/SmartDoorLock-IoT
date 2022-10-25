#ifndef LOCK_STATUS_H
#define LOCK_STATUS_H
#include "user_info.h"

typedef enum  {
    locked = 0,
    unlocked = 1,
} lock_state_t;

typedef enum  {
    led_idle = 0,
    led_locked = 1,
    led_unlocked = 2,
    led_sleep = 3
} led_status_t;


//void set_lock_status(lock_status_t status);

void set_led_status(led_status_t s);

void lock_lock();

void unlock_lock();

void disconnect_lock();

void sleep_lock();
#endif // LOCK_STATUS_H

