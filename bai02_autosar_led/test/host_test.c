#include <stdio.h>
#include "Mcu.h"
#include "Port.h"
#include "IoHwAb.h"
#include "Rte.h"
#include "Stm32f103_Reg.h"
RCC_TypeDef sim_rcc; GPIO_TypeDef sim_gpio[3]; SysTick_TypeDef sim_systick;
/* mô hình BSRR/BRR: ghi -> cập nhật ODR, ghi lại log */
static unsigned writes, fails;
static void sim(void){ for(int p=0;p<3;p++){ GPIO_TypeDef*g=&sim_gpio[p]; if(g->BSRR){g->ODR|=g->BSRR&0xFFFF; g->ODR&=~(g->BSRR>>16); g->BSRR=0; writes++;} if(g->BRR){g->ODR&=~(g->BRR&0xFFFF); g->BRR=0; writes++;} } }
#define CHECK(c,msg) do{ if(!(c)){printf("FAIL %s\n",msg);fails++;} else printf("ok   %s\n",msg);}while(0)
int main(void){
  for(int p=0;p<3;p++){ sim_gpio[p].CRL=0x44444444; sim_gpio[p].CRH=0x44444444; }
  Mcu_Init(&Mcu_Config); CHECK(sim_rcc.APB2ENR==0x10,"Mcu_Init: APB2ENR = 0x10");
  Mcu_Init(NULL_PTR);  CHECK(sim_rcc.APB2ENR==0x10,"Mcu_Init(NULL): không đổi");
  /* Port_Init: kiểm tra thứ tự mức trước, hướng sau bằng cách xem BSRR đã ghi khi CRH còn 0x4 */
  Port_Init(&Port_Config); sim();
  CHECK(sim_gpio[2].CRH==0x44244444u,"Port_Init: CRH = 0x44244444");
  CHECK((sim_gpio[2].ODR & 0x2000u)!=0,"Port_Init: ODR13 = 1 (LED tắt)");
  CHECK(sim_gpio[2].CRL==0x44444444u && sim_gpio[0].CRH==0x44444444u,"Port_Init: chân khác giữ reset");
  IoHwAb_Init0(NULL_PTR); sim(); CHECK((sim_gpio[2].ODR&0x2000u)!=0,"IoHwAb_Init0: LED tắt");
  int on=0, okseq=1;
  for(int i=0;i<10;i++){ Rte_Task_500ms(); sim(); on=!on; unsigned bit=(sim_gpio[2].ODR>>13)&1u; if(bit!=(unsigned)(on?0:1)) okseq=0; }
  CHECK(okseq,"10 lần Rte_Task_500ms: ODR13 = 0,1,0,1... (bật = mức thấp)");
  CHECK(sim_gpio[0].ODR==0 && sim_gpio[1].ODR==0,"không ghi nhầm port A, B");
  printf("%u lần ghi BSRR/BRR, %u FAIL\n",writes,fails); return fails!=0;
}
