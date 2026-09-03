#include "kernel.h"


RAMFile fs[MAX_FILES];
char currentPath[PATH_LEN] = "/";

DmesgEntry dmesg[DMESG_LINES];
uint8_t dmesgIndex = 0;

AliasEntry aliases[MAX_ALIASES];
char cmd[32] = "";



#define NUM_CMDS (sizeof(cmd_table) / sizeof(Command))

void (*reset_vector)(void) = 0;

extern uint8_t __heap_start;
extern uint8_t *__brkval;

uint16_t freeMemory(void) {
    uint16_t heap_top = (__brkval == 0) ? (uint16_t)&__heap_start : (uint16_t)__brkval;
    return SP - heap_top;
}

void addDmesg(const char *msg){
    if(dmesgIndex >= DMESG_LINES) dmesgIndex = 0;
    dmesg[dmesgIndex].timeStamp = sysTime() / 1000;
    strncpy_P(dmesg[dmesgIndex].message, msg, DMESG_LEN - 1);
    dmesg[dmesgIndex].message[DMESG_LEN - 1] = '\0';
    dmesgIndex++;
}

void addDmesgRam(const char *msg){
    if(dmesgIndex >= DMESG_LINES) dmesgIndex = 0;
    dmesg[dmesgIndex].timeStamp = sysTime() / 1000;
    strncpy(dmesg[dmesgIndex].message, msg, DMESG_LEN - 1);
    dmesg[dmesgIndex].message[DMESG_LEN - 1] = '\0';
    dmesgIndex++;
}

void init_FS(){
    uint8_t d, i;

    const char* dirs[2] = {"home", "dev"};
    for(d = 0; d < 2; d++){
        for(i = 0; i < MAX_FILES; i++){
            if(!fs[i].active){
                strncpy(fs[i].name, dirs[d], NAME_LEN - 1);
                fs[i].name[NAME_LEN - 1] = '\0';
                strncpy(fs[i].parentDir, "/", PATH_LEN - 1);
                fs[i].parentDir[PATH_LEN - 1] = '\0';
                fs[i].isDirectory = 1;
                fs[i].active = 1;
                break;
            }
        }
    }

    const char devPath[PATH_LEN] = "/dev/";
    const char* pins[3] = {"pin2", "pin3", "pin4"};
    for(d = 0; d < 3; d++){
        for(i = 0; i < MAX_FILES; i++){
            if(!fs[i].active){
                strncpy(fs[i].name, pins[d], NAME_LEN - 1);
                fs[i].name[NAME_LEN - 1] = '\0';
                strncpy(fs[i].parentDir, devPath, PATH_LEN - 1);
                fs[i].parentDir[PATH_LEN - 1] = '\0';
                fs[i].isDirectory = 0;
                fs[i].content[0] = '\0';
                fs[i].active = 1;
                break;
            }
        }
    }

    addDmesg(Pmem("Kernel initialised"));
    addDmesg(Pmem("File system mounted"));
    addDmesg(Pmem("Ready for commands"));
}

void printPrompt(){
    print_flash(Pmem("\033[1;32mroot@arduino:\033[0m"));
    print_flash(Pmem("\033[1;34m"));
    print(currentPath);
    print_flash(Pmem("# \033[0m"));
}

int8_t indexOf(const char* str,const char* substr){
    uint8_t slen = strlen(str), sublen = strlen(substr);
    for(uint8_t i = 0; i <= slen - sublen; i++){
        uint8_t match = 1;
        for(uint8_t j = 0; j < sublen; j++){
            if(str[i + j] != substr[j]){
                match = 0;
                break;
            }
        }
        if(match) return i;
    }
    return -1;
}

uint16_t atoi_safe(const char* str) {
  int num = 0;
  while (*str >= '0' && *str <= '9') {
    num = num * 10 + (*str - '0');
    str++;
  }
  return num;
}


void itoa_(char* out, uint8_t n){
    if(n >= 10){
        *(out) = n / 10 + '0';
        *(out + 1) = n % 10 + '0';
        *(out + 2) = '\0';
    }
    else{
        *out = n + '0';
        *(out + 1) = '\0';
    }
}

