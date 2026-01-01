# ESP32-S3 SD Card Test (SPI Mode)

Simple diagnostic tool to test SD card using SPI interface on ESP32-S3 SPK board.

> [!WARNING]
> **AI-Powered Project Disclaimer**
>
> This project was developed with AI assistance. The code is provided "as is" without any warranty or guarantee.
>
> **Use at your own risk!**

## For ESP32-S3 SPK Board

This test is specifically for **ESP32-S3 SPK** which uses **SPI** for SD card (not SD_MMC).

## Pin Configuration

From ESP32-S3 SPK schematic:

```
SD Card SPI Pins:
├── CS (Chip Select):  GPIO2  (DAT3)
├── MOSI (Data Out):   GPIO3  (CMD/DI)
├── MISO (Data In):    GPIO12 (DATA0/D0)
└── CLK (Clock):       GPIO11 (CLK/SCLK)
```

## Installation

1. Open `sd-card-test-spi.ino` in Arduino IDE
2. Select board: **ESP32S3 Dev Module**
3. Upload to your ESP32-S3 SPK
4. Open Serial Monitor (115200 baud)
5. Watch the diagnostic output

## Expected Output

### Success:
```
========================================
ESP32-S3 SD Card Test (SPI Mode)
========================================

Using SPI pins:
  CS  : GPIO2
  MOSI: GPIO3
  MISO: GPIO12
  CLK : GPIO11

TEST: Attempting SD.begin()...
✓ SUCCESS: SD card mounted in SPI mode

--- SD CARD INFORMATION ---
Card Type: SDHC
Card Size: 7580 MB
---------------------------
```

## Troubleshooting

1. **Ensure SD card is FAT32 formatted**
2. **Check SD card is fully inserted**
3. **Try different SD card**

## License

Open source. Feel free to modify.
