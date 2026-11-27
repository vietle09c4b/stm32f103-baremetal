/*
 * spi1.c - Driver SPI1 mức thanh ghi cho STM32F103C8, chế độ master full-duplex, polling
 *
 * Tài liệu: RM0008 Rev 21
 *   7.3.7   RCC_APB2ENR (IOPAEN bit 2, SPI1EN bit 12)
 *   9.1.11  Table 25: cấu hình GPIO cho SPI
 *   25.3.3  Thủ tục cấu hình master
 *   25.3.5  Thủ tục truyền/nhận full-duplex
 *   25.3.8  Thủ tục tắt SPI
 *   25.3.10 Cờ lỗi MODF, OVR
 */
#include "stm32f1xx.h"
#include "spi1.h"

#define SPI_TIMEOUT     100000UL        /* số vòng chờ tối đa cho mỗi cờ */

#define CS_PIN          4U              /* PA4 */

/* ---------------------------------------------------------------- GPIO */
void spi1_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    /* CRL: mỗi chân 4 bit = CNF[1:0] MODE[1:0]  (RM0008 9.2.1)
     *   PA4 CS   : output push-pull 50 MHz   CNF=00 MODE=11 -> 0x3
     *   PA5 SCK  : AF push-pull 50 MHz       CNF=10 MODE=11 -> 0xB
     *   PA6 MISO : input pull-up/pull-down   CNF=10 MODE=00 -> 0x8 (ODR6 = 1 chọn pull-up)
     *   PA7 MOSI : AF push-pull 50 MHz       CNF=10 MODE=11 -> 0xB  */
    GPIOA->BSRR = (1UL << CS_PIN) | (1UL << 6);     /* CS nghỉ mức cao, chọn pull-up cho MISO */
    uint32_t crl = GPIOA->CRL;
    crl &= ~((0xFUL << 16) | (0xFUL << 20) | (0xFUL << 24) | (0xFUL << 28));
    crl |=  (0x3UL << 16) | (0xBUL << 20) | (0x8UL << 24) | (0xBUL << 28);
    GPIOA->CRL = crl;
}

void spi1_cs_low(void)  { GPIOA->BRR  = (1UL << CS_PIN); }   /* ghi nguyên tử, không read-modify-write */
void spi1_cs_high(void) { GPIOA->BSRR = (1UL << CS_PIN); }

/* ---------------------------------------------------------- chờ cờ SR */
static spi_status_t wait_set(uint32_t mask)
{
    for (uint32_t t = 0; t < SPI_TIMEOUT; t++) {
        uint32_t sr = SPI1->SR;
        if (sr & mask)          return SPI_OK;
        if (sr & SPI_SR_MODF)   return SPI_ERR_MODF;
    }
    return SPI_ERR_TIMEOUT;
}

static spi_status_t wait_bsy_clear(void)
{
    for (uint32_t t = 0; t < SPI_TIMEOUT; t++) {
        if ((SPI1->SR & SPI_SR_BSY) == 0U) return SPI_OK;
    }
    return SPI_ERR_TIMEOUT;
}

/* ------------------------------------------------------ bật / tắt SPI */
/* 25.3.8, full-duplex: nhận byte cuối (RXNE), chờ TXE = 1, chờ BSY = 0, rồi SPE = 0 */
void spi1_disable(void)
{
    if (SPI1->CR1 & SPI_CR1_SPE) {
        (void)wait_set(SPI_SR_TXE);
        (void)wait_bsy_clear();
        if (SPI1->SR & SPI_SR_RXNE) (void)SPI1->DR;
        SPI1->CR1 &= ~SPI_CR1_SPE;
    }
}

/* 25.3.3: BR -> CPOL/CPHA -> DFF -> LSBFIRST -> SSM/SSI -> MSTR, cuối cùng SPE.
 * DFF, CPOL, CPHA, BR chỉ được đổi khi SPE = 0 nên ghi toàn bộ CR1 trong một lần khi SPI đang tắt. */
