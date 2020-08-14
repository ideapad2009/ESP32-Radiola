/******************************************************************************
 * 
 * Copyright 2017 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define TAG "GPIO"
#include "driver/gpio.h"
#include "gpios.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "main.h"
#include "eeprom.h"

static xSemaphoreHandle muxnvs = NULL;
const char hardware[] = {"hardware"};
const char gpio_space[] = {"gpio_space"};

// init a gpio as output
// void gpio_output_conf(gpio_num_t gpio)
// {
// 	gpio_config_t gpio_conf;
// 	gpio_conf.pin_bit_mask = ((uint64_t)(((uint64_t)1) << gpio));
// 	gpio_conf.mode = GPIO_MODE_OUTPUT;
// 	gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
// 	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
// 	gpio_conf.intr_type = GPIO_INTR_DISABLE;
// 	gpio_config(&gpio_conf);
// 	gpio_set_level(gpio, 1);
// }

// open and read the gpio hardware setting
//
esp_err_t open_partition(const char *partition_label, const char *namespace, nvs_open_mode open_mode, nvs_handle *handle)
{
	esp_err_t err;
	if (muxnvs == NULL)
		muxnvs = xSemaphoreCreateMutex();
	xSemaphoreTake(muxnvs, portMAX_DELAY);
	err = nvs_flash_init_partition(partition_label);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "Hardware partition not found");
		return err;
	}
	//	ESP_ERROR_CHECK(nvs_open_from_partition(partition_label, namespace, open_mode, handle));
	err = nvs_open_from_partition(partition_label, namespace, open_mode, handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "Namespace %s not found, ERR: %x", namespace, err);
		nvs_flash_deinit_partition(partition_label);
		xSemaphoreGive(muxnvs);
	}
	return err;
}
void close_partition(nvs_handle handle, const char *partition_label)
{
	nvs_commit(handle); // if a write is pending
	nvs_close(handle);
	nvs_flash_deinit_partition(partition_label);
	xSemaphoreGive(muxnvs);
}

esp_err_t gpio_get_nvs(const char *name_pin, gpio_num_t *gpio_num)
{
	nvs_handle hardware_handle;
	uint8_t val;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READONLY, &hardware_handle);
	if (err == ESP_OK)
	{
		err |= nvs_get_u8(hardware_handle, name_pin, &val);
		*gpio_num = val;
	}
	else
		ESP_LOGE("gpio_get_nvs", "for %s fail, err No: 0x%x", name_pin, err);
	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_nvs(const char *name_pin, gpio_num_t gpio_num)
{
	nvs_handle hardware_handle;
	uint8_t val = gpio_num;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);
	if (err == ESP_OK)
		err |= nvs_set_u8(hardware_handle, name_pin, val);
	else
		ESP_LOGE("gpio_set_nvs", "for %s fail, err No: 0x%x", name_pin, err);
	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_set_spi_bus(uint8_t spi_no, gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_spi_bus");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "K_SPI", spi_no);
	err |= nvs_set_u8(hardware_handle, "P_MISO", (uint8_t)miso);
	err |= nvs_set_u8(hardware_handle, "P_MOSI", (uint8_t)mosi);
	err |= nvs_set_u8(hardware_handle, "P_CLK", (uint8_t)sclk);
	if (err != ESP_OK)
		ESP_LOGE(TAG, "gpio_set_spi_bus err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_spi_bus(uint8_t *spi_no, gpio_num_t *miso, gpio_num_t *mosi, gpio_num_t *sclk)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		if (miso != NULL)
			*miso = PIN_NUM_MISO;
		if (mosi != NULL)
			*mosi = PIN_NUM_MOSI;
		if (sclk != NULL)
			*sclk = PIN_NUM_CLK;
		if (spi_no != NULL)
			*spi_no = KSPI;
		return err;
	}

	if (spi_no != NULL)
	{
		gpio_num_t val;
		err |= gpio_get_nvs("K_SPI", &val);
		*spi_no = val;
	}
	if (miso != NULL)
	{
		err |= gpio_get_nvs("P_MISO", miso);
	}
	if (mosi != NULL)
	{
		err |= gpio_get_nvs("P_MOSI", mosi);
	}
	if (sclk != NULL)
	{
		err |= gpio_get_nvs("P_CLK", sclk);
	}

	return err;
}

esp_err_t gpio_get_vs1053(gpio_num_t *xcs, gpio_num_t *xdcs, gpio_num_t *dreq)
{
	// init default
	esp_err_t err = ESP_OK;
	if (!option_get_gpio_mode())
	{
		*xcs = PIN_NUM_XCS;
		*xdcs = PIN_NUM_XDCS;
		*dreq = PIN_NUM_DREQ;
		return err;
	}

	err |= gpio_get_nvs("P_XCS", xcs);
	err |= gpio_get_nvs("P_XDCS", xdcs);
	err |= gpio_get_nvs("P_DREQ", dreq);
	return err;
}

esp_err_t gpio_set_vs1053(gpio_num_t xcs, gpio_num_t xdcs, gpio_num_t dreq)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_vs1053");
		return err;
	}
	err = nvs_set_u8(hardware_handle, "P_XCS", (uint8_t)xcs);
	err |= nvs_set_u8(hardware_handle, "P_XDCS", (uint8_t)xdcs);
	err |= nvs_set_u8(hardware_handle, "P_DREQ", (uint8_t)dreq);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_vs1053 err 0x%x", err);
	close_partition(hardware_handle, hardware);
	return err;
}

void option_get_lcd_rotat(uint8_t *rt)
{
	*rt = ((g_device->options) & Y_ROTAT) ? 1 : 0;
}

void option_set_lcd_rotat(uint8_t rt)
{
	if (rt == 0)
		g_device->options &= N_ROTAT;
	else
		g_device->options |= Y_ROTAT;
	saveDeviceSettings(g_device);
}

void option_get_ddmm(uint8_t *ddmm)
{
	*ddmm = ((g_device->options) & Y_DDMM) ? 1 : 0;
}

void option_set_ddmm(uint8_t ddmm)
{
	if (ddmm == 0)
		g_device->options &= N_DDMM;
	else
		g_device->options |= Y_DDMM;
	saveDeviceSettings(g_device);
}

bool option_get_gpio_mode()
{
	if (g_device->options & Y_GPIOMODE)
		return true;
	else
		return false;
}

void option_set_gpio_mode(uint8_t gpio_mode)
{
	if (gpio_mode == 0)
		g_device->options &= N_GPIOMODE;
	else
		g_device->options |= Y_GPIOMODE;
	saveDeviceSettings(g_device);
}

void option_get_lcd_out(uint32_t *lcd_out)
{
	*lcd_out = g_device->lcd_out;
}
void option_set_lcd_out(uint32_t lcd_out)
{
	g_device->lcd_out = lcd_out;
	saveDeviceSettings(g_device);
}

esp_err_t gpio_get_encoders(gpio_num_t *enca, gpio_num_t *encb, gpio_num_t *encbtn)
{
	// init default
	esp_err_t err = ESP_OK;
	if (!option_get_gpio_mode())
	{
		*enca = PIN_ENC0_A;
		*encb = PIN_ENC0_B;
		*encbtn = PIN_ENC0_BTN;
		return err;
	}

	err |= gpio_get_nvs("P_ENC0_A", enca);
	err |= gpio_get_nvs("P_ENC0_B", encb);
	err |= gpio_get_nvs("P_ENC0_BTN", encbtn);
	return err;
}

esp_err_t gpio_set_encoders(gpio_num_t enca, gpio_num_t encb, gpio_num_t encbtn)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_encoders");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_ENC0_A", (uint8_t)enca);
	err |= nvs_set_u8(hardware_handle, "P_ENC0_B", (uint8_t)encb);
	err |= nvs_set_u8(hardware_handle, "P_ENC0_BTN", (uint8_t)encbtn);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_encoders err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_i2c(gpio_num_t *sda, gpio_num_t *scl)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*scl = PIN_I2C_SCL;
		*sda = PIN_I2C_SDA;
		return err;
	}

	err |= gpio_get_nvs("P_I2C_SCL", scl);
	err |= gpio_get_nvs("P_I2C_SDA", sda);

	return err;
}

esp_err_t gpio_set_i2c(gpio_num_t sda, gpio_num_t scl)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_i2c");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_I2C_SCL", (uint8_t)scl);
	err |= nvs_set_u8(hardware_handle, "P_I2C_SDA", (uint8_t)sda);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_i2c err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}
esp_err_t gpio_get_uart(gpio_num_t *rxd, gpio_num_t *txd)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*rxd = PIN_NUM_RXD;
		*txd = PIN_NUM_TXD;
		return err;
	}

	err |= gpio_get_nvs("P_RXD", rxd);
	err |= gpio_get_nvs("P_TXD", txd);

	return err;
}

esp_err_t gpio_set_uart(gpio_num_t rxd, gpio_num_t txd)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_uart");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_TXD", (uint8_t)txd);
	err |= nvs_set_u8(hardware_handle, "P_RXD", (uint8_t)rxd);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_uart err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_spi_lcd(gpio_num_t *cs, gpio_num_t *a0)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*cs = PIN_LCD_CS;
		*a0 = PIN_LCD_A0;
		return err;
	}

	err |= gpio_get_nvs("P_LCD_CS", cs);
	err |= gpio_get_nvs("P_LCD_A0", a0);

	return err;
}

esp_err_t gpio_set_spi_lcd(gpio_num_t cs, gpio_num_t a0)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_spi_lcd");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_LCD_CS", (uint8_t)cs);
	err |= nvs_set_u8(hardware_handle, "P_LCD_A0", (uint8_t)a0);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_spi_lcd err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_ir_signal(gpio_num_t *ir)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*ir = PIN_IR_SIGNAL;
		return err;
	}

	err |= gpio_get_nvs("P_IR_SIGNAL", ir);

	return err;
}

esp_err_t gpio_set_ir_signal(gpio_num_t ir)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_ir_signal");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_IR_SIGNAL", (uint8_t)ir);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_ir_signal err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_backlightl(gpio_num_t *led)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*led = PIN_LCD_BACKLIGHT;
		return err;
	}

	err |= gpio_get_nvs("P_BACKLIGHT", led);

	return err;
}

esp_err_t gpio_set_backlightl(gpio_num_t led)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_backlightl");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_BACKLIGHT", (uint8_t)led);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_backlightl err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_tachometer(gpio_num_t *tach)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*tach = PIN_NUM_TACH;
		return err;
	}

	err |= gpio_get_nvs("P_TACHOMETER", tach);

	return err;
}

esp_err_t gpio_set_tachometer(gpio_num_t tach)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_tachometer");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_TACHOMETER", (uint8_t)tach);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_tachometer err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_ds18b20(gpio_num_t *ds18b20)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*ds18b20 = PIN_NUM_DS18B20;
		return err;
	}

	err |= gpio_get_nvs("P_DS18B20", ds18b20);

	return err;
}

esp_err_t gpio_set_ds18b20(gpio_num_t ds18b20)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_ds18b20");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_DS18B20", (uint8_t)ds18b20);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_ds18b20 err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_stb(gpio_num_t *stb)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*stb = PIN_NUM_STB;
		return err;
	}
	err |= gpio_get_nvs("PIN_NUM_STB", stb);
	return err;
}

esp_err_t gpio_set_stb(gpio_num_t stb)
{
	return gpio_set_nvs("PIN_NUM_STB", stb);
}

esp_err_t gpio_get_touch(gpio_num_t *touch)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*touch = PIN_TOUCH_CS;
		return err;
	}

	err |= gpio_get_nvs("P_TOUCH_CS", touch);

	return err;
}

esp_err_t gpio_set_touch(gpio_num_t touch)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_touch");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_TOUCH_CS", (uint8_t)touch);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_touch err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_fanspeed(gpio_num_t *fanspeed)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*fanspeed = PIN_NUM_PWM;
		return err;
	}

	err |= gpio_get_nvs("P_FAN_SPEED", fanspeed);

	return err;
}

esp_err_t gpio_set_fanspeed(gpio_num_t fanspeed)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_fanspeed");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_FAN_SPEED", (uint8_t)fanspeed);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_fanspeed err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_ldr(gpio_num_t *ldr)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*ldr = PIN_NUM_LDR;
		return err;
	}

	err |= gpio_get_nvs("P_LDR", ldr);

	return err;
}

esp_err_t gpio_set_ldr(gpio_num_t ldr)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_ldr");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_LDR", (uint8_t)ldr);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_ldr err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}
esp_err_t gpio_get_buzzer(gpio_num_t *buzzer)
{
	esp_err_t err = ESP_OK;
	// init default
	if (!option_get_gpio_mode())
	{
		*buzzer = PIN_NUM_BUZZ;
		return err;
	}

	err |= gpio_get_nvs("P_BUZZER", buzzer);

	return err;
}

esp_err_t gpio_set_buzzer(gpio_num_t buzzer)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_buzzer");
		return err;
	}

	err = nvs_set_u8(hardware_handle, "P_BUZZER", (uint8_t)buzzer);
	if (err != ESP_OK)
		ESP_LOGD(TAG, "gpio_set_buzzer err 0x%x", err);

	close_partition(hardware_handle, hardware);
	return err;
}

esp_err_t gpio_get_ir_key(nvs_handle handle, const char *key, uint32_t *out_set1, uint32_t *out_set2)
{
	// init default
	esp_err_t err = ESP_OK;
	*out_set1 = 0;
	*out_set2 = 0;
	size_t required_size;
	err |= nvs_get_str(handle, key, NULL, &required_size);
	if (required_size > 1)
	{
		char *string = malloc(required_size);
		err |= nvs_get_str(handle, key, string, &required_size);
		sscanf(string, "%x %x", out_set1, out_set2);
		free(string);
	}
	ESP_LOGD(TAG, "Load from NVS Key: %s, set1: %x, set2: %x, err: %d", key, *out_set1, *out_set2, err);

	return err;
}

esp_err_t gpio_set_ir_key(const char *key, char *ir_keys)
{
	nvs_handle hardware_handle;
	esp_err_t err = open_partition(hardware, gpio_space, NVS_READWRITE, &hardware_handle);

	if (err != ESP_OK)
	{
		ESP_LOGD(TAG, "in gpio_set_buzzer");
		return err;
	}

	err = nvs_set_str(hardware_handle, key, ir_keys);
	ESP_LOGD(TAG, "Save Key: %s, ir_keys: %s, err: %d", key, ir_keys, err);
	close_partition(hardware_handle, hardware);
	return err;
}