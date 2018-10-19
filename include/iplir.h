/* Работа с VipNet Client (Инфотекс). (c) gsr 2003 */
/* Текущая поддерживаемая версия -- 2.8-166 */

#if !defined IPLIR_H
#define IPLIR_H

#include <netinet/in.h>
#include "sysdefs.h"

/* Расположение некоторых файлов VipNet Client */
#define IPLIR_KEYS	"/etc/iplir.key/"
#define IPLIR_MAIN_CONF	IPLIR_KEYS "user/iplir.conf"
#define IPLIR_ETH0_CONF	IPLIR_KEYS "uset/iplir.conf-eth0"
#define IPLIR_UPGRD	IPLIR_KEYS "upgrade/"
#define IPLIR_CTL	"/sbin/iplir"
#define MFTP_CTL	"/sbin/mftp"
#define IPLIR_UNMERGE	"/sbin/unmerge"
#define IPLIR_PSW	"/etc/iplirpsw"
#define IPLIR_NET_PSW	"/etc/iplirnetpsw"
#define IPLIR_VERSION	"/home/iplir/version"

#define IPLIR_MOD_NAME	"drviplir"
#define IPLIR_MOD1_NAME	"itcscrpt"

#define IPLIR_MAX_NAME_LEN		64
#define IPLIR_MAX_VAL_LEN		256

/* Секции файла iplir.conf */
enum {
	SECT_ASIS = 0,	/* не анализируется и записывается в файл как есть */
	SECT_ID,
	SECT_ADAPTER,
	SECT_DYNAMIC,
	SECT_MISC,
	SECT_SERVERS,
	SECT_SERVICE,
	SECT_VIRTUALIP,
/* Секции файла iplir.conf-iface */
	IFACE_SECT_IP,
	IFACE_SECT_MODE,
	IFACE_SECT_DB,
};

/* Параметры секции [id] */
enum {
	PARAM_ASIS = 0,	/* не анализируется и записывается в файл как есть */
	PARAM_ID,
	PARAM_NAME,
	PARAM_IP,
	PARAM_FIREWALLIP,
	PARAM_PORT,
	PARAM_PROXYID,
	PARAM_VIRTUALIP,
	PARAM_FORCEREAL,
	PARAM_FILTERDEFAULT,
	PARAM_FILTERTCP,
	PARAM_FILTERUDP,
	PARAM_FILTERICMP,
	PARAM_FILTERIP,
	PARAM_FILTERSERVICE,
	PARAM_TUNNEL,
	PARAM_VERSION,
	PARAM_DYNAMICFIREWALLIP,
	PARAM_DYNAMICPORT,
	PARAM_USEFIREWALL,
/* Параметры секции [adapter] */
	PARAM_TYPE,
/* Параметры секции [dynamic] */
	PARAM_ALWAYS_USE_SERVER,
/* Параметры секции [misc] */
	PARAM_PACKETTYPE,
	PARAM_TIMEDIFF,
	PARAM_TIMEDIFF1,
	PARAM_POLLINTERVAL,
	PARAM_IPARPONLY,
	PARAM_DYNAMICPROXY,
	PARAM_IFCHECKTIMEOUT,
/* Параметры секции [servers] */
	PARAM_SERVER,
	PARAM_ACTIVE,
	PARAM_PRIMARY,
	PARAM_SECONDARY,
/* Параметры секции [service] */
	PARAM_PARENT,
/* Параметры секции [virtualip] */
	PARAM_STARTVIRTUALIP,
	PARAM_ENDVIRTUALIP,
	PARAM_STARTVIRTUALIPHASH,
/* Параметры секций файлов iplir.conf-iface */
/* Параметры секции [ip] */
	PARAM_IFACEIP,
/* Параметры секции [mode] */
	PARAM_MODE,
	PARAM_BOOMERANGTYPE,
/* Параметры секции [db] */
	PARAM_MAXSIZE,
	PARAM_REGISTERALL,
	PARAM_REGISTERBROADCAST,
	PARAM_REGISTERTCPSERVERPORT,
};

