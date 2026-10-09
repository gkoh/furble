// Hardware support adapted from Free-Ink/freeink-sdk (MIT), commit
// 425d200a8ea447326b4b9696e4e47dc84ad9d7f6: XteinkDetect, InputManager,
// BoardConfig and UC8253/UC8279 drivers. See vendor/FreeInk-LICENSE.txt.
#include "x3/FurbleX3Platform.h"
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_rom_sys.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "FurbleFatal.h"
#include "FurbleX3Renderer.h"
#include "vendor/Uc8253X3Luts.h"
#include "vendor/Uc8279X3Luts.h"
#include "x3/FurbleX3Keys.h"

namespace Furble {
namespace {
constexpr gpio_num_t SCK = GPIO_NUM_8, MOSI = GPIO_NUM_10, CS = GPIO_NUM_21, DC = GPIO_NUM_4,
                     RESET = GPIO_NUM_5, BUSY = GPIO_NUM_6, POWER = GPIO_NUM_3;
constexpr int WIDTH = 792, HEIGHT = 528, STRIDE = 99, FRAME_SIZE = STRIDE * HEIGHT;
const char *TAG = "x3";
spi_device_handle_t spi = nullptr;
i2c_master_dev_handle_t gauge = nullptr;
adc_oneshot_unit_handle_t adc = nullptr;
SemaphoreHandle_t displayLock = nullptr;
uint8_t *frame = nullptr;
bool uc8279 = false, first = true, failed = false, asleep = false, shuttingDown = false;
X3KeyDebouncer debounce;
X3KeyReadGuard keyRead;
uint8_t delivered = 0;
uint32_t tick() {
  return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}
void delayMs(unsigned ms) {
  vTaskDelay(std::max<TickType_t>(1, pdMS_TO_TICKS(ms)));
}
void output(gpio_num_t pin, int value) {
  // Direction alone leaves native UART/JTAG IO_MUX functions selected.
  // In particular GPIO21 must become GPIO output before it can drive EPD CS.
  gpio_config_t config = {};
  config.pin_bit_mask = 1ULL << pin;
  config.mode = GPIO_MODE_OUTPUT;
  config.intr_type = GPIO_INTR_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&config));
  gpio_set_level(pin, value);
}
bool failureOutput(gpio_num_t pin, int value) {
  gpio_config_t config = {};
  config.pin_bit_mask = 1ULL << pin;
  config.mode = GPIO_MODE_OUTPUT;
  config.intr_type = GPIO_INTR_DISABLE;
  const esp_err_t configured = gpio_config(&config);
  if (configured != ESP_OK) {
    ESP_LOGE(TAG, "Startup shutdown GPIO %d configuration failed: %s", pin,
             esp_err_to_name(configured));
    return false;
  }
  const esp_err_t set = gpio_set_level(pin, value);
  if (set != ESP_OK) {
    ESP_LOGE(TAG, "Startup shutdown GPIO %d level failed: %s", pin, esp_err_to_name(set));
    return false;
  }
  return true;
}
void reset() {
  gpio_set_level(RESET, 1);
  delayMs(10);
  gpio_set_level(RESET, 0);
  delayMs(50);
  gpio_set_level(RESET, 1);
  delayMs(50);
}
bool waitBusy() {
  // Commands assert BUSY asynchronously; require the low edge before idle.
  const uint32_t start = tick();
  while (gpio_get_level(BUSY) && tick() - start < 1000)
    delayMs(1);
  if (gpio_get_level(BUSY)) {
    ESP_LOGE(TAG, "Panel did not assert BUSY");
    failed = true;
    return false;
  }
  while (!gpio_get_level(BUSY) && tick() - start < 15000)
    delayMs(1);
  if (!gpio_get_level(BUSY)) {
    ESP_LOGE(TAG, "Panel BUSY timeout");
    failed = true;
  }
  return !failed;
}
void write(const uint8_t *bytes, size_t size) {
  if ((failed && !shuttingDown) || !size)
    return;
  spi_transaction_t t = {};
  t.length = size * 8;
  t.tx_buffer = bytes;
  const esp_err_t err = spi_device_polling_transmit(spi, &t);
  if (err != ESP_OK) {
    failed = true;
    ESP_LOGE(TAG, "SPI: %s", esp_err_to_name(err));
  }
}
void cmd(uint8_t command) {
  gpio_set_level(CS, 0);
  gpio_set_level(DC, 0);
  write(&command, 1);
  gpio_set_level(CS, 1);
}
void data(const uint8_t *bytes, size_t size) {
  gpio_set_level(CS, 0);
  gpio_set_level(DC, 1);
  write(bytes, size);
  gpio_set_level(CS, 1);
}
void reg(uint8_t command, std::initializer_list<uint8_t> bytes) {
  cmd(command);
  data(bytes.begin(), bytes.size());
}
void plane(uint8_t command, bool white = false) {
  uint8_t row[STRIDE];
  memset(row, 0xff, sizeof(row));
  cmd(command);
  gpio_set_level(CS, 0);
  gpio_set_level(DC, 1);
  // Controller gate order is bottom-first. CS remains low throughout DTM.
  for (int y = HEIGHT - 1; y >= 0 && !failed; --y)
    write(white ? row : frame + y * STRIDE, STRIDE);
  gpio_set_level(CS, 1);
}
bool probe() {
  output(CS, 1);
  output(SCK, 0);
  output(DC, 1);
  output(MOSI, 0);
  output(RESET, 1);
  reset();
  uint32_t start = tick();
  while (!gpio_get_level(BUSY) && tick() - start < 300)
    delayMs(1);
  gpio_set_level(CS, 0);
  gpio_set_level(DC, 0);
  for (int bit = 7; bit >= 0; --bit) {
    gpio_set_level(SCK, 0);
    gpio_set_level(MOSI, (0x70 >> bit) & 1);
    esp_rom_delay_us(2);
    gpio_set_level(SCK, 1);
    esp_rom_delay_us(2);
  }
  gpio_set_level(SCK, 0);
  gpio_set_level(DC, 1);
  gpio_set_direction(MOSI, GPIO_MODE_INPUT);
  gpio_set_pull_mode(MOSI, GPIO_FLOATING);
  esp_rom_delay_us(2);
  uint8_t ver[3] = {};
  for (auto &byte : ver) {
    for (int bit = 0; bit < 8; ++bit) {
      gpio_set_level(SCK, 0);
      esp_rom_delay_us(2);
      gpio_set_level(SCK, 1);
      esp_rom_delay_us(2);
      byte = (byte << 1) | (gpio_get_level(MOSI) ? 1 : 0);
    }
    gpio_set_level(SCK, 0);
    esp_rom_delay_us(2);
  }
  gpio_set_level(CS, 1);
  gpio_set_direction(MOSI, GPIO_MODE_OUTPUT);
  ESP_LOGI(TAG, "Stock X3 VER probe: %02x %02x %02x", ver[0], ver[1], ver[2]);
  uc8279 = ver[2] == 0x66;
  if (ver[2] == 0xff) {
    // Older field modules have unreadable VER but drive FLG and RMTP.
    // FreeInk's generic probe positively fingerprints that blank-MTP run.
    auto readRegister = [](uint8_t command, uint8_t *out, size_t count) {
      gpio_set_direction(MOSI, GPIO_MODE_OUTPUT);
      gpio_set_level(DC, 0);
      gpio_set_level(CS, 0);
      for (int bit = 7; bit >= 0; --bit) {
        gpio_set_level(MOSI, (command >> bit) & 1);
        esp_rom_delay_us(1);
        gpio_set_level(SCK, 1);
        esp_rom_delay_us(1);
        gpio_set_level(SCK, 0);
      }
      gpio_set_level(DC, 1);
      gpio_set_direction(MOSI, GPIO_MODE_INPUT);
      gpio_set_pull_mode(MOSI, GPIO_PULLUP_ONLY);
      for (size_t i = 0; i < count; ++i) {
        out[i] = 0;
        for (int bit = 0; bit < 8; ++bit) {
          esp_rom_delay_us(1);
          out[i] = (out[i] << 1) | gpio_get_level(MOSI);
          gpio_set_level(SCK, 1);
          esp_rom_delay_us(1);
          gpio_set_level(SCK, 0);
        }
      }
      gpio_set_level(CS, 1);
      gpio_set_direction(MOSI, GPIO_MODE_OUTPUT);
      gpio_set_pull_mode(MOSI, GPIO_FLOATING);
    };
    uint8_t flags1 = 0, flags2 = 0, mtp1[49] = {}, mtp2[49] = {};
    readRegister(0x71, &flags1, 1);
    readRegister(0x71, &flags2, 1);
    if (flags1 == flags2 && flags1 != 0 && flags1 != 0xff && (flags1 & 1)) {
      readRegister(0xa2, mtp1, 49);
      readRegister(0xa2, mtp2, 49);
      bool uniform = true;
      for (unsigned i = 2; i < 49; ++i)
        if (mtp1[i] != mtp1[1])
          uniform = false;
      const bool repeated = memcmp(mtp1 + 1, mtp2 + 1, 48) == 0;
      uc8279 = repeated && (mtp1[1] == 0xa5 || !uniform);
      // UC8253 does not implement RMTP; upstream field observations identify
      // its pulled-up read as uniform ff. Preserve the stock VER=ff fallback
      // only for that consistent read, not arbitrary unknown MTP contents.
      const bool uc8253 = repeated && uniform && mtp1[1] == 0xff;
      ESP_LOGI(TAG, "FLG=%02x MTP=%02x %02x %02x %02x uniform=%d repeat=%d panel=%s", flags1,
               mtp1[1], mtp1[2], mtp1[3], mtp1[4], uniform, repeated,
               uc8279   ? "UC8279d"
               : uc8253 ? "UC8253"
                        : "unknown");
      if (!uc8279 && !uc8253)
        return false;
    } else if (flags1 != 0xff || flags2 != 0xff)
      return false;
  }
  return uc8279 || ver[2] == 0xff;
}
void controllerInit() {
  reset();
  first = true;
  if (uc8279) {
    const auto *script = freeink::kUc8279X3_Init;
    size_t i = 0;
    while (i < sizeof(freeink::kUc8279X3_Init)) {
      uint8_t command = script[i++], count = script[i++];
      cmd(command);
      data(script + i, count);
      i += count;
    }
  } else {
    reg(0x00, {0x3f, 0x0a});
    reg(0x61, {0x03, 0x18, 0x02, 0x58});
    reg(0x65, {0, 0, 0, 0});
    reg(0x03, {0x20});
    reg(0x01, {7, 0x17, 0x3f, 0x3f, 0x17});
    reg(0x82, {0x24});
    reg(0x06, {0x25, 0x25, 0x3c, 0x37});
    reg(0x30, {9});
    reg(0xe1, {2});
    plane(0x10, true);
    cmd(0x11);
    plane(0x13, true);
    cmd(0x11);
  }
}
void refresh() {
  using namespace freeink;
  if (uc8279) {
    cmd(0x91);
    if (first) {
      plane(0x10, true);
      cmd(0x11);
    }
    plane(0x13);
    cmd(0x11);
    reg(0x50, {first ? kUc8279X3_CdiFirst : kUc8279X3_CdiLater});
    for (int i = 0; i < 5; ++i) {
      cmd(kUc8279X3_BwGc[i][0]);
      data(kUc8279X3_BwGc[i] + 1, 42);
    }
  } else {
    reg(0x50, {0x29, 7});
    const uint8_t *lut[] = {lut_x3_vcom_full, lut_x3_ww_full, lut_x3_bw_full, lut_x3_wb_full,
                            lut_x3_bb_full};
    for (int i = 0; i < 5; ++i) {
      cmd(0x20 + i);
      data(lut[i], 42);
    }
    plane(0x10, true);
    cmd(0x11);
    plane(0x13);
  }
  cmd(0x04);
  if (!waitBusy())
    return;
  cmd(0x12);
  if (!waitBusy())
    return;
  if (uc8279)
    reg(0x50, {kUc8279X3_CdiLater});
  plane(0x10);
  cmd(0x11);
  if (uc8279)
    cmd(0x92);
  cmd(0x02);
  if (!waitBusy())
    return;
  first = false;
}
void draw(const X3View &view) {
  xSemaphoreTake(displayLock, portMAX_DELAY);
  if (!failed && !asleep) {
    X3Renderer(frame).render(view);
    refresh();
  }
  xSemaphoreGive(displayLock);
}
bool poll(X3App::KeyEvent &event) {
  int bank1 = -1, bank2 = -1;
  const bool valid = adc_oneshot_read(adc, ADC_CHANNEL_1, &bank1) == ESP_OK
                     && adc_oneshot_read(adc, ADC_CHANNEL_2, &bank2) == ESP_OK;
  const bool wasFailed = keyRead.failed();
  uint8_t sample = 0;
  if (!keyRead.sample(valid, x3DecodeKeys(bank1, bank2, !gpio_get_level(POWER)), sample))
    return false;
  if (!wasFailed && keyRead.failed())
    ESP_LOGW(TAG, "ADC input failed; releasing held keys");
  if (wasFailed && valid)
    ESP_LOGI(TAG, "ADC input recovered");
  uint8_t stable = debounce.update(sample, tick());
  const uint8_t changed = stable ^ delivered;
  // Releases first prevents overlapping commands on a changing ladder.
  for (int pressed = 0; pressed < 2; ++pressed)
    for (unsigned i = 0; i < 7; ++i) {
      uint8_t bit = 1u << i;
      if ((changed & bit) && !!(stable & bit) == !!pressed) {
        delivered ^= bit;
        event = {static_cast<X3App::Key>(i), bool(pressed)};
        return true;
      }
    }
  return false;
}
int16_t batteryPercent() {
  if (!gauge)
    return -1;
  uint8_t reg = 0x2c, bytes[2];
  if (i2c_master_transmit_receive(gauge, &reg, 1, bytes, 2, 100) != ESP_OK)
    return -1;
  unsigned percent = bytes[0] | (bytes[1] << 8);
  return percent <= 100 ? percent : -1;
}
void powerOff() {
  xSemaphoreTake(displayLock, portMAX_DELAY);
  asleep = true;
  // A failed refresh may leave PON asserted. Bypass the render fault latch
  // for this bounded best-effort shutdown; each transfer still reports errors.
  shuttingDown = true;
  if (failed)
    reset();
  cmd(0x02);
  delayMs(10);
  const uint32_t start = tick();
  while (!gpio_get_level(BUSY) && tick() - start < 500)
    delayMs(1);
  if (!gpio_get_level(BUSY))
    ESP_LOGW(TAG, "Shutdown BUSY timeout; attempting DSLP");
  reg(0x07, {0xa5});
  output(GPIO_NUM_13, 0);  // SD rail, not a system latch.
  // The EPD supply is ungated: RESET must stay HIGH to preserve DSLP.
  output(RESET, 1);
  gpio_hold_en(RESET);
  gpio_hold_en(GPIO_NUM_13);
  gpio_deep_sleep_hold_en();
  // LOW wake must be armed after releasing the initiating power press.
  while (!gpio_get_level(POWER))
    delayMs(10);
  delayMs(30);
  ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(1ULL << POWER, ESP_GPIO_WAKEUP_GPIO_LOW));
  esp_deep_sleep_start();
}
}  // namespace

