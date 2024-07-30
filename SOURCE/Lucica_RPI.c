#include <stdio.h>
#include "../INCLUDE/EVE.h"

void main(void)
{
	
	EVE_Init();
	EVE_BEGIN(EVE_BEGIN_BITMAPS);
	EVE_VERTEX2F(200, 200);	//koordinate
	EVE_COLOR_RGB(211, 32, 170);	//neka ljubičasta
	EVE_END();

	}