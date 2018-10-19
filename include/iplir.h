/* ����� � VipNet Client (���⥪�). (c) gsr 2003 */
/* ������ �����ন������ ����� -- 2.8-166 */

#if !defined IPLIR_H
#define IPLIR_H

#include <netinet/in.h>
#include "sysdefs.h"

/* ��ᯮ������� �������� 䠩��� VipNet Client */
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

/* ���樨 䠩�� iplir.conf */
enum {
	SECT_ASIS = 0,	/* �� ������������ � �����뢠���� � 䠩� ��� ���� */
	SECT_ID,
	SECT_ADAPTER,
	SECT_DYNAMIC,
	SECT_MISC,
	SECT_SERVERS,
	SECT_SERVICE,
	SECT_VIRTUALIP,
/* ���樨 䠩�� iplir.conf-iface */
	IFACE_SECT_IP,
	IFACE_SECT_MODE,
	IFACE_SECT_DB,
};

/* ��ࠬ���� ᥪ樨 [id] */
enum {
	PARAM_ASIS = 0,	/* �� ������������ � �����뢠���� � 䠩� ��� ���� */
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
/* ��ࠬ���� ᥪ樨 [adapter] */
	PARAM_TYPE,
/* ��ࠬ���� ᥪ樨 [dynamic] */
	PARAM_ALWAYS_USE_SERVER,
/* ��ࠬ���� ᥪ樨 [misc] */
	PARAM_PACKETTYPE,
	PARAM_TIMEDIFF,
	PARAM_TIMEDIFF1,
	PARAM_POLLINTERVAL,
	PARAM_IPARPONLY,
	PARAM_DYNAMICPROXY,
	PARAM_IFCHECKTIMEOUT,
/* ��ࠬ���� ᥪ樨 [servers] */
	PARAM_SERVER,
	PARAM_ACTIVE,
	PARAM_PRIMARY,
	PARAM_SECONDARY,
/* ��ࠬ���� ᥪ樨 [service] */
	PARAM_PARENT,
/* ��ࠬ���� ᥪ樨 [virtualip] */
	PARAM_STARTVIRTUALIP,
	PARAM_ENDVIRTUALIP,
	PARAM_STARTVIRTUALIPHASH,
/* ��ࠬ���� ᥪ権 䠩��� iplir.conf-iface */
/* ��ࠬ���� ᥪ樨 [ip] */
	PARAM_IFACEIP,
/* ��ࠬ���� ᥪ樨 [mode] */
	PARAM_MODE,
	PARAM_BOOMERANGTYPE,
/* ��ࠬ���� ᥪ樨 [db] */
	PARAM_MAXSIZE,
	PARAM_REGISTERALL,
	PARAM_REGISTERBROADCAST,
	PARAM_REGISTERTCPSERVERPORT,
};

/* ������� ���祭�� ��ࠬ��஢ */
#define IPLIR_ON		"on"
#define IPLIR_OFF		"off"
#define IPLIR_YES		"yes"
#define IPLIR_NO		"no"

/* ���祭�� ��ࠬ��஢ filter* */
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

/* ���ࠢ����� ᮥ������� */
#define IPLIR_DIR_SEND		"send"
#define IPLIR_DIR_RECV		"recv"
#define IPLIR_DIR_ANY		"any"
enum {
	iplir_dir_send,
	iplir_dir_recv,
	iplir_dir_any,
};

/* �������� ���⮢ TCP/UDP */
struct port_range {
	int from;
	int to;
};

/* ������ TCP/UDP/ICMP */
struct iplir_filter {
	struct port_range local;
	struct port_range remote;
	int action;
	int dir;
	bool enabled;
};

/* ������ IP */
struct iplir_filterip {
	int proto;
};

/* ������ ⨯� filterservice */
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

/* ���ᠭ�� �ࢥ� � ᥪ樨 [servers] */
struct iplir_server {
	uint32_t id;
	char *name;
};

/* ��� ������ � ᥪ樨 [adapter] */
#define IPLIR_ADAPTER_INTERNAL	"internal"
#define IPLIR_ADAPTER_EXTERNAL	"external"
enum {
	iplir_adapter_int = 0,
	iplir_adapter_ext,
};

/* ����� �㬥࠭�� */
#define IPLIR_BOOMERANG_HARD	"hard"
#define IPLIR_BOOMERANG_SOFT	"soft"
enum {
	iplir_boomerang_hard = 0,
	iplir_boomerang_soft,
};

/* ���᮪ ip ���ᮢ */
struct iplir_ip_entry {
	struct iplir_ip_entry *next;
	uint32_t from;
	uint32_t to;
};

/* �����吝� ᯨ᮪ ��ࠬ��஢ ᥪ樨 䠩�� ���䨣��樨 */
struct iplir_param {
	struct iplir_param *next;
	int id;					/* ��� ��ࠬ��� */
/* ���祭�� ��ࠬ��� */
	union {
		bool b_val;			/* on/off yes/no */
		int i_val;			/* ���筮 enum */
		uint32_t dw_val;			/* �᫮ ��� ����� ��� ip-���� */
		char *s_val;			/* ��ப� */
		void *val;			/* ���� ��㣮� */
	} un;
};

/* �����吝� ᯨ᮪ ᥪ権 䠩�� ���䨣��樨 */
struct iplir_section {
	struct iplir_section *next;
	int id;			/* ��� �����⭮� ᥪ樨 */
	char *name;		/* ��� �������⭮� ᥪ樨 */
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