// Initialization can fail before SPI, framebuffer or displayLock exists.
// Do not run normal panel shutdown or reboot into the same failure.
[[noreturn]] void X3Platform::startupFailure() {
  ESP_LOGE(TAG, "Startup failed; entering deep sleep");
  const bool sdOff = failureOutput(GPIO_NUM_13, 0);
  const bool panelReset = failureOutput(RESET, 1);
  if (panelReset)
    gpio_hold_en(RESET);
  if (sdOff)
    gpio_hold_en(GPIO_NUM_13);
  gpio_deep_sleep_hold_en();
  gpio_config_t powerInput = {};
  powerInput.pin_bit_mask = 1ULL << POWER;
  powerInput.mode = GPIO_MODE_INPUT;
  powerInput.intr_type = GPIO_INTR_DISABLE;
  const esp_err_t powerConfigured = gpio_config(&powerInput);
  if (powerConfigured != ESP_OK) {
    ESP_LOGE(TAG, "Power button configuration failed: %s", esp_err_to_name(powerConfigured));
  } else {
    const esp_err_t pullConfigured = gpio_set_pull_mode(POWER, GPIO_PULLUP_ONLY);
    if (pullConfigured != ESP_OK) {
      ESP_LOGE(TAG, "Power button pull-up failed: %s", esp_err_to_name(pullConfigured));
    } else {
      while (!gpio_get_level(POWER))
        delayMs(10);
      delayMs(30);
      const esp_err_t wake =
          esp_deep_sleep_enable_gpio_wakeup(1ULL << POWER, ESP_GPIO_WAKEUP_GPIO_LOW);
      if (wake != ESP_OK)
        ESP_LOGE(TAG, "Power button wake configuration failed: %s", esp_err_to_name(wake));
    }
  }
  esp_deep_sleep_start();
  for (;;)
    delayMs(1000);
}

