# M031BSP_SPI_PDMA_FLASH
 M031BSP_SPI_PDMA_FLASH

update @ 2021/11/08

1. with SPI inital to access SPI flash , by regular write/read and PDMA write/read

2. platform : M032 EVM with UART terminal (PB.12 , PB.13)

3. SPI flash pin configuration

	- SPI SS : PA.3
	
	- SPI CLK : PA.2	
	
	- SPI MISO : PA.1
	
	- SPI MOSI : PA.0
	
	- SPI WP : PA.4 , set output as high
	
	- SPI HOLD : PA.5 , set output as high

4. open terminal , press ? , will display description as below 

![image](https://github.com/released/M031BSP_SPI_PDMA_FLASH/blob/mian/KEY_questionmark.jpg)
	
5. key 1 ~ 5 , separate function with 1 key

6. key 6 : combo function , erase SPI flash > fill data > write > read > compare

![image](https://github.com/released/M031BSP_SPI_PDMA_FLASH/blob/main/KEY_6.jpg)

7. key 7 : combo function , erase SPI flash > fill data > PDMA write > PDMA read > compare

![image](https://github.com/released/M031BSP_SPI_PDMA_FLASH/blob/main/KEY_7.jpg)
	
8. under project_config.h , enable ENABLE_MANUAL_SS or ENABLE_AUTO_SS , to test control SS pin by manual or auto

9. under project_config.h , enable TEST_SPI_PAGE or TEST_SPI_SECTOR , to test read/write page with page size or sector size