void toLowercase(char* str){
    while(*str){
        if(*str >= 'A' && *str <= 'Z') *str = *str - 'A' + 'a';
        str++;
    }
}

uint8_t safeConcatPath(char* dest, const char* add){
    uint8_t destLen = strlen(dest);
    uint8_t addLen = strlen(add);
    if(destLen + addLen + 2 >= PATH_LEN) return 0;
    strncat(dest, add, PATH_LEN - destLen - 1);
    strncat(dest, "/", PATH_LEN - strlen(dest) - 1);
    return 1;
}

void digital_init(){
        TCCR2A &= ~(1U << COM2B1);
        TCCR0A &= ~(1U << COM0B1);
        TCCR0A &= ~(1U << COM0A1);
        TCCR1A &= ~(1U << COM1A1);
        TCCR1A &= ~(1U << COM1B1);
        TCCR2A &= ~(1U << COM2A1);
}

void cmd_reboot(const char* args){
    print_flash(Pmem("Rebooting...\n"));
    addDmesg(Pmem("System reboot"));
    uint32_t lt = sysTime();
    while(1){
        if(sysTime() - lt >= 500){
            break;
        }
    }
    reset_vector();
}

void cmd_pinmode(const char* args){
    int8_t spc = indexOf(args, " ");
    char buff[40];
    if(spc == -1){ print_flash(Pmem("Usage: pinmode [pin] [in/out]\n")); return;}
    uint8_t pin = atoi_safe(args);
    if(pin < 2 || pin > 13){
        print_flash(Pmem("Use pins 2-13\n"));
        return;
    }

    char mode[8] = "";
    strncpy(mode, args + spc + 1, 7);
    mode[7] = '\0';
    toLowercase(mode);

    volatile uint8_t* ddr = (pin < 8)? &DDRD : &DDRB;    
    uint8_t bit = (pin < 8)? pin : (pin - 8);

    if(strcmp_P(mode, Pmem("out")) == 0){
        *ddr |= (1U << bit);
        snprintf_P(buff, sizeof(buff), Pmem("Pin %d is set to OUTPUT"), pin);
        addDmesgRam(buff);
        print_flash(Pmem("Pin set to OUTPUT\n"));
    }
    else if(strcmp_P(mode, Pmem("in")) == 0){
        volatile uint8_t* port = (pin < 8)? &PORTD : &PORTB;
        *ddr &= ~(1U << bit);
        *port |= (1U << bit);
        snprintf_P(buff, sizeof(buff), Pmem("Pin %d is set to INPUT"), pin);
        addDmesgRam(buff);
        print_flash(Pmem("Pin set to INPUT\n"));
    }
    
}

void cmd_write(const char* args){
    int8_t spc = indexOf(args, " ");
    char buff[40];
    uint8_t v = 0;
    if(spc == -1){print_flash(Pmem("Usage: write [pin] [high/low]\n")); return;}
    uint8_t pin = atoi_safe(args);
    if(pin < 2 || pin > 13){
        print_flash(Pmem("Use pins 2-13\n"));
        return;
    }
    digital_init();
    char val[8] = "";
    strncpy(val, args + spc +1, 7);
    val[7] = '\0';
    toLowercase(val);

    volatile uint8_t *port = (pin < 8)? &PORTD : &PORTB;
    uint8_t bit = (pin < 8)? pin : (pin - 8);

    if(strcmp_P(val, Pmem("high")) == 0){
        *port |= (1u << bit);
        v = 1;
    }
    else{
        *port &= ~(1U << bit);
    }
    snprintf_P(buff, sizeof(buff), Pmem("Pin %d wrote %s"), pin,  ((v)? "HIGH" : "LOW"));
    addDmesgRam(buff);
    print_flash(Pmem("Write OK\n"));

}

void cmd_read(const char* args){
    uint8_t pin = atoi_safe(args);
    char buff[40];
    volatile uint8_t *pin_ = (pin < 8)? &PIND : &PINB;
    uint8_t bit = (pin < 8)? pin : (pin - 8);
    uint8_t val = (*pin_ >> bit) & 1U;
    print_flash(Pmem("Pin ")); print(args);
    print_flash(Pmem(" value: ")); print_char('0' + val); print_char('\n');
    snprintf_P(buff, sizeof(buff), Pmem("Pin %d read %d"), pin, val);
    addDmesgRam(buff);
}

