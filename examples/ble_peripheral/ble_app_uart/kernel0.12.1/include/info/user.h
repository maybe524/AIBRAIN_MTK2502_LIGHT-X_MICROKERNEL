#pragma once

struct user_info {
	__u8 id;
	__u8 name[16];
	__u8 age;
	__u8 sex;
	__u8 height;
	__u32 weight;
	__u8 birthday[11];
};