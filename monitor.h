#ifndef MIST32_MONITOR_H
#define MIST32_MONITOR_H

#define MONITOR_PORT 1032
#define MONITOR_BUF_SIZE 4096

void monitor_init(void);
void monitor_close(void);
void monitor_method_recv(void);
void monitor_send_queue(void);

/* method call */
void monitor_disconnect(void);
void monitor_display_queue_draw(unsigned int x, unsigned int y, unsigned int color);

#endif /* MIST32_MONITOR_H */
