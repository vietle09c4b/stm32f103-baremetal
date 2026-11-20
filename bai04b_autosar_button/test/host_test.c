/* host_test.c - kiểm thử MCAL + IoHwAb + RTE + SWC trên máy tính (thanh ghi thay bằng biến RAM). */
#include <stdio.h>
#include "Mcu.h"
#include "Port.h"
#include "IoHwAb.h"
#include "IoHwAb_Cfg.h"
#include "Rte.h"
#include "Rte_ButtonLed.h"
#include "Stm32f103_Reg.h"
RCC_TypeDef sim_rcc; GPIO_TypeDef sim_gpio[3]; SysTick_TypeDef sim_systick;
static unsigned fails;
/* BSRR/BRR -> ODR; IDR: PA0 = mức chân do test đặt, PC13 = ODR13 */
static int pa0_pin = 1;
static void sim(void){ for(int p=0;p<3;p++){ GPIO_TypeDef*g=&sim_gpio[p];
    if(g->BSRR){g->ODR|=g->BSRR&0xFFFF; g->ODR&=~(g->BSRR>>16); g->BSRR=0;}
    if(g->BRR){g->ODR&=~(g->BRR&0xFFFF); g->BRR=0;} }
  sim_gpio[0].IDR = (sim_gpio[0].IDR & ~1u) | (unsigned)pa0_pin; }
#define CHECK(c,msg) do{ if(!(c)){printf("FAIL %s\n",msg);fails++;} else printf("ok   %s\n",msg);}while(0)
static int led_on(void){ return ((sim_gpio[2].ODR>>13)&1u)==0u; }
/* mô phỏng Os: mỗi ms, task 5 ms trước rồi task 10 ms */
static unsigned tick;
static void run_ms(unsigned ms, const int *pattern){
  for(unsigned i=0;i<ms;i++){ tick++; if(pattern) pa0_pin=pattern[i]; sim();
    if(tick%5u==0){ IoHwAb_MainFunction_Button(); sim(); }
    if(tick%10u==0){ Rte_Task_10ms(); sim(); } } }
int main(void){
  for(int p=0;p<3;p++){ sim_gpio[p].CRL=0x44444444; sim_gpio[p].CRH=0x44444444; }
  pa0_pin=1; sim();
  Mcu_Init(&Mcu_Config); CHECK(sim_rcc.APB2ENR==0x14,"Mcu_Init: APB2ENR = 0x14 (IOPAEN | IOPCEN)");
  Port_Init(&Port_Config); sim();
  CHECK(sim_gpio[0].CRL==0x44444448u,"Port_Init: GPIOA_CRL = 0x44444448 (PA0 = 1000)");
  CHECK((sim_gpio[0].ODR&1u)==1u,"Port_Init: ODR0 = 1 (pull-up)");
  CHECK(sim_gpio[2].CRH==0x44244444u,"Port_Init: GPIOC_CRH = 0x44244444");
  CHECK((sim_gpio[2].ODR&0x2000u)!=0,"Port_Init: ODR13 = 1 (LED tắt)");
  CHECK(sim_gpio[0].CRH==0x44444444u && sim_gpio[1].CRL==0x44444444u,"Port_Init: chân khác giữ reset");
  IoHwAb_Init0(NULL_PTR); sim();
  boolean p=TRUE; CHECK(IoHwAb_Button_Get(&p)==E_OK && p==FALSE,"IoHwAb_Init0: nút thả -> Get = FALSE");
  CHECK(IoHwAb_Button_Get(NULL_PTR)==E_NOT_OK,"IoHwAb_Button_Get(NULL) = E_NOT_OK");
  CHECK(!led_on(),"IoHwAb_Init0: LED tắt");
  CHECK(IOHWAB_BUTTON_DEBOUNCE_CNT==5u,"DEBOUNCE_CNT = 20 ms / 5 ms + 1 = 5 mẫu");
  /* dội: đổi mức mỗi 1 ms trong 6 ms rồi giữ 0 */
  int pat[200]; for(int i=0;i<200;i++) pat[i]=(i<6)?(i&1):0;
  run_ms(200,pat); CHECK(led_on() && ButtonLed_PressCount==1u,"nhấn có dội -> LED sáng, PressCount = 1");
  for(int i=0;i<200;i++) pat[i]=(i<6)?!(i&1):1;
  run_ms(200,pat); CHECK(!led_on() && ButtonLed_PressCount==1u,"nhả có dội -> LED tắt, PressCount giữ 1");
  /* xung nhiễu 19 ms (< 20 ms) không được chấp nhận, ở mọi pha */
  int okg=1; for(int ph=0;ph<10;ph++){ for(int i=0;i<100;i++) pat[i]=(i>=ph&&i<ph+19)?0:1;
    unsigned before=ButtonLed_PressCount; run_ms(100,pat); if(ButtonLed_PressCount!=before) okg=0; }
  CHECK(okg,"xung 19 ms bị lọc ở mọi pha");
  /* xung 25 ms phải được chấp nhận */
  { unsigned before=ButtonLed_PressCount; for(int i=0;i<100;i++) pat[i]=(i>=2&&i<27)?0:1; run_ms(100,pat);
    CHECK(ButtonLed_PressCount==before+1u,"xung 25 ms được chấp nhận"); }
  /* độ trễ nhấn -> LED theo pha: quét điểm nhấn trong chu kỳ 10 ms */
  unsigned dmin=999,dmax=0;
  for(unsigned ph=0; ph<20; ph++){
    pa0_pin=1; run_ms(100+ph,NULL); unsigned t0=tick; pa0_pin=0; unsigned d=0;
    while(!led_on()&&d<200){ run_ms(1,NULL); d++; }
    if(d<dmin){dmin=d;}
    if(d>dmax){dmax=d;}
    pa0_pin=1; run_ms(100,NULL); (void)t0; }
  printf("     độ trễ nhấn -> LED: min %u ms, max %u ms\n",dmin,dmax);
  CHECK(dmin>=20u && dmax<=30u,"độ trễ nằm trong [20, 30] ms");
  CHECK(sim_gpio[1].ODR==0,"không ghi nhầm port B");
  printf("RawChanges = %u, PressCount = %u, %u FAIL\n",(unsigned)IoHwAb_Button_RawChanges,(unsigned)ButtonLed_PressCount,fails);
  return fails!=0;
}
