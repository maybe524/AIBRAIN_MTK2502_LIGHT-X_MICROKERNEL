#pragma once

#define list_ui_handle(n)		for ((n) = 0; (n) < ARRAY_ELEM_NUM(ui_fhdls); (n)++)
#define UI_HANDLE_ITEM(ui_type, ui_handler, ui_region_info)	\
			{.type = ui_type, .handler = ui_handler, .region_info = ui_region_info}
#define UI_FILTREGION_INIT(name, x, y, w, h, f)	\
			static struct fb_regioninfo name = {	\
					.xreg = (x),		\
					.yreg = (y),		\
					.width	= (w),		\
					.height = (h),		\
					.flags = (f),	\
			}
#define UI_FILTREGION_INFO(name)		\
			&name
#define UI_LOG(fmt, argc...)	\
			printk("UI: " fmt, ##argc);
#define UI_GEN_STEP_ENTRY(s)	\
			if ((s) == ((struct ui_data *)data)->ui_filregion_info->step)

#define UI_SIGNAL_AREADY_FULLFILT		(1 << 0)

#define UI_MEMUS_REDRAW		(1 << 0)

#define UI_SHOWNONE		0x00
#define UI_SHOWMENU		0x01
#define UI_SHOWWATING	0x02
#define UI_SHOWTIME		0x03

#define UI_SHOWCONN		0x04
#define UI_SHOWDISCONN	0x05

#pragma anon_unions
#pragma pack(push)
#pragma pack(1)
struct ui_msgfmt {
	__u16 ui_msg_id;
	__u8  ui_type;
	__u8  ui_head[3];
	__u8  ui_colon;
	__u8  ui_comment[20];
};
#pragma pack(pop)

struct ui_data {
	struct task_struct *task;
	struct localtime *ltime;
	struct msg_msg *msg;
	struct ui_filregion *ui_filregion_info;
};

struct ui_filregion {
	__u8  type;
	__u8  step;
	__u32 time_recode;
	int (*handler)(void *data);

	__u8 flags;
	void *private;
	struct fb_regioninfo *region_info;
};

struct ui_menu {
	__u32 logo_idx;
	int (*handler)(void *data);

	int (*set_result)(struct ui_menu *menu, void *data);
	__u32 result[7];
	kd_bool result_isvalid;
};

#define UI_FBMEM_ISDIRTY	(1 << 0)

struct ui_fbmem {
	__u32 xreg, yreg, width, height;
	__u8 *fbmem;
	__u32 size;
	__u32 flags;
};

#define UI_FBMEM_INIT(fmname, x, y, w, h)		\
	static __u8 ____##fmname##ui_fbmem_buff[(w) * (h) * (1) / (8) + 100] = {0};	\
	static struct ui_fbmem fmname##ui_fbmem = {		\
					.xreg = (x),		\
					.yreg = (y),		\
					.width	= (w),		\
					.height = (h),		\
					.fbmem = ____##fmname##ui_fbmem_buff,	\
					.size = sizeof(____##fmname##ui_fbmem_buff),	\
		}
#define UI_FBMEM_INFO(fmname)	\
		&fmname##ui_fbmem