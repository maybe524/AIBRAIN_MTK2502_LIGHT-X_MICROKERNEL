
int pager_init(void)
{
	pager_alarm_init();
	pager_ui_init();
	pager_file_manager_init();

	return 0;
}
