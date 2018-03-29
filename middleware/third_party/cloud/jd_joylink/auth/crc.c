#include "crc.h"
unsigned short CRC16(const unsigned char* buffer, uint32_t size)
{
	uint16_t crc = 0xFFFF;  

	if (buffer && size)    
		while (size--)    
		{
			crc = (crc >> 8) | (crc << 8);
			crc ^= *buffer++;
			crc ^= ((unsigned char) crc) >> 4;
			crc ^= crc << 12;
			crc ^= (crc & 0xFF) << 5;
		}  
	return crc;
}

unsigned char crc8(unsigned char *A,unsigned char n)
{
	unsigned char i;
	unsigned char checksum = 0;

	while(n--)
	{
		for(i=1;i!=0;i*=2)
		{
			if( (checksum&1) != 0 )
			{
				checksum /= 2;
				checksum ^= 0X8C;
			}
			else
			{
				checksum /= 2;
			}

			if( (*A & i) != 0 )
			{
				checksum ^= 0X8C;
			}
		}
		A++;
	}
	return(checksum);
}

// mtk54101 modify : slim, delete the crc32_table, because it is const.
/*
uint32_t crc32_table[256];
void make_crc32_table()
{
    uint32_t c;
    int i = 0;
    int bit = 0;
      
    for(i = 0; i < 256; i++)
    {
        c  = (uint32_t)i;
        for(bit = 0; bit < 8; bit++)
        {
            if(c&1)
                c = (c >> 1)^(0xEDB88320);
            else
                c =  c >> 1;
        }
        crc32_table[i] = c;
    }
}

uint32_t make_crc(uint32_t crc, unsigned char *string, uint32_t size)
{
    while(size--)
        crc = (crc >> 8)^(crc32_table[(crc ^ *string++)&0xff]);

    return crc;  
}
*/
void make_crc32_table()
{

}

uint32_t make_crc(uint32_t crc, unsigned char *string, uint32_t size)
{    
    uint32_t c;
    int i = 0;
    int bit = 0;
    uint32_t crc32_table[256]; 
    
    for(i = 0; i < 256; i++)
    {
        c  = (uint32_t)i;
        for(bit = 0; bit < 8; bit++)
        {
            if(c&1)
                c = (c >> 1)^(0xEDB88320);
            else
                c =  c >> 1;
        }
        crc32_table[i] = c;
    }
    
    while(size--)
        crc = (crc >> 8)^(crc32_table[(crc ^ *string++)&0xff]);

    return crc;  
}

