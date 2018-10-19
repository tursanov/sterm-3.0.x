/* Работа с клавиатурой терминала и динамиком. (c) gsr 2000-2001, 2010 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "ds1990a.h"
#include "kbd.h"
#include "serial.h"

int kbd=-1;
int oldkbmode;
struct termios old,new;

int shift_state=0;

key_metric_array kbd_win_keys = {
	{KEY_ESCAPE	,'\x1b'	,'\x1b'},
	{KEY_1		,'!'	,'!'},
	{KEY_2		,'"'	,'@'},
	{KEY_3		,'#'	,'#'},
	{KEY_4		,';'	,'¤'},
	{KEY_5		,'%'	,'%'},
	{KEY_6		,':'	,'^'},
	{KEY_7		,'?'	,'&'},
	{KEY_8		,'*'	,'*'},
	{KEY_9		,'('	,'('},
	{KEY_0		,')'	,')'},
	{KEY_MINUS	,'-'	,'_'},
	{KEY_PLUS	,'='	,'+'},
	{KEY_Q		,'Й'	,'Q'},
	{KEY_W		,'Ц'	,'W'},
	{KEY_E		,'У'	,'E'},
	{KEY_R		,'К'	,'R'},
	{KEY_T		,'Е'	,'T'},
	{KEY_Y		,'Н'	,'Y'},
	{KEY_U		,'Г'	,'U'},
	{KEY_I		,'Ш'	,'I'},
	{KEY_O		,'Щ'	,'O'},
	{KEY_P		,'З'	,'P'},
	{KEY_LFBRACE	,'Х'	,'['},
	{KEY_RFBRACE	,'.'	,']'},
	{KEY_A		,'Ф'	,'A'},
	{KEY_S		,'Ы'	,'S'},
	{KEY_D		,'В'	,'D'},
	{KEY_F		,'А'	,'F'},
	{KEY_G		,'П'	,'G'},
	{KEY_H		,'Р'	,'H'},
	{KEY_J		,'О'	,'J'},
	{KEY_K		,'Л'	,'K'},
	{KEY_L		,'Д'	,'L'},
	{KEY_COLON	,'Ж'	,':'},
	{KEY_QUOTE	,'Э'	,'"'},
	{KEY_TILDE	,';'	,'\''},
	{KEY_BKSLASH	,','	,'\\'},
	{KEY_Z		,'Я'	,'Z'},
	{KEY_X		,'Ч'	,'X'},
	{KEY_C		,'С'	,'C'},
	{KEY_V		,'М'	,'V'},
	{KEY_B		,'И'	,'B'},
	{KEY_N		,'Т'	,'N'},
	{KEY_M		,'Ь'	,'M'},
	{KEY_COMMA	,'Б'	,'<'},
	{KEY_DOT	,'Ю'	,'>'},
	{KEY_SLASH	,'/'	,'?'},
	{KEY_NUMMUL	,'*'	,'*'},
	{KEY_SPACE	,' '	,' '},
	{KEY_NUMHOME	,'7'	,'7'},
	{KEY_NUMUP	,'8'	,'8'},
	{KEY_NUMPGUP	,'9'	,'9'},
	{KEY_NUMMINUS	,'-'	,'-'},
	{KEY_NUMLEFT	,'4'	,'4'},
	{KEY_NUMDOT	,'5'	,'5'},
	{KEY_NUMRIGHT	,'6'	,'6'},
	{KEY_NUMPLUS	,'+'	,'+'},
	{KEY_NUMEND	,'1'	,'1'},
	{KEY_NUMDOWN	,'2'	,'2'},
	{KEY_NUMPGDN	,'3'	,'3'},
	{KEY_NUMINS	,'0'	,'0'},
	{KEY_NUMDEL	,'.'	,'.'},
	{KEY_NUMSLASH	,'/'	,'/'},
};

struct key_metric	*kbd_keys = kbd_win_keys;

int	 	kbd_lang = lng_rus;
time_t 		kbd_last_click=0;
int		pressed_keys[PKEY_BUF_LEN];

int kbd_switch_language(void)
{
	int old_lang=kbd_lang;
	kbd_lang = (kbd_lang == lng_rus) ? lng_lat : lng_rus;
	return old_lang;
}

void kbd_check_language(struct kbd_event *e)
{
	if ((e != NULL) && !e->repeated){
		switch (e->key){
			case KEY_LSHIFT:
			case KEY_RSHIFT:
				kbd_switch_language();
				break;
			case KEY_CAPS:
				if (e->pressed)
					kbd_switch_language();
				break;
		}
	}
}

bool kbd_mark_key(struct kbd_event *e)
{
	int i;
	if (e != NULL){
		if (e->pressed){
			if (!kbd_check_repeat(e)){
				for (i=0; i < PKEY_BUF_LEN; i++)
					if (pressed_keys[i] == 0){
						pressed_keys[i]=e->key;
						return true;
					}
			}
		}else{
			for (i=0; i < PKEY_BUF_LEN; i++)
				if (pressed_keys[i] == e->key){
					pressed_keys[i]=0;
					return true;
				}
		}
	}
	return false;
}

bool kbd_check_repeat(struct kbd_event *e)
{
	if ((e != NULL) && e->pressed){
		int i;
		for (i=0; i < PKEY_BUF_LEN; i++)
			if (pressed_keys[i] == e->key)
				return true;
	}
	return false;
}

void kbd_set_shift(int flag,bool set)
{
	if (set)
		shift_state |= flag;
	else
		shift_state &= ~flag;
}

bool kbd_check_shift(struct kbd_event *e)
{
	if ((e != NULL) && !e->repeated){
		switch (e->key){
			case KEY_LSHIFT:
				kbd_set_shift(SHIFT_LSHIFT,e->pressed);
				return true;
			case KEY_RSHIFT:
				kbd_set_shift(SHIFT_RSHIFT,e->pressed);
				return true;
			case KEY_LCTRL:
				kbd_set_shift(SHIFT_LCTRL,e->pressed);
				return true;
			case KEY_RCTRL:
				kbd_set_shift(SHIFT_RCTRL,e->pressed);
				return true;
			case KEY_LALT:
				kbd_set_shift(SHIFT_LALT,e->pressed);
				return true;
			case KEY_RALT:
				kbd_set_shift(SHIFT_RALT,e->pressed);
				return true;
		}
	}
	return false;
}

time_t kbd_idle_interval(void)
{
	return time(NULL)-kbd_last_click;
}

void kbd_reset_idle_interval(void)
{
	kbd_last_click = time(NULL);
}

bool kbd_set_rate(int rate,int delay)
{
	struct kbd_repeat kbd_rep={delay,rate};
	return ioctl(kbd,KDKBDREP,&kbd_rep) == 0;
}

int kbd_get_char(int key)
{
	int i;
	for (i=0; i < N_CHAR_KEYS; i++)
		if (kbd_keys[i].key == key)
			return (kbd_lang == lng_rus) ? kbd_keys[i].rus : kbd_keys[i].lat;
	return 0;
}

bool kbd_get_event(struct kbd_event *e)
{
	bool ret = false;
	if (e != NULL){
		memset(e,0,sizeof(struct kbd_event));
		e->key=KEY_NONE;
		if (read(kbd,&e->key,sizeof(e->key)) == sizeof(e->key)){
			e->pressed = !(e->key & 0x80);
			e->key &= 0x7f;
			e->repeated=kbd_check_repeat(e);
			kbd_check_shift(e);
			e->shift_state=shift_state;
			kbd_check_language(e);
			e->ch=kbd_get_char(e->key);
			kbd_mark_key(e);
			kbd_last_click=time(NULL);
			ret = true;
		}
	}
	return ret;
}

bool kbd_wait_event(struct kbd_event *e)
{
	if (e != NULL){
		while (!kbd_get_event(e));
		return true;
	}else
		return false;
}

void kbd_flush_queue(void)
{
	struct kbd_event e;
	while (kbd_get_event(&e));
}

/* В shift_state присутствуют только указанные биты */
bool kbd_exact_shift_state(struct kbd_event *e, int state)
{
	if (e == NULL)
		return false;
	return (e->shift_state & state) && !(e->shift_state & ~state);
}

