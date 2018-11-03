/* Обмен данными с ОФД. (c) gsr 2018 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cfg.h"
#include "kkt/cmd.h"
#include "kkt/fs.h"
#include "kkt/kkt.h"

/* Заголовок сеансового уровня */
struct fdo_session_header {
	uint32_t sign;
#define SESSION_SIGN		0x0a41082a
	uint16_t sversion;
#define SVERSION_0_1		0xa281
#define SVERSION_CURRENT	SVERSION_0_1
	uint16_t aversion;
#define AVERSION_0_1		0x0100
#define AVERSION_CURRENT	AVERSION_0_1
	char     fr_number[KKT_FS_NR_LEN];
	uint16_t data_len;
	uint16_t flags;
#define SFLAG_PRIORITY_MASK	0x00c0
#define SFLAG_PRIORITY_NORMAL	0x0000
#define SFLAG_FEATURES_MASK	0x0030
#define SFLAG_FEATURE_NORESP	0x0000
#define SFLAG_FEATURE_RESP	0x0010
#define SFLAG_CONTAINER		0x0004
#define SFLAG_CRC_MASK		0x0003
#define SFLAG_NO_CRC		0x0000
#define SFLAG_HEAD_CRC		0x0001
#define SFLAG_FULL_CRC		0x0002
	uint16_t crc16;
	uint8_t container[0];
} __attribute__((__packed__));

/* Максимальная длина данных сеансового уровня */
#define MAX_SMSG_LEN		65536

#define FDO_TX_BUF_LEN		(MAX_SMSG_LEN + sizeof(struct fdo_session_header))
#define FDO_RX_BUF_LEN		FDO_TX_BUF_LEN

/* Буфер приёма от ОФД */
static uint8_t fdo_rx[FDO_RX_BUF_LEN];
static size_t fdo_rx_len = 0;

static inline void fdo_reset_rx(void)
{
	fdo_rx_len = 0;
}

/* Поток для работы с ОФД */
static pthread_t fdo_thread = 0;

/* Состояния потока работы с ОФД */
enum {
	fdo_thread_active,
	fdo_thread_suspended,
	fdo_thread_stopped,
};

/* Состояние потока для работы с ОФД */
static int fdo_thread_state = fdo_thread_active;

static pthread_mutex_t fdo_mtx = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* Используется для сигнализации об изменении состояния */
static int fdo_thread_evt = -1;

static bool fdo_set_thread_event(void)
{
	uint64_t evt = 1;
	return write(fdo_thread_evt, &evt, sizeof(evt)) == sizeof(evt);
}

static bool fdo_set_thread_state(int state)
{
	bool ret = false;
	if ((state != fdo_thread_state) && (pthread_mutex_lock(&fdo_mtx) == 0)){
		fprintf(stderr, "%s: %d --> %d\n", __func__, fdo_thread_state, state);
		fdo_thread_state = state;
		if (state == fdo_thread_active)
			fdo_reset_rx();
		pthread_mutex_unlock(&fdo_mtx);
		fdo_set_thread_event();
		ret = true;
	}
	return ret;
}

bool fdo_suspend(void)
{
	return fdo_set_thread_state(fdo_thread_suspended);
}

bool fdo_resume(void)
{
	return fdo_set_thread_state(fdo_thread_active);
}

static void fdo_stop_thread(void)
{
	fdo_set_thread_state(fdo_thread_stopped);
	pthread_join(fdo_thread, NULL);
}

static bool fdo_sleep(uint32_t ms)
{
	struct pollfd pfd = {
		.fd		= fdo_thread_evt,
		.events		= POLLOUT,
		.revents	= 0
	};
	int rc = poll(&pfd, 1, ms);
	return (rc > 0) && ((pfd.revents & POLLIN) == 0);
}

/* Время последнего опроса ККТ */
static uint32_t fdo_t0 = 0;

static inline void fdo_set_t0(void)
{
	fdo_t0 = u_times();
}

static inline uint32_t fdo_dt(void)
{
	return u_times() - fdo_t0;
}

/* Таймаут соединения с сервером ОФД */
#define FDO_CONNECT_TIMEOUT		3000	/* 3 сек */
/* Таймаут передачи данных ОФД */
#define FDO_SEND_TIMEOUT		3000	/* 3 сек */

#define get_timeb(t) \
	struct timeb t; \
	ftime(&t)

