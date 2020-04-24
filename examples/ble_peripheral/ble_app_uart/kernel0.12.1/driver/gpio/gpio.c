#include <include/gpio.h>
#include <include/errno.h>

LIST_HEAD(__gpio_board_list);

static struct gpio_board_info *gpio_match_board_info(__u32 pin)
{
	struct list_head *iter;

	list_for_each(iter, &__gpio_board_list) {
		struct gpio_board_info *board_info = container_of(iter, struct gpio_board_info, list);
		if (board_info->rang_begin <= pin && pin <= board_info->rang_end)
			return board_info;
	}

	return NULL;
}

int gpio_set_mode(__u32 pin, __u8 mode)
{
	return 0;
}

int gpio_set_dir(__u32 pin, __u8 dir, __u32 config)
{
	struct gpio_board_info *board_info = gpio_match_board_info(pin);

	if (board_info)
		return board_info->g_op->set_dir(pin, dir, config);

	return -ENOTSUPP;
}

int gpio_set_out(__u32 pin, __u8 out)
{
	struct gpio_board_info *board_info = gpio_match_board_info(pin);

	if (board_info)
		return board_info->g_op->set_out(pin, out);

	return -ENOTSUPP;
}

int gpio_get_val(__u32 pin)
{
	struct gpio_board_info *board_info = gpio_match_board_info(pin);

	if (board_info)
		return board_info->g_op->get_val(pin);

	return -ENOTSUPP;
}

int gpio_register_board_info(struct gpio_board_info *info, unsigned n)
{
	struct list_head *iter;

	if (!info || !info->g_op)
		return -ENOTSUPP;

	list_for_each(iter, &__gpio_board_list) {
		struct gpio_board_info *board_info = container_of(iter, struct gpio_board_info, list);
		if (!kstrcmp(info->name, board_info->name) || 
				info->rang_begin == board_info->rang_begin || 
				info->rang_end == board_info->rang_end)
			return -EBUSY;
	}

	list_add(&info->list, &__gpio_board_list);

	return 0;
}
