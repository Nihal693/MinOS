#include <stdint.h>
#include "kernel.h"


char inputBuffer[64] = "";
uint8_t inputLen = 0;


int main(void){

    uart_init();
    adc_init();
    timer0_init();
    timer1_init();
    timer2_init();
    init_FS();
    uint32_t lastTime = sysTime();

    while(1){
        if(sysTime() - lastTime >= 5000) break;
    }
    print("\n\033[31m--- MinOS v1.0 ---\033[0m\n");
    print("Type '\033[36mhelp\033[0m' for commands\n");
    printPrompt();
    
    char ch = 0, lastChar = 0;
    uint8_t newLine = 0;

    while(1){

        if(UCSR0A & (1U << RXC0)){
            ch = recieve();
            if(ch == '\n'){
                if(inputLen > 0){
                    inputBuffer[inputLen - 1] = '\0';
                    print("\n");
                    executeCommand(inputBuffer);
                    inputLen = 0;
                    memset(inputBuffer, 0, 64);
                    printPrompt();
                }
                else{
                    print("\n");
                    printPrompt();
                }
            }
            else if(ch == 8 || ch == 127){
                if(inputLen > 0){
                    if(newLine) inputLen++;
                    if(lastChar == '\n'){
                        newLine = 1; lastChar = '\b';
                    }
                    else{newLine = 0;}
                    inputLen--;
                    inputBuffer[inputLen] = '\0';
                    print_flash(Pmem("\b \b"));
                }
            }
            else if(inputLen < 63){
                print_char(ch);
                if(lastChar == 92 && ch == 110){
                    inputLen--;
                    ch = '\n';
                }
                if(newLine && ch == 110){ch = '\n'; newLine = 0;}
                inputBuffer[inputLen] = ch;
                inputLen++;
                lastChar = ch;
            }
        }
    }
   return 0;
}

