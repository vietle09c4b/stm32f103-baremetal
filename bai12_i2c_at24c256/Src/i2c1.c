/*
 * i2c1.c - I2C1 master polling trên STM32F103C8, mức thanh ghi
 *
 * Tài liệu:
 *   RM0008 Rev 21  26.3.3  master mode: EV5 (SB), EV6 (ADDR), EV8/EV8_2 (TxE, BTF),
 *                          nhận theo "Method 2" (dùng cho polling): N = 1 (EV6_3), N = 2 (POS, EV7_3),
 *                          N > 2 (BTF ở 3 byte cuối, EV7_2)
 *                  26.6.1  CR1: PE b0, START b8, STOP b9, ACK b10, POS b11, SWRST b15
 *                  26.6.2  CR2: FREQ[5:0] = PCLK1 tính bằng MHz
 *                  26.6.6  SR1: SB b0, ADDR b1, BTF b2, RxNE b6, TxE b7, BERR b8, ARLO b9, AF b10
 *                  26.6.7  SR2: BUSY b1
 *                  26.6.8  CCR (Sm: Thigh = Tlow = CCR x TPCLK1), 26.6.9 TRISE
 *   ES096 Rev 15   2.8.1 / 2.8.2  che ngắt quanh các bước EV6_1, EV6_3, EV7_2, EV7_3 (workaround 2)
 *                  2.8.4  timeout khi chờ cờ, hết hạn thì SWRST
 *                  2.8.5  repeated START ở Sm: giữ SCL dưới 88 kHz
 *                  2.8.7  chuỗi chuyển mức SCL/SDA bằng GPIO trước khi SWRST
 */
#include "stm32f1xx.h"
#include "i2c1.h"

#define I2C_TIMEOUT   200000UL          /* số vòng chờ tối đa cho một cờ (vài chục ms ở 8 MHz) */
#define SCL_PIN       6U
#define SDA_PIN       7U

/* 4 bit cấu hình một chân trong GPIOx_CRL (RM0008 9.2.1): MODE[1:0] + CNF[1:0] */
#define CFG_OUT_OD_2MHZ    0x6UL         /* CNF = 01 general purpose open-drain, MODE = 10 */
#define CFG_AF_OD_2MHZ     0xEUL         /* CNF = 11 alternate function open-drain, MODE = 10 */

volatile uint32_t i2c1_error_count;
volatile uint32_t i2c1_last_sr1;

static uint32_t s_pclk1_hz;
static uint32_t s_scl_hz;

/* ------------------------------------------------------------------ GPIO */
static void pb67_config(uint32_t cfg)
{
    uint32_t crl = GPIOB->CRL;
    crl &= ~((0xFUL << (SCL_PIN * 4U)) | (0xFUL << (SDA_PIN * 4U)));
    crl |=  (cfg << (SCL_PIN * 4U)) | (cfg << (SDA_PIN * 4U));
    GPIOB->CRL = crl;
}

static void half_bit(void)              /* khoảng 5 µs ở 8 MHz, nhịp cho clock GPIO ~100 kHz */
{
    for (volatile uint32_t i = 0; i < 10U; i++) { }
}

static void scl(uint32_t high) { GPIOB->BSRR = high ? (1UL << SCL_PIN) : (1UL << (SCL_PIN + 16U)); half_bit(); }
static void sda(uint32_t high) { GPIOB->BSRR = high ? (1UL << SDA_PIN) : (1UL << (SDA_PIN + 16U)); half_bit(); }

/*
 * Giải phóng bus bằng GPIO open-drain, khi PE = 0:
 *  1) slave còn giữ SDA = 0 (MCU reset giữa lúc slave đang gửi): phát tối đa 9 xung SCL
 *     (thủ tục software reset của AT24C256C: START nếu được, 9 xung clock, START, STOP);
 *  2) ES096 2.8.7 bước 2-11: SDA xuống rồi SCL xuống, SCL lên rồi SDA lên, tức một START rồi
 *     một STOP; mỗi cạnh làm bộ lọc tương tự của chân I2C cập nhật lại mức.
 */
static void bus_recover(void)
{
    GPIOB->BSRR = (1UL << SCL_PIN) | (1UL << SDA_PIN);   /* ODR = 1: thả hai đường */
    pb67_config(CFG_OUT_OD_2MHZ);
    half_bit();

    for (uint32_t i = 0; i < 9U && (GPIOB->IDR & (1UL << SDA_PIN)) == 0U; i++) {
        scl(0U);
        scl(1U);
    }

    sda(0U);            /* START: SDA 1 -> 0 khi SCL = 1 */
    scl(0U);
    scl(1U);
    sda(1U);            /* STOP: SDA 0 -> 1 khi SCL = 1 */
}