/*
 * Удаляет из очереди нажатых клавиш клавишу Ctrl. Это необходимо для 
 * переключения консолей (__CONSOLE_SWITCHING__)
 */
#if defined __CONSOLE_SWITCHING__
int strip_ctrl(void)
{
	int i, *p = pressed_keys, ret = 0;
	for (i = 0; i < ASIZE(pressed_keys); i++, p++){
		if ((*p == KEY_LCTRL) || (*p == KEY_RCTRL)){
			*p = 0;
			ret++;
		}
	}
	shift_state &= ~SHIFT_CTRL;
	return ret;
}
#endif

/* various sound functions */
static bool sound_on=false;

/* Устройство для управления динамиком через COM-порт */
static int sound_com = -1;

/* Установка линии DTR COM-порта */
static bool set_com_dtr(int fd, bool val)
{
	uint32_t lines = serial_get_lines(fd);
	if (val)
		lines &= ~TIOCM_DTR;
	else
		lines |= TIOCM_DTR;
	return serial_set_lines(fd, lines);
}

/* Включение динамика посредством DTR COM-порта */
static bool com_sound_on(void)
{
	bool ret = false;
	if (sound_com != -1)
		ret = set_com_dtr(sound_com, false);
	return ret;
}

/* Выключение динамика посредством DTR COM-порта */
static bool com_sound_off(void)
{
	bool ret = false;
	if (sound_com != -1)
		ret = set_com_dtr(sound_com, true);
	return ret;
}