static uint32_t time_diff(const struct timeb *t0)
{
	get_timeb(t);
	return (t.time - t0->time) * 1000 +
		((time_t)t.millitm - (time_t)t0->millitm);
}

static int fdo_sock = -1;

static bool fdo_connected = false;

static bool fdo_sock_close(void)
{
	bool ret = true;
	if (fdo_sock != -1){
		if (shutdown(fdo_sock, SHUT_RDWR) == -1){
			fprintf(stderr, "%s: ошибка shutdown() для сокета: %s.\n",
				__func__, strerror(errno));
			ret = false;
		}
		if (close(fdo_sock) == -1){
			fprintf(stderr, "%s: ошибка close() для сокета: %s.\n",
				__func__, strerror(errno));
			ret = false;
		}
		fdo_sock = -1;
	}else
		ret = false;
	fdo_connected = false;
	return ret;
}

static bool fdo_sock_open(void)
{
	bool ret = false;
	if (fdo_sock != -1)
		fdo_sock_close();
	fdo_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fdo_sock == -1)
		fprintf(stderr, "%s: ошибка socket(): %s.\n", __func__, strerror(errno));
	else if (fcntl(fdo_sock, F_SETFL, O_NONBLOCK) == 0)
		ret = true;
	else{
		fprintf(stderr, "%s: ошибка fcntl(O_NONBLOCK): %s.\n", __func__,
			strerror(errno));
		fdo_sock_close();
	}
	return ret;
}

static bool fdo_sock_open_if_need(void)
{
	return (fdo_sock == -1) ? fdo_sock_open() : true;
}

static int fdo_get_sock_error(void)
{
	int err = 0;
	socklen_t len = sizeof(err);
	if (getsockopt(fdo_sock, SOL_SOCKET, SO_ERROR, &err, &len) == -1)
		err = errno;
	return err;
}

