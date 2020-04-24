#include <include/task.h>
#include <include/stdio.h>
#include <include/errno.h>
#include <include/sched.h>
#include <include/lock.h>
#include <include/fb.h>
#include <include/msg.h>
#include <include/assert.h>

#include "./inc/pager_time.h"
#include "./inc/pager_ui.h"
#include "./inc/pager_measure.h"

#define UI_FBMEMSIZE		(6 * 24)

static kd_bool ui_showtime_needupdate = kd_false, ui_allregion_needupdate = kd_false;
static __u32 ui_result_bakup[7] = {0};
static __u8 ui_showchage_logoidx = 0;
static __u8 ui_filregion_cnt = 0;
static __u8 ui_menu_curridx = 0;
static __u8 ui_current_show = 0;
static __u8 ui_showconn_logo = 0;
static __u8 ui_showwait_logoidx = 0, ui_showwait_logotype = 0, ui_showwait_timeout = 0;

UI_FBMEM_INIT(ui_showmain, 8, 4, 71, 180);
UI_FILTREGION_INIT(regioninfo_showmain, 8, 12, 71, 19, FB_REGION_FLAGS_USEBUFF);

extern int ui_filregion_notify(void);
struct msg_msg *task_check_msg(struct task_struct *task);
static int ui_get_temperature_result(void *data);

static int ui_filregion_infbmem(struct ui_fbmem *ufbmem, __u8 *src, __u32 x, __u32 y, __u32 w, __u32 h)
{
	__u32 h2, w2;
	__u32 membase = x + y * ((ufbmem->width + 1) / 8);
	__u32 srccnt = 0;

	if (membase + w * h / 8 > ufbmem->size)
		return -EINVAL;

	for (h2 = 0; h2 < h; h2++) {
		for (w2 = 0; w2 < w / 8; w2++) {
			ufbmem->fbmem[membase + (w2 + h2 * ((ufbmem->width + 1) / 8))] = src[srccnt];
			srccnt++;
		}
	}

	return 0;
}

static const __u8 *ui_font_get(__u32 style, __u32 *len)
{
	switch (style >> 8 & 0xFFFF) {
		case 'NU':
		case 'SU':
			return (const __u8 *)font_get72x20_1(style & 0xFF, len);
		case 'NL':
		case 'SL':
			return (const __u8 *)font_get65x15_1(style & 0xFF, len);
	}

	return NULL;
}

static int ui_filregion_info2fbmem(struct fb_regioninfo *fbinfo, struct ui_fbmem *ufbmem)
{
	if (!fbinfo || !ufbmem)
		return -EINVAL;

	fbinfo->xreg = ufbmem->xreg;
	fbinfo->yreg = ufbmem->yreg;
	fbinfo->width  = ufbmem->width;
	fbinfo->height = ufbmem->height;
	fbinfo->buff = ufbmem->fbmem;

	return 0;
}

static int ui_filregion_tofbdev(struct fb_regioninfo *uinfo)
{
	int fd, ret;

	fd = open("/dev/fb0", O_WRONLY);
	if (fd < 0)
		return fd;

	ret = ioctl(fd, FB_IOCTRL_FILREGION, (__u32)uinfo);
	ret = ioctl(fd, FB_IOCTRL_SYNC, (__u32)uinfo);
	close(fd);

	ui_filregion_cnt++;
	return 0;
}

int ui_update_showtime(void)
{
	ui_showtime_needupdate = kd_true;

	return 0;
}

static int ui_get_temperature_result(void *data)
{
	float value;

	get_temperature_value(&value);

	//ui_menulist[ui_menu_curridx].result_isvalid = kd_true;
	UI_LOG("value = %x\r\n", value);
	return 0;
}

static int ui_get_bhrr_result(void *data)
{
	int ret;

	ret = mesure_start_up("bhrr");
	return 0;
}

static int ui_set_bhrr_result(struct ui_menu *menu, void *data)
{
	int ret;
	struct bhrr_msg_s *result = (struct bhrr_msg_s *)data;

	menu->result[0] = 'NU' << 8 | '0' + (result->h_bp / 100) & 0xFF;
	menu->result[1] = 'NU' << 8 | '0' + (result->h_bp % 100 / 10) & 0xFF;
	menu->result[2] = 'NU' << 8 | '0' + (result->h_bp % 100 % 10) & 0xFF;
	menu->result[3] = 'SL' << 8 | '/';
	menu->result[4] = 'NL' << 8 | '0' + (result->l_bp / 100) & 0xFF;
	menu->result[5] = 'NL' << 8 | '0' + (result->l_bp % 100 / 10) & 0xFF;
	menu->result[6] = 'NL' << 8 | '0' + (result->l_bp % 100 % 10) & 0xFF;

	menu->result_isvalid = kd_true;
	return 0;
}

