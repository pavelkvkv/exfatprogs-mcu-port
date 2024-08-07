// logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

void initLogger();

// Уровни логирования
#define LOG_LEVEL_NO  -1
#define LOG_LEVEL_VERBOSE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_ERROR 4

// Глобальная константа для уровня логирования
extern const int MyLogLevel;
extern SemaphoreHandle_t logMutex;

// Макрос для получения текущего времени в формате "HH:MM:SS:mmmm"
#define CURRENT_TIME() ({ \
    TickType_t ticks = xTaskGetTickCount(); \
    unsigned long ms = ticks % 1000; \
    unsigned long seconds = (ticks / 1000) % 60; \
    unsigned long minutes = (ticks / 60000) % 60; \
    unsigned long hours = (ticks / 3600000) % 24; \
    static char buf[16]; \
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu:%03lu", hours, minutes, seconds, ms); \
    buf; \
})

// Макросы для логирования
#ifndef TAG
#define TAG __FILE__
#endif

#define logV(fmt, ...) \
    do { if (MyLogLevel <= LOG_LEVEL_VERBOSE) { \
        xSemaphoreTake(logMutex, portMAX_DELAY); \
        printf("V (%s) %s: (%s) " fmt "\n", CURRENT_TIME(), TAG, __FUNCTION__, ##__VA_ARGS__); \
        xSemaphoreGive(logMutex); \
    }} while (0)

#define logD(fmt, ...) \
    do { if (MyLogLevel <= LOG_LEVEL_DEBUG) { \
        xSemaphoreTake(logMutex, portMAX_DELAY); \
        printf("D (%s) %s: (%s) " fmt "\n", CURRENT_TIME(), TAG, __FUNCTION__, ##__VA_ARGS__); \
        xSemaphoreGive(logMutex); \
    }} while (0)

#define logI(fmt, ...) \
    do { if (MyLogLevel <= LOG_LEVEL_INFO) { \
        xSemaphoreTake(logMutex, portMAX_DELAY); \
        printf("I (%s) %s: (%s) " fmt "\n", CURRENT_TIME(), TAG, __FUNCTION__, ##__VA_ARGS__); \
        xSemaphoreGive(logMutex); \
    }} while (0)

#define logW(fmt, ...) \
    do { if (MyLogLevel <= LOG_LEVEL_WARN) { \
        xSemaphoreTake(logMutex, portMAX_DELAY); \
        printf("W (%s) %s: (%s) " fmt "\n", CURRENT_TIME(), TAG, __FUNCTION__, ##__VA_ARGS__); \
        xSemaphoreGive(logMutex); \
    }} while (0)

#define logE(fmt, ...) \
    do { if (MyLogLevel <= LOG_LEVEL_ERROR) { \
        xSemaphoreTake(logMutex, portMAX_DELAY); \
        printf("E (%s) %s: (%s) " fmt "\n", CURRENT_TIME(), TAG, __FUNCTION__, ##__VA_ARGS__); \
        xSemaphoreGive(logMutex); \
    }} while (0)

#endif // LOGGER_H

