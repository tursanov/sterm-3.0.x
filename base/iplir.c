/* Работа с VipNet Client (Инфотекс). (c) gsr 2003 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "base64.h"
#include "cfg.h"
#include "iplir.h"
#include "tki.h"

bool iplir_disabled = false;		/* работа с VipNet невозможна */
bool has_iplir = false;			/* ключевой дистрибутив распакован нормально */

/* Буфер для чтения/записи файла конфигурации */
static uint8_t cfg_buf[8192];
/* Длина данных в буфере */
static int cfg_data_len = 0;
static int cfg_index = 0;
static int cfg_char = 0;

static void cfg_reset(void)
{
	cfg_index = cfg_char = 0;
}

static int cfg_next_char(void)
{
	if (cfg_index < cfg_data_len)
		cfg_char = cfg_buf[cfg_index++];
	else
		cfg_char = EOF;
	return cfg_char;
}

static void cfg_back_char(void)
{
	if (cfg_index > 0){
		cfg_index--;
		if (cfg_index > 0)
			cfg_char = cfg_buf[cfg_index - 1];
		else
			cfg_char = 0;
	}
}

/* Буфер для структур при разборе файла конфигурации */
#define POOL_SIZE	16394
static uint8_t pool[POOL_SIZE];
/* Первый свободный байт */
static int pool_top = 0;

#define MEM_ALIGN	4

static uint8_t *pool_malloc(int size)
{
	uint8_t *p = NULL;
	size = ((size + MEM_ALIGN - 1) / MEM_ALIGN) * MEM_ALIGN;
	if (size < (POOL_SIZE - pool_top)){
		p = pool + pool_top;
		pool_top += size;
	}
	return p;
}

#define pool_new(t) (typeof(t) *)pool_malloc(sizeof(t))

static void pool_clear(void)
{
	pool_top = 0;
}

static char *pool_begin_str(void)
{
	if (pool_top < (POOL_SIZE - 1))
		return (char *)(pool + pool_top);
	else
		return NULL;
}

static bool pool_add_char(uint8_t c)
{
	if (pool_top < (POOL_SIZE - 1)){
		pool[pool_top++] = c;
		return true;
	}else
		return false;
}

static bool pool_end_str(void)
{
	if (pool_top < POOL_SIZE){
		pool[pool_top++] = 0;
		pool_top = ((pool_top + MEM_ALIGN - 1) / MEM_ALIGN) * MEM_ALIGN;
		if (pool_top > POOL_SIZE)
			pool_top = POOL_SIZE;
		return true;
	}else
		return false;
}

static void pool_rollback(uint8_t *p)
{
	if ((p >= pool) && (p < (pool + POOL_SIZE)))
		pool_top = p - pool;
}

/* Чтение данных в буфер */
static bool read_cfg_buf(char *name)
{
	struct stat st;
	int fd;
	if (name == NULL)
		return false;
	if (stat(name, &st) == -1){
		fprintf(stderr, "Ошибка получения информации о %s: %s.\n",
			name, strerror(errno));
		return false;
	}else if (st.st_size > sizeof(cfg_buf)){
		fprintf(stderr, "Размер файла %ld байт превышает максимальный (%d байт).\n",
				st.st_size, sizeof(cfg_buf));
		return false;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, "Ошибка открытия %s для чтения: %s.\n",
			name, strerror(errno));
		return false;
	}
	cfg_data_len = read(fd, cfg_buf, st.st_size);
	if (cfg_data_len != st.st_size)
		fprintf(stderr, "Ошибка чтения из %s: %s.\n",
			name, strerror(errno));
	close(fd);
	cfg_reset();
	return cfg_data_len == st.st_size;
}

/* Идентификатор текущей секции */
static int current_section = SECT_ASIS;

struct pair {
	char *name;
	int id;
};

static struct pair sections[] = {
	{"id",		SECT_ID},
	{"adapter",	SECT_ADAPTER},
	{"dynamic",	SECT_DYNAMIC},
	{"misc",	SECT_MISC},
	{"servers",	SECT_SERVERS},
	{"service",	SECT_SERVICE},
	{"virtualip",	SECT_VIRTUALIP},
	{"ip",		IFACE_SECT_IP},
	{"mode",	IFACE_SECT_MODE},
	{"db",		IFACE_SECT_DB},
};

static struct pair params[] = {
/* Параметры секции [id] */
	{"id",			PARAM_ID},
	{"name",		PARAM_NAME},
	{"ip",			PARAM_IP},
	{"firewallip",		PARAM_FIREWALLIP},
	{"port",		PARAM_PORT},
	{"proxyid",		PARAM_PROXYID},
	{"virtualip",		PARAM_VIRTUALIP},
	{"forcereal",		PARAM_FORCEREAL},
	{"filterdefault",	PARAM_FILTERDEFAULT},
	{"filtertcp",		PARAM_FILTERTCP},
	{"filterudp",		PARAM_FILTERUDP},
	{"filtericmp",		PARAM_FILTERICMP},
	{"filterip",		PARAM_FILTERIP},
	{"filterservice",	PARAM_FILTERSERVICE},
	{"tunnel",		PARAM_TUNNEL},
	{"version",		PARAM_VERSION},
	{"dynamic_firewallip",	PARAM_DYNAMICFIREWALLIP},
	{"dynamic_port",	PARAM_DYNAMICPORT},
	{"usefirewall",		PARAM_USEFIREWALL},
/* Параметры секции [adapter] */
	{"type",		PARAM_TYPE},
/* Параметры секции [dynamic] */
	{"always_use_server",	PARAM_ALWAYS_USE_SERVER},
/* Параметры секции [misc] */
	{"packettype",		PARAM_PACKETTYPE},
	{"timediff",		PARAM_TIMEDIFF},
	{"pollinterval",	PARAM_POLLINTERVAL},
	{"iparponly",		PARAM_IPARPONLY},
	{"dynamic_proxy",	PARAM_DYNAMICPROXY},
	{"ifcheck_timeout",	PARAM_IFCHECKTIMEOUT},
/* Параметры секции [servers] */
	{"server",		PARAM_SERVER},
	{"active",		PARAM_ACTIVE},
	{"primary",		PARAM_PRIMARY},
	{"secondary",		PARAM_SECONDARY},
/* Параметры секции [service] */
	{"parent",		PARAM_PARENT},
/* Параметры секции [virtualip] */
	{"startvirtualip",	PARAM_STARTVIRTUALIP},
	{"endvirtualip",	PARAM_ENDVIRTUALIP},
	{"startvirtualiphash",	PARAM_STARTVIRTUALIPHASH},
/* Параметры секций файлов iplir.conf-iface */
/* Параметры секции [ip] */
	{"ip",			PARAM_IFACEIP},
/* Параметры секции [mode] */
	{"mode",		PARAM_MODE},
	{"boomerangtype",	PARAM_BOOMERANGTYPE},
/* Параметры секции [db] */
	{"maxsize",		PARAM_MAXSIZE},
	{"timediff",		PARAM_TIMEDIFF1},	/* см. iplir.conf-iface */
	{"registerall",		PARAM_REGISTERALL},
	{"registerbroadcast",	PARAM_REGISTERBROADCAST},
	{"registertcpserverport",	PARAM_REGISTERTCPSERVERPORT},
};