static struct ui_menu ui_menulist[] =
{
	{.logo_idx = 'LXXL'},
	{.logo_idx = 'XYCL', .handler = ui_get_bhrr_result, .set_result = ui_set_bhrr_result},
	{.logo_idx = 'JSXL'},
	{.logo_idx = 'HXPL'},
	{.logo_idx = 'BS01'},
	{.logo_idx = 'WDHS', .handler = ui_get_temperature_result},
	{.logo_idx = 'WDCS', .handler = ui_get_temperature_result},
};

int ui_set_result(int result_idx, void *data)
{
	int i, ret = 0;

	for (i = 0; i < ARRAY_ELEM_NUM(ui_menulist); i++) {
		if (result_idx == ui_menulist[i].logo_idx && ui_menulist[i].set_result)
			ret = ui_menulist[i].set_result(&ui_menulist[i], data);
	}

	return ret;
}

static int ui_update_mesureresult(struct ui_menu *umenu, __u8 flags)
{
	int fd, ret, i;
	const __u8 *reschar;
	__u32 reslen;
	__u8 showtype = 0;
	struct ui_fbmem *ufbmem = UI_FBMEM_INFO(ui_showmain);
	int font_w, font_h;
	int fbmem_base, font_gap, font_some;
	__u16 fonttype = 0;

	reschar = (const __u8 *)logo_getother('RDR1', &reslen);
	kmemset(ufbmem->fbmem, 0xFF, reslen + 8 * (ufbmem->width + 1) / 8);
	for (i = 0; i < 7; i++) {
		if (!(flags & UI_MEMUS_REDRAW) && ui_result_bakup[i] == umenu->result[i])
			continue;
		ui_result_bakup[i] = umenu->result[i];
		fonttype = umenu->result[i] >> 8 & 0xFFFF;
		if (fonttype == 'NU' || fonttype == 'SU') {
			font_w = 72;
			font_h = 20;
			fbmem_base = 1;
			font_gap = 3;
			font_some = 7;
		}
		else if (fonttype == 'NL' || fonttype == 'SL') {
			font_w = 65;
			font_h = 15;
			fbmem_base = 3;
			font_gap = 2;
			font_some = 8;
		}
		else
			continue;
		reschar = ui_font_get(umenu->result[i], &reslen);
		ui_filregion_infbmem(ufbmem, (__u8 *)reschar, 0, (fbmem_base + i * font_gap) * font_some, font_w, font_h);
	}

	return 0;
}

static int ui_showtime_handle(void *data)
{
	int fd, ret, id;
	__u8 tbuf[5] = {0};
	struct ui_filregion *uhdl;
	struct localtime *ltim, time_test = {0};
	struct ui_data *udat;
	const __u8 *numchar;
	__u32 numlen;
	struct ui_fbmem *ufbmem = UI_FBMEM_INFO(ui_showmain);
	__u8 ui_showtime_updatecount = 1;

	udat = (struct ui_data *)data;
	uhdl = udat->ui_filregion_info;
	ltim = udat->ltime;

UI_GEN_STEP_ENTRY(0) {
		if (!ltim)
			return TSLEEP;
		else if (ui_current_show == UI_SHOWNONE) {
			ui_current_show = UI_SHOWTIME;
			ui_showtime_updatecount = 2;
			uhdl->step++;
		}
		else if (uhdl->flags & UI_SIGNAL_AREADY_FULLFILT) {
			if (ui_current_show == UI_SHOWTIME)
				uhdl->step++;
			uhdl->flags &= ~UI_SIGNAL_AREADY_FULLFILT;
		}
		else if (ui_showtime_needupdate) {
			uhdl->step++;
			ui_showtime_needupdate = kd_false;
		}
		else if (ltim->Second == 0)
			uhdl->step++;
		else
			return TSLEEP;
	}

UI_GEN_STEP_ENTRY(1) {
		tbuf[0] = ltim->Hour / 10 + '0';
		tbuf[1] = ltim->Hour % 10 + '0';
		tbuf[2] = ':';
		tbuf[3] = ltim->Minute / 10 + '0';
		tbuf[4] = ltim->Minute % 10 + '0';

		numchar = (const __u8 *)logo_getother('RDR1', &numlen);
		ui_filregion_info2fbmem(uhdl->region_info, ufbmem);
		kmemset(ufbmem->fbmem, 0xFF, numlen + (8 * (ufbmem->width + 1) / 8));
		for (id = 0; id < 5; id++) {
			numchar = (const __u8 *)font_get72x20_1((__u8)tbuf[id], &numlen);
			kmemcpy((__u8 *)uhdl->region_info->buff + 8 * ((uhdl->region_info->width + 1) / 8) + \
					24 * id * (uhdl->region_info->width + 1) / 8,
					numchar,
					numlen);
		}
		for (id = 0; id < ui_showtime_updatecount; id++) {
			ret = ui_filregion_tofbdev(uhdl->region_info);
			if (ret)
				return TSLEEP;
		}

		uhdl->step = TASK_STEP(0);
	}

	return TSLEEP;
}
UI_FILTREGION_INIT(regioninfo_showtime, 8, 12, 71, 19, FB_REGION_FLAGS_USEBUFF);

