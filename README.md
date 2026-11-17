# STM32F103 bare-metal (Blue Pill)

Lập trình STM32F103C8T6 ở mức thanh ghi: không dùng HAL, không dùng LL, không dùng header CMSIS của thiết bị. Mỗi thanh ghi được tạo từ địa chỉ trong RM0008.

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
