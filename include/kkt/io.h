/* Работа с ККТ по виртуальному COM-порту. (c) gsr 2018 */

#if !defined KKT_IO_H
#define KKT_IO_H

#include "devinfo.h"

#define RX_BUF_LEN	4096
#define TX_BUF_LEN	262144

extern void kkt_io_init(const struct dev_info *di);
extern void kkt_io_release(void);

extern bool open_dev(void);
extern bool open_dev_if_need(void);
extern void close_dev(void);

extern void begin_batch_mode(void);
extern void end_batch_mode(void);

extern bool on_com_error(uint32_t timeout);

extern uint8_t tx[TX_BUF_LEN];
extern size_t tx_len;

extern void reset_tx(void);

extern uint8_t rx[RX_BUF_LEN];
extern size_t rx_len;
extern size_t rx_exp_len;

extern void reset_rx(void);

extern ssize_t kkt_io_write(uint32_t *timeout);
extern ssize_t kkt_io_read(size_t len, uint32_t *timeout);

#endif		/* KKT_IO_H */
