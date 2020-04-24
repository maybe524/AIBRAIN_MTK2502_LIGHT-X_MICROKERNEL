#pragma once

enum file_idx{
	FIDX_MESURE_HISTORY = 0x01,
	FIDX_BLE_LOCAL_LOG,
	FIDX_MT2502_CAM_RAWDAT,
	FIDX_MT2502_GRAY_RAWDAT,
	FIDX_MAX,
};

struct file_manager_list {
	int fid;
	char *fname;
};