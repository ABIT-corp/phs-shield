#include "stm32f4xx_hal.h"
#include "flash_memory.h"


static uint32_t GetSector(uint32_t Address);

int32_t
flash_sector_write(uint8_t sector, uint8_t *buffer, uint16_t *total_byte)
{
    int32_t status, i;
    uint32_t address, data, total;

    if(*total_byte > 1024){
        return 2;
    }
    if(sector > 128){
        return 3;
    }

    if(sector == 0 &&  *total_byte == 5){
        if(buffer[0] == 'E' && buffer[1] == 'R' && buffer[2] == 'A' && buffer[3] == 'S' && buffer[4] == 'E'){
            status = flash_erase(ADDR_FLASH_SECTOR_11, ADDR_FLASH_SECTOR_12);
            return status;
        }else{
            return 4;
        }
    }

    total = *total_byte;
    for(i=0;i<total;i+=4){
        address = ADDR_FLASH_SECTOR_11 + 0x400 * (sector-1) + i;
        data = buffer[i+3];
        data <<= 8;
        data |= buffer[i+2];
        data <<= 8;
        data |= buffer[i+1];
        data <<= 8;
        data |= buffer[i+0];
        status = flash_write(address, data);
        if(status){
            break;
        }
        *total_byte = i+4;
    }
    if(total < *total_byte){
        *total_byte = total;
    }
    return 0;
}


int32_t
flash_sector_read(uint8_t sector, uint8_t *buffer, uint16_t *total_byte)
{
    int32_t i;
    uint32_t address, data;

    if(*total_byte > 1024){
        return 2;
    }

    if(sector == 0 || sector > 128){
        return 3;
    }

    for(i=0;i<*total_byte;i+=4){
        address = ADDR_FLASH_SECTOR_11 + 0x400 * (sector-1) + i;
        data = *(int *)address;

        if(i+0<*total_byte){
            buffer[i+0] = (data >>  0) & 0xff;
        }else{
            break;
        }
        if(i+1<*total_byte){
            buffer[i+1] = (data >>  8) & 0xff;
        }else{
            break;
        }
        if(i+2<*total_byte){
            buffer[i+2] = (data >> 16) & 0xff;
        }else{
            break;
        }
        if(i+3<*total_byte){
            buffer[i+3] = (data >> 24) & 0xff;
        }else{
            break;
        }
    }

    return 0;
}


int32_t
flash_erase(uint32_t start_address, uint32_t end_address)
{
    FLASH_EraseInitTypeDef erase_struct;
    uint32_t sector_error, status;
    uint32_t first_sector, number_sectors;

    /* Unlock the Flash to enable the flash control register access */
    HAL_FLASH_Unlock();

    /* Get the 1st sector to erase */
    first_sector = GetSector(start_address);
    /* Get the number of sector to erase from 1st sector*/
    number_sectors = GetSector(end_address) - first_sector + 1;

    /* Fill EraseInit structure*/
    erase_struct.TypeErase    = TYPEERASE_SECTORS;
    erase_struct.VoltageRange = VOLTAGE_RANGE_3;
    erase_struct.Sector       = first_sector;
    erase_struct.NbSectors    = number_sectors;

    status = 0;
    if (HAL_FLASHEx_Erase(&erase_struct, &sector_error) != HAL_OK){
        status = 1;
    }

    /* Lock the Flash to disable the flash control register access */
    HAL_FLASH_Lock();

    return status;
}


int32_t
flash_write(uint32_t address, uint32_t data)
{
    uint32_t error_count, read_data;

    /* Unlock the Flash to enable the flash control register access */
    HAL_FLASH_Unlock();

    /* Program the user Flash area word by word */
    if (HAL_FLASH_Program(TYPEPROGRAM_WORD, address, data) != HAL_OK){
        return 1;
    }

    /* Lock the Flash to disable the flash control register access */
    HAL_FLASH_Lock();

    /* Check if the programmed data is OK */
    read_data = *(uint32_t *)address;

    error_count = 0;
    if (read_data != data){
        error_count++;
    }

    /*Check if there is an issue to program data*/
    if (error_count){
        return 2;
    }else{
        return 0;
    }
}


/**
  * Gets the sector of a given address
  * @param  None
  * @return The sector of a given address
  */
static uint32_t 
GetSector(uint32_t Address)
{
  uint32_t sector = 0;

    if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0)){
      sector = 0;
    }else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1)){
      sector = 1;
    }else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2)){
      sector = 2;
    }else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3)){
      sector = 3;
    }else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4)){
      sector = 4;
    }else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5)){
      sector = 5;
    }else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6)){
      sector = 6;
    }else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7)){
      sector = 7;
    }else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8)){
      sector = 8;
    }else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9)){
      sector = 9;
    }else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10)){
      sector = 10;
    }else if((Address < ADDR_FLASH_SECTOR_12) && (Address >= ADDR_FLASH_SECTOR_11)){
      sector = 11;
    }else{
      sector = 11;
  }

  return sector;
}