static int ui_showpower_handle(void *data)
{
	int ret;
	struct ui_filregion *uhdl;
	struct ui_data *udat;
	struct ui_msgfmt *umsg;
	struct msg_msg *mmsg;
	const __u8 *chargechar;
	__u32 chargelen;
	struct ui_fbmem *ufbmem = UI_FBMEM_INFO(ui_showmain);

	udat = (struct ui_data *)data;
	mmsg = udat->msg;
	umsg = (struct ui_msgfmt *)mmsg->msg_comment;
	uhdl = udat->ui_filregion_info;

UI_GEN_STEP_ENTRY(0) {
		if (ui_current_show != UI_SHOWTIME && uhdl->flags & UI_SIGNAL_AREADY_FULLFILT)
			uhdl->flags &= ~UI_SIGNAL_AREADY_FULLFILT;
		else if (ui_current_show == UI_SHOWTIME && uhdl->flags & UI_SIGNAL_AREADY_FULLFILT) {
			uhdl->step++;
			uhdl->flags &= ~UI_SIGNAL_AREADY_FULLFILT;
		}
		else if (ui_current_show != UI_SHOWTIME)
			uhdl->step = 20;
		else if (bq25120_check_charging())
			uhdl->step++;
		else
			return TSLEEP;
		ui_showchage_logoidx = 0;
	}

UI_GEN_STEP_ENTRY(1) {
		chargechar = (const __u8 *)logo_get24x24_1('CHG' << 8 | (ui_showchage_logoidx + '0'), &chargelen);

		ui_filregion_info2fbmem(uhdl->region_info, ufbmem);
		ui_filregion_infbmem(ufbmem, (__u8 *)chargechar, 41, 132 + 8 + 1, 24, 24);
		ret = ui_filregion_tofbdev(uhdl->region_info);
		if (ret)
			return TSLEEP;

		ui_showchage_logoidx++;
		if (ui_showchage_logoidx > 5)
			ui_showchage_logoidx = 0;
		
		uhdl->step++;
		uhdl->time_recode = 0;
	}

UI_GEN_STEP_ENTRY(2) {
		if (!bq25120_check_charging())
			uhdl->step = 0;
		else {
			if (!uhdl->time_recode)
				uhdl->time_recode = get_systime();
			if (get_systime() - uhdl->time_recode >= Second(3))
				uhdl->step = 1;
			return TSLEEP;
		}
	}

UI_GEN_STEP_ENTRY(20) {
		if (ui_current_show == UI_SHOWTIME)
			uhdl->step = 1;
		else
			return TSLEEP;
	}

	return TSLEEP;
}
UI_FILTREGION_INIT(regioninfo_showpower, 48, 148, 23, 23, FB_REGION_FLAGS_USEBUFF);