static bool fdo_parse_addr(const uint8_t *data, size_t len, uint32_t *ip, uint16_t *port)
{
	if ((data == NULL) || (len != 21) || (ip == NULL) || (port == NULL))
		return false;
	uint32_t v[4] = {[0 ... ASIZE(v) - 1] = 0};
	bool err = false;
	for (int i = 0, k = 0; i < 15; i++){
		uint8_t b = data[i];
		if ((i == 3) || (i == 7) || (i == 11)){
			if (b == '.')
				k++;
			else
				err = true;
		}else if (isdigit(b)){
			v[k] *= 10;
			v[k] += b - '0';
		}else
			err = true;
		if (err)
			break;
	}
	if (!err)
		err = (v[0] > 255) || (v[1] > 255) || (v[2] > 255) || (v[3] >= 255);
	if (!err){
		*ip = (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
		uint16_t p = 0;
		for (int i = 0; i < 5; i++){
			uint8_t b = data[i + 16];
			if (isdigit(b)){
				p *= 10;
				p += b - '0';
			}else{
				err = true;
				break;
			}
		}
		if (!err)
			*port = htons(p);
	}
	return !err;
}

/* Установка соединения с ОФД */
static uint16_t fdo_connect(const uint8_t *data, size_t len)
{
	fdo_connected = false;
	struct sockaddr_in addr = {
		.sin_family	= AF_INET,
		.sin_port	= 0,
		.sin_addr	= {
			.s_addr	= 0
		}
	};
	if (!fdo_parse_addr(data, len, &addr.sin_addr.s_addr, &addr.sin_port))
		return FDO_OPEN_BAD_ADDRESS;
	else if (!fdo_sock_open_if_need())
		return FDO_OPEN_ERROR;
	else if ((connect(fdo_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) &&
				(errno != EINPROGRESS)){
		fprintf(stderr, "%s: ошибка connect(): %s.\n", __func__,
			strerror(errno));
		return FDO_OPEN_ERROR;
	}
	struct pollfd fds[] = {
		{
			.fd		= fdo_sock,
			.events		= POLLOUT,
			.revents	= 0
		},
		{
			.fd		= fdo_thread_evt,
			.events		= POLLIN,
			.revents	= 0
		},
	};
	uint16_t ret = FDO_OPEN_ERROR;
	int rc = poll(fds, ASIZE(fds), FDO_CONNECT_TIMEOUT);
	if (rc == -1)
		fprintf(stderr, "%s: ошибка poll(): %s.\n", __func__, strerror(errno));
	else if (rc == 0)
		fprintf(stderr, "%s: таймаут соединения.\n", __func__);
	else if (fds[1].revents & POLLIN)
		fprintf(stderr, "%s: операция прервана.\n", __func__);
	else if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
		fprintf(stderr, "%s: ошибка сокета: %s.\n", __func__,
			strerror(fdo_get_sock_error()));
	else{
		fprintf(stderr, "%s: соединение с ОФД установлено.\n", __func__);
		fdo_connected = true;
		ret = FDO_OPEN_CONNECTED;
	}
	return ret;
}

/* Закрытие соединения с ОФД */
static uint16_t fdo_close(void)
{
	uint16_t ret = FDO_CLOSE_COMPLETE;
	if (!fdo_connected)
		ret = FDO_CLOSE_NOT_CONNECTED;
	else if (!fdo_sock_close())
		ret = FDO_CLOSE_ERROR;
	else
		fprintf(stderr, "%s: соединение с ОФД закрыто.\n", __func__);
	return ret;
}

/* Определение состояния соединения с ОФД */
static uint16_t fdo_get_connection_status(void)
{
	return fdo_connected ? FDO_STATUS_CONNECTED : FDO_STATUS_NOT_CONNECTED;
}

/* Передача данных для ОФД */
static uint16_t fdo_send(const uint8_t *data, size_t len)
{
	if ((data == NULL) || (len == 0) || (len > FDO_TX_BUF_LEN))
		return FDO_SEND_ERROR;
	uint16_t ret = FDO_SEND_ERROR;
	size_t sent_len = 0;
	uint32_t dt = 0;
	get_timeb(t0);
	while (sent_len < len){
		bool flag = false;
		struct pollfd fds[] = {
			{
				.fd		= fdo_sock,
				.events		= POLLOUT,
				.revents	= 0
			},
			{
				.fd		= fdo_thread_evt,
				.events		= POLLIN,
				.revents	= 0
			},
		};
		int rc = poll(fds, ASIZE(fds), FDO_SEND_TIMEOUT - dt);
		dt = time_diff(&t0);
		if (rc == -1)
			fprintf(stderr, "%s: оибка poll(): %s.\n", __func__, strerror(errno));
		else if (rc == 0)
			fprintf(stderr, "%s: таймаут передачи.\n", __func__);
		else if (fds[1].revents & POLLIN)
			fprintf(stderr, "%s: операция прервана.\n", __func__);
		else if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
			fprintf(stderr, "%s: ошибка сокета: %s.\n", __func__,
				strerror(fdo_get_sock_error()));
		else
			flag = true;
		if (!flag)
			break;
		ssize_t l = send(fdo_sock, data + sent_len, len - sent_len, MSG_NOSIGNAL);
		if (l == -1){
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
				flag = true;
			else
				fprintf(stderr, "%s: ошибка send(): %s.\n", __func__,
					strerror(errno));
		}else if (l > 0){
			fprintf(stderr, "%s: ОФД отправлено %d байт.\n", __func__, l);
			sent_len += l;
		}
	}
	if (sent_len == len)
		ret = FDO_SEND_OK;
	return ret;
}

/* Приём данных из ОФД */
static ssize_t fdo_recv(uint8_t *data, size_t len)
{
	if (!fdo_connected || (data == NULL) || (len == 0))
		return -1;
	ssize_t ret = recv(fdo_sock, data, len, 0);
	if (ret == -1){
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
			ret = 0;
		else
			fprintf(stderr, "%s: ошибка приёма: %s.\n", __func__,
				strerror(errno));
	}else if (ret == 0){
		fprintf(stderr, "%s: соединение закрыто со стороны ОФД.\n", __func__);
		ret = -1;
	}else
		fprintf(stderr, "%s: из ОФД получено %d байт.\n", __func__, ret);
	if (ret == -1)
		fdo_sock_close();
	return ret;
}

/* Передача в ККТ данных, принятых от ОФД */
static uint16_t fdo_send_kkt(void)
{
	uint16_t ret = FDO_RCV_OK;
/*	if (!fdo_connected)
		ret = FDO_RCV_NOT_CONNECTED;
	else*/ if (fdo_rx_len == 0)
		ret = FDO_RCV_NO_DATA;
	else{
		uint8_t status = kkt_send_fdo_data(fdo_rx, fdo_rx_len);
		if (status == FDO_DATA_STATUS_OK){
			fprintf(stderr, "%s: в ККТ передано %u байт.\n",
				__func__, fdo_rx_len);
			fdo_reset_rx();
		}else
			fprintf(stderr, "%s: ошибка передачи данных в ККТ: 0x%.2hhx.\n",
				__func__, status);
	}
	return ret;
}

static bool fdo_wait_suspend(void)
{
	bool ret = false;
	while (true){
		uint64_t evt = 0;
		if (read(fdo_thread_evt, &evt, sizeof(evt)) == sizeof(evt)){
			if (fdo_thread_state != fdo_thread_suspended){
				ret = true;
				break;
			}
		}else
			break;
	}
	return ret;
}

static uint8_t fdo_prev_op = UINT8_MAX;
static uint16_t fdo_prev_op_status = 0;

static void fdo_poll_kkt(void)
{
	static uint8_t data[65536];
	size_t data_len = sizeof(data);
	uint8_t cmd = 0;
	if (kkt_get_fdo_cmd(fdo_prev_op, fdo_prev_op_status,
			&cmd, data, &data_len) == KKT_STATUS_OK){
		fprintf(stderr, "%s: cmd = %.2hhx; data_len = %u.\n", __func__,
			cmd, data_len);
		fdo_prev_op = cmd;
		bool can_pause = false;
		switch (cmd){
			case FDO_REQ_NOP:
			case FDO_REQ_MESSAGE:
				can_pause = true;
				break;
			case FDO_REQ_OPEN:
				fdo_prev_op_status = fdo_connect(data, data_len);
				break;
			case FDO_REQ_CLOSE:
				fdo_prev_op_status = fdo_close();
				break;
			case FDO_REQ_CONN_ST:
				fdo_prev_op_status = fdo_get_connection_status();
				break;
			case FDO_REQ_SEND:
				fdo_prev_op_status = fdo_send(data, data_len);
				break;
			case FDO_REQ_RECEIVE:
				fdo_prev_op_status = fdo_send_kkt();
				can_pause = fdo_prev_op_status != FDO_RCV_OK;
				break;
		}
		if (can_pause)
			fdo_set_t0();
	}
}

static void *fdo_thread_proc(void *arg __attribute__((unused)))
{
	uint32_t poll_period = cfg.fdo_poll_period * 100;
	while (fdo_thread_state != fdo_thread_stopped){
		if (fdo_thread_state == fdo_thread_suspended)
			fdo_wait_suspend();
		else if (fdo_thread_state == fdo_thread_active){
			uint32_t dt = fdo_dt();
			if (dt < poll_period){
				ssize_t rc = fdo_recv(fdo_rx + fdo_rx_len,
					sizeof(fdo_rx) - fdo_rx_len);
				if (rc > 0)
					fdo_rx_len += rc;
				else{
					if (rc == -1)
						fdo_reset_rx();
					fdo_sleep((poll_period - dt) * 10);
				}
			}else if (cfg.has_kkt && (kkt != NULL))
				fdo_poll_kkt();
		}else if (fdo_thread_state == fdo_thread_stopped)
			break;
	}
	return NULL;
}

bool fdo_init(void)
{
	bool ret = false;
	fdo_thread_evt = eventfd(0, 0);
	if (fdo_thread_evt == -1)
		fprintf(stderr, "%s: ошибка eventfd(): %s.\n", __func__, strerror(errno));
	else if (pthread_create(&fdo_thread, NULL, fdo_thread_proc, NULL) == 0){
		fprintf(stderr, "%s: модуль ОФД готов к работе.\n", __func__);
		ret = true;
	}else
		fprintf(stderr, "%s: ошибка pthread_create(): %s.\n", __func__,
			strerror(errno));
	return ret;
}

void fdo_release(void)
{
	fdo_stop_thread();
	pthread_mutex_unlock(&fdo_mtx);
	fdo_sock_close();
	if (fdo_thread_evt != -1){
		if (close(fdo_thread_evt) == -1)
			fprintf(stderr, "%s: ошибка close(fdo_thread_evt): %s.\n",
				__func__, strerror(errno));
	}
	fprintf(stderr, "%s: модуль ОФД завершил работу.\n", __func__);
}
