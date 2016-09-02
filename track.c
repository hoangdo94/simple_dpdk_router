#include "main.h"

static int after_receive_packet(void) {
  printf(app.track_packets);
  return 0;
}

static int before_send_packet(void) {
  printf(app.track_packets);
  return 0;
}

static int table_lookup_hit(void) {
  printf(app.track_packets);
  return 0;
}

static int table_lookup_miss(void) {
  printf(app.track_packets);
  return 0;
}