/* Определение идентификатора по имени */
static int id_from_name(struct pair *p, int n, char *name)
{
	int i, id = PARAM_ASIS;
	if ((p == NULL) || (name == NULL))
		return id;
	for (i = 0; i < n; i++){
		if (strcmp(p[i].name, name) == 0){
			id = p[i].id;
			break;
		}
	}
	if ((id == PARAM_IP) && (current_section == IFACE_SECT_IP))
		id = PARAM_IFACEIP;
	return id;
}

/* Определение имени по идентификатору */
static char *name_from_id(struct pair *p, int n, int id)
{
	int i;
	if (p == NULL)
		return NULL;
	for (i = 0; i < n; i++)
		if (p[i].id == id)
			return p[i].name;
	return NULL;
}

static bool is_crlf(int c)
{
	return (c == '\r') || (c == '\n');
}

/* Чтение строки 'name= value' из файла конфигурации */
static bool read_name_val(char *name, int name_len, char *val, int val_len)
{
	enum { st_start, st_name, st_eq, st_val, st_stop, st_err };
	int i = 0, st = st_start;
	if ((name == NULL) || (val == NULL))
		return false;
	while ((st != st_stop) && (st != st_err)){
		if (cfg_next_char() == EOF){
			if (st != st_val)
				st = st_err;
			else
				st = st_stop;
			break;
		}
		switch (st) {
			case st_start:
				if (isspace(cfg_char))
					break;
				else{
					st = st_name;
					i = 0;
				}
			case st_name:
				if (cfg_char != '='){
					if (i < name_len)
						name[i++] = cfg_char;
					else
						st = st_err;
				}else{
					name[i] = 0;
					st = st_eq;
				}
				break;
			case st_eq:
				if (cfg_char != ' ')
					st = st_err;
				else{
					st = st_val;
					i = 0;
				}
				break;
			case st_val:
				if (!is_crlf(cfg_char)){
					if (i < val_len)
						val[i++] = cfg_char;
					else
						st = st_err;
				}else{
					val[i] = 0;
					st = st_stop;
				}
				break;
		}
	}
	return st == st_stop;
}

/* Чтение названия секции */
static bool read_section_name(char *name, int name_len)
{
	enum { st_start, st_name, st_stop, st_err };
	int i = 0, st = st_start;
	if (name == NULL)
		return false;
	while ((st != st_stop) && (st != st_err)){
		if (cfg_next_char() == EOF){
			st = st_err;
			break;
		}
		switch (st){
			case st_start:
				if (cfg_char == '['){
					st = st_name;
					i = 0;
				}else if (!isspace(cfg_char))
					st = st_err;
				break;
			case st_name:
				if (cfg_char == ']'){
					name[i] = 0;
					st = st_stop;
				}else if (i < name_len)
					name[i++] = cfg_char;
				else
					st = st_err;
		}
	}
	return st == st_stop;
}

/* Пропуск комментария */
static void read_comment(void)
{
	enum { st_comment, st_nl, st_stop };
	int st = st_comment;
	while (st != st_stop){
		if (cfg_next_char() == EOF)
			break;
		switch (st){
			case st_comment:
				if (is_crlf(cfg_char))
					st = st_nl;
				break;
			case st_nl:
				if (!is_crlf(cfg_char)){
					st = st_stop;
					cfg_back_char();
				}
				break;
		}
	}
}

/* Тип строки файла конфигурации */
enum {
	cfg_section,
	cfg_param,
	cfg_comment,
	cfg_eof,
};

/* Чтение очередной строки файла конфигурации */
static int read_cfg_line(char *name, int name_len, char *val, int val_len)
{
	int type;
	do{
		cfg_next_char();
		if (isspace(cfg_char))
			continue;
		else if (cfg_char == EOF)
			return cfg_eof;
		else{
			switch (cfg_char){
				case '[':
					type = cfg_section;
					break;
				case '#':
					type = cfg_comment;
					break;
				default:
					type = cfg_param;
			}
			cfg_back_char();
			break;
		}
	} while (true);
	switch (type){
		case cfg_section:
			if (read_section_name(name, name_len))
				return cfg_section;
			else
				return cfg_eof;
		case cfg_param:
			if (read_name_val(name, name_len, val, val_len))
				return cfg_param;
			else
				return cfg_eof;
		case cfg_comment:
			read_comment();
			return cfg_comment;
		default:
			return cfg_eof;
	}
}

/* Чтение параметра типа int */
static int read_int(char *val)
{
	char *p;
	int v = strtol(val, &p, 10);
	return *p ? 0 : v;
}

/* Чтение параметра типа uint32_t */
static uint32_t read_dword(char *val)
{
	char *p;
	uint32_t v = strtoul(val, &p, 0);
	return *p ? 0 : v;
}

/* Чтение значения типа bool */
static bool read_bool(char *val)
{
	if (val != NULL)
		return !strcasecmp(val, IPLIR_ON) ||
			!strcasecmp(val, IPLIR_YES);
	else
		return false;
}