/* Открытие устройства управления динамиком */
static bool open_sound_com(void)
{
	bool ret = false;
	sound_com = open(SPEAKER_DEV, O_RDONLY);
	if (sound_com != -1){
		com_sound_off();
		ret = true;
	}
	return ret;
}

/* Закрытие устройства управления динамиком */
static void close_sound_com(void)
{
	if (sound_com != -1){
		close(sound_com);
		sound_com = -1;
	}
}

void sound(unsigned __freq)
{
	if (kbd != -1){
		__freq = TIMER_FREQ_MHZ * 1000000.0 / __freq;
		ioctl(kbd, KIOCSOUND, __freq);
		com_sound_on();
		sound_on = true;
	}
}

void nosound(void)
{
	if (kbd != -1){
		ioctl(kbd, KIOCSOUND, 0);
		com_sound_off();
		sound_on = false;
	}
}

static void sound_callback(int __n)
{
	nosound();
	signal(SIGALRM,SIG_IGN);
}

void beep(unsigned __freq, unsigned __ms)
{
	if (!sound_on && (kbd != -1)){
		sound(__freq);
		if (__ms != 0){	/* 0 -- бесконечный звук */
			static struct itimerval tv={{0, 0}, {0, 0}};
			tv.it_value.tv_sec=__ms / 1000;
			tv.it_value.tv_usec=(__ms % 1000) * 1000;
			signal(SIGALRM, sound_callback);
			setitimer(ITIMER_REAL, &tv, NULL);
		}
	}
}

void beep_sync(unsigned __freq,unsigned __ms)
{
	if (kbd != -1){
		sound(__freq);
		delay(__ms);
		nosound();
	}
}

bool init_kbd(void)
{
	int i;
	kbd_lang=lng_rus;
	kbd_last_click=time(NULL);
	for (i=0; i < PKEY_BUF_LEN; pressed_keys[i++]=0);
	kbd = open(IO_TTY, O_RDONLY);
	if (kbd != -1){
		ioctl(kbd,KDGKBMODE,&oldkbmode);
		fcntl(kbd,F_SETFL,O_NONBLOCK);
		tcgetattr(kbd,&old);
		memcpy(&new,&old,sizeof(struct termios));
		new.c_iflag=0;
		new.c_lflag &= ~(ICANON | ECHO | ISIG);
		new.c_cc[VTIME]=1;
#if defined __CONSOLE_SWITCHING__
		new.c_cc[VINTR] = 0;
		new.c_cc[VQUIT] = 0;
#endif
		tcsetattr(kbd,TCSAFLUSH,&new);
		ioctl(kbd,KDSKBMODE,K_MEDIUMRAW);
		open_sound_com();
		return true;
	}else
		return false;
}

void release_kbd(void)
{
	close_sound_com();
	ioctl(kbd,KDSKBMODE,oldkbmode);
	tcsetattr(kbd,0,&old);
	fcntl(kbd,F_SETFL,O_SYNC);
	close(kbd);
	kbd=-1;
}
