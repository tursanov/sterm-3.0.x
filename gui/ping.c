/*
 * Проверка линии TCP/IP посылкой icmp-пакетов (ping).
 * (c) gsr, Alex P. Popov 2002, 2004, 2005.
 */

#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "genfunc.h"
#include "kbd.h"
#include "paths.h"
#include "sterm.h"
#include "gui/exgdi.h"
#include "gui/ping.h"

enum {
	HOST_GW = 0,
	HOST_X3,
/*	HOST_X3_ALT,*/
	HOST_BANK1,
	HOST_BANK2,
	NR_HOSTS,
};

static struct ping_rec {
	struct in_addr ip;
	uint16_t id;
	uint16_t seq;
	int nr_replies;
	uint32_t t0;
} hosts[NR_HOSTS];

static int ping_sock = -1;

static void make_host_name(int n, uint32_t ip)
{
	if ((n >= 0) && (n < NR_HOSTS)){
		hosts[n].ip.s_addr = ip;
		hosts[n].id = (ip == INADDR_NONE) ? 0 : rand() & 0xffff;
		hosts[n].seq = 0;
		hosts[n].nr_replies = 0;
		hosts[n].t0 = 0;
	}
}

static void make_host_names(void)
{
	make_host_name(HOST_GW, cfg.gateway);
	make_host_name(HOST_X3, get_x3_ip());
	if (cfg.bank_system){
		uint32_t ip = ntohl(cfg.bank_proc_ip);
		make_host_name(HOST_BANK1, htonl(ip++));
		make_host_name(HOST_BANK2, htonl(ip));
	}else{
		make_host_name(HOST_BANK1, INADDR_NONE);
		make_host_name(HOST_BANK2, INADDR_NONE);
	}
}

static int create_icmp_socket(void)
{
	struct protoent *proto = NULL;	//getprotobyname(ICMP_PROTO_NAME);
	int s = socket(PF_INET,SOCK_RAW,
			(proto != NULL) ? proto->p_proto : IPPROTO_ICMP);
	if (s != -1){
		if (fcntl(s, F_SETFL, O_NONBLOCK) == -1){
			close(s);
			s = -1;
		}
	}
	return s;
}

static int in_cksum(uint16_t *buf, int sz)
{
	int nleft = sz;
	int sum = 0;
	uint16_t *w = buf, ans = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(uint16_t *) (&ans) = *(uint8_t *) w;
		sum += ans;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	ans = ~sum;
	return ans;
}

static bool send_ping(struct ping_rec *rec, uint32_t t)
{
	struct icmp *pkt;
	char packet[PING_DATA_LEN + 8];
	struct sockaddr_in sa;

	if (rec == NULL)
		return false;

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = rec->ip.s_addr;
	pkt = (struct icmp *) packet;

	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_code = 0;
	pkt->icmp_cksum = 0;
	pkt->icmp_seq = rec->seq++;
	pkt->icmp_id = rec->id;

	strcpy((char *)pkt->icmp_data, ICMP_DATA);
	pkt->icmp_cksum = in_cksum((uint16_t *)pkt, sizeof(packet));

	if (sendto(ping_sock, packet, sizeof(packet), 0,
			(struct sockaddr *)&sa, sizeof(sa)) == sizeof(packet)){
		rec->t0 = t;
		return true;
	}else
		return false;
}

static int parse_reply(char *buf, int sz, struct sockaddr_in *from)
{
	struct icmp *icmp_pkt;
	struct iphdr *ip_hdr;
	int i, hlen;

	if (sz < (PING_DATA_LEN + ICMP_MINLEN))
		return -1;
	ip_hdr = (struct iphdr *)buf;
	hlen = ip_hdr->ihl << 2;
	sz -= hlen;
	icmp_pkt = (struct icmp *)(buf + hlen);
	for (i = 0; i < NR_HOSTS; i++){
		if (hosts[i].ip.s_addr != from->sin_addr.s_addr)
			continue;
		if ((icmp_pkt->icmp_type == ICMP_ECHOREPLY) &&
				(icmp_pkt->icmp_id == hosts[i].id) &&
				(icmp_pkt->icmp_seq < hosts[i].seq)){
			hosts[i].nr_replies++;
			return i;
		}
	}
	return -1;
}

#define MAX_PING_LINES	22
#define MAX_STR_LEN	78

static PixmapFontPtr ping_font = NULL;
static int cur_ping_line;
static char ping_lines[MAX_PING_LINES][MAX_STR_LEN + 1];

static bool draw_ping_hints(void)
{
	char *p = "Esc -- выход";
	const int hint_h = 34;
	int cx = DISCX - 5;
	int cy = hint_h*2+4;
	GCPtr pGC = CreateGC(4, DISCY - hint_h*2-8, cx, cy);
	GCPtr pMemGC = CreateMemGC(cx, cy);
	FontPtr pFont = CreateFont(_("fonts/terminal10x18.fnt"), false);
	
	SetFont(pMemGC, pFont);
	ClearGC(pMemGC, clBtnFace);
	DrawBorder(pMemGC, 0, 0, GetCX(pMemGC), GetCY(pMemGC), 1,
		clBtnShadow, clBtnHighlight);
	
	DrawText(pMemGC, 0,0,pMemGC->box.width,pMemGC->box.height, p, 0);
	CopyGC(pGC, 0, 0, pMemGC, 0, 0, cx, cy);
	DeleteFont(pFont);
	DeleteGC(pMemGC);
	DeleteGC(pGC);

	return true;
}