/* Чтение строки */
static char *read_str(char *val)
{
	char *s;
	if (val == NULL)
		return NULL;
	s = pool_begin_str();
	if (s == NULL)
		return NULL;
	for (; (*val != 0) && (*val != ','); val++){
		if (!pool_add_char(*val)){
			pool_rollback((uint8_t *)s);
			return NULL;
		}
	}
	if (!pool_end_str()){
		pool_rollback((uint8_t *)s);
		s = NULL;
	}
	return s;
}

static char *read_asis(char *name, char *val)
{
	char *s;
	if ((name == NULL) || (val == NULL))
		return NULL;
	s = pool_begin_str();
	if (s == NULL)
		return NULL;
	for (; *name; name++){
		if (!pool_add_char(*name)){
			pool_rollback((uint8_t *)s);
			return NULL;
		}
	}
	if (!pool_add_char('=') || !pool_add_char(' '))
		return false;
	for (; *val; val++){
		if (!pool_add_char(*val)){
			pool_rollback((uint8_t *)s);
			return NULL;
		}
	}
	if (!pool_end_str()){
		pool_rollback((uint8_t *)s);
		s = NULL;
	}
	return s;
}

/* Чтение ip адреса. Возвращает длину считанной строки */
static int read_ip(char *val, uint32_t *ip)
{
	int i, j, k;
	uint32_t v;
	if ((val == NULL) || (ip == NULL))
		return 0;
	*ip = 0;
	for (i = 0, k = 0; i < 4; i++, k++){
		v = 0;
		for (j = 0; j < 3; j++, k++){
			if (isdigit(val[k])){
				v *= 10;
				v += val[k] - 0x30;
				if (v > 255)
					return 0;
			}else
				break;
		}
		*ip >>= 8;
		*ip |= (v << 24);
		if ((i < 3) && (val[k] != '.'))
			return 0;
	}
	return k ? k - 1 : k;
}

/* Чтение диапазона портов */
static char *read_port_range(char *val, struct port_range *r)
{
	char *p;
	if ((val == NULL) || (r == NULL))
		return NULL;
	r->to = r->from = strtol(val, &p, 10);
	switch (*p){
		case ',':
			return p + 1;
		case 0:
			return p;
		case '-':
			r->to = strtol(p + 1, &p, 10);
			switch (*p){
				case ',':
					return p + 1;
				case 0:
					return p;
			}
	}
	return NULL;
}

/* Чтение фильтра по умолчанию */
static int read_filterdefault(char *val)
{
	if (val == NULL)
		return iplir_filter_undef;
	for (; isspace(*val); val++);
	if (strstr(val, IPLIR_FILTER_PASS) == val)
		return iplir_filter_pass;
	else if (strstr(val, IPLIR_FILTER_DROP) == val)
		return iplir_filter_drop;
	else
		return iplir_filter_undef;
}

/* Чтение фильтра */
static struct iplir_filter *read_filter(char *val)
{
	struct iplir_filter *flt;
	char *p;
	if (val == NULL)
		return NULL;
	flt = pool_new(struct iplir_filter);
	if (flt == NULL)
		return NULL;
	p = read_port_range(val, &flt->local);
	if ((p == NULL) || !*p)
		return NULL;
	p = read_port_range(p, &flt->remote);
	if ((p == NULL) || !*p)
		return NULL;
	for (; isspace(*p); p++);
	if (*p == ','){
		flt->action = iplir_filter_undef;
		p++;
	}else if (strstr(p, IPLIR_FILTER_PASS) == p){
		flt->action = iplir_filter_pass;
		p += strlen(IPLIR_FILTER_PASS) + 1;
	}else if (strstr(p, IPLIR_FILTER_DROP) == p){
		flt->action = iplir_filter_drop;
		p += strlen(IPLIR_FILTER_DROP) + 1;
	}else
		return NULL;
	for (; isspace(*p); p++);
	if (strstr(p, IPLIR_DIR_SEND) == p){
		flt->dir = iplir_dir_send;
		p += strlen(IPLIR_DIR_SEND);
	}else if (strstr(p, IPLIR_DIR_RECV) == p){
		flt->dir = iplir_dir_recv;
		p += strlen(IPLIR_DIR_RECV);
	}else if (strstr(p, IPLIR_DIR_ANY) == p){
		flt->dir = iplir_dir_any;
		p += strlen(IPLIR_DIR_ANY);
	}else
		return NULL;
	if (!*p)
		flt->enabled = true;
	else if (*p == ','){
		p++;
		for (; isspace(*p); p++);
		if (strstr(p, IPLIR_FILTER_DISABLE) == p)
			flt->enabled = false;
		else
			return NULL;
	}
	return flt;
}

/* Чтение фильтра ip */
static struct iplir_filterip *read_filterip(char *val)
{
	struct iplir_filterip *flt;
	if (val == NULL)
		return NULL;
	flt = pool_new(struct iplir_filterip);
	if (flt == NULL)
		return NULL;
	flt->proto = read_int(val);
	return flt;
}

/* Чтение фильтра сервиса */
static struct iplir_filterservice *read_filterservice(char *val)
{
	struct iplir_filterservice *flt;
	char *p;
	if (val == NULL)
		return NULL;
	flt = pool_new(struct iplir_filterservice);
	if (flt == NULL)
		return NULL;
	flt->name = read_str(val);
	if (flt->name == NULL)
		return NULL;
	p = val + strlen(flt->name);
	if (*p != ',')
		return NULL;
	p++;
	for (; isspace(*p); p++);
	if (strstr(p, IPLIR_FILTER_PASS) == p){
		flt->action = iplir_filter_pass;
		p += strlen(IPLIR_FILTER_PASS);
	}else if (strstr(p, IPLIR_FILTER_DROP) == p){
		flt->action = iplir_filter_drop;
		p += strlen(IPLIR_FILTER_DROP);
	}else
		return NULL;
	if (!*p)
		flt->inform = false;
	else if (*p == ','){
		p++;
		for (; isspace(*p); p++);
		if (strstr(p, IPLIR_FILTER_INFORM) == p)
			flt->inform = true;
		else
			return NULL;
	}
	return flt;
}

