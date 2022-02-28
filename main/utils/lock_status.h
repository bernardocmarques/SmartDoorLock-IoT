#include "user_info.h"

typedef enum  {
     idle = 0,
     locked = 1,
     unlocked = 2,
 } lock_status_t;




void set_lock_status(lock_status_t status);

void lock_lock();

void unlock_lock();

void disconnect_lock();