void cmd_gpio(const char* args){
    int8_t spc = indexOf(args, " ");
    char buff[40];
    if(spc == -1){
        print_flash(Pmem("Usage: gpio [pin] [on/off/toggle] or gpio vixa [count]\n")); return;
    }
    volatile uint8_t *ddr, *port, *pin_reg;
    uint8_t bit;
    char pinStr[8] = "";
    strncpy(pinStr, args, spc);
    pinStr[spc] = '\0';
    char action[8] = "";
    strncpy(action, args + spc + 1, 7);
    action[7] = '\0';
    toLowercase(action);
    digital_init();

    if(strcmp_P(pinStr, Pmem("vixa")) == 0){
        uint8_t count = atoi_safe(action);
        if(count <= 0) count = 10;
        uint8_t cycle, p;
        DDRD = 0xFC;
        DDRB = 0x3F;
        for(cycle = 0; cycle < count; cycle++){
            for(p = 2; p <= 13; p++){
                port = (p < 8)? &PORTD : &PORTB;
                bit = (p < 8)? p : (p - 8);

                *port |= (1u << bit);
                uint32_t lastTime = sysTime();
                while(1){
                    if(sysTime() - lastTime >= 50) break;
                }
                *port &= ~(1U << bit);
            }
        }
        print_flash(Pmem("Chain blinking finished\n"));
        addDmesg(Pmem("Chain blink complete"));
    }
    else{
        uint8_t pin = atoi_safe(pinStr);
        ddr = (pin < 8)? &DDRD : &DDRB;
        port = (pin < 8)? &PORTD : &PORTB;
        pin_reg = (pin < 8)? &PIND : &PINB;
        bit = (pin < 8)? pin : (pin - 8);
        *ddr |= (1U << bit);

        if(strcmp_P(action, Pmem("on")) == 0){
            *port |= (1U << bit);
            snprintf_P(buff, sizeof(buff), Pmem("GPIO %d ON"), pin);
            addDmesgRam(buff);
            print_flash(Pmem("GPIO ")); print(pinStr); print_flash(Pmem(" ON\n"));
        }
        else if(strcmp_P(action, Pmem("off")) == 0){
            *port &= ~(1U << bit);
            snprintf_P(buff, sizeof(buff), Pmem("GPIO %d OFF"), pin);
            addDmesgRam(buff);
            print_flash(Pmem("GPIO ")); print(pinStr); print_flash(Pmem(" OFF\n"));
        }
        else if(strcmp_P(action, Pmem("toggle")) == 0){
            *pin_reg = (1U << bit);
            snprintf_P(buff, sizeof(buff), Pmem("GPIO %d TOGGLED"), pin);
            addDmesgRam(buff);
            print_flash(Pmem("GPIO ")); print(pinStr); print_flash(Pmem(" TOGGLED\n"));
        }
    }
}

void cmd_ls(const char* args){
    uint8_t i, empty = 1;
    for(i = 0; i < MAX_FILES; i++){
        if(fs[i].active && (strcmp(fs[i].parentDir,currentPath) == 0)){
            if(fs[i].isDirectory) print_flash(Pmem("\033[33m"));
            print(fs[i].name);
            if(fs[i].isDirectory) print_flash(Pmem("/\033[0m"));
            empty = 0;
            print_flash(Pmem("  "));
        }
    }
    if(empty) print_flash(Pmem("(empty)"));
    print_char('\n');
}

void cmd_mkdir(const char* args){
    uint8_t i, full = 1;
    for(i = 0; i < MAX_FILES; i++){
        if(!(fs[i].active)){
            strncpy(fs[i].name, args, NAME_LEN - 1);
            fs[i].name[NAME_LEN - 1] = '\0';
            strncpy(fs[i].parentDir, currentPath, PATH_LEN - 1);
            fs[i].parentDir[PATH_LEN - 1] = '\0';
            fs[i].isDirectory = (strcmp_P(cmd, Pmem("mkdir")) == 0);
            fs[i].content[0] = '\0';
            fs[i].active = 1;
            print_flash(Pmem("OK\n"));
            full = 0;
            break;  
        }
    }
    if(full) print_flash(Pmem("No space\n"));
}