/* Чтение информации о туннелировании */
static struct iplir_tunnel *read_tunnel(char *val)
{
	struct iplir_tunnel *tnl;
	int k;
	if (val == NULL)
		return NULL;
	tnl = pool_new(struct iplir_tunnel);
	if (tnl == NULL)
		return NULL;
	k = read_ip(val, &tnl->ip1);
	if ((k == 0) || (val[k] != '-'))
		return NULL;
	val += k + 1;
	k = read_ip(val, &tnl->ip2);
	if (k == 0)
		return NULL;
	for (; isspace(val[k]); k++);
	if (memcmp(val + k, "to", 2))
		return NULL;
	for (k += 2; isspace(val[k]); k++);
	val += k;
	k = read_ip(val, &tnl->ip3);
	if ((k == 0) || (val[k] != '-'))
		return NULL;
	val += k + 1;
	k = read_ip(val, &tnl->ip4);
	if (k == 0)
		return NULL;
	return tnl;
}

/* Чтение информации о сервере */
static struct iplir_server *read_server(char *val)
{
	struct iplir_server *srv;
	char *p;
	if (val == NULL)
		return NULL;
	srv = pool_new(struct iplir_server);
	if (srv == NULL)
		return NULL;
	srv->id = strtoul(val, &p, 0);
	if (*p != ',')
		return NULL;
	for (p++; isspace(*p); p++);
	srv->name = read_str(p);
	return srv->name == NULL ? NULL : srv;
}

/* Чтение элемента списка ip адресов */
static int read_ip_entry(char *val, uint32_t *ip1, uint32_t *ip2)
{
	int k, m = 0;
	if ((val == NULL) || (ip1 == NULL) || (ip2 == NULL))
		return 0;
	k = read_ip(val, ip1);
	if (k == 0)
		return 0;
	if (val[k] == '-'){
		k++;
		m = read_ip(val + k, ip2);
		if (m == 0)
			return 0;
	}else
		*ip2 = *ip1;
	return k + m;
}

/* Чтение списка ip адресов */
static struct iplir_ip_entry *read_ips(char *val)
{
	struct iplir_ip_entry *ips = NULL, *ip = NULL, *p;
	uint32_t from, to;
	int k;
	if (val == NULL)
		return NULL;
	while ((k = read_ip_entry(val, &from, &to)) > 0){
		p = pool_new(struct iplir_ip_entry);
		if (p == NULL)
			return NULL;
		p->next = NULL;
		p->from = from;
		p->to = to;
		if (ip != NULL)
			ip->next = p;
		else
			ips = p;
		ip = p;
		val += k;
		if (*val == ',')
			val++;
		for (; isspace(*val); val++);
	}
	return ips;
}

/* Чтение типа адаптера (internal/external) */
static int read_adapter_type(char *val)
{
	if (val == NULL)
		return -1;
	else if (strcmp(val, IPLIR_ADAPTER_INTERNAL) == 0)
		return iplir_adapter_int;
	else if (strcmp(val, IPLIR_ADAPTER_EXTERNAL) == 0)
		return iplir_adapter_ext;
	else
		return -1;
}

/* Чтение формата шифрованных пакетов. Старшее слово -- major, младшее -- minor */
static uint32_t read_packet_type(char *val)
{
	uint16_t major = 0, minor = 0;
	if (val == NULL)
		return 0;
	if (sscanf(val, "%hu.%hu", &major, &minor) == 2)
		return ((uint32_t)major << 16) | minor;
	else
		return 0;
}

/* Чтение типа бумеранга */
static int read_boomerang_type(char *val)
{
	if (val == NULL)
		return -1;
	else if (strcmp(val, IPLIR_BOOMERANG_HARD) == 0)
		return iplir_boomerang_hard;
	else if (strcmp(val, IPLIR_BOOMERANG_SOFT) == 0)
		return iplir_boomerang_soft;
	else
		return -1;
}

/* Чтение параметра конфигурации */
static bool read_param(struct iplir_param *p, char *name, char *val)
{
	if ((p == NULL) || (name == NULL) || (val == NULL))
		return false;
	switch (p->id){
		case PARAM_ID:
		case PARAM_PORT:
		case PARAM_DYNAMICPORT:
		case PARAM_PROXYID:
		case PARAM_ACTIVE:
		case PARAM_PRIMARY:
		case PARAM_SECONDARY:
		case PARAM_STARTVIRTUALIPHASH:
			p->un.dw_val = read_dword(val);
			break;
		case PARAM_NAME:
		case PARAM_PARENT:
		case PARAM_VERSION:
			p->un.s_val = read_str(val);
			break;
		case PARAM_ASIS:
			p->un.s_val = read_asis(name, val);
			break;
		case PARAM_IP:
		case PARAM_FIREWALLIP:
		case PARAM_DYNAMICFIREWALLIP:
		case PARAM_VIRTUALIP:
		case PARAM_STARTVIRTUALIP:
		case PARAM_ENDVIRTUALIP:
			p->un.dw_val = inet_addr(val);
			break;
		case PARAM_FORCEREAL:
		case PARAM_IPARPONLY:
		case PARAM_DYNAMICPROXY:
		case PARAM_USEFIREWALL:
		case PARAM_REGISTERALL:
		case PARAM_REGISTERBROADCAST:
		case PARAM_REGISTERTCPSERVERPORT:
		case PARAM_ALWAYS_USE_SERVER:
			p->un.b_val = read_bool(val);
			break;
		case PARAM_FILTERDEFAULT:
			p->un.i_val = read_filterdefault(val);
			break;
		case PARAM_FILTERTCP:
		case PARAM_FILTERUDP:
		case PARAM_FILTERICMP:
			p->un.val = read_filter(val);
			break;
		case PARAM_FILTERIP:
			p->un.val = read_filterip(val);
			break;
		case PARAM_FILTERSERVICE:
			p->un.val = read_filterservice(val);
			break;
		case PARAM_TUNNEL:
			p->un.val = read_tunnel(val);
			break;
		case PARAM_TYPE:
			p->un.i_val = read_adapter_type(val);
			break;
		case PARAM_PACKETTYPE:
			p->un.dw_val = read_packet_type(val);
			break;
		case PARAM_TIMEDIFF:
		case PARAM_TIMEDIFF1:
		case PARAM_POLLINTERVAL:
		case PARAM_MODE:
		case PARAM_IFCHECKTIMEOUT:
			p->un.i_val = read_int(val);
			break;
		case PARAM_SERVER:
			p->un.val = read_server(val);
			break;
		case PARAM_IFACEIP:
			p->un.val = read_ips(val);
			break;
		case PARAM_BOOMERANGTYPE:
			p->un.i_val = read_boomerang_type(val);
			break;
		case PARAM_MAXSIZE:	/* суффикс Mbytes не позволит использовать read_int */
			sscanf(val, "%d", &p->un.i_val);
			break;
	}
	return true;
}