static int ui_showmenu_handle(void *data)
{
	int ret;
	struct ui_filregion *uhdl;
	struct localtime *ltim;
	struct ui_data *udat;
	struct ui_msgfmt *umsg;
	struct msg_msg *mmsg;
	const __u8 *menuchar;
	__u32 menulen;
	struct ui_fbmem *ufbmem = UI_FBMEM_INFO(ui_showmain);
	kd_bool ui_menu_isshowresult = kd_false;

	udat = (struct ui_data *)data;
	uhdl = udat->ui_filregion_info;
	mmsg = udat->msg;
	ltim = (struct localtime *)uhdl->private;

ui_showmenu:
UI_GEN_STEP_ENTRY(00) {
		if (mmsg) {
			umsg = (struct ui_msgfmt *)mmsg->msg_comment;
			if (!kstrncmp("SINGLE", umsg->ui_comment, mmsg->msg_len)) {
				ui_current_show = UI_SHOWMENU;
				uhdl->step = 01;
			}
			else if (!kstrncmp("DOUBLE", umsg->ui_comment, mmsg->msg_len)) {
				ui_current_show = UI_SHOWWATING;
				uhdl->step = 30;
			}
			task_del_msg(udat->task, udat->msg);
		}
		else if (ui_current_show == UI_SHOWMENU || ui_current_show == UI_SHOWWATING) {
			if (!uhdl->time_recode)
				uhdl->time_recode = get_systime();
			if (get_systime() - uhdl->time_recode >= Second(10)) {
				ui_current_show = UI_SHOWNONE;
				uhdl->time_recode = 0;
			}
			else if (uhdl->flags & UI_SIGNAL_AREADY_FULLFILT) {
				uhdl->step = 10;
				uhdl->flags &= ~UI_SIGNAL_AREADY_FULLFILT;
			}
			else
				return TSLEEP;
		}
		else if (ui_current_show != UI_SHOWMENU && uhdl->flags & UI_SIGNAL_AREADY_FULLFILT)
			uhdl->flags &= ~UI_SIGNAL_AREADY_FULLFILT;
		else
			return TSLEEP;
	}

UI_GEN_STEP_ENTRY(01) {
		if (ui_menu_curridx >= ARRAY_ELEM_NUM(ui_menulist))
			ui_menu_curridx = 0;
		uhdl->step = 10;
	}

UI_GEN_STEP_ENTRY(10) {
		menuchar = (const __u8 *)logo_get24x24_1(ui_menulist[ui_menu_curridx].logo_idx, &menulen);

		/* Menus logo update*/
		ui_filregion_info2fbmem(uhdl->region_info, ufbmem);
		ui_filregion_infbmem(ufbmem, (__u8 *)menuchar, 41, 132 + 8, 24, 24);

		/* Result number update */
		ui_update_mesureresult(&ui_menulist[ui_menu_curridx], UI_MEMUS_REDRAW);	
		if (!ui_menu_isshowresult)
			ui_menu_curridx++;
		uhdl->step++;
	}

UI_GEN_STEP_ENTRY(11) {
		ret = ui_filregion_tofbdev(uhdl->region_info);
		if (ret)
			return TSLEEP;
		uhdl->step = 0;
		ui_menu_isshowresult = kd_false;
	}

UI_GEN_STEP_ENTRY(30) {
		if (!ui_menulist[ui_menu_curridx - 1].handler) {
			uhdl->step = 0;
			return TSLEEP;
		}
		ui_menulist[ui_menu_curridx - 1].result_isvalid = kd_false;
		ret = ui_menulist[ui_menu_curridx - 1].handler(data);
		ui_showwait_logoidx = 0;

		menuchar = (const __u8 *)logo_getother('RDR1', &menulen);
		kmemset(ufbmem->fbmem, 0xFF, menulen + (8 * (ufbmem->width + 1) / 8));
		uhdl->step++;
		uhdl->time_recode = 0;
	}

UI_GEN_STEP_ENTRY(31) {
		if (mmsg) {
			umsg = (struct ui_msgfmt *)mmsg->msg_comment;
			if (!kstrncmp("DOUBLE", umsg->ui_comment, mmsg->msg_len)) {
				uhdl->step = 0;
				UI_LOG("Cancel measure!!!\r\n");
			}
			task_del_msg(udat->task, udat->msg);
		}
		else if (!ui_menulist[ui_menu_curridx - 1].result_isvalid) {
			if (!uhdl->time_recode)
				uhdl->time_recode = get_systime();
			if (get_systime() - uhdl->time_recode < Second(1))
				return TSLEEP;
			uhdl->time_recode = 0;
			ui_current_show = UI_SHOWWATING;

			menuchar = (void *)logo_get24x4_1(ui_showwait_logotype ? 'WT1B' : 'WT1W', NULL);

			ui_filregion_info2fbmem(uhdl->region_info, ufbmem);
			ui_filregion_infbmem(ufbmem, (__u8 *)menuchar, 12, 10 + ui_showwait_logoidx * 12, 24, 4);
			ret = ui_filregion_tofbdev(uhdl->region_info);
			if (ret)
				return TSLEEP;
			ui_showwait_timeout++;
			ui_showwait_logoidx++;
			if (ui_showwait_logoidx >= 10) {
				ui_showwait_logoidx = 0;
				ui_showwait_logotype = ui_showwait_logotype ? 0 : 1;
			}
			if (ui_showwait_timeout > Second(20)) {
				ui_showwait_timeout = 0;
				uhdl->step = 0;
				ui_current_show = UI_SHOWNONE;
			}
			return TSLEEP;
		}
		else {
			ui_menu_curridx--;
			ui_menu_isshowresult = kd_true;
			uhdl->step = TASK_STEP(10);
			goto ui_showmenu;
		}
	}

	return TSLEEP;
}
UI_FILTREGION_INIT(regioninfo_showmenu, 48, 148, 23, 23, FB_REGION_FLAGS_USEBUFF);

