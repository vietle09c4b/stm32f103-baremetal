/*
 * i2c1.c - I2C1 master polling cho STM32F103 (RM0008 mục 26.3.3, Figure 273)
 *
 * Trình tự gửi (master transmitter, địa chỉ 7 bit):
 *   START -> chờ SB -> ghi địa chỉ vào DR -> chờ ADDR (hoặc AF = NACK)
 *   -> đọc SR1 rồi SR2 để xóa ADDR -> mỗi byte: chờ TXE, ghi DR
 *   -> chờ BTF -> STOP.
 */
#include "i2c1.h"
#include "stm32f103_reg.h"

#define I2C_TIMEOUT   100000UL     /* số vòng chờ tối đa cho mỗi cờ */
#define SCL_PIN       6U
#define SDA_PIN       7U

/* 4 bit cấu hình của một chân trong GPIOx_CRL (chân 0..7) */
#define CRL_SHIFT(pin)     ((pin) * 4U)
#define CFG_OUT_OD_2MHZ    0x6UL   /* CNF = 01 open-drain, MODE = 10 */
#define CFG_AF_OD_2MHZ     0xEUL   /* CNF = 11 AF open-drain, MODE = 10 */

static void short_delay(void)
{
    for (volatile uint32_t i = 0; i < 20U; i++) { }   /* khoảng 5 us ở 8 MHz */
}

static void pb67_config(uint32_t cfg)
{
    uint32_t crl = GPIOB->CRL;
    crl &= ~((0xFUL << CRL_SHIFT(SCL_PIN)) | (0xFUL << CRL_SHIFT(SDA_PIN)));
    crl |=  (cfg << CRL_SHIFT(SCL_PIN)) | (cfg << CRL_SHIFT(SDA_PIN));
    GPIOB->CRL = crl;
}

/*
 * Giải phóng bus: nếu một slave đang giữ SDA ở mức 0 (do MCU reset giữa
 * giao dịch), phát tối đa 9 xung SCL rồi tạo điều kiện STOP bằng GPIO.
 */
static void bus_recover(void)
{
    GPIOB->BSRR = (1UL << SCL_PIN) | (1UL << SDA_PIN);   /* thả cả hai lên 1 */
    pb67_config(CFG_OUT_OD_2MHZ);
    short_delay();

    for (uint32_t i = 0; i < 9U && (GPIOB->IDR & (1UL << SDA_PIN)) == 0U; i++) {
        GPIOB->BRR  = (1UL << SCL_PIN);  short_delay();
        GPIOB->BSRR = (1UL << SCL_PIN);  short_delay();
    }

    /* STOP: SDA từ 0 lên 1 khi SCL đang ở mức 1 */
    GPIOB->BRR  = (1UL << SCL_PIN);  short_delay();
    GPIOB->BRR  = (1UL << SDA_PIN);  short_delay();
    GPIOB->BSRR = (1UL << SCL_PIN);  short_delay();
    GPIOB->BSRR = (1UL << SDA_PIN);  short_delay();
}

static uint32_t i2c_pclk1_hz;

/* Biến chẩn đoán, xem được bằng debugger (Live Expressions) */
volatile uint32_t i2c1_error_count;
volatile uint32_t i2c1_last_sr1;

void i2c1_init(uint32_t pclk1_hz)
{
    uint32_t mhz = pclk1_hz / 1000000UL;
    i2c_pclk1_hz = pclk1_hz;

    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;

    bus_recover();
    pb67_config(CFG_AF_OD_2MHZ);

    /* Reset mềm khối I2C để xóa cờ BUSY có thể bị kẹt */
    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0;

    I2C1->CR2   = mhz;                          /* FREQ = PCLK1 tính bằng MHz */
    I2C1->CCR   = pclk1_hz / (2UL * 100000UL);  /* Sm: Thigh = Tlow = CCR x Tpclk1 */
    I2C1->TRISE = mhz + 1UL;                    /* Sm: tr max 1000 ns */
    I2C1->CR1   = I2C_CR1_PE;
}

static int wait_sr1(uint32_t mask)
{
    for (uint32_t t = 0; t < I2C_TIMEOUT; t++) {
        uint32_t sr1 = I2C1->SR1;
        if (sr1 & mask)                          return 0;
        if (sr1 & (I2C_SR1_AF))                  return -1;
        if (sr1 & (I2C_SR1_BERR | I2C_SR1_ARLO)) return -2;
    }
    return -3;
}

static i2c_status_t finish_error(int code)
{
    i2c1_last_sr1 = I2C1->SR1;
    i2c1_error_count++;

    if (code == -1) {
        /* NACK: bus vẫn bình thường, chỉ cần xóa AF và phát STOP */
        I2C1->SR1 = ~I2C_SR1_AF;                  /* AF là cờ rc_w0 */
        I2C1->CR1 |= I2C_CR1_STOP;
        for (uint32_t t = 0; (I2C1->CR1 & I2C_CR1_STOP) && t < I2C_TIMEOUT; t++) { }
        return I2C_ERR_NACK;
    }

    /* Lỗi bus, mất trọng tài hoặc quá thời gian: khởi tạo lại toàn bộ I2C1 */
    i2c1_init(i2c_pclk1_hz);
    return (code == -2) ? I2C_ERR_BUS : I2C_ERR_TIMEOUT;
}

i2c_status_t i2c1_write(uint8_t addr7, const uint8_t *data, uint32_t len)
{
    uint32_t t;
    int rc;

    if (I2C1->SR1 & (I2C_SR1_BERR | I2C_SR1_ARLO)) {   /* lỗi còn treo từ trước */
        i2c1_last_sr1 = I2C1->SR1;
        i2c1_error_count++;
        i2c1_init(i2c_pclk1_hz);
    }

    for (t = 0; (I2C1->SR2 & I2C_SR2_BUSY) && t < I2C_TIMEOUT; t++) { }
    if (t == I2C_TIMEOUT) {                     /* BUSY kẹt: giải phóng bus */
        i2c1_last_sr1 = I2C1->SR1;
        i2c1_error_count++;
        i2c1_init(i2c_pclk1_hz);
        return I2C_ERR_BUSY;
    }

    I2C1->CR1 |= I2C_CR1_START;
    rc = wait_sr1(I2C_SR1_SB);
    if (rc) return finish_error(rc);

    I2C1->DR = (uint32_t)addr7 << 1;            /* bit 0 = 0: master ghi */
    rc = wait_sr1(I2C_SR1_ADDR);
    if (rc) return finish_error(rc);
    (void)I2C1->SR1;
    (void)I2C1->SR2;                            /* đọc SR1 rồi SR2 để xóa ADDR */

    for (uint32_t i = 0; i < len; i++) {
        rc = wait_sr1(I2C_SR1_TXE);
        if (rc) return finish_error(rc);
        I2C1->DR = data[i];
    }

    if (len > 0U) {
        rc = wait_sr1(I2C_SR1_BTF);              /* byte cuối đã ra hết bus */
        if (rc) return finish_error(rc);
    }
    I2C1->CR1 |= I2C_CR1_STOP;

    /* Phần cứng tự xóa bit STOP sau khi phát xong điều kiện STOP */
    for (t = 0; (I2C1->CR1 & I2C_CR1_STOP) && t < I2C_TIMEOUT; t++) { }
    return I2C_OK;
}

i2c_status_t i2c1_probe(uint8_t addr7)
{
    return i2c1_write(addr7, (const uint8_t *)0, 0U);
}