void spi1_config(uint32_t br, uint32_t mode, uint32_t frame16)
{
    spi1_disable();

    uint32_t cr1 = ((br & 7U) << SPI_CR1_BR_Pos)
                 | (mode & 3U)                       /* bit 1 CPOL, bit 0 CPHA */
                 | (frame16 ? SPI_CR1_DFF : 0U)      /* 0: 8 bit, 1: 16 bit */
                 | SPI_CR1_SSM | SPI_CR1_SSI         /* NSS nội = SSI = 1: không bị MODF */
                 | SPI_CR1_MSTR;                     /* LSBFIRST = 0: MSB trước; RXONLY = 0, BIDIMODE = 0: full-duplex */
    SPI1->CR2 = 0U;                                  /* không ngắt, không DMA, SSOE = 0 */
    SPI1->CR1 = cr1;
    SPI1->CR1 = cr1 | SPI_CR1_SPE;
}

/* ------------------------------------------------- trao đổi từng frame */
/* Ghi DR rồi chờ RXNE và đọc DR: mỗi byte gửi đi có đúng một byte nhận về, RX không bao giờ tràn */
spi_status_t spi1_xfer8(uint8_t tx, uint8_t *rx)
{
    spi_status_t s = wait_set(SPI_SR_TXE);
    if (s != SPI_OK) return s;
    SPI1->DR = tx;                                   /* 25.5: truy cập 16/32 bit; frame 8 bit chỉ dùng DR[7:0] */
    s = wait_set(SPI_SR_RXNE);
    if (s != SPI_OK) return s;
    *rx = (uint8_t)SPI1->DR;                         /* đọc DR xóa RXNE */
    return SPI_OK;
}

spi_status_t spi1_xfer16(uint16_t tx, uint16_t *rx)
{
    spi_status_t s = wait_set(SPI_SR_TXE);
    if (s != SPI_OK) return s;
    SPI1->DR = tx;
    s = wait_set(SPI_SR_RXNE);
    if (s != SPI_OK) return s;
    *rx = (uint16_t)SPI1->DR;
    return SPI_OK;
}

/* Truyền khối, mỗi byte chờ xong rồi mới nạp byte sau (có khoảng nghỉ giữa các byte) */
spi_status_t spi1_transfer(const uint8_t *tx, uint8_t *rx, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        spi_status_t s = spi1_xfer8(tx[i], &rx[i]);
        if (s != SPI_OK) return s;
    }
    spi_status_t s = wait_bsy_clear();               /* TXE đã = 1 vì byte cuối đã nhận xong */
    return s;
}

/* Truyền khối liên tục theo 25.3.5: nạp byte i+1 vào Tx buffer trong lúc byte i đang dịch.
 * Nếu CPU đọc RX không kịp trước khi byte kế tiếp dịch xong thì OVR. */
spi_status_t spi1_transfer_burst(const uint8_t *tx, uint8_t *rx, uint32_t n)
{
    spi_status_t s;
    if (n == 0U) return SPI_OK;

    if ((s = wait_set(SPI_SR_TXE)) != SPI_OK) return s;
    SPI1->DR = tx[0];
    for (uint32_t i = 1; i < n; i++) {
        if ((s = wait_set(SPI_SR_TXE)) != SPI_OK) return s;
        SPI1->DR = tx[i];
        if ((s = wait_set(SPI_SR_RXNE)) != SPI_OK) return s;
        rx[i - 1U] = (uint8_t)SPI1->DR;
    }
    if ((s = wait_set(SPI_SR_RXNE)) != SPI_OK) return s;
    rx[n - 1U] = (uint8_t)SPI1->DR;
    if ((s = wait_set(SPI_SR_TXE)) != SPI_OK) return s;
    if ((s = wait_bsy_clear()) != SPI_OK) return s;

    if (SPI1->SR & SPI_SR_OVR) {                     /* 25.3.10: xóa OVR = đọc DR rồi đọc SR */
        (void)SPI1->DR;
        (void)SPI1->SR;
        return SPI_ERR_OVR;
    }
    return SPI_OK;
}
