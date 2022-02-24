#include "user_info.h"

typedef enum  {
     idle = 0,
     locked = 1,
     unlocked = 2,
 } lock_status_t;

const int red[3] = {25,0,0};
const int green[3] = {0,25,0};
const int blue[3] = {0,0,25};
const int yellow[3] = {25,25,0};

int color_from_status[3][3]  = {{25,25,0}, {0,0,25}, {0,25,0}};

void set_lock_status(lock_status_t status);

void lock_lock();

void unlock_lock();

void disconnect_lock();

