# Host test (chạy trên máy tính, không cần board)

Thanh ghi RCC, GPIOA..C, SysTick được thay bằng biến trong RAM (`Stm32f103_Reg_host.h`), BSRR/BRR được mô phỏng cập nhật ODR.

```
cd bai02_autosar_led
gcc -std=c99 -Wall -Wextra -Itest -IInc -include test/Stm32f103_Reg_host.h \
    test/host_test.c Src/Mcal/*.c Src/EcuAbs/IoHwAb.c Src/Rte/Rte.c Src/Swc/LedBlinker.c \
    -fsanitize=address,undefined -o host_test && ./host_test
```

Kết quả mong đợi: 8 dòng `ok`, `0 FAIL`.
