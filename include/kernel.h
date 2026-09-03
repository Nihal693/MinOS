#ifndef KERNEL_H
#define KERNEL_H

#include <avr/pgmspace.h>
#include <stdint.h>
#include <avr/boot.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/common.h>
#include "porg_memo.h"
#include "uart.h"
#include "systime.h"
#include "adc.h"


#define MAX_FILES 10
#define NAME_LEN 12
#define CONTENT_LEN 32
#define PATH_LEN 16
#define DMESG_LINES 6
#define DMESG_LEN 40

typedef void (*commandHandler)(const char* args);

typedef struct{
    char name[NAME_LEN];
    char content[CONTENT_LEN];
    char parentDir[PATH_LEN];
    uint8_t isDirectory;
    uint8_t active;
}RAMFile;

typedef struct{
    uint32_t timeStamp;
    char message[DMESG_LEN];
}DmesgEntry;

typedef struct{
    const char* name;
    commandHandler func;
}Command;


#define MAX_ALIASES 4
#define ALIAS_NAME_LEN 6
#define ALIAS_VAL_LEN 20

typedef struct{
    char name[ALIAS_NAME_LEN];
    char value[ALIAS_VAL_LEN];
    uint8_t active;
}AliasEntry;

/*typedef struct{
    uint8_t skip_execution;
    uint8_t loop_active;
    uint8_t 
}*/


uint16_t freeMemory();
void addDmesg(const char *msg);
void addDmesgRam(const char *msg);
void init_FS();
void printPrompt();
int8_t indexOf(const char* str, const char* substr);
uint16_t atoi_safe(const char* str);
void itoa_(char* out, uint8_t n);
void toLowercase(char* str);
uint8_t safeConcatPath(char* dest, const char* add);
void executeCommand(const char* line);
void runScript(const char* content);
void analogWrite(uint8_t pin, uint16_t duty);

#endif