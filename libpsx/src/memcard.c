/*
 * PSXSDK Memory Card Helper Functions
 *
 * These functions help to manage memory card loading/saving
 *
 * Normal file functions can be used to do this, but it will be very tedious...
 */
 
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <fcntl.h>
 #include <memcard.h>
 #include <string.h>
 #include <psx.h>

static unsigned char card_cmd[140];
static unsigned char arr[140];

enum MemCardCmd
{
	MEMCARD_ACCESS_CMD = 0x81,
	MEMCARD_READ_CMD = 'R',
	MEMCARD_WRITE_CMD = 'W'
};

unsigned char McReadSector(int card_slot, int sector, unsigned char *buffer)
{	
	memset(&card_cmd[0], 0, 140);
	
	card_cmd[0] = MEMCARD_ACCESS_CMD;		/*MC access*/
	card_cmd[1] = MEMCARD_READ_CMD;		/*Read command*/
	
	/*Copy frame number to command*/
	card_cmd[4] = sector >> 8;			/*Frame MSB*/
	card_cmd[5] = sector & 0xFF;			/*Frame LSB*/
	
	memset(arr,0,140);
	QueryPAD(card_slot, card_cmd, arr, sizeof(card_cmd));
	
	/*Copy received frame data*/
	memcpy(buffer, &arr[10], 128);
	
	// Positions 6 and 7 belong to ACK's
	// They always should be 0x5C and 0x5D, respectively
	// On the other hand, positions 8 and 9 return MSB and LSB sectors.
	
	if(arr[6] != 0x5C)
	{
		dprintf("Error when writing from Memory Card!\n");
		dprintf("BAD ACK1! Expected 0x5C, got %d\n",arr[6]);
		return '1';
	}
	
	if(arr[7] != 0x5D)
	{
		dprintf("Error when writing from Memory Card!\n");
		dprintf("BAD ACK2! Expected 0x5D, got %d\n",arr[7]);
		return '2';
	}
	
	if(arr[8] != card_cmd[4])
	{
		dprintf("Error when writing from Memory Card!\n");
		dprintf("BAD MSB sector! Expected %d, got %d\n",card_cmd[4],arr[8]);
		return 'M';
	}
	
	if(arr[9] != card_cmd[5])
	{
		dprintf("Error when writing from Memory Card!\n");
		dprintf("BAD LSB sector! Expected %d, got %d\n",card_cmd[5],arr[9]);
		return 'L';
	}
	
	/*Return RW status*/
	return arr[139];
}

/*
 * 
 * name: unsigned char McWriteSector(int card_slot, int sector, char *buffer)
 * @param
 * 			
 * @return
 * 
 */
 
unsigned char McWriteSector(int card_slot, int sector, unsigned char *buffer)
{
	int i;
	
	memset(&card_cmd[0], 0, 140);
	
	card_cmd[0] = MEMCARD_ACCESS_CMD;		/*MC access*/
	card_cmd[1] = MEMCARD_WRITE_CMD;		/*Write command*/
	
	/*Copy frame number to command*/
	card_cmd[4] = sector >> 8;			/*Frame MSB*/
	card_cmd[5] = sector & 0xFF;			/*Frame LSB*/
	
	memcpy(&card_cmd[6], buffer, 128);

	/* Compute checksum */
	for(i = 4, card_cmd[134] = 0; i < 134; i++)
		card_cmd[134] ^= card_cmd[i];
	
	memset(arr,0,140);
	QueryPAD(card_slot, card_cmd, arr, sizeof(card_cmd));
	
	if(arr[135] != 0x5C)
	{
		dprintf("Error when writing from Memory Card!\n");
		dprintf("BAD ACK1! Expected 0x5C, got %d\n",arr[135]);
		// Positions 6 and 7 belong to ACK's
		// They always should be 0x5C and 0x5D, respectively
		// On the other hand, positions 8 and 9 return MSB and LSB sectors.
		return '1';
	}
	
	if(arr[136] != 0x5D)
	{
		dprintf("Error when writing from Memory Card!\n");
		dprintf("BAD ACK1! Expected 0x5C, got %d\n",arr[136]);
		return '2';
	}
	
/*	if(arr[4] != card_cmd[4])
	{
		dprintf("Error when writing from Memory Card!\n");
		dprintf("BAD MSB Sector! Expected %d, got %d\n",card_cmd[4],arr[4]);
		return 0;
	}
	
	if(arr[5] != card_cmd[5])
	{
		dprintf("Error when writing from Memory Card!\n");
		dprintf("BAD LSB Sector! Expected %d, got %d\n",card_cmd[5],arr[5]);
		return 0;
	} */

	/*Return RW status*/
	return arr[137];
}

MEMCARD_STATUS McGetStatus(int card_slot)
{
	unsigned int status = MEMCARD_STATUS_UNKNOWN;
	
	memset(&card_cmd[0], 0, 140);
	
	card_cmd[0] = 0x81;		/*MC access*/
	card_cmd[1] = 0x52;		/*Read command*/
	
	/*Copy frame number to command*/
	card_cmd[4] = 0;//sector >> 8;			/*Frame MSB*/
	card_cmd[5] = 0;//sector & 0xFF;			/*Frame LSB*/
	
	QueryPAD(card_slot, card_cmd, arr, sizeof(card_cmd));

	if(arr[2] == 0x5a && arr[3] == 0x5d)
	{
		status |= MEMCARD_CONNECTED;
	}
	
	if(arr[6] == 'M' && arr[7] == 'C')
	{
		status |= MEMCARD_FORMATTED;
	}
	
	return status;
}