void cmd_cd(const char* args){
    if((strcmp_P(args, Pmem("..")) == 0) || (strcmp_P(args, Pmem("/")) == 0)){
        strncpy(currentPath, "/", PATH_LEN - 1);
        currentPath[PATH_LEN - 1] = '\0';
        return;
    }
    else{
        uint8_t i, found = 0;
        for(i = 0; i < MAX_FILES; i++){
            if((strcmp(fs[i].name, args) == 0) &&
               (strcmp(currentPath, fs[i].parentDir) == 0) &&
                fs[i].isDirectory && fs[i].active){
                if(!safeConcatPath(currentPath, args)){
                    strncpy(currentPath, "/", PATH_LEN - 1);
                    currentPath[PATH_LEN - 1] = '\0';
                    print_flash(Pmem("Path too long\n"));
                    return; 
                }
                found = 1;
                break;
            }
        }
        if(!found) print_flash(Pmem("No dir\n"));
    }
}

void cmd_pwd(const char* args){
    print(currentPath);
    print_char('\n');
}

void cmd_echo(const char* args){
    int8_t sign = indexOf(args, " > ");
    char name[NAME_LEN] = "";
    char* txt = (char*)args;
    
    if(sign == -1){
        print(args); print_char('\n'); return;
    }
    else{
        strncpy(name, args + sign + 3, NAME_LEN - 1);
        name[NAME_LEN - 1] = '\0';
        txt[sign] = '\0';
        uint8_t i, found = 0;
        for(i = 0; i < MAX_FILES; i++){
            if(fs[i].active && (!fs[i].isDirectory)
                && (strcmp(fs[i].name, name) == 0) &&
                (strcmp(fs[i].parentDir, currentPath) == 0)){
                    strncpy(fs[i].content, txt, CONTENT_LEN - 1);
                    fs[i].content[CONTENT_LEN - 1] = '\0';
                    print_flash(Pmem("Saved\n"));
                    found = 1;
                    if((strcmp_P(fs[i].parentDir, Pmem("/dev/")) == 0) && (strncmp_P(name, Pmem("pin"), 3) == 0)){
                        uint8_t pin = atoi_safe(name + 3), val = (txt[0] == '1')? 1 : 0;
                        char buff[40];
                        digital_init();
                        volatile uint8_t* ddr = (pin < 8)? &DDRD : &DDRB;
                        volatile uint8_t* port = (pin < 8)? &PORTD : &PORTB;
                        uint8_t bit = (pin < 8)? pin : (pin - 8);

                        *ddr |= (1U << bit);
                        if(val){
                            *port |= (1U << bit);
                        }
                        else{
                            *port &= ~(1U << bit);
                        }
                        snprintf_P(buff, sizeof(buff), Pmem("GPIO %d set %d via echo"), pin, val);
                        addDmesgRam(buff);
                    }
                    break;
            }
        }
        if(!found) print_flash(Pmem("File not found\n"));

    }
}

void cmd_cat(const char* args){
    uint8_t i, found = 0;
    for(i = 0; i < MAX_FILES; i++){
        if(fs[i].active && (!fs[i].isDirectory) &&
           (strcmp(fs[i].name, args) == 0) && (strcmp(fs[i].parentDir, currentPath) == 0)){
            print(fs[i].content);
            print_char('\n');
            found = 1;
            break;
        }
    }
    if(!found) print_flash(Pmem("File not found\n"));

}