/* ------------------------------------------------------------------ init */
void i2c1_init(uint32_t pclk1_hz, uint32_t scl_hz)
{
    uint32_t mhz = pclk1_hz / 1000000UL;
    s_pclk1_hz = pclk1_hz;
    s_scl_hz   = scl_hz;

    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;

    I2C1->CR1 = 0U;                              /* PE = 0 trước khi điều khiển chân bằng GPIO */
    bus_recover();
    pb67_config(CFG_AF_OD_2MHZ);

    I2C1->CR1 = I2C_CR1_SWRST;                   /* xóa BUSY và mọi cờ còn treo */
    I2C1->CR1 = 0U;

    I2C1->CR2   = mhz;                           /* FREQ = 8 -> TPCLK1 = 125 ns          */
    I2C1->CCR   = (pclk1_hz + scl_hz) / (2UL * scl_hz);  /* làm tròn lên: 8 MHz, 87 kHz -> 46 */
    I2C1->TRISE = mhz + 1UL;                     /* Sm: tr max 1000 ns -> 1000/125 + 1 = 9 */
    I2C1->CR1   = I2C_CR1_PE;
}

/* ------------------------------------------------------------------ helpers */
/* 0 = cờ lên; -1 = AF (NACK); -2 = BERR/ARLO; -3 = hết thời gian */
static int wait_sr1(uint32_t mask)
{
    for (uint32_t t = 0; t < I2C_TIMEOUT; t++) {
        uint32_t sr1 = I2C1->SR1;
        if (sr1 & mask)                          return 0;
        if (sr1 & I2C_SR1_AF)                    return -1;
        if (sr1 & (I2C_SR1_BERR | I2C_SR1_ARLO)) return -2;
    }
    return -3;
}

static void wait_stop_done(void)
{
    /* Phần cứng xóa bit STOP khi đã phát xong điều kiện STOP */
    for (uint32_t t = 0; (I2C1->CR1 & I2C_CR1_STOP) && t < I2C_TIMEOUT; t++) { }
}

static i2c_status_t fail(int code)
{
    i2c1_last_sr1 = I2C1->SR1;
    I2C1->CR1 &= ~(I2C_CR1_POS | I2C_CR1_ACK);

    if (code == -1) {
        /* NACK là kết quả bình thường khi ACK polling EEPROM, không tính là lỗi bus */
        /* NACK: master phải phát STOP (RM0008 26.3.4, Acknowledge failure) */
        I2C1->SR1 = (uint16_t)~I2C_SR1_AF;       /* AF là rc_w0: ghi 0 vào AF, ghi 1 vào bit khác không đổi */
        I2C1->CR1 |= I2C_CR1_STOP;
        wait_stop_done();
        return I2C_ERR_NACK;
    }
    i2c1_error_count++;
    i2c1_init(s_pclk1_hz, s_scl_hz);             /* ES096 2.8.4: lỗi bus hoặc hết giờ -> SWRST */
    return (code == -2) ? I2C_ERR_BUS : I2C_ERR_TIMEOUT;
}

static int wait_bus_free(void)
{
    for (uint32_t t = 0; t < I2C_TIMEOUT; t++) {
        if ((I2C1->SR2 & I2C_SR2_BUSY) == 0U) return 0;
    }
    return -3;
}

/* EV5 + phát byte địa chỉ, dừng lại khi ADDR = 1 (chưa xóa ADDR) */
static int start_and_address(uint8_t addr_byte)
{
    int rc;
    I2C1->CR1 |= I2C_CR1_START;                  /* đang là master: thành repeated START */
    if ((rc = wait_sr1(I2C_SR1_SB)) != 0) return rc;
    I2C1->DR = addr_byte;                        /* EV5: đọc SR1 (trong wait_sr1) rồi ghi DR */
    return wait_sr1(I2C_SR1_ADDR);
}

static void clear_addr(void)
{
    (void)I2C1->SR1;                             /* EV6: đọc SR1 rồi đọc SR2 */
    (void)I2C1->SR2;
}

/* Phần gửi: các byte của buf, kết thúc khi BTF = 1 (DR và thanh ghi dịch đều trống) */
static int send_bytes(const uint8_t *buf, uint32_t len)
{
    int rc;
    for (uint32_t i = 0; i < len; i++) {
        if ((rc = wait_sr1(I2C_SR1_TXE)) != 0) return rc;   /* EV8 */
        I2C1->DR = buf[i];
    }
    if (len > 0U) {
        if ((rc = wait_sr1(I2C_SR1_BTF)) != 0) return rc;   /* EV8_2: TxE = 1, BTF = 1 */
    }
    return 0;
}

/*
 * Phần nhận theo RM0008 26.3.3 "Method 2" + ES096 2.8.1/2.8.2 workaround 2 (che ngắt).
 * Gọi khi bus đã thuộc về master (sau phần gửi) hoặc khi bus rỗi.
 */
