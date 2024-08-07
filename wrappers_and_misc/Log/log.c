// log.c
#include "log.h"

// Уровень логирования
const int MyLogLevel = LOG_LEVEL_DEBUG; // Замените на нужный уровень

// Мьютекс для логирования
SemaphoreHandle_t logMutex=NULL;

void initLogger()
{
    printf("initLogger\n");
    logMutex = xSemaphoreCreateMutex();
    if (logMutex == NULL)
    {
        // Обработка ошибки создания мьютекса
        printf("Failed to create log mutex\n");
        while (1); // Остановить выполнение, если не удалось создать мьютекс
    }
}
