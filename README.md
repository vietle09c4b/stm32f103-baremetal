# STM32F103 bare-metal (Blue Pill)

Lập trình STM32F103C8T6 ở mức thanh ghi: không dùng HAL, không dùng LL. Bài 1–3 tự tạo thanh ghi từ địa chỉ trong RM0008; từ bài 4 dùng header CMSIS của ST (`chip_headers/`, chỉ chứa địa chỉ, struct thanh ghi và tên bit, giấy phép Apache 2.0).

`bai02_autosar_led`, `bai04b_autosar_button`, `bai10b_autosar_can` và `bai15b_autosar_nvm` là stack AUTOSAR viết tay để minh họa kiến trúc (API theo AUTOSAR CP SWS R25-11; riêng memory stack của `bai15b_autosar_nvm` theo R20-11), không phải MCAL đạt chuẩn: không có ARXML, RTE generator, DET hay AUTOSAR OS.

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
| [`bai08_interrupt`](bai08_interrupt) | Ngắt: SysTick 1 ms (TICKINT), TIM2 update 1 Hz (UIE, IRQ 28), EXTI0 nút PA0 cạnh xuống (AFIO_EXTICR1, IRQ 6), NVIC ISER/IPR ghi trực tiếp, vòng lặp chính rỗng | Đã chạy trên Blue Pill: 5 lần nhấn = g_presses 5, đọc NVIC/EXTI/AFIO, call stack trong TIM2_IRQHandler |
| [`bai09_dma`](bai09_dma) | DMA1 Channel 4 gửi USART1 (PA9, 115200, CCR4 DIR/MINC/TCIE, USART1_CR3.DMAT, IRQ 14) + memory-to-memory Channel 1 (32 word flash → RAM) | Đã chạy trên Blue Pill với CP2102 + PuTTY: main chạy ~833–855 vòng trong mỗi lần gửi, đọc CCR4/CNDTR4/CPAR4/ISR, breakpoint trong ISR |
| [`bai10_can`](bai10_can) | bxCAN CAN1 500 kbit/s (HSE 8 MHz, CAN_BTR 0x001C0000), remap PB8 RX / PB9 TX, bộ lọc bank 0 list 32 bit (0x321, 0x123), TX mailbox, ngắt FIFO0 (IRQ 20); chế độ Loop back + Silent hoặc Normal | Đã chạy trên Blue Pill: loopback tx = rx; Normal với TJA1050 + PCAN-USB (PCAN-View 500 kbit/s), 0x321 bật/tắt LED PC13, ESR = 0; lỗi ACK khi thiếu node: TEC 128, error passive |
| [`bai10b_autosar_can`](bai10b_autosar_can) | Cùng bài toán CAN, tổ chức theo AUTOSAR CP R25-11: SWC → RTE → Com → PduR → LSduR → CanIf → Can Driver → bxCAN; Com gửi I-PDU 0x123 theo chu kỳ 100 ms, CanIf lọc phần mềm 0x321, TxConfirmation bằng polling, nhận bằng ngắt FIFO0 | Đã chạy trên Blue Pill với TJA1050 + PCAN-USB: 0x123 mỗi 100 ms, 0x321 bật/tắt LED PC13, call stack gửi/nhận qua đủ các lớp, ESR = 0 |
| [`bai11_can_wakeup`](bai11_can_wakeup) | Stop mode + đánh thức qua CAN: lệnh 0x321 byte0 = 02 đưa bxCAN vào Sleep và MCU vào Stop (LPDS = 1); cạnh xuống của RXD trên PB8 (CAN_RX) qua EXTI8 đánh thức MCU; chạy lại HSE rồi xóa SLEEP | Đã chạy trên Blue Pill với TJA1050 + PCAN-USB: 0x123 dừng khi ngủ, chạy lại khi có frame; thức bằng HSI, bật HSE ≈ 400 µs, bxCAN đồng bộ ≈ 100 µs; frame đánh thức tới nhờ PCAN-USB phát lại khi thiếu ACK |
| [`bai12_i2c_at24c256`](bai12_i2c_at24c256) | EEPROM AT24C256C qua I2C1 (PB6/PB7, CCR 46 ≈ 87 kHz): random read với repeated START, nhận 1/2/N byte theo RM0008 Method 2 (POS, BTF) có che ngắt theo ES096, page write tách theo biên 64 byte, ACK polling đo tWR bằng DWT, bộ đếm khởi động ở 0x7FC0 | Đã chạy trên Blue Pill: ghi/đọc 1, 2, 3, 64, 100 byte không lỗi, roll-over trong trang đúng datasheet, tWR ≈ 2,7 ms (16 lần NACK), đọc 64 byte 7,15 ms, bộ đếm tăng 1 mỗi lần cấp điện |
| [`bai13_spi_loopback`](bai13_spi_loopback) | SPI1 master polling (PA5 SCK, PA6 MISO pull-up, PA7 MOSI, PA4 CS bằng GPIO), CR1 ghi một lần khi SPE = 0, SSM = SSI = 1, trao đổi từng byte (TXE → DR → RXNE → DR), truyền liên tục theo RM0008 25.3.5, CRC-8 phần cứng, cố ý gây OVR và MODF | Đã chạy trên Blue Pill với jumper PA7→PA6: 4 mode + khung 16 bit 0 byte sai, SCK nghỉ = CPOL, CRC = 0x27 khớp CRC phần mềm, OVR (SR 0x43) và MODF (CR1 0x210) xóa được, kiểu liên tục OVR ở /2–/8 (-O0), rút jumper nhận 0xFF |
| [`bai14_watchdog`](bai14_watchdog) | IWDG (LSI, khóa KR 0xCCCC/0x5555/0xAAAA, PR/RLR, chờ PVU/RVU) và WWDG (WDGTB = 3, cửa sổ W = 0x50, ngắt EWI), đọc/xóa cờ RCC_CSR, máy trạng thái qua reset lưu trong BKP_DR1..DR10 | Đã chạy trên Blue Pill: IWDG danh định 1 s reset sau 1008 ms (LSI ≈ 39,7 kHz), nạp WWDG ở 50 ms gây reset, nạp ở 230 ms không reset, EWI 255 ms / reset 259 ms, PINRSTF bật kèm IWDGRSTF/WWDGRSTF |
| [`bai15_flash`](bai15_flash) | Flash nội: unlock FPEC (KEY1/KEY2), xóa trang (PER, AR, STRT), ghi half-word (PG), xử lý ES096 2.2.11, đo thời gian BSY bằng DWT, nhật ký đếm khởi động ghi nối tiếp ở trang 63; linker giảm FLASH còn 62 KB | Đã chạy trên Blue Pill: xóa trang BSY 21,99 ms, ghi half-word 57,5–62,25 µs, PGERR khi ghi đè và ngoại lệ 0x0000, bộ đếm tăng 3 → 5 qua 2 lần rút điện |
| [`bai15b_autosar_nvm`](bai15b_autosar_nvm) | Memory stack AUTOSAR CP R20-11: SWC → RTE → NvM → MemIf → Fee → Fls → FPEC; NvM block NATIVE với CRC16 và ROM default, ReadAll lúc khởi động, WriteBlock, WriteAll khi tắt máy (giữ nút 2 s); Fee 2 bank × 2 KB ghi nối tiếp và dọn rác; linker FLASH 60 KB | Đã chạy trên Blue Pill: lần đầu RESTORED_DEFAULTS, dữ liệu giữ qua rút điện, CRC phát hiện ô bị ghi đè, 400 lần ghi với 2 lần dọn rác không lỗi, WriteAll 4,0 ms |

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
