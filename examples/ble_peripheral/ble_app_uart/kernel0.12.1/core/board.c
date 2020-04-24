#include "include/stdio.h"
#include "include/string.h"
#include "include/board.h"
#include "include/errno.h"
#include "include/uart/uart.h"

module_extern(mem_normal_init);
module_extern(ramfs_init);
module_extern(devfs_init);
module_extern(ltxfs_init);
module_extern(nrfs_nand_init);
module_extern(mtdblock_init);
module_extern(populate_rootfs_init);
module_extern(buffer_init);
module_extern(nrfs_i2c_master_init);
module_extern(nrfs_irq_init);
module_extern(shell_init);
module_extern(nrfs_drv_uart_init);
module_extern(sched_init);
module_extern(misc_init);
module_extern(nrfs_spi_init);
module_extern(nrfs_gpio_init);
module_extern(fbmem_init);
module_extern(systime_init);
module_extern(msg_init);
module_extern(nrfs_fbmem_init);
module_extern(nrfs_display_init);
module_extern(nrfs_sched_init);
module_extern(nrfs_button_init);

__INIT_DRV__ INIT_CALL_LEVEL(2) =
{
	/*
	 *	1) kernel system init
	 */
	module_include(systime_init),
	module_include(sched_init),

	module_include(mem_normal_init),
	module_include(ramfs_init),
	module_include(buffer_init),
	module_include(devfs_init),
	module_include(ltxfs_init),
	module_include(populate_rootfs_init),
	module_include(mtdblock_init),
	module_include(fbmem_init),
	module_include(msg_init),

	/*
	 *	2) nrfs platform board init
	 */
	module_include(nrfs_sched_init),
	module_include(nrfs_irq_init),
	module_include(nrfs_gpio_init),
	module_include(nrfs_nand_init),
	module_include(nrfs_drv_uart_init),
	module_include(nrfs_i2c_master_init),
	module_include(nrfs_spi_init),
	module_include(nrfs_display_init),
	module_include(nrfs_fbmem_init),
	module_include(nrfs_button_init),

	/*
	 *	3) misc device init
	 */
	module_include(misc_init),
	module_include(shell_init),
	NULL,
};

