# Host test (chạy trên máy tính, không cần board)

Thanh ghi RCC, GPIOA..C, SysTick được thay bằng biến trong RAM (`Stm32f103_Reg_host.h`), BSRR/BRR được mô phỏng cập nhật ODR, IDR0 do test đặt (mô phỏng nút và dội). Bộ lập lịch được mô phỏng: mỗi 1 ms, task 5 ms (IoHwAb) chạy trước task 10 ms (RTE -> SWC).

```
cd bai04b_autosar_button
gcc -std=c99 -Wall -Wextra -Wconversion -Itest -IInc -include test/Stm32f103_Reg_host.h \
    test/host_test.c Src/Mcal/*.c Src/EcuAbs/IoHwAb.c Src/Rte/Rte.c Src/Swc/ButtonLed.c \
    -fsanitize=address,undefined -o host_test && ./host_test
```

Kết quả mong đợi: 16 dòng `ok`, `0 FAIL`; độ trễ nhấn -> LED từ 21 đến 30 ms.
