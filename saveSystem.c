#include "saveSystem.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>

float read_float_from_flash() {
    const uint32_t *flash_data = (const uint32_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    float value;
    memcpy(&value, flash_data, sizeof(float));
    return value;
}

void write_float_to_flash(float value) {
    uint32_t ints = save_and_disable_interrupts();

    // Converte o float para uint32_t para armazenar como bytes
    uint32_t float_bytes;
    memcpy(&float_bytes, &value, sizeof(float));

    // Apaga o setor (4 KB) onde será gravado o valor
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);

    // Grava o valor na flash (em blocos de 256 bytes)
    flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*) &float_bytes, sizeof(float));

    restore_interrupts(ints);
}

void clearSaveData() 
{
	uint32_t interruptions = save_and_disable_interrupts();    
	flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);    
	flash_range_program(FLASH_TARGET_OFFSET, 0x00, FLASH_PAGE_SIZE);    
	restore_interrupts(interruptions);
}