[[noreturn]] void fatal() {
  X3Platform::startupFailure();
}

esp_err_t X3Platform::init() {
  gpio_hold_dis(RESET);
  gpio_hold_dis(GPIO_NUM_13);
  gpio_deep_sleep_hold_dis();
  gpio_config_t inputs = {};
  inputs.pin_bit_mask = (1ULL << BUSY) | (1ULL << POWER);
  inputs.mode = GPIO_MODE_INPUT;
  inputs.intr_type = GPIO_INTR_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&inputs));
  ESP_ERROR_CHECK(gpio_set_pull_mode(POWER, GPIO_PULLUP_ONLY));
  // The card shares SCLK/MOSI. An unpowered card can clamp these lines even
  // with its CS high, so power it before controller probing/display use.
  output(GPIO_NUM_12, 1);
  output(GPIO_NUM_13, 1);
  delayMs(10);
  if (!probe()) {
    ESP_LOGE(TAG, "Unrecognized X3 panel ID; refusing wrong waveform");
    return ESP_ERR_NOT_SUPPORTED;
  }
  displayLock = xSemaphoreCreateMutex();
  if (!displayLock)
    return ESP_ERR_NO_MEM;
  frame = static_cast<uint8_t *>(heap_caps_malloc(FRAME_SIZE, MALLOC_CAP_8BIT));
  if (!frame)
    return ESP_ERR_NO_MEM;
  spi_bus_config_t bus = {};
  bus.mosi_io_num = MOSI;
  bus.miso_io_num = -1;
  bus.sclk_io_num = SCK;
  bus.quadwp_io_num = -1;
  bus.quadhd_io_num = -1;
  bus.max_transfer_sz = STRIDE;
  esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
  if (err != ESP_OK)
    return err;
  spi_device_interface_config_t dev = {};
  dev.clock_speed_hz = 10000000;
  dev.mode = 0;
  dev.spics_io_num = -1;
  dev.queue_size = 1;
  err = spi_bus_add_device(SPI2_HOST, &dev, &spi);
  if (err != ESP_OK)
    return err;
  adc_oneshot_unit_init_cfg_t unit = {};
  unit.unit_id = ADC_UNIT_1;
  err = adc_oneshot_new_unit(&unit, &adc);
  if (err != ESP_OK)
    return err;
  adc_oneshot_chan_cfg_t channel = {};
  channel.atten = ADC_ATTEN_DB_12;
  channel.bitwidth = ADC_BITWIDTH_12;
  err = adc_oneshot_config_channel(adc, ADC_CHANNEL_1, &channel);
  if (err != ESP_OK)
    return err;
  err = adc_oneshot_config_channel(adc, ADC_CHANNEL_2, &channel);
  if (err != ESP_OK)
    return err;
  i2c_master_bus_config_t ibus = {};
  ibus.i2c_port = I2C_NUM_0;
  ibus.sda_io_num = GPIO_NUM_20;
  ibus.scl_io_num = GPIO_NUM_0;
  ibus.clk_source = I2C_CLK_SRC_DEFAULT;
  ibus.glitch_ignore_cnt = 7;
  ibus.flags.enable_internal_pullup = true;
  i2c_master_bus_handle_t ih = nullptr;
  err = i2c_new_master_bus(&ibus, &ih);
  if (err != ESP_OK)
    return err;
  i2c_device_config_t idev = {};
  idev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  idev.device_address = 0x55;
  idev.scl_speed_hz = 400000;
  err = i2c_master_bus_add_device(ih, &idev, &gauge);
  if (err != ESP_OK)
    return err;
  controllerInit();
  ESP_LOGI(TAG, "%s; framebuffer %d bytes; free heap %u; largest block %u",
           uc8279 ? "UC8279d" : "UC8253", FRAME_SIZE,
           static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
           static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
  return failed ? ESP_FAIL : ESP_OK;
}
X3App::Hooks X3Platform::hooks() {
  return {tick, poll, draw, batteryPercent, powerOff};
}
}  // namespace Furble