static int ui_showconnection_handle(void *data)
{
	int ret;
	struct ui_filregion *uhdl;
	struct ui_data *udat;
	kd_bool is_connected;
	const __u8 *connchar;
	__u32 connlen;
	int conn_logoidx;
	struct ui_fbmem *ufbmem = UI_FBMEM_INFO(ui_showmain);

	udat = (struct ui_data *)data;
	uhdl = udat->ui_filregion_info;
	is_connected = app_nus_ble_isconnected();

UI_GEN_STEP_ENTRY(0) {
		if (uhdl->flags & UI_SIGNAL_AREADY_FULLFILT) {
			uhdl->step++;
			uhdl->flags &= ~UI_SIGNAL_AREADY_FULLFILT;
		}
		else if (is_connected && ui_showconn_logo != UI_SHOWCONN) {
			ui_showconn_logo = UI_SHOWCONN;
			uhdl->step++;
		}
		else if (!is_connected && ui_showconn_logo != UI_SHOWDISCONN) {
			ui_showconn_logo = UI_SHOWDISCONN;
			uhdl->step++;
		}
		else
			return TSLEEP;
	}

UI_GEN_STEP_ENTRY(1) {
		if (ui_showconn_logo == UI_SHOWCONN)
			conn_logoidx = 'CONN';
		else if (ui_showconn_logo == UI_SHOWDISCONN)
			conn_logoidx = 'DISC';
		connchar = (void *)logo_get24x24_1(conn_logoidx, &connlen);

		ui_filregion_info2fbmem(uhdl->region_info, ufbmem);
		ui_filregion_infbmem(ufbmem, (__u8 *)connchar, 37, 132 + 8 + 1, 24, 24);
		ret = ui_filregion_tofbdev(uhdl->region_info);
		if (ret)
			return TSLEEP;
		uhdl->step = 0;
	}

	return TSLEEP;
}
UI_FILTREGION_INIT(regioninfo_showconnection, 16, 148, 23, 23, FB_REGION_FLAGS_USEBUFF);

static int ui_autorefresh_handle(void *data)
{
	int ret;
	struct ui_filregion *uhdl;
	struct ui_data *udat;
	struct ui_msgfmt *umsg;
	struct msg_msg *mmsg;
	struct ui_fbmem *ufbmem = UI_FBMEM_INFO(ui_showmain);
	struct fb_regioninfo *fbinfo;

	udat = (struct ui_data *)data;
	mmsg = udat->msg;
	umsg = (struct ui_msgfmt *)mmsg->msg_comment;
	uhdl = udat->ui_filregion_info;

UI_GEN_STEP_ENTRY(0) {
		if (ui_filregion_cnt > 5) {
			if (!uhdl->time_recode)
				uhdl->time_recode = get_systime();
			if (get_systime() - uhdl->time_recode >= Second(5)) {
				uhdl->time_recode = 0;
				uhdl->step = 10;
			}
		}
		else
			return TSLEEP;
	}

UI_GEN_STEP_ENTRY(10) {
		__u32 chargelen;
		struct full_update_buff {
			const __u8 *buff_r10;
			const __u8 *buff_r13;
		} full_update;

		full_update.buff_r10 = (const __u8 *)logo_get24x24_1('B110', &chargelen);
		full_update.buff_r13 = (const __u8 *)logo_get24x24_1('B113', &chargelen);
		
		uhdl->region_info->buff = (void *)&full_update;
		ret = ui_filregion_tofbdev(uhdl->region_info);
		if (ret)
			return TSLEEP;
		ui_filregion_cnt = 0;
		ui_filregion_notify();
		uhdl->step = 0;
	}

	return TSLEEP;
}
UI_FILTREGION_INIT(regioninfo_refresh_background,
		0, 0, 88, 148,
		FB_REGION_FLAGS_USEBUFF | FB_REGION_FLAGS_UPDATEFULL);