/* Сканирование буфера конфигурации */
static struct iplir_section *scan_cfg(void)
{
	char name[IPLIR_MAX_NAME_LEN + 1], val[IPLIR_MAX_VAL_LEN + 1];
	int type, id;
	struct iplir_section *shead = NULL, *s = NULL, *ss;
	struct iplir_param *p = NULL, *pp;
	while ((type = read_cfg_line(name, sizeof(name) - 1, val, sizeof(val) - 1)) != cfg_eof){
		if (type == cfg_section){
			current_section = id_from_name(sections, ASIZE(sections), name);
			ss = pool_new(struct iplir_section);
			if (ss == NULL){
				printf("%s: ошибка выделения памяти\n", __func__);
				break;
			}
			ss->next = NULL;
			ss->id = current_section;
			if (current_section == SECT_ASIS)
				ss->name = read_str(name);
			else
				ss->name = NULL;
			ss->params = NULL;
			if (s == NULL)
				shead = ss;
			else
				s->next = ss;
			s = ss;
			p = NULL;
		}else if (type == cfg_param){
			if (s == NULL){
				printf("%s: не указана секция\n", __func__);
				continue;
			}
			id = id_from_name(params, ASIZE(params), name);
			if (id == -1){
				printf("%s: неизвестный параметр: '%s'\n",
					__func__, name);
				continue;
			}
			pp = pool_new(struct iplir_param);
			if (pp == NULL){
				printf("%s: ошибка выделения памяти\n", __func__);
				break;
			}
			pp->next = NULL;
			pp->id = id;
			if (!read_param(pp, name, val)){
				printf("%s: ошибка чтения параметра '%s'\n",
					__func__, name);
				pool_rollback((uint8_t *)pp);
				continue;
			}
			if (p == NULL)
				s->params = pp;
			else
				p->next = pp;
			p = pp;
		}
	}
	return shead;
}

/* Чтение файла конфигурации */
struct iplir_section *read_iplir_cfg(char *name)
{
	pool_clear();
	if (read_cfg_buf(name))
		return scan_cfg();
	else
		return NULL;
}

/* Вывод значения uint32_t */
static bool write_dword(FILE *f, struct iplir_param *p)
{
	fprintf(f, "0x%.8x", p->un.dw_val);
	return true;
}

/* Вывод номера порта */
static bool write_port(FILE *f, struct iplir_param *p)
{
	fprintf(f, "%u", p->un.dw_val);
	return true;
}

/* Вывод значения int */
static bool write_int(FILE *f, struct iplir_param *p)
{
	fprintf(f, "%d", p->un.i_val);
	return true;
}

/* Вывод значения bool */
static bool write_bool(FILE *f, struct iplir_param *p)
{
	fprintf(f, p->un.b_val ? IPLIR_ON : IPLIR_OFF);
	return true;
}

/* Вывод ip адреса */
static bool write_ip(FILE *f, struct iplir_param *p)
{
	fprintf(f, inet_ntoa(dw2ip(p->un.dw_val)));
	return true;
}

/* Вывод списка ip адресов */
static void write_ip_entry(FILE *f, struct iplir_ip_entry *entry)
{
	fprintf(f, "%s", inet_ntoa(dw2ip(entry->from)));
	if (entry->from != entry->to)
		fprintf(f, "-%s", inet_ntoa(dw2ip(entry->to)));
	if (entry->next != NULL)
		fprintf(f, ",");
}

static bool write_ips(FILE *f, struct iplir_param *p)
{
	struct iplir_ip_entry *ip;
	for (ip = (struct iplir_ip_entry *)p->un.val; ip != NULL; ip = ip->next)
		write_ip_entry(f, ip);
	return true;
}

/* Вывод строки */
static bool write_str(FILE *f, struct iplir_param *p)
{
	fprintf(f, p->un.s_val);
	return true;
}

/* Вывод фильтра по умолчанию */
static bool write_filterdefault(FILE *f, struct iplir_param *p)
{
	char *s = NULL;
	switch (p->un.i_val){
		case iplir_filter_pass:
			s = IPLIR_FILTER_PASS;
			break;
		case iplir_filter_drop:
			s = IPLIR_FILTER_DROP;
			break;
	}
	if (s != NULL){
		fprintf(f, s);
		return true;
	}else
		return false;
}

/* Вывод диапазона портов */
static bool write_port_range(FILE *f, struct port_range *range)
{
	if (range == NULL)
		return false;
	if (range->from != range->to)
		fprintf(f, "%d-%d", range->from, range->to);
	else
		fprintf(f, "%d", range->from);
	return true;
}

/* Вывод действия над пакетом */
static void write_action(FILE *f, int action)
{
	if (action == iplir_filter_pass)
		fprintf(f, IPLIR_FILTER_PASS);
	else
		fprintf(f, IPLIR_FILTER_DROP);
}

/* Вывод направления установки соединения */
static void write_dir(FILE *f, int dir)
{
	if (dir == iplir_dir_send)
		fprintf(f, IPLIR_DIR_SEND);
	else if (dir == iplir_dir_recv)
		fprintf(f, IPLIR_DIR_RECV);
	else
		fprintf(f, IPLIR_DIR_ANY);
}

