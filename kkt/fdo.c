/* Обмен данными с ОФД. (c) gsr 2018 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/timeb.h>
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
#include "kkt/fdo.h"
#include "kkt/fs.h"

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

/* Таймауты (мс) взаимодействия с ОФД */

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

static pthread_mutex_t fdo_mtx = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static int fdo_reset_fd = -1;

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
	if (!err)
		*ip = (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
	if (err){
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
static uint8_t fdo_connect(const uint8_t *data, size_t len)
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
	else if (!fdo_sock_open_if_need() || ((connect(fdo_sock,
			(struct sockaddr *)&addr, sizeof(addr)) != 0) &&
				(errno != EINPROGRESS)))
		return FDO_OPEN_ERROR;
	struct pollfd fds[] = {
		{
			.fd		= fdo_sock,
			.events		= POLLOUT,
			.revents	= 0
		},
		{
			.fd		= fdo_reset_fd,
			.events		= POLLIN,
			.revents	= 0
		},
	};
	uint8_t ret = FDO_OPEN_ERROR;
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
		fdo_connected = true;
		ret = FDO_OPEN_CONNECTED;
	}
	return ret;
}

/* Закрытие соединения с ОФД */
static uint8_t fdo_close(void)
{
	uint8_t ret = FDO_CLOSE_COMPLETE;
	if (!fdo_connected)
		ret = FDO_CLOSE_NOT_CONNECTED;
	else if (!fdo_sock_close())
		ret = FDO_CLOSE_ERROR;
	return ret;
}

/* Передача данных для ОФД */
static uint8_t fdo_send(const uint8_t *data, size_t len)
{
	if ((data == NULL) || (len == 0) || (len > FDO_TX_BUF_LEN))
		return FDO_SEND_ERROR;
	uint8_t ret = FDO_SEND_ERROR;
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
				.fd		= fdo_reset_fd,
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
		}else if (l > 0)
			sent_len += l;
	}
	if (sent_len == len)
		ret = FDO_SEND_OK;
	return ret;
}

static ssize_t fdo_recv(uint8_t *data, size_t len)
{
	if ((data == NULL) || (len == 0))
		return false;
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
	}
	if (ret == -1)
		fdo_sock_close();
	return ret;
}
