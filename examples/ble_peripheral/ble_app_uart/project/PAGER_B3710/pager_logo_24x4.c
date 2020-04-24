#include <include/types.h>

// gImage_20x27_1__0
struct logo_info {
	__u32 idx;
	__u32 len;
	const __u8 *data;
};

const unsigned char gImage_waiting_white[12] = { /* 0X00,0X01,0X18,0X00,0X04,0X00, */
0XFE,0X00,0X7F,0XFC,0X00,0X3F,0XFC,0X00,0X3F,0XFE,0X00,0X7F,};

const unsigned char gImage_waiting_black[12] = { /* 0X00,0X01,0X18,0X00,0X04,0X00, */
0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,};

static const struct logo_info logo_list[] = 
{
	{'WT1W', 12, gImage_waiting_white},
	{'WT1B', 12, gImage_waiting_black},
};


const __u8 *logo_get24x4_1(int idx, __u32 *len)
{
	int i;

	for (i = 0; i < ARRAY_ELEM_NUM(logo_list); i++) {
		if (idx == logo_list[i].idx) {
			if (len)
				*len = logo_list[i].len;
			return logo_list[i].data;
		}
	}

	return NULL;
}

