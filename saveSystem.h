#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <stdint.h>

// Offset para salvar os dados na flash (1,5 MB após o inicio, evitando sobrescrever o codigo)
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

// Prototipos das funções
float read_float_from_flash();
void write_float_to_flash(float value);
void clearSaveData();


#endif
