#ifndef FLASH_MEMORY_H
#define FLASH_MEMORY_H

/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000)
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000)
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000)
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000)
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000)
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000)
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000)
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000)
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000)
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000)
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000)
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000)
#define ADDR_FLASH_SECTOR_12    ((uint32_t)0x08100000)  /* virtual sector */


int32_t flash_erase(uint32_t start_address, uint32_t end_address);
int32_t flash_write(uint32_t address, uint32_t data);
int32_t flash_sector_write(uint8_t sector, uint8_t *buffer, uint16_t *total_byte);
int32_t flash_sector_read(uint8_t sector, uint8_t *buffer, uint16_t *total_byte);


#endif
