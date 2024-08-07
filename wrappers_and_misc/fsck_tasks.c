#include "global.h"
#include "fsck.h"
#include "log.h"
#include "fsck_tasks.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sdcard_lowlevel_ops.h"

static void FSCK_SDcard_task(void);
static void FSCK_run_task(void *param);

/* Структура для передачи параметров в задачу run_fsck */
typedef struct {
    const char *path;
    bool *status;
	TaskHandle_t task_to_notify;
} fsck_param_t;

/* Запуск проверок фс
 */
void FSCK_start(void)
{
	//SysState.Fs_sys_ok = true; // временная отключалка проверок - проверка не работает
	//SysState.Fs_data_ok = true;
	//return;

	// Запуск проверки файловой системы
	xTaskCreate((TaskFunction_t)FSCK_SDcard_task,
				(const char *)"FSCK_sd",
				(uint16_t)4096,
				(void *)NULL,
				(UBaseType_t)1,
				&Task_h.FSCK_sd_task);
}
static void run_fsck(const char *device, bool *fs_ok_flag)
{
	logI("Run fsck on device %s", device);
	const char *const argv[] = {
		"fsck.exfat", // argv[0] - имя программы
		"-p",		  // argv[1] - первый аргумент
		"-s",		  // argv[2] - второй аргумент
		"-v",		  // argv[3] - третий аргумент
		"-v",
		"-v",
		device,
		NULL // массив должен заканчиваться NULL
	};

	// Количество аргументов (argc)
	int argc = sizeof(argv) / sizeof(argv[0]) - 1;

	SysState.SD_checking = true;

	int ret = fsck_exfat_entry_point(argc, argv);

	SysState.SD_checking = false;

	switch (ret)
	{
		case 0:
			logI("FSCK success on device %s\n", device);
			*fs_ok_flag = true;
			break;

		case 1:
			logI("FSCK successfully corrected errors on device %s\n", device);
			*fs_ok_flag = true;
			break;

		default:
			logE("FSCK failed on device %s code %d\n", device, ret);
			break;
	}
}


/* Задача проверки файловой системы двух разделов на карточке.
 * Карточка должна быть отмонтирована
 */
static void FSCK_SDcard_task(void){
    TaskHandle_t fsck_sys_task_handle;
    TaskHandle_t fsck_dat_task_handle;

    // Ожидаем, пока SD карта будет готова
    Waitfor(SysState.SD_drive_ok);

    // Включаем возможность записи в MBR
    sdcard_mbr_write_enable();

    // Параметры для задач fsck
    fsck_param_t sys_param = {"/sys", &SysState.Fs_sys_ok, xTaskGetCurrentTaskHandle()};
    fsck_param_t dat_param = {"/dat", &SysState.Fs_data_ok, xTaskGetCurrentTaskHandle()};

    // Создание задачи fsck для /sys
	Set_Operation(OP_CHECKING_SYS, 0);
    xTaskCreate(FSCK_run_task, "FSCK_sys", 4096, &sys_param, 1, &fsck_sys_task_handle);

    // Ожидание завершения задачи fsck для /sys
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Создание задачи fsck для /dat
	Set_Operation(OP_CHECKING_DAT, 0);
    xTaskCreate(FSCK_run_task, "FSCK_dat", 4096, &dat_param, 1, &fsck_dat_task_handle);

    // Ожидание завершения задачи fsck для /dat
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Отключаем возможность записи в MBR
    sdcard_mbr_write_disable();

	Set_Operation_End;
	
    // Удаляем текущую задачу
    vTaskDelete(NULL);
}

/* Задача, выполняющая run_fsck */
static void FSCK_run_task(void *param)
{
    fsck_param_t *fsck_param = (fsck_param_t *)param;

    // Выполнение run_fsck
    run_fsck(fsck_param->path, fsck_param->status);

    // Уведомление родительской задачи о завершении
    xTaskNotifyGive(fsck_param->task_to_notify);

    // Удаляем текущую задачу
    vTaskDelete(NULL);
}