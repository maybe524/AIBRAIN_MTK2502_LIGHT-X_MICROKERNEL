#pragma once

#pragma anon_unions
#pragma pack(push)
#pragma pack(1)
struct bhrr_msg_s {
	__u8	zone;
	__u32	timestamp;
	__u16	h_bp;
	__u16	l_bp;
	__u8	mesure_mode;
	__u8	person_status;
	__u8	temp1;
	__u8	temp2;
	__u8	reserve[4];
};

static struct mesure_packet {
	__u8	idx;
	__u8	key;
	__u8	len;
	union ___comm {
		__u8	val[17];
		struct bhrr_msg_s bhrr_msg;
	} comm;
};
#pragma pack(pop)

