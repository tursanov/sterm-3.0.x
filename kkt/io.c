/* Работа с ККТ по виртуальному COM-порту. (c) gsr 2018 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "kkt/cmd.h"
#include "kkt/io.h"
#include "kkt/parser.h"
#include "serial.h"

static int kkt_dev = -1;

static const struct dev_info *kkt_di = NULL;

static bool batch_mode = false;

void close_dev(void)
{
	if (!batch_mode && (kkt_dev != -1)){
		serial_close(kkt_dev);
		kkt_dev = -1;
	}
}

void begin_batch_mode(void)
{
	batch_mode = true;
}

void end_batch_mode(void)
{
	batch_mode = false;
	close_dev();
}

bool open_dev(void)
{
	bool ret = false;
	if (kkt_di != NULL){
		if (kkt_dev != -1)
			close_dev();
		kkt_dev = serial_open(kkt_di->ttyS_name, &kkt_di->ss, O_RDWR);
		ret = kkt_dev != -1;
	}
	if (!ret)
		kkt_status = KKT_STATUS_COM_ERROR;
	return ret;
}

bool open_dev_if_need(void)
{
	bool ret = true;
	if (kkt_dev == -1)
		ret = open_dev();
	return ret;
}

bool on_com_error(uint32_t timeout)
{
	if (timeout == 0)
		kkt_status = KKT_STATUS_OP_TIMEOUT;
	else
		kkt_status = KKT_STATUS_COM_ERROR;
	return false;
}

void kkt_io_init(const struct dev_info *di)
{
	kkt_di = di;
	batch_mode = false;
}

void kkt_io_release(void)
{
	close_dev();
}

uint8_t tx[TX_BUF_LEN];
size_t tx_len = 0;

void reset_tx(void)
{
	tx_len = 0;
	tx_prefix = tx_cmd = 0;
}

uint8_t rx[RX_BUF_LEN];
size_t rx_len = 0;
size_t rx_exp_len = 0;

void reset_rx(void)
{
	rx_len = rx_exp_len = 0;
	kkt_status = KKT_STATUS_OK;
}

ssize_t kkt_io_write(uint32_t *timeout)
{
	ssize_t ret = serial_write(kkt_dev, tx, tx_len, timeout);
/*	if (ret > 0)
		write(STDOUT_FILENO, tx, ret);*/
	return ret;
}

ssize_t kkt_io_read(size_t len, uint32_t *timeout)
{
	ssize_t ret = serial_read(kkt_dev, rx + rx_len, len, timeout);
/*	if (ret > 0)
		write(STDOUT_FILENO, rx + rx_len, ret);*/
	return ret;
}