void cmd_info(const char* args){
    uint8_t i, found = 0;
    for(i = 0; i < MAX_FILES; i++){
        if(fs[i].active && (strcmp(fs[i].parentDir, currentPath) == 0) && (strcmp(fs[i].name, args) == 0)){
            char num[3];
            itoa_(num, strlen(fs[i].content));
            print_flash(Pmem("Name: ")); print(fs[i].name); print_char('\n');
            print_flash(Pmem("Type: ")); print_flash(((fs[i].isDirectory)? Pmem("Directory\n") : Pmem("File\n")));
            print_flash(Pmem("Size: ")); print(num); print_flash(Pmem(" byte\n"));
            found = 1;
            break;
        }
    }
    if(!found) print_flash(Pmem("File not found\n"));
}

void cmd_rm(const char* args){
    uint8_t i, found = 0;
    for(i = 0; i < MAX_FILES; i++){
        if(fs[i].active && (strcmp(fs[i].name, args) == 0) && (strcmp(fs[i].parentDir, currentPath) == 0)){
            if(fs[i].isDirectory){
                char dirPath[PATH_LEN];
                snprintf(dirPath, sizeof(dirPath), "%s%s/", currentPath, args); 
                uint8_t j;
                for(j = 0; j < MAX_FILES; j++){
                    if(fs[j].active && (strcmp(dirPath, fs[j].parentDir) == 0)){
                        fs[j].active = 0;
                    }
                }
            }
            found = 1;
            fs[i].active = 0;
            print_flash(Pmem("Removed\n")); break;
        }
    }
    if(!found) print_flash(Pmem("File not found\n"));
}

void cmd_dmesg(const char* args){
    char num[5];
    for(uint8_t i = 0; i < DMESG_LINES; i++){
        if(dmesg[i].message[0] != '\0'){
            itoa(dmesg[i].timeStamp, num, 10);
            print_flash(Pmem("["));
            print(num);
            print_flash(Pmem("]"));
            print(dmesg[i].message); print_char('\n');
        }
    }
}

void cmd_uptime(const char* args){
    char num[3];
    uint32_t s = sysTime() / 1000;          
    uint8_t h = (s / 3600);
    itoa_(num, h);
    print(num);print_flash(Pmem("h"));
    uint8_t m = (s % 3600) / 60;
    itoa_(num, m);
    print(num);print_flash(Pmem("m"));
    uint8_t sec = s % 60;
    itoa_(num, sec);
    print(num);print_flash(Pmem("s"));print_char('\n');
    addDmesg(Pmem("Uptime command"));
}

void cmd_free(const char* args){
    char num[5];
    itoa((freeMemory()), num, 10);
    print_flash(Pmem("Free RAM: "));
    print(num);
    print_flash(Pmem(" bytes\n"));
}

void cmd_whoami(const char* args){
    print_flash(Pmem("root\n"));
}

void cmd_uname(const char* args){
    char num[5];
    itoa(freeMemory(), num, 10);
    print_flash(Pmem("MinOS v1.0\n"));
    print_flash(Pmem("Hardware: Arduino UNO\n"));
    print_flash(Pmem("RAM: "));
    print(num); print_flash(Pmem(" bytes free\n"));
}

void cmd_clear(const char* args){
    uint8_t i;
    for(i = 0; i < 30; i++){
        print_char('\n');
    }
}

void cmd_sh(const char* args){
    if(args[0] == '\0'){
        print_flash(Pmem("Usage: sh [script]\n")); return;
    }uint8_t i, found = 0;
    for(i = 0; i < MAX_FILES; i++){
        if(fs[i].active && (!fs[i].isDirectory) &&
           (strcmp(fs[i].parentDir, currentPath) == 0) && (strcmp(fs[i].name, args) == 0)){
            found = 1;
            addDmesg(Pmem("Running script"));
            runScript(fs[i].content);
            break;          
        }
    }
    if(!found) print_flash(Pmem("Script not found\n"));
}

void cmd_pwm(const char* args){
    int8_t spc = indexOf(args, " ");
    if(spc == -1){
        print_flash(Pmem("Usage: pwm [pin] [duty]")); return;
    }
    uint16_t duty;
    uint8_t pin;
    char buf[40];
    char num[8] = "";
    strncpy(num, args, spc);
    num[spc] = '\0';
    pin = atoi_safe(num);
    strncpy(num, args + spc + 1, 7);
    num[7] = '\0';
    duty = atoi_safe(num);
    analogWrite(pin, duty);
    //if(duty == 65000) print("hi\n");
    snprintf_P(buf, sizeof(buf), Pmem("PWM pin %d set %u"), pin, duty);
    addDmesgRam(buf);
    print(buf); print_char('\n');
}