static int receive_bytes(uint8_t addr7, uint8_t *rd, uint32_t n)
{
    int rc;
    uint32_t pm;
    uint8_t addr_r = (uint8_t)((addr7 << 1) | 1U);

    if (n == 2U) {
        I2C1->CR1 |= I2C_CR1_POS | I2C_CR1_ACK;          /* POS: ACK áp cho byte kế tiếp */
    } else {
        I2C1->CR1 = (I2C1->CR1 & ~I2C_CR1_POS) | I2C_CR1_ACK;
    }

    if ((rc = start_and_address(addr_r)) != 0) return rc;

    if (n == 1U) {                                       /* EV6_3 */
        I2C1->CR1 &= ~I2C_CR1_ACK;                       /* ACK = 0 trước khi xóa ADDR */
        pm = __get_PRIMASK(); __disable_irq();
        clear_addr();
        I2C1->CR1 |= I2C_CR1_STOP;                       /* STOP ngay sau khi xóa ADDR */
        __set_PRIMASK(pm);
        if ((rc = wait_sr1(I2C_SR1_RXNE)) != 0) return rc;
        rd[0] = (uint8_t)I2C1->DR;
    } else if (n == 2U) {                                /* EV6_1 + EV7_3 */
        pm = __get_PRIMASK(); __disable_irq();
        clear_addr();
        I2C1->CR1 &= ~I2C_CR1_ACK;                       /* trước xung ACK của byte 1: byte 2 nhận NACK */
        __set_PRIMASK(pm);
        if ((rc = wait_sr1(I2C_SR1_BTF)) != 0) return rc; /* byte 1 trong DR, byte 2 trong thanh ghi dịch */
        pm = __get_PRIMASK(); __disable_irq();
        I2C1->CR1 |= I2C_CR1_STOP;
        rd[0] = (uint8_t)I2C1->DR;
        rd[1] = (uint8_t)I2C1->DR;
        __set_PRIMASK(pm);
        I2C1->CR1 &= ~I2C_CR1_POS;
    } else {                                             /* N > 2 */
        uint32_t i = 0U;
        clear_addr();
        while ((n - i) > 3U) {                           /* EV7 */
            if ((rc = wait_sr1(I2C_SR1_RXNE)) != 0) return rc;
            rd[i++] = (uint8_t)I2C1->DR;
        }
        if ((rc = wait_sr1(I2C_SR1_BTF)) != 0) return rc; /* EV7_2: N-2 trong DR, N-1 trong thanh ghi dịch */
        I2C1->CR1 &= ~I2C_CR1_ACK;                       /* byte N sẽ nhận NACK */
        pm = __get_PRIMASK(); __disable_irq();
        rd[i++] = (uint8_t)I2C1->DR;                     /* đọc N-2: bắt đầu nhận byte N */
        I2C1->CR1 |= I2C_CR1_STOP;
        rd[i++] = (uint8_t)I2C1->DR;                     /* đọc N-1 */
        __set_PRIMASK(pm);
        if ((rc = wait_sr1(I2C_SR1_RXNE)) != 0) return rc;
        rd[i] = (uint8_t)I2C1->DR;                       /* byte N */
    }
    wait_stop_done();
    return 0;
}

/* ------------------------------------------------------------------ API */
i2c_status_t i2c1_write(uint8_t addr7, const uint8_t *data, uint32_t len)
{
    int rc;
    if (wait_bus_free() != 0) { (void)fail(-3); return I2C_ERR_BUSY; }

    if ((rc = start_and_address((uint8_t)(addr7 << 1))) != 0) return fail(rc);
    clear_addr();
    if ((rc = send_bytes(data, len)) != 0) return fail(rc);
    /* len = 0 (probe): STOP ngay sau ADDR, khi TxE chưa lên vì chưa có byte nào */

    I2C1->CR1 |= I2C_CR1_STOP;
    wait_stop_done();
    return I2C_OK;
}

i2c_status_t i2c1_write_read(uint8_t addr7, const uint8_t *wr, uint32_t wlen,
                             uint8_t *rd, uint32_t rlen)
{
    int rc;
    if (rlen == 0U) return i2c1_write(addr7, wr, wlen);
    if (wait_bus_free() != 0) { (void)fail(-3); return I2C_ERR_BUSY; }

    if (wlen > 0U) {
        if ((rc = start_and_address((uint8_t)(addr7 << 1))) != 0) return fail(rc);
        clear_addr();
        if ((rc = send_bytes(wr, wlen)) != 0) return fail(rc);
        /* BTF = 1 giữ SCL ở mức thấp; bit START tiếp theo tạo repeated START */
    }
    if ((rc = receive_bytes(addr7, rd, rlen)) != 0) return fail(rc);
    return I2C_OK;
}

i2c_status_t i2c1_probe(uint8_t addr7)
{
    return i2c1_write(addr7, (const uint8_t *)0, 0U);
}