/* Вывод фильтра */
static bool write_filter(FILE *f, struct iplir_param *p)
{
	struct iplir_filter *flt = (struct iplir_filter *)p->un.val;
	if (flt == NULL)
		return false;
	if (!write_port_range(f, &flt->local))
		return false;
	fprintf(f, ", ");
	if (!write_port_range(f, &flt->remote))
		return false;
	fprintf(f, ",");
	if (current_section != SECT_SERVICE){
		fprintf(f, " ");
		write_action(f, flt->action);
	}
	fprintf(f, ", ");
	write_dir(f, flt->dir);
	if (!flt->enabled)
		fprintf(f, ", %s", IPLIR_FILTER_DISABLE);
	return true;
}

/* Вывод фильтра ip */
static bool write_filterip(FILE *f, struct iplir_param *p)
{
	struct iplir_filterip *flt = (struct iplir_filterip *)p->un.val;
	if (flt == NULL)
		return false;
	fprintf(f, "%d", flt->proto);
	return true;
}

/* Вывод фильтра сервиса */
static bool write_filterservice(FILE *f, struct iplir_param *p)
{
	struct iplir_filterservice *flt = (struct iplir_filterservice *)p->un.val;
	if (flt == NULL)
		return false;
	fprintf(f, "%s, ", flt->name);
	write_action(f, flt->action);
	if (flt->inform)
		fprintf(f, ", %s", IPLIR_FILTER_INFORM);
	return true;
}

/* Вывод сведений о туннеллировании */
static bool write_tunnel(FILE *f, struct iplir_param *p)
{
	struct iplir_tunnel *tnl = (struct iplir_tunnel *)p->un.val;
	if (tnl == NULL)
		return false;
	fprintf(f, "%s-%s to %s-%s",
		inet_ntoa(dw2ip(tnl->ip1)),
		inet_ntoa(dw2ip(tnl->ip2)),
		inet_ntoa(dw2ip(tnl->ip3)),
		inet_ntoa(dw2ip(tnl->ip4)));
	return true;
}

/* Вывод типа адаптера */
static bool write_adapter_type(FILE *f, struct iplir_param *p)
{
	if (p->un.i_val == iplir_adapter_int)
		fprintf(f, IPLIR_ADAPTER_INTERNAL);
	else if (p->un.i_val == iplir_adapter_ext)
		fprintf(f, IPLIR_ADAPTER_EXTERNAL);
	else
		return false;
	return true;
}

/* Вывод типа пакета */
static bool write_packettype(FILE *f, struct iplir_param *p)
{
	int type = p->un.dw_val;
	fprintf(f, "%u.%u", type >> 16, type & 0xffff);
	return true;
}

/* Вывод информации о сервере */
static bool write_server(FILE *f, struct iplir_param *p)
{
	struct iplir_server *srv = (struct iplir_server *)p->un.val;
	fprintf(f, "0x%.8x, %s", srv->id, srv->name);
	return true;
}

/* Вывод типа бумеранга */
static bool write_boomerang_type(FILE *f, struct iplir_param *p)
{
	bool flag = true;
	if (p->un.i_val == iplir_boomerang_hard)
		fprintf(f, IPLIR_BOOMERANG_HARD);
	else if (p->un.i_val == iplir_boomerang_soft)
		fprintf(f, IPLIR_BOOMERANG_SOFT);
	else
		flag = false;
	return flag;
}

/* Вывод размера файла журнала */
static bool write_log_size(FILE *f, struct iplir_param *p)
{
	fprintf(f, "%d MBytes", p->un.i_val);
	return true;
}

/* Вывод имени параметра */
static bool write_param_name(FILE *f, int id)
{
	char *name;
	if (id == PARAM_ASIS)
		return true;
	name = name_from_id(params, ASIZE(params), id);
	if (name != NULL){
		fprintf(f, "%s= ", name);
		return true;
	}else
		return false;
}

/* Вывод параметра конфигурации */
static bool write_param(FILE *f, struct iplir_param *p)
{
	struct {
		int id;
		bool (*fn)(FILE *, struct iplir_param *);
	} write_tbl[] = {
		{PARAM_ASIS,			write_str},
		{PARAM_ID,			write_dword},
		{PARAM_NAME,			write_str},
		{PARAM_IP,			write_ip},
		{PARAM_FIREWALLIP,		write_ip},
		{PARAM_PORT,			write_port},
		{PARAM_PROXYID,			write_dword},
		{PARAM_VIRTUALIP,		write_ip},
		{PARAM_FORCEREAL,		write_bool},
		{PARAM_FILTERDEFAULT,		write_filterdefault},
		{PARAM_FILTERTCP,		write_filter},
		{PARAM_FILTERUDP,		write_filter},
		{PARAM_FILTERICMP,		write_filter},
		{PARAM_FILTERIP,		write_filterip},
		{PARAM_FILTERSERVICE,		write_filterservice},
		{PARAM_TUNNEL,			write_tunnel},
		{PARAM_VERSION,			write_str},
		{PARAM_DYNAMICFIREWALLIP,	write_ip},
		{PARAM_DYNAMICPORT,		write_port},
		{PARAM_USEFIREWALL,		write_bool},
		{PARAM_TYPE,			write_adapter_type},
		{PARAM_ALWAYS_USE_SERVER,	write_bool},
		{PARAM_PACKETTYPE,		write_packettype},
		{PARAM_TIMEDIFF,		write_int},
		{PARAM_TIMEDIFF1,		write_int},
		{PARAM_POLLINTERVAL,		write_int},
		{PARAM_IPARPONLY,		write_bool},
		{PARAM_DYNAMICPROXY,		write_bool},
		{PARAM_IFCHECKTIMEOUT,		write_int},
		{PARAM_SERVER,			write_server},
		{PARAM_ACTIVE,			write_dword},
		{PARAM_PRIMARY,			write_dword},
		{PARAM_SECONDARY,		write_dword},
		{PARAM_PARENT,			write_str},
		{PARAM_STARTVIRTUALIP,		write_ip},
		{PARAM_ENDVIRTUALIP,		write_ip},
		{PARAM_STARTVIRTUALIPHASH,	write_dword},
		{PARAM_IFACEIP,			write_ips},
		{PARAM_MODE,			write_int},
		{PARAM_BOOMERANGTYPE,		write_boomerang_type},
		{PARAM_MAXSIZE,			write_log_size},
		{PARAM_REGISTERALL,		write_bool},
		{PARAM_REGISTERBROADCAST,	write_bool},
		{PARAM_REGISTERTCPSERVERPORT,	write_bool},
	};
	int i;
	if (!write_param_name(f, p->id))
		return false;
	for (i = 0; i < ASIZE(write_tbl); i++){
		if (p->id == write_tbl[i].id){
			if (write_tbl[i].fn(f, p)){
				fprintf(f, "\n");
				return true;
			}
		}
	}
	return false;
}