void cmd_alias(const char* args){
    if(args[0] == '\0'){
        uint8_t i, any = 0;
        for(i = 0; i < MAX_ALIASES; i++){
            if(aliases[i].active){
                print(aliases[i].name);
                print_flash(Pmem("='"));
                print(aliases[i].value);
                print_flash(Pmem("'\n"));
                any = 1;
            }
        }
        if(!any){ print_flash(Pmem("No aliases\n")); return;}
    }
    else{
        int8_t sign = indexOf(args, "=");
        if(sign == -1){
            uint8_t found = 0;
            for(uint8_t i = 0; i < MAX_ALIASES; i++){
                if(aliases[i].active && (strcmp(aliases[i].name, args) == 0)){
                    print(args); print_flash(Pmem("='")); print(aliases[i].value); print_flash(Pmem("'\n"));
                    found = 1;
                    break;
                }
            }
            if(!found) print_flash(Pmem("No such aliases\n"));
        }
        else{
            int8_t slot = -1;
            char aname[ALIAS_NAME_LEN] = "";
            char aval[ALIAS_VAL_LEN] = "";
            strncpy(aname, args, sign < ALIAS_NAME_LEN ? sign : ALIAS_NAME_LEN - 1);
            aname[ALIAS_NAME_LEN - 1] = '\0';
            strncpy(aval, args + sign + 1, ALIAS_VAL_LEN - 1);
            aval[ALIAS_VAL_LEN - 1] = '\0';
            uint8_t i;
            for(i = 0; i < MAX_ALIASES; i++){
                if(aliases[i].active && (strcmp(aliases[i].name, aname) == 0)){slot = i; break;}
            }
            if(slot == -1){
                for(i = 0; i < MAX_ALIASES; i++){
                    if(!aliases[i].active){
                        slot = i; break;
                    }
                }
            }
            if(slot == -1){print_flash(Pmem("Alias table full\n")); return;}
            strncpy(aliases[slot].name, aname, ALIAS_NAME_LEN);
            //aliases[slot].name[ALIAS_NAME_LEN - 1] = '\0';
            strncpy(aliases[slot].value, aval, ALIAS_VAL_LEN);
            //aliases[i].value[ALIAS_VAL_LEN - 1] = '\0';
            aliases[slot].active = 1;
            print_flash(Pmem("Alias set\n"));
        }
    }
}

void cmd_slots(const char* args){
    uint8_t i, num = 0;
    char chs[4] = "";
    for(i = 0; i < MAX_FILES; i++){
        if(fs[i].active) num++;
    }
    itoa_(chs, num);
    print_flash(Pmem("("));
    print(chs);
    print_flash(Pmem("/ 10)\n"));
}

void cmd_find(const char* args){
    uint8_t i, found = 0;
    for(i = 0; i < MAX_FILES; i++){
        if(fs[i].active && (strcmp(fs[i].name, args) == 0)){
            print(fs[i].parentDir);
            print(fs[i].name); print_char('\n');
            found = 1;
        }
    }
    if(!found) print_flash(Pmem("No such file or directory\n"));
}

void cmd_help(const char* args){
    print_flash(Pmem("Commands: ls, cd, pwd, mkdir, touch, cat, echo, rm, info\n"));
    print_flash(Pmem("          pinmode, write, read, gpio, pwm, sh\n"));
    print_flash(Pmem("          uptime, uname, dmesg, df, free, whoami, clear, reboot\n"));
    print_flash(Pmem("          alias, slots, find, adc, temp\n"));
    print_flash(Pmem("GPIO: gpio [pin] on/off/toggle  |  gpio vixa [count]\n"));
    print_flash(Pmem("SH:   sh [file]  -- run script (use ; as line separator)\n"));
}

