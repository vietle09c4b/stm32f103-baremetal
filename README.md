# STM32F103 bare-metal (Blue Pill)

Lập trình STM32F103C8T6 ở mức thanh ghi: không dùng HAL, không dùng LL. Bài 1–3 tự tạo thanh ghi từ địa chỉ trong RM0008; từ bài 4 dùng header CMSIS của ST (`chip_headers/`, chỉ chứa địa chỉ, struct thanh ghi và tên bit, giấy phép Apache 2.0).

`bai02_autosar_led` là stack AUTOSAR viết tay để minh họa kiến trúc (API theo AUTOSAR CP SWS R25-11), không phải MCAL đạt chuẩn: không có ARXML, RTE generator, DET hay AUTOSAR OS.

Repo này là mã nguồn cho series bài viết **Bare-metal STM32F103** trên [vietc4b.com](https://vietc4b.com).

## Phần cứng

| Thành phần | Ghi chú |
|---|---|
| Board | Blue Pill, STM32F103C8T6 (Cortex-M3, 64 KB Flash, 20 KB SRAM) |
| LED người dùng | PC13, nối kiểu sink: PC13 = 0 thì LED sáng |
| Nạp/gỡ lỗi | ST-LINK V2 qua SWD: 3V3, SWDIO (PA13), SWCLK (PA14), GND |
| IDE | STM32CubeIDE 2.2.0 |

## Danh sách bài

| Thư mục | Nội dung | Trạng thái |
|---|---|---|
| [`bai01_led_pc13`](bai01_led_pc13) | Tạo thanh ghi từ địa chỉ (macro và struct), bật clock GPIOC, cấu hình CRH, nháy LED PC13 bằng BSRR/BRR | Đã chạy trên Blue Pill |
| [`bai02_autosar_led`](bai02_autosar_led) | Cùng bài toán nháy LED PC13, tổ chức theo kiến trúc AUTOSAR: SWC → RTE → IoHwAb → MCAL (Mcu, Port, Dio), SysTick 1 ms làm nhịp | Đã chạy trên Blue Pill, debug call stack + SFRs |
| [`bai03_lcd1602_clock`](bai03_lcd1602_clock) | Đồng hồ trên LCD1602 qua module I2C PCF8574: driver I2C1 polling (PB6/PB7), HD44780 4 bit, SysTick 1 ms từ HSE 8 MHz, lịch có năm nhuận | Đã chạy trên Blue Pill (module 0x27), host test 33/33 |
| [`bai04_gpio_input`](bai04_gpio_input) | Đọc nút nhấn PA0 (input pull-up: CNF0 = 10, ODR0 = 1), chống dội 20 ms bằng SysTick, LED PC13 theo nút; header CMSIS của ST | Đã chạy trên Blue Pill, debug SFRs, host test 11/11 |
| [`bai04b_autosar_button`](bai04b_autosar_button) | Cùng bài toán nút nhấn PA0, tổ chức theo AUTOSAR: Port bật pull-up qua cấu hình, Dio đọc IDR, IoHwAb đảo mức và chống dội 20 ms (5 mẫu cách 5 ms), SWC qua RTE client-server | Đã chạy trên Blue Pill, debug call stack + SFRs, host test 16/16 |
| [`bai05_uart`](bai05_uart) | USART1 PA9/PA10 qua CP2102: AF push-pull / input pull-up trong CRH, BRR = 0x45 (115200 baud ở 8 MHz), ghi chờ TXE, đọc chờ RXNE, printf qua `__io_putchar`, LED PC13 theo ký tự | Đã chạy trên Blue Pill qua CP2102 + PuTTY, debug SFRs USART1/GPIOA |
| [`bai06_systick`](bai06_systick) | SysTick của lõi Cortex-M3: LOAD = 7999 (1 ms ở HCLK 8 MHz), xóa VAL, CTRL = CLKSOURCE \| ENABLE, delay chờ COUNTFLAG, LED PC13 đảo mỗi 1000 ms | Đã chạy trên Blue Pill, debug SFRs SysTick (CTRL 0x5, LOAD 0x1F3F, CALIB 0x40002328) + Live Expressions |
| [`bai07_timer`](bai07_timer) | TIM2 time base 1 Hz (PSC 7999, ARR 999, UG) + output compare toggle PA0, TIM3 input capture PA6 (10 kHz, CC1S = 01), jumper PA0 → PA6 | Đã chạy trên Blue Pill: chu kỳ đo 20000 tick = 2000 ms, debug TIM2/TIM3/GPIOA |

## Cách mở project

1. STM32CubeIDE: **File → Import → General → Existing Projects into Workspace**, chọn thư mục repo.
2. Chọn project, bấm **Build** (biểu tượng búa) rồi **Run**.
3. Console phải có dòng `Download verified successfully`.

## Kiểm tra code đã thật sự được nạp

Nhiều Blue Pill xuất xưởng đã có sẵn chương trình nháy LED PC13. Vì vậy LED nháy chưa chứng minh code mới đang chạy.

Trong `bai01_led_pc13/Src/main.c`, đổi `BLINK_DELAY` từ `400000U` thành `100000U`, rồi nạp lại. LED phải nháy nhanh hơn khoảng 4 lần. Phép thử này đã được thực hiện trên board thật.

## Ghi chú ST-LINK

STM32CubeIDE 2.2.0 yêu cầu firmware ST-LINK mới. Với ST-LINK V2 dùng trong repo này, firmware đã được nâng từ V2J40S7 lên V2J48S7 bằng **Help → ST-LINK Upgrade**. Nếu công cụ báo "ST-Link is not in the DFU mode", rút ST-LINK ra, cắm lại, bấm **Refresh device list** rồi **Open in update mode**.

## Tài liệu gốc

- RM0008 Rev 21: STM32F101xx/F102xx/F103xx/F105xx/F107xx reference manual
- DS5319 Rev 20: STM32F103x8/xB datasheet
- PM0056: STM32F10xxx/20xxx/21xxx/L1xxxx Cortex-M3 programming manual

## License

MIT