/* Вывод имени секции */
static bool write_section_name(FILE *f, struct iplir_section *section)
{
	char *name;
	if (section == NULL)
		return false;
	else if (section->id == SECT_ASIS)
		name = section->name;
	else
		name = name_from_id(sections, ASIZE(sections), section->id);
	if (name != NULL){
		fprintf(f, "[%s]\n", name);
		return true;
	}else
		return false;
}

/* Вывод секции */
static bool write_section(FILE *f, struct iplir_section *section)
{
	struct iplir_param *p;
	if ((f == NULL) || (section == NULL))
		return false;
	current_section = section->id;
	if (!write_section_name(f, section))
		return false;
	for (p = section->params; p != NULL; p = p->next)
		if (!write_param(f, p))
			return false;
	fprintf(f, "\n");
	return true;
}

/* Запись файла конфигурации */
bool write_iplir_cfg(char *name, struct iplir_section *cfg)
{
	FILE *f;
	struct iplir_section *p;
	if ((name == NULL) || (cfg == NULL))
		return false;
	f = fopen(name, "wt");
	if (f == NULL){
		fprintf(stderr, "Ошибка открытия %s для записи: %s.\n",
			name, strerror(errno));
		return false;
	}
	for (p = cfg; p != NULL; p = p->next)
		if (!write_section(f, p))
			break;
	fclose(f);
	return p == NULL;
}

/* Проверка наличия файла iplir.conf */
bool check_iplir(void)
{
	struct stat st;
	has_iplir = !iplir_disabled &&
		(system(IPLIR_UNMERGE " " IPLIR_DST " " IPLIR_KEYS) == 0) &&
		create_psw_files() &&
		(system(IPLIR_CTL " check") == 0) &&
		(system(MFTP_CTL " check") == 0) &&
		(stat(IPLIR_MAIN_CONF, &st) == 0);
	return has_iplir;
}

/* Проверка конфигурации iplir */
bool cfg_iplir(void)
{
	return has_iplir && (system(IPLIR_CTL " check") == 0);
}

/* Запуск iplir */
bool start_iplir(void)
{
	if (has_iplir && (system(IPLIR_CTL " start") == 0) &&
			(system(MFTP_CTL " start") == 0)){
		sleep(1);
		return true;
	}else
		return false;
}

/* Остановка iplir */
bool stop_iplir(void)
{
	if (has_iplir && (system(IPLIR_CTL " stop") == 0) &&
			(system(MFTP_CTL " stop") == 0)){
		sleep(1);
		return true;
	}else
		return false;
}

/* Остановка iplir и выгрузка драйвера */
bool unload_iplir(void)
{
	if (has_iplir){
		if (is_iplir_loaded())
			return	(system(IPLIR_CTL " unload") == 0) &&
				(system(MFTP_CTL " stop") == 0);
		else
			return stop_iplir();
	}else
		return false;
}

/* Проверка присутствия модуля с заданным именем */
static bool is_module_loaded(const char *name)
{
	bool ret = false;
	char s[128];
	const char *p1, *p2;
	FILE *f = fopen("/proc/modules", "r");
	if (f != NULL){
		while (fgets(s, sizeof(s), f) != NULL){
			for (p1 = name, p2 = s; *p1 && (*p1 == *p2); p1++, p2++);
			if ((*p1 == 0) &&
					((*p2 == ' ')  ||
					 (*p2 == '\t') ||
					 (*p2 == '\n') ||
					 (*p2 == 0))){
				ret = true;
				break;
			}
		}
		fclose(f);
	}
	return ret;
}

/* Проверка наличия модуля iplir */
bool is_iplir_loaded(void)
{
	return	is_module_loaded(IPLIR_MOD_NAME) &&
		is_module_loaded(IPLIR_MOD1_NAME);
}

/* Создание парольных файлов на основе iplir.psw */
bool create_psw_files(void)
{
	int fd, l;
	FILE *f;
	struct stat st;
	uint8_t buf[64];
	if ((stat(IPLIR_PSW_DATA, &st) == -1) || (st.st_size > sizeof(buf)))
		return false;
	fd = open(IPLIR_PSW_DATA, O_RDONLY);
	if (fd == -1)
		return false;
	l = read(fd, buf, st.st_size);
	close(fd);
	if (l != st.st_size)
		return false;
	l = base64_decode(buf, l, buf);
	l = base64_decode(buf, l, buf);
	f = fopen(IPLIR_PSW, "wt");
	if (f == NULL)
		return false;
	fprintf(f, "%s\n%.*s\n", IPLIR_KEYS, l, buf);
	fclose(f);
/*	f = fopen(IPLIR_NET_PSW, "wt");
	if (f == NULL)
		return false;
	fprintf(f, "%.*s\n127.0.0.1\n", l, buf);
	fclose(f);*/
	return true;
}

/* Создание в файле iplir.conf записи для ppp0 */
bool add_iplir_ppp0(struct iplir_section *cfg)
{
	struct iplir_section *p, *iface = NULL, *tmp;
	struct iplir_param *prm;
	char *name = "ppp0", *s;
	if (cfg == NULL)
		return false;
	for (p = cfg; p != NULL; p = p->next){
		if (p->id != SECT_ADAPTER)
			continue;
		for (prm = p->params; prm != NULL; prm = prm->next){
			if (prm->id != PARAM_NAME)
				continue;
			if (strcmp(prm->un.s_val, name) == 0)
				return true;	/* запись уже есть */
			else
				iface = p;
		}
	}
	p = pool_new(struct iplir_section);
	if (p == NULL)
		return false;
	p->next = NULL;
	p->id = SECT_ADAPTER;
	prm = pool_new(struct iplir_param);
	if (prm == NULL)
		return false;
	prm->next = NULL;
	p->params = prm;
	prm->id = PARAM_NAME;
	s = (char *)pool_malloc(strlen(name) + 1);
	if (s == NULL)
		return false;
	strcpy(s, name);
	prm->un.s_val = s;
	prm->next = pool_new(struct iplir_param);
	if (prm->next == NULL)
		return false;
	prm = prm->next;
	prm->next = NULL;
	prm->id = PARAM_TYPE;
	prm->un.i_val = iplir_adapter_int;
	if (iface == NULL){
		for (tmp = cfg; tmp->next != NULL; tmp = tmp->next);
		tmp->next = p;
	}else{
		tmp = iface->next;
		iface->next = p;
		p->next = tmp;
	}
	return true;
}