static bool draw_ping_lines(void)
{
	int cx = DISCX-2, cy = DISCY-42;
	GCPtr pGC = CreateGC(20, 35, cx, cy);
	GCPtr pMemGC = CreateMemGC(MAX_STR_LEN*ping_font->max_width, ping_font->max_height);
	int n, m;

	for (n=0, m=0; n<MAX_PING_LINES; n++) {
		if (ping_lines[n][0] != 0x0)
			m=n+1;
	}

	for (n=0; n<m; n++) {
		ClearGC(pMemGC, cfg.bg_color);
		PixmapTextOut(pMemGC, ping_font, 0, 0, ping_lines[n]);
		CopyGC(pGC, 0, ping_font->max_height*n, pMemGC,
			0, 0, GetCX(pMemGC), GetCY(pMemGC));
	}
	
	DeleteGC(pMemGC);
	DeleteGC(pGC);

	return true;
}

static void make_gap_line(void)
{
	int i;
	for (i = 0; i < MAX_PING_LINES - 1; i++)
		strcpy(ping_lines[i], ping_lines[i+1]);
	memset(ping_lines[MAX_PING_LINES - 1], 0, MAX_STR_LEN+1);
}

static void check_pos(void)
{
	if (cur_ping_line >= MAX_PING_LINES) {
		cur_ping_line = MAX_PING_LINES-1;
		make_gap_line();
	}
}

#define TAB_SPACES	8
#define PING_LINE_LEN	1024

static bool add_ping_line(char *format, ...)
{
	int i, n = strlen(ping_lines[cur_ping_line]), m;
	char s[PING_LINE_LEN + 1], *text;
	va_list p;
	void new_line(void)
	{
		n = 0;
		cur_ping_line++;
		check_pos();
	}
	if (!format)
		return false;
	va_start(p, format);
	vsnprintf(s, PING_LINE_LEN, format, p);
	s[PING_LINE_LEN] = 0;
	va_end(p);
	for (text = s; *text; text++) {
		switch (*text) {
		case '\t':
			m = (n / TAB_SPACES + 1) * TAB_SPACES - n;
			for (i = 0; i < m; i++) {
				ping_lines[cur_ping_line][n++] = ' ';
				if (n == MAX_STR_LEN)
					new_line();
			}
			break;
		case '\n':
			new_line();
			break;
		default:
			ping_lines[cur_ping_line][n++] = *text;
			if (n == MAX_STR_LEN) {
				new_line();
			}
		}
	}
	draw_ping_lines();
	return true;
}

bool draw_ping(void)
{
	draw_ping_hints();
	return true;
}


bool init_ping()
{
	make_host_names();
	ping_sock = create_icmp_socket();
	if (ping_sock == -1)
		return false;
	ping_active = true;

	cur_ping_line = 0;
	memset(ping_lines, 0, sizeof(ping_lines));
	set_term_busy(true);
	hide_cursor();
	set_scr_mode(m80x20, false, false);
	clear_text_field();

	set_term_state(st_ping);
	set_term_astate(ast_none);

	scr_show_pgnum(false);
	scr_show_mode(false);
	scr_show_language(false);
	scr_show_log(false);
/*	scr_visible = false;*/

	ping_font = CreatePixmapFont(_("fonts/sterm/80x20/courier10x21.fnt"), 
		cfg.rus_color, cfg.bg_color);
	draw_ping();
	draw_clock(true);
	return true;
}

void release_ping(void)
{
	DeletePixmapFont(ping_font);
	close(ping_sock);
	ping_active = false;
}

static char *get_ping_line(int n)
{
	static char buf[128];
	int l;
	switch (n){
		case HOST_GW:
			strcpy(buf, "Проверка шлюза:\t\t\t\t");
			break;
		case HOST_X3:
			strcpy(buf, "Проверка хост-ЭВМ \"Экспресс-3\":\t\t");
			break;
/*		case HOST_X3_ALT:
			strcpy(buf, "Проверка хост-ЭВМ-2 \"Экспресс-3\":\t");
			break;*/
		case HOST_BANK1:
			strcpy(buf, "Проверка процессингового центра #1:\t");
			break;
		case HOST_BANK2:
			strcpy(buf, "Проверка процессингового центра #2:\t");
			break;
		default:
			buf[0] = 0;
	}
	l = strlen(buf);
	sprintf(buf + l, "%d/%hu", hosts[n].nr_replies, hosts[n].seq);
	return buf;
}

bool process_ping(struct kbd_event *e)
{
	uint32_t t = u_times();
	char packet[PING_DATA_LEN + MAX_IP_LEN + MAX_ICMP_LEN];
	struct sockaddr_in from;
	int i, l, n, from_len = sizeof(from);
	if (e->pressed && (e->key == KEY_ESCAPE))
		return false;
	l = recvfrom(ping_sock, packet, sizeof(packet), 0,
		(struct sockaddr *)&from, (socklen_t *)&from_len);
	if (l > 0){
		n = parse_reply(packet, l, &from);
		if (n != -1){
			cur_ping_line = n;
			ping_lines[n][0] = 0;
			add_ping_line(get_ping_line(n));
		}
	}
	for (i = 0; i < NR_HOSTS; i++){
		if ((hosts[i].ip.s_addr == INADDR_NONE) ||
				(hosts[i].nr_replies == NR_PINGS))
			continue;
		if ((hosts[i].seq < NR_PINGS) &&
				(t - hosts[i].t0) > ICMP_PING_INTERVAL){
			if (send_ping(hosts + i, t)){
				cur_ping_line = i;
				ping_lines[i][0] = 0;
				add_ping_line(get_ping_line(i));
			}
		}
	}
	return true;
}
