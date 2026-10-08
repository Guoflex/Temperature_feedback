#include "flash.h"

volatile FLASH_Status FLASHStatus = FLASH_BUSY; //Flash操作状态变量

/*
 * Name:	    WriteFlashOneWord
 * Function:	向内部Flash写入32位数据
 * Input:	    WriteAddress：数据要写入的目标地址（偏移地址）
 *              WriteData：   写入的数据
 */
void WriteFlashOneWord(uint32_t WriteAddress, uint32_t WriteData)
{   
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    
    // 解锁Flash
    if(HAL_FLASH_Unlock() != HAL_OK)
    {
        FLASHStatus = FLASH_ERROR_PG1;
        return;
    }
    
    // 配置擦除参数
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Page = (WriteAddress - FLASH_BASE) / FLASH_PAGE_SIZE;
    EraseInitStruct.NbPages = 1;
    
    // 执行擦除操作
    if(HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
    {
        FLASHStatus = FLASH_ERROR_PG1;
        HAL_FLASH_Lock();
        return;
    }
    
    // 写入数据（STM32G0支持8字节编程）
    uint64_t data64 = WriteData;
    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, WriteAddress, data64) != HAL_OK)
    {
        FLASHStatus = FLASH_ERROR_PG1;
        HAL_FLASH_Lock();
        return;
    }
    
    // 锁定Flash
    HAL_FLASH_Lock();
    FLASHStatus = FLASH_COMPLETE;
}