/* Проверка возможности установки параметра active секции [serveers] */
bool can_set_active_server(struct iplir_section *cfg)
{
	bool primary = false, secondary = false;
	struct iplir_section *p;
	struct iplir_param *prm;
	if (cfg == NULL)
		return false;
	for (p = cfg; p != NULL; p = p->next){
		if (p->id == SECT_SERVERS){
			for (prm = p->params; prm != NULL; prm = prm->next){
				if (prm->id == PARAM_PRIMARY)
					primary = true;
				else if (prm->id == PARAM_SECONDARY)
					secondary = true;
			}
		}
	}
	return primary && secondary;
}

/*
 * Установка значения active секции [servers] при переключении
 * основного/резервного хостов.
 */
bool set_active_server(struct iplir_section *iplir_cfg)
{
	bool ret = false;
	struct iplir_section *p;
	struct iplir_param *prm, *active = NULL, *last = NULL;
	uint32_t primary = -1UL, secondary = -1UL;
	if (iplir_cfg == NULL)
		return false;
	for (p = iplir_cfg; p != NULL; p = p->next){
		if (p->id == SECT_SERVERS){
			for (prm = p->params; prm != NULL; prm = prm->next){
				if (prm->id == PARAM_PRIMARY){
					if (primary != -1UL)
						fprintf(stderr, "Повторное указание параметра primary в секции [servers].\n");
					primary = prm->un.dw_val;
				}else if (prm->id == PARAM_SECONDARY){
					if (secondary != -1UL)
						fprintf(stderr, "Повторное указание параметра secondary в секции [servers].\n");
					secondary = prm->un.dw_val;
				}else if (prm->id == PARAM_ACTIVE){
					if (active != NULL)
						fprintf(stderr, "Повторное указание параметра active в секции [servers].\n");
					active = prm;
				}
				if (prm->next == NULL)
					last = prm;
			}
			if (primary == -1UL)
				fprintf(stderr, "В секции [servers] не найден параметр primary.\n");
			else if (secondary == -1UL)
				fprintf(stderr, "В секции [servers] не найден параметр secondary.\n");
			else{
				if (active == NULL){
					fprintf(stderr, "В секции [servers] не найден параметр active; добавляем его.\n");
					if (last != NULL){
						last->next = pool_new(struct iplir_param);
						if (last->next != NULL){
							active = last->next;
							active->next = NULL;
							active->id = PARAM_ACTIVE;
						}
					}
				}
				if (active == NULL)
					fprintf(stderr, "Ошибка добавления параметра active в секцию [servers].\n");
				else{
					if (cfg.use_p_ip)
						active->un.dw_val = primary;
					else
						active->un.dw_val = secondary;
					ret = true;
				}
			}
			break;
		}
	}
	return ret;
}

#if defined __CHECK_HOST_IP__
/* Список допустимых ip-адресов хост-ЭВМ */
#define MAX_IPS_ENTRIES		128

static struct {
	uint32_t from;
	uint32_t to;
} valid_host_ips[MAX_IPS_ENTRIES];

/* Количество ненулевых элементов в массиве */
static int nr_valid_ips;

/* Заполнение массива на основании файла /etc/ipliradr.do$ */
bool load_valid_host_ips(void)
{
	FILE *f;
	char buf[128];
	uint8_t v[8];
	nr_valid_ips = 0;
	f = fopen(IPLIR_KEYS "ipliradr.do$", "r");
	if (f == NULL){
		fprintf(stderr, "Ошибка открытия " IPLIR_KEYS "ipliradr.do$ для чтения: %s.\n",
			strerror(errno));
		return false;
	}
	while (nr_valid_ips < ASIZE(valid_host_ips)){
		if (fgets(buf, sizeof(buf), f) == NULL)
			break;
		if (sscanf(buf, "%*X %*d %*1[Ss]:%hhu.%hhu.%hhu.%hhu-%hhu.%hhu.%hhu.%hhu\r\n",
				v, v + 1, v + 2, v + 3,
				v + 4, v + 5, v + 6, v + 7) == 8){
			valid_host_ips[nr_valid_ips].from =
				((uint32_t)v[0] << 24) | ((uint32_t)v[1] << 16) |
				((uint32_t)v[2] << 8) | (uint32_t)v[3];
			valid_host_ips[nr_valid_ips].to =
				((uint32_t)v[4] << 24) | ((uint32_t)v[5] << 16) |
				((uint32_t)v[6] << 8) | (uint32_t)v[7];
			nr_valid_ips++;
		}else if (sscanf(buf, "%*X %*d %*1[Ss]:%hhu.%hhu.%hhu.%hhu\r\n",
				v, v + 1, v + 2, v + 3) == 4){
			valid_host_ips[nr_valid_ips].from =
				((uint32_t)v[0] << 24) | ((uint32_t)v[1] << 16) |
				((uint32_t)v[2] << 8) | (uint32_t)v[3];
			valid_host_ips[nr_valid_ips].to = valid_host_ips[nr_valid_ips].from;
			nr_valid_ips++;
		}
	}
	return nr_valid_ips > 0;
}

/* Проверка принадлежности заданного ip-адреса множеству допустимых адресов */
bool check_host_ip(uint32_t ip)
{
	int i;
	if (!cfg.use_iplir)
		return true;
	for (i = 0; i < nr_valid_ips; i++){
		if ((ip >= valid_host_ips[i].from) && (ip <= valid_host_ips[i].to))
			return true;
	}
	return false;
}
#endif		/* __CHECK_HOST_IP__ */