static struct ui_filregion ui_fhdls[] =
{
	UI_HANDLE_ITEM(0x04, ui_autorefresh_handle, UI_FILTREGION_INFO(regioninfo_refresh_background)),
	UI_HANDLE_ITEM(0x01, ui_showtime_handle, UI_FILTREGION_INFO(regioninfo_showtime)),
	UI_HANDLE_ITEM(0x02, ui_showmenu_handle, UI_FILTREGION_INFO(regioninfo_showmenu)),
	UI_HANDLE_ITEM(0x03, ui_showpower_handle, UI_FILTREGION_INFO(regioninfo_showpower)),
	UI_HANDLE_ITEM(0x05, ui_showconnection_handle, UI_FILTREGION_INFO(regioninfo_showconnection)),
};


static int ui_filregion_notify(void)
{
	int idx;

	list_ui_handle(idx)
		ui_fhdls[idx].flags |= UI_SIGNAL_AREADY_FULLFILT;

	return 0;
}

static unsigned char __sched
	ui_thread(struct task_struct *task, void *data)
{
	int idx, ret, gtr;
	struct localtime ltim;
	struct msg_msg *msg;

	if (check_lock(&task->lock))
		return TSLEEP;
	lock(&task->lock);

	msg = task_check_msg(task);
	gtr = get_localtime(&ltim);
	list_ui_handle(idx) {
		struct ui_filregion *uihld = &ui_fhdls[idx];
		struct ui_data uidat = {0};

		if (!uihld->handler)
			continue;

		if (!gtr)
			uidat.ltime = &ltim;

		if (msg) {
			struct ui_msgfmt *ui_msg = (struct ui_msgfmt *)msg->msg_comment;
			if (ui_msg->ui_type == uihld->type)
				uidat.msg = msg;
		}

		uidat.task = task;
		uidat.ui_filregion_info = uihld;

		ret = uihld->handler((void *)&uidat);
	}

	unlock(&task->lock);

	return TSLEEP;
}
TASK_INIT(CMD_UI, ui_thread, kd_false, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_HIGH);
MSG_USER_INIT(ui_msg_user, CMD_UI, TASK_INFO(ui_thread));

int pager_ui_init(void)
{
	int fd, ret, idx;
	struct ui_fbmem *ufbmem = UI_FBMEM_INFO(ui_showmain);

	kmemset(ufbmem->fbmem, 0xFF, ufbmem->size);

	fd  = open("/dev/fb0", O_WRONLY);
	list_ui_handle(idx) {
		if (ui_fhdls[idx].region_info) {
			ret = ioctl(fd, FB_IOCTRL_REGISTER_REGIONINFO, (__u32)ui_fhdls[idx].region_info);
			if (ret < 0)
				UI_LOG("Register regioninfo fail, ret = %d\r\n", ret);
		}
	}
	close(fd);

	for (idx = 0; idx < ARRAY_ELEM_NUM(ui_menulist); idx++) {
		for (int result_idx = 0; result_idx < 7; result_idx++) {
			if (result_idx > 2) {
				if (result_idx == 3)
					ui_menulist[idx].result[result_idx] = 'NL' << 8 | '-';
				else
					ui_menulist[idx].result[result_idx] = 'NL' << 8 | '-';
			}
			else
				ui_menulist[idx].result[result_idx] = 'NU' << 8 | '-';
			ui_menulist[idx].result_isvalid = kd_true;
		}
	}

	ret = sched_task_create(TASK_INFO(ui_thread), SCHED_COMMON_TYPE, EXEC_TRUE);
	ret = msg_user_register(MSG_USER_INFO(ui_msg_user));

	return 0;
}