/* Некоторые значения параметров */
#define IPLIR_ON		"on"
#define IPLIR_OFF		"off"
#define IPLIR_YES		"yes"
#define IPLIR_NO		"no"

/* Значения параметров filter* */
#define IPLIR_FILTER_PASS	"pass"
#define IPLIR_FILTER_DROP	"drop"
#define IPLIR_FILTER_DISABLE	"disable"
#define IPLIR_FILTER_INFORM	"inform"
enum {
	iplir_filter_pass,
	iplir_filter_drop,
	iplir_filter_undef,
	iplir_filter_disable,
};

/* Направление соединения */
#define IPLIR_DIR_SEND		"send"
#define IPLIR_DIR_RECV		"recv"
#define IPLIR_DIR_ANY		"any"
enum {
	iplir_dir_send,
	iplir_dir_recv,
	iplir_dir_any,
};

/* Диапазон портов TCP/UDP */
struct port_range {
	int from;
	int to;
};

/* Фильтр TCP/UDP/ICMP */
struct iplir_filter {
	struct port_range local;
	struct port_range remote;
	int action;
	int dir;
	bool enabled;
};

/* Фильтр IP */
struct iplir_filterip {
	int proto;
};

/* Фильтр типа filterservice */
struct iplir_filterservice {
	char *name;
	int action;
	bool inform;
};

struct iplir_tunnel {
	uint32_t ip1;
	uint32_t ip2;
	uint32_t ip3;
	uint32_t ip4;
};

/* Описание сервера в секции [servers] */
struct iplir_server {
	uint32_t id;
	char *name;
};

/* Тип адаптера в секции [adapter] */
#define IPLIR_ADAPTER_INTERNAL	"internal"
#define IPLIR_ADAPTER_EXTERNAL	"external"
enum {
	iplir_adapter_int = 0,
	iplir_adapter_ext,
};

/* Режим бумеранга */
#define IPLIR_BOOMERANG_HARD	"hard"
#define IPLIR_BOOMERANG_SOFT	"soft"
enum {
	iplir_boomerang_hard = 0,
	iplir_boomerang_soft,
};

/* Список ip адресов */
struct iplir_ip_entry {
	struct iplir_ip_entry *next;
	uint32_t from;
	uint32_t to;
};

/* Односвязный список параметров секции файла конфигурации */
struct iplir_param {
	struct iplir_param *next;
	int id;					/* имя параметра */
/* Значение параметра */
	union {
		bool b_val;			/* on/off yes/no */
		int i_val;			/* обычно enum */
		uint32_t dw_val;			/* число без знака или ip-адрес */
		char *s_val;			/* строка */
		void *val;			/* нечто другое */
	} un;
};

/* Односвязный список секций файла конфигурации */
struct iplir_section {
	struct iplir_section *next;
	int id;			/* для известной секции */
	char *name;		/* для неизвестной секции */
	struct iplir_param *params;
};

extern bool iplir_disabled;
extern bool has_iplir;

extern struct iplir_section *read_iplir_cfg(char *name);
extern bool write_iplir_cfg(char *name, struct iplir_section *cfg);
extern bool check_iplir(void);
extern bool cfg_iplir(void);
extern bool start_iplir(void);
extern bool stop_iplir(void);
extern bool unload_iplir(void);
extern bool is_iplir_loaded(void);
extern bool create_psw_files(void);
extern bool add_iplir_ppp0(struct iplir_section *cfg);

extern bool can_set_active_server(struct iplir_section *cfg);
extern bool set_active_server(struct iplir_section *cfg);

#if defined __CHECK_HOST_IP__
extern bool load_valid_host_ips(void);
extern bool check_host_ip(uint32_t ip);
#endif

#endif		/* IPLIR_H */