void cmd_adc(const char* args){
    if(args[0] == '\0'){
        print_flash(Pmem("Usage: adc [pin]\n")); return;
    }
    uint8_t pin; uint16_t adc_val;
    pin = atoi_safe(args);
    adc_val = adc_read(pin);
    float v_val = ((5.0) / (1023.0)) * adc_val;
    char num[5] = "";
    dtostrf(v_val, 4, 2, num);
    print_flash(Pmem("ADC"));print(args);print_flash(Pmem("="));print(num);print_flash(Pmem("V\n"));
}

void cmd_delay(const char* args){
    uint32_t lastTime = sysTime();
    uint16_t delay = atoi_safe(args);
    while(1){
        if(sysTime() - lastTime >= delay) break;
    }
}

/*void cmd_temp(const char* args){
    uint16_t adc_val;
    adc_val = adc_read(8);
    uint8_t ts_offset = boot_signature_byte_get(0x0002);
    uint8_t ts_gain = boot_signature_byte_get(0x0003);
    float temp = (((adc_val - (373 - ts_offset))* 128) / (float)ts_gain) + 25;
    char num[10] = "";
    char num1[5] = "";
    itoa(ts_offset, num, 10);
    itoa(ts_gain, num1, 10);
    print(num);print_char('\n');print(num1);print_char('\n');
    dtostrf(temp, 4, 2, num);
    print_flash(Pmem("Chip temp: "));print(num);print_flash(Pmem(" degree Celcius\n"));
}*/

void cmd_temp(const char* args){
    uint16_t v_val = adc_read(0);
    float R_th = 10000.0f * ((1023.0f / (float)v_val) - 1.0f);
    float temp = logf(R_th / 10000.0f);
    temp /= 3950.0f;
    temp += 1.0f / 298.15f;
    if(temp == 0) return;
    temp = 1.0f / temp;
    temp -= 273.15f;
    char num[8] = "";
    dtostrf(temp, 4, 2, num);
    print_flash(Pmem("The temperature is "));print(num);print_flash(Pmem(" degree Celsius\n"));
}


const char name_reboot[] prog = "reboot";
const char name_pinmode[] prog = "pinmode";
const char name_write[] prog = "write";
const char name_read[] prog = "read";
const char name_gpio[] prog = "gpio";
const char name_ls[] prog = "ls";
const char name_mkdir[] prog = "mkdir";
const char name_touch[] prog = "touch";
const char name_cd[] prog = "cd";
const char name_pwd[] prog = "pwd";
const char name_echo[] prog = "echo";
const char name_cat[] prog = "cat";
const char name_info[] prog = "info";
const char name_rm[] prog = "rm";
const char name_dmesg[] prog = "dmesg";
const char name_uptime[] prog = "uptime";
const char name_df[] prog = "df";
const char name_free[] prog = "free";
const char name_whoami[] prog = "whoami";
const char name_uname[] prog = "uname";
const char name_clear[] prog = "clear";
const char name_sh[] prog = "sh";
const char name_pwm[] prog = "pwm";
const char name_alias[] prog = "alias";
const char name_slots[] prog = "slots";
const char name_find[] prog = "find";
const char name_help[] prog = "help";
const char name_adc[] prog = "adc";
const char name_delay[] prog = "delay";
const char name_temp[] prog = "temp";

const Command cmd_table[] prog = {
    {name_reboot, cmd_reboot},
    {name_pinmode, cmd_pinmode},
    {name_write, cmd_write},
    {name_read, cmd_read},
    {name_gpio, cmd_gpio},
    {name_ls, cmd_ls},
    {name_mkdir, cmd_mkdir},
    {name_touch, cmd_mkdir},
    {name_cd, cmd_cd},
    {name_pwd, cmd_pwd},
    {name_echo, cmd_echo},
    {name_cat, cmd_cat},
    {name_info, cmd_info},
    {name_rm, cmd_rm},
    {name_dmesg, cmd_dmesg},
    {name_uptime, cmd_uptime},
    {name_df, cmd_free},
    {name_free, cmd_free},
    {name_whoami, cmd_whoami},
    {name_uname, cmd_uname},
    {name_clear, cmd_clear},
    {name_sh, cmd_sh},
    {name_pwm, cmd_pwm},
    {name_alias, cmd_alias},
    {name_slots, cmd_slots},
    {name_find, cmd_find},
    {name_help, cmd_help},
    {name_adc, cmd_adc},
    {name_delay, cmd_delay},
    {name_temp, cmd_temp}
};

void executeCommand(const char* line){
    char args[32] = "";

    char* lineSpace = strchr(line, ' ');
    uint8_t cmdLen = lineSpace - line;
    if(lineSpace != NULL){
        //uint8_t cmdLen = lineSpace - line;
        strncpy(cmd, line, cmdLen);
        cmd[cmdLen] = '\0';
        strncpy(args, lineSpace + 1, 31);
        args[31] = '\0';
    }
    else{
        strncpy(cmd, line, 31);
        cmd[31] = '\0';
    }

    toLowercase(cmd);
    if((cmd[0] == '\0')) return;
    
    uint8_t i;
    for(i = 0; i < NUM_CMDS; i++){
        Command c;
        memcpy_P(&c, &cmd_table[i], sizeof(Command));

        if(strcmp_P(cmd, c.name) == 0){
            c.func(args);
            return;
        }
    }
    uint8_t found = 0;
    for(i = 0; i < MAX_ALIASES; i++){
        if(aliases[i].active && (strcmp(aliases[i].name, cmd) == 0)){
            if(args[0] == '\0'){
                executeCommand(aliases[i].value);
            }
            else{
                strncpy(cmd, aliases[i].value, cmdLen);
                cmd[cmdLen] = '\0';
                strncat(cmd, lineSpace, (31 - cmdLen));
                executeCommand(cmd);
            }
            found = 1;
            break;
        }
    }
    if(!found) print_flash(Pmem("Unknown Command\n"));
    return;
}   

void runScript(const char* content){
    // check for a esc seq to use for exiting from execution since while 1 will loop forever
    uint8_t ci = 0, li = 0, lineNum = 0;
    char line[32], num[3];
    char ch;
    uint8_t len = strlen(content);

    while(ci <= len){
        if(UCSR0A & (1U << RXC0)){
            char ch_key = recieve();
            if(ch_key == 0x03 || ch_key == 0x1B){
                print_flash(Pmem("\nProgram terminated\n")); return;
            }
        }

        ch = (ci < len)? content[ci] : ';';
        ci++;
        if(ch == '\n' || ch == ';'){
            if(li > 0){
                if(strncmp_P(line, Pmem("fi"), 2) == 0){}
                if(strncmp_P(line, Pmem("if "), 3) == 0){}
                if(strncmp_P(line, Pmem("while "), 6) == 0){}
                if(strncmp_P(line, Pmem("done"), 4) == 0){}

                line[li] = '\0';
                lineNum++;
                itoa(lineNum, num, 10);
                print_flash(Pmem("[sh:")); print(num); print_flash(Pmem("] "));
                print(line); print_char('\n');
                executeCommand(line); li = 0;
            }
        }
        else{
            if(li < 31) line[li++] = ch;
        }
    }
    addDmesg(Pmem("sh: script done"));
    print_flash(Pmem("[sh] done\n"));
}

void analogWrite(uint8_t pin, uint16_t duty){
    switch(pin){
        case 3:
        DDRD |= (1U << DD3);
        OCR2B = (0xFF & duty);
        TCCR2A |= (1U << COM2B1);
        break;
        case 5:
        DDRD |= (1U << DD5);
        OCR0B = (0xFF & duty);
        TCCR0A |= (1U << COM0B1);
        break;
        case 6:
        DDRD |= (1U << DD6);
        OCR0A = (0xFF & duty);
        TCCR0A |= (1U << COM0A1);
        break;
        case 9:
        DDRB |= (1U << DD1);
        OCR1A = (0x03FF & duty);
        TCCR1A |= (1U << COM1A1);
        break;
        case 10:
        DDRB |= (1U << DD2);
        OCR1B = (0x03FF & duty);
        TCCR1A |= (1U << COM1B1);
        break;
        case 11:
        DDRB |= (1U << DD3);
        OCR2A = (0XFF & duty);
        TCCR2A |= (1U << COM2A1);
        break;
    }
}

