#include "stm32f10x.h"
#include "EIE3810_TFTLCD.h"
#include "Font.H"

extern void Delay(u32 count);

/* write a 16-bit command and then a 16-bit data to the TFTLCD */
void EIE3810_TFTLCD_WrCmdData(u16 cmd, u16 data)
{
    *(u16 *)LCD_COMMAND = cmd;
    *(u16 *)LCD_DATA = data;
}

/* write a 16-bit command to the TFTLCD */
void EIE3810_TFTLCD_WrCmd(u16 cmd)
{
    *(u16 *)LCD_COMMAND = cmd;
}

/* write a 16-bit data to the TFTLCD */
void EIE3810_TFTLCD_WrData(u16 data)
{
    *(u16 *)LCD_DATA = data;
}

/* draw a dot with the index of x and y */
void EIE3810_TFTLCD_DrawDot(
    u16 x, u16 y, u16 color)
{
    /* set the starting address of column */
    EIE3810_TFTLCD_WrCmd(0x2A00);
    EIE3810_TFTLCD_WrData(x >> 8);
    EIE3810_TFTLCD_WrCmd(0x2A01);
    EIE3810_TFTLCD_WrData(x & 0xFF);

    /* set the starting address of row */
    EIE3810_TFTLCD_WrCmd(0x2B00);
    EIE3810_TFTLCD_WrData(y >> 8);
    EIE3810_TFTLCD_WrCmd(0x2B01);
    EIE3810_TFTLCD_WrData(y & 0xFF);

    /* write in the color once */
    EIE3810_TFTLCD_WrCmd(0x2C00);
    EIE3810_TFTLCD_WrData(color);
}

/* fill a rectangle with indices and sizes */
void EIE3810_TFTLCD_DrawRectangle(
    u16 start_x, u16 length_x,
    u16 start_y, u16 length_y, u16 color)
{
    u32 index = 0;
    /* set the XS */
    EIE3810_TFTLCD_WrCmd(0x2A00);
    EIE3810_TFTLCD_WrData(start_x >> 8);
    EIE3810_TFTLCD_WrCmd(0x2A01);
    EIE3810_TFTLCD_WrData(start_x & 0xFF);
    /* set the XE */
    EIE3810_TFTLCD_WrCmd(0x2A02);
    EIE3810_TFTLCD_WrData((start_x + length_x - 1) >> 8);
    EIE3810_TFTLCD_WrCmd(0x2A03);
    EIE3810_TFTLCD_WrData((start_x + length_x - 1) & 0xFF);
    /* set the YS */
    EIE3810_TFTLCD_WrCmd(0x2B00);
    EIE3810_TFTLCD_WrData(start_y >> 8);
    EIE3810_TFTLCD_WrCmd(0x2B01);
    EIE3810_TFTLCD_WrData(start_y & 0xFF);
    /* set the YE */
    EIE3810_TFTLCD_WrCmd(0x2B02);
    EIE3810_TFTLCD_WrData((start_y + length_y - 1) >> 8);
    EIE3810_TFTLCD_WrCmd(0x2B03);
    EIE3810_TFTLCD_WrData((start_y + length_y - 1) & 0xFF);
    /* write in the color */
    EIE3810_TFTLCD_WrCmd(0x2C00);
    for (index = 0; index < length_x * length_y; index++)
        EIE3810_TFTLCD_WrData(color);
}

void EIE3810_TFTLCD_SevenSegment(
    u16 start_x, u16 start_y, u8 digit, u16 color, u16 bgcolor)
{

    /* paramters for segment a to g */
    u16 segment_para[7][4] = {
        {10, 55, 0, 10},   // a - top
        {65, 10, 10, 55},  // b - right high
        {65, 10, 75, 55},  // c - right low
        {10, 55, 130, 10}, // d - bottom
        {0, 10, 75, 55},   // e - left low
        {0, 10, 10, 55},   // f - left high
        {10, 55, 65, 10}   // g - center
    };

    /* corresponding segments paramters for digits */
    u16 digit_para[10][7] = {
        {1, 1, 1, 1, 1, 1, 0}, // 0
        {0, 1, 1, 0, 0, 0, 0}, // 1
        {1, 1, 0, 1, 1, 0, 1}, // 2
        {1, 1, 1, 1, 0, 0, 1}, // 3
        {0, 1, 1, 0, 0, 1, 1}, // 4
        {1, 0, 1, 1, 0, 1, 1}, // 5
        {0, 0, 1, 1, 1, 1, 1}, // 6
        {1, 1, 1, 0, 0, 0, 0}, // 7
        {1, 1, 1, 1, 1, 1, 1}, // 8
        {1, 1, 1, 0, 0, 1, 1}  // 9
    };

    for (u32 i = 0; i <= 6; i++)
    {
        if (digit_para[digit][i] == 1)
        {
            EIE3810_TFTLCD_DrawRectangle(
                start_x + segment_para[i][0], segment_para[i][1],
                start_y + segment_para[i][2], segment_para[i][3],
                color);
        }
        else
        {
            EIE3810_TFTLCD_DrawRectangle(
                start_x + segment_para[i][0], segment_para[i][1],
                start_y + segment_para[i][2], segment_para[i][3],
                bgcolor);
        }
    }
}

void EIE3810_TFTLCD_DrawCircle(u16 x0, u16 y0, u8 r, u8 full, u16 color)
/* x0, y0 : center of the circle; r: radius; full: whether the circle is hollow or solid; color: color of the circle.*/
{
    int a, b;
    int di, yi;
    a = 0;
    b = r;
    di = 3 - (r << 1);
    if (!full) // Hollow circle
    {
        while (a <= b)
        {
            EIE3810_TFTLCD_DrawDot(x0 + a, y0 - b, color);
            EIE3810_TFTLCD_DrawDot(x0 + b, y0 - a, color);
            EIE3810_TFTLCD_DrawDot(x0 + b, y0 + a, color);
            EIE3810_TFTLCD_DrawDot(x0 + a, y0 + b, color);
            EIE3810_TFTLCD_DrawDot(x0 - a, y0 + b, color);
            EIE3810_TFTLCD_DrawDot(x0 - b, y0 + a, color);
            EIE3810_TFTLCD_DrawDot(x0 - a, y0 - b, color);
            EIE3810_TFTLCD_DrawDot(x0 - b, y0 - a, color);
            a++;
            // Use Bresenham algorithm
            if (di < 0)
                di += 4 * a + 6;
            else
            {
                di += 10 + 4 * (a - b);
                b--;
            }
        }
    }
    else //  Solid circle
    {
        while (a <= b)
        {
            for (yi = a; yi <= b; yi++)
            {
                EIE3810_TFTLCD_DrawDot(x0 + a, y0 - yi, color);
                EIE3810_TFTLCD_DrawDot(x0 + yi, y0 - a, color);
                EIE3810_TFTLCD_DrawDot(x0 + yi, y0 + a, color);
                EIE3810_TFTLCD_DrawDot(x0 + a, y0 + yi, color);
                EIE3810_TFTLCD_DrawDot(x0 - a, y0 + yi, color);
                EIE3810_TFTLCD_DrawDot(x0 - yi, y0 + a, color);
                EIE3810_TFTLCD_DrawDot(x0 - a, y0 - yi, color);
                EIE3810_TFTLCD_DrawDot(x0 - yi, y0 - a, color);
            }
            a++;
            // Use Bresenham algorithm
            if (di < 0)
                di += 4 * a + 6;
            else
            {
                di += 10 + 4 * (a - b);
                b--;
            }
        }
    }
}

/* show 12 * 06 fonts */
void EIE3810_TFTLCD_ShowChar1206(u16 x, u16 y, u8 ascii, u16 color, u16 bgcolor)
{
    u8 i, j;
    u8 index;
    u8 height = 12, length = 6;
    if (ascii < 32 || ascii > 127)
        return;
    ascii -= 32;

    EIE3810_TFTLCD_WrCmd(0x2A00);
    EIE3810_TFTLCD_WrData(x >> 8);
    EIE3810_TFTLCD_WrCmd(0x2A01);
    EIE3810_TFTLCD_WrData(x & 0xFF);
    EIE3810_TFTLCD_WrCmd(0x2A02);
    EIE3810_TFTLCD_WrData((length + x - 1) >> 8);
    EIE3810_TFTLCD_WrCmd(0x2A03);
    EIE3810_TFTLCD_WrData((length + x - 1) & 0xFF);

    EIE3810_TFTLCD_WrCmd(0x2B00);
    EIE3810_TFTLCD_WrData(y >> 8);
    EIE3810_TFTLCD_WrCmd(0x2B01);
    EIE3810_TFTLCD_WrData(y & 0xFF);
    EIE3810_TFTLCD_WrCmd(0x2B02);
    EIE3810_TFTLCD_WrData((height + y - 1) >> 8);
    EIE3810_TFTLCD_WrCmd(0x2B03);
    EIE3810_TFTLCD_WrData((height + y - 1) & 0xFF);

    EIE3810_TFTLCD_WrCmd(0x2C00);

    for (j = 0; j < height / 8; j++)
    {
        for (i = 0; i < height / 3; i++)
        {
            for (index = 0; index < length; index++)
            {
                if ((asc2_2412[ascii][index * 3 + j] << i) & 0x80)
                    EIE3810_TFTLCD_WrData(color);
                else
                    EIE3810_TFTLCD_WrData(bgcolor);
            }
        }
    }
}

/* show 16 * 08 fonts */
void EIE3810_TFTLCD_ShowChar1608(
    u16 x, u16 y, u8 ASCII, u16 color, u16 bgcolor)
{
    u16 col, row;
    char pixels[16];

    if (ASCII < 32)
    {
        EIE3810_TFTLCD_DrawRectangle(x, 8, y, 16, bgcolor);
        return;
    }

    for (u16 i = 0; i <= 15; i++)
        pixels[i] = asc2_1608[ASCII - 32][i];

    int index_pixel;

    for (col = 0; col <= 7; col++)
    {
        for (row = 0; row <= 15; row++)
        {
            index_pixel = (col * 2) + (row / 8);
            if ((pixels[index_pixel] & (1 << (7 - ((row + 8) % 8)))) != 0)
            {
                EIE3810_TFTLCD_DrawDot(x + col, y + row, color);
            }
            else
            {
                EIE3810_TFTLCD_DrawDot(x + col, y + row, bgcolor);
            }
        }
    }
}

/* show 24 * 12 fonts */
void EIE3810_TFTLCD_ShowChar2412(u16 x, u16 y, u8 ascii, u16 color, u16 bgcolor)
{
    u8 i, j;
    u8 index;
    u8 height = 24, length = 12;
    if (ascii < 32 || ascii > 127)
        return;
    ascii -= 32;

    EIE3810_TFTLCD_WrCmd(0x2A00);
    EIE3810_TFTLCD_WrData(x >> 8);
    EIE3810_TFTLCD_WrCmd(0x2A01);
    EIE3810_TFTLCD_WrData(x & 0xFF);
    EIE3810_TFTLCD_WrCmd(0x2A02);
    EIE3810_TFTLCD_WrData((length + x - 1) >> 8);
    EIE3810_TFTLCD_WrCmd(0x2A03);
    EIE3810_TFTLCD_WrData((length + x - 1) & 0xFF);

    EIE3810_TFTLCD_WrCmd(0x2B00);
    EIE3810_TFTLCD_WrData(y >> 8);
    EIE3810_TFTLCD_WrCmd(0x2B01);
    EIE3810_TFTLCD_WrData(y & 0xFF);
    EIE3810_TFTLCD_WrCmd(0x2B02);
    EIE3810_TFTLCD_WrData((height + y - 1) >> 8);
    EIE3810_TFTLCD_WrCmd(0x2B03);
    EIE3810_TFTLCD_WrData((height + y - 1) & 0xFF);

    EIE3810_TFTLCD_WrCmd(0x2C00);

    for (j = 0; j < height / 8; j++)
    {
        for (i = 0; i < height / 3; i++)
        {
            for (index = 0; index < length; index++)
            {
                if ((asc2_2412[ascii][index * 3 + j] << i) & 0x80)
                    EIE3810_TFTLCD_WrData(color);
                else
                    EIE3810_TFTLCD_WrData(bgcolor);
            }
        }
    }
}

void EIE3810_TFTLCD_ShowString(u16 x, u16 y, char *thestr, u16 color, u16 bgcolor, u16 size_flag)
{
    u8 height, length;
    void (*func_p)();
    int str_len = strlen(thestr);
    u16 start_x = x;
    /* define height, length, and func based on sizeflag */
    switch (size_flag)
    {
    case 0:
        height = 12;
        func_p = EIE3810_TFTLCD_ShowChar1206;
        break;
    case 1:
        height = 16;
        func_p = EIE3810_TFTLCD_ShowChar1608;
        break;
    case 2:
        height = 24;
        func_p = EIE3810_TFTLCD_ShowChar2412;
        break;
    default:
        height = 16;
        func_p = EIE3810_TFTLCD_ShowChar1608;
        break;
    }
    length = height / 2;

    for (int i = 0; i <= str_len - 1; i++)
    {
        func_p(start_x + i*length, y, thestr[i], color, bgcolor);
    }
}

/* clear the screen with color */
void EIE3810_TFTLCD_Clear(u16 color)
{
    u16 XS = 0;   // the starting address of col
    u16 XE = 479; // the ending address of col
    EIE3810_TFTLCD_WrCmd(0x2A00);
    EIE3810_TFTLCD_WrData(XS >> 8);
    EIE3810_TFTLCD_WrCmd(0x2A01);
    EIE3810_TFTLCD_WrData(XS & 0xFF);
    EIE3810_TFTLCD_WrCmd(0x2A02);
    EIE3810_TFTLCD_WrData(XE >> 8);
    EIE3810_TFTLCD_WrCmd(0x2A03);
    EIE3810_TFTLCD_WrData(XE & 0xFF);

    u16 YS = 0;   // the starting address of row
    u16 YE = 799; // the ending address of row
    EIE3810_TFTLCD_WrCmd(0x2B00);
    EIE3810_TFTLCD_WrData(YS >> 8);
    EIE3810_TFTLCD_WrCmd(0x2B01);
    EIE3810_TFTLCD_WrData(YS & 0xFF);
    EIE3810_TFTLCD_WrCmd(0x2B02);
    EIE3810_TFTLCD_WrData(YE >> 8);
    EIE3810_TFTLCD_WrCmd(0x2B03);
    EIE3810_TFTLCD_WrData(YE & 0xFF);

    /* write in the color */
    EIE3810_TFTLCD_WrCmd(0x2C00);
    for (u32 i = 0; i < ((XE + 1) * (YE + 1)); i++)
        EIE3810_TFTLCD_WrData(color);
}

/* initialize the TFTLCD */
void EIE3810_TFTLCD_Init(void)
{
    RCC->AHBENR |= 1 << 8;    // Enable FSMC clock
    RCC->APB2ENR |= 1 << 3;   // Enable PORTB clock
    RCC->APB2ENR |= 1 << 5;   // Enable PORTD clock
    RCC->APB2ENR |= 1 << 6;   // Enable PORTE clock
    RCC->APB2ENR |= 1 << 8;   // Enable PORTG clock
    GPIOB->CRL &= 0XFFFFFFF0; // PB0
    GPIOB->CRL |= 0X00000003;

    // PORTD
    GPIOD->CRH &= 0X00FFF000;
    GPIOD->CRH |= 0XBB000BBB;
    GPIOD->CRL &= 0XFF00FF00;
    GPIOD->CRL |= 0X00BB00BB;

    // PORTE
    GPIOE->CRH &= 0X00000000;
    GPIOE->CRH |= 0XBBBBBBBB;
    GPIOE->CRL &= 0X0FFFFFFF;
    GPIOE->CRL |= 0XB0000000;

    // PORTG12
    GPIOG->CRH &= 0XFFF0FFFF;
    GPIOG->CRH |= 0X000B0000;
    GPIOG->CRL &= 0XFFFFFFF0; // PG0->RS
    GPIOG->CRL |= 0X0000000B;

    // Set FSMC registers to 0
    FSMC_Bank1->BTCR[6] = 0X00000000;  // FSMC_BCR4
    FSMC_Bank1->BTCR[7] = 0X00000000;  // FSMC_BTR4
    FSMC_Bank1E->BWTR[6] = 0X00000000; // FSMC_BWTR4

    FSMC_Bank1->BTCR[6] |= 1 << 12; // enable write operations
    FSMC_Bank1->BTCR[6] |= 1 << 14; // enable extended mode, values insides FSMC_BWTR register are taken into account
    FSMC_Bank1->BTCR[6] |= 1 << 4;  // set memory databus width to 16 bits (01)

    FSMC_Bank1->BTCR[7] |= 0 << 28;  // set ACCMOD[1:0] = 00, access mode A
    FSMC_Bank1->BTCR[7] |= 1 << 0;   // set ADDSET[3:0]  = 0001, ADDSET phase duration = 2 × HCLK clock cycle
    FSMC_Bank1->BTCR[7] |= 0XF << 8; // set DATAST[7:0]  = 00001111, data-phase duration = 16 × HCLK clock cycles

    FSMC_Bank1E->BWTR[6] |= 0 << 28; // set ACCMOD[1:0] = 00, access mode A
    FSMC_Bank1E->BWTR[6] |= 0 << 0;  // set ADDSET[3:0]  = 0000, ADDSET phase duration = 1 × HCLK clock cycle
    FSMC_Bank1E->BWTR[6] |= 3 << 8;  // set DATAST[7:0]  = 00000011, data-phase duration = 4 × HCLK clock cycles

    FSMC_Bank1->BTCR[6] |= 1 << 0; // enable the memory bank

    Delay(500000);

    EIE3810_TFTLCD_SetParameter();

    LCD_LIGHT_ON;
}

/* set the parameter of the TFTLCD */
void EIE3810_TFTLCD_SetParameter(void)
{

    EIE3810_TFTLCD_WrCmdData(0xF000, 0x55);
    EIE3810_TFTLCD_WrCmdData(0xF001, 0xAA);
    EIE3810_TFTLCD_WrCmdData(0xF002, 0x52);
    EIE3810_TFTLCD_WrCmdData(0xF003, 0x08);
    EIE3810_TFTLCD_WrCmdData(0xF004, 0x01);
    // AVDD Set AVDD 5.2V
    EIE3810_TFTLCD_WrCmdData(0xB000, 0x0D);
    EIE3810_TFTLCD_WrCmdData(0xB001, 0x0D);
    EIE3810_TFTLCD_WrCmdData(0xB002, 0x0D);
    // AVDD ratio
    EIE3810_TFTLCD_WrCmdData(0xB600, 0x35);
    EIE3810_TFTLCD_WrCmdData(0xB601, 0x35);
    EIE3810_TFTLCD_WrCmdData(0xB602, 0x35);
    // AVEE -5.2V
    EIE3810_TFTLCD_WrCmdData(0xB100, 0x0D);
    EIE3810_TFTLCD_WrCmdData(0xB101, 0x0D);
    EIE3810_TFTLCD_WrCmdData(0xB102, 0x0D);
    // AVEE ratio
    EIE3810_TFTLCD_WrCmdData(0xB700, 0x35);
    EIE3810_TFTLCD_WrCmdData(0xB701, 0x35);
    EIE3810_TFTLCD_WrCmdData(0xB702, 0x35);
    // VCL -2.5V
    EIE3810_TFTLCD_WrCmdData(0xB200, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xB201, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xB202, 0x00);
    // VCL ratio
    EIE3810_TFTLCD_WrCmdData(0xB800, 0x24);
    EIE3810_TFTLCD_WrCmdData(0xB801, 0x24);
    EIE3810_TFTLCD_WrCmdData(0xB802, 0x24);
    // VGH 15V (Free pump)
    EIE3810_TFTLCD_WrCmdData(0xBF00, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xB300, 0x08);
    EIE3810_TFTLCD_WrCmdData(0xB301, 0x08);
    EIE3810_TFTLCD_WrCmdData(0xB302, 0x08);
    // VGH ratio
    EIE3810_TFTLCD_WrCmdData(0xB900, 0x34);
    EIE3810_TFTLCD_WrCmdData(0xB901, 0x34);
    EIE3810_TFTLCD_WrCmdData(0xB902, 0x34);
    // VGLX ratio
    EIE3810_TFTLCD_WrCmdData(0xBA00, 0x24);
    EIE3810_TFTLCD_WrCmdData(0xBA01, 0x24);
    EIE3810_TFTLCD_WrCmdData(0xBA02, 0x24);
    // VGMP/VGSP 4.7V/0V
    EIE3810_TFTLCD_WrCmdData(0xBC00, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xBC01, 0x88);
    EIE3810_TFTLCD_WrCmdData(0xBC02, 0x00);
    // VGMN/VGSN -4.7V/0V
    EIE3810_TFTLCD_WrCmdData(0xBD00, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xBD01, 0x88);
    EIE3810_TFTLCD_WrCmdData(0xBD02, 0x00);
    // VCOM
    EIE3810_TFTLCD_WrCmdData(0xBE00, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xBE01, 0xA3);
    // Gamma Setting
    EIE3810_TFTLCD_WrCmdData(0xD100, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD101, 0x05);
    EIE3810_TFTLCD_WrCmdData(0xD102, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD103, 0x40);
    EIE3810_TFTLCD_WrCmdData(0xD104, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD105, 0x60);
    EIE3810_TFTLCD_WrCmdData(0xD106, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD107, 0x90);
    EIE3810_TFTLCD_WrCmdData(0xD108, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD109, 0x99);
    EIE3810_TFTLCD_WrCmdData(0xD10A, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD10B, 0xBB);
    EIE3810_TFTLCD_WrCmdData(0xD10C, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD10D, 0xDC);
    EIE3810_TFTLCD_WrCmdData(0xD10E, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD10F, 0x04);
    EIE3810_TFTLCD_WrCmdData(0xD110, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD111, 0x25);
    EIE3810_TFTLCD_WrCmdData(0xD112, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD113, 0x59);
    EIE3810_TFTLCD_WrCmdData(0xD114, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD115, 0x82);
    EIE3810_TFTLCD_WrCmdData(0xD116, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD117, 0xC6);
    EIE3810_TFTLCD_WrCmdData(0xD118, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD119, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD11A, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD11B, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD11C, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD11D, 0x39);
    EIE3810_TFTLCD_WrCmdData(0xD11E, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD11F, 0x79);
    EIE3810_TFTLCD_WrCmdData(0xD120, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD121, 0xA1);
    EIE3810_TFTLCD_WrCmdData(0xD122, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD123, 0xD9);
    EIE3810_TFTLCD_WrCmdData(0xD124, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD125, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD126, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD127, 0x38);
    EIE3810_TFTLCD_WrCmdData(0xD128, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD129, 0x67);
    EIE3810_TFTLCD_WrCmdData(0xD12A, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD12B, 0x8F);
    EIE3810_TFTLCD_WrCmdData(0xD12C, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD12D, 0xCD);
    EIE3810_TFTLCD_WrCmdData(0xD12E, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD12F, 0xFD);
    EIE3810_TFTLCD_WrCmdData(0xD130, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD131, 0xFE);
    EIE3810_TFTLCD_WrCmdData(0xD132, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD133, 0xFF);

    EIE3810_TFTLCD_WrCmdData(0xD200, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD201, 0x05);
    EIE3810_TFTLCD_WrCmdData(0xD202, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD203, 0x40);
    EIE3810_TFTLCD_WrCmdData(0xD204, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD205, 0x6D);
    EIE3810_TFTLCD_WrCmdData(0xD206, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD207, 0x90);
    EIE3810_TFTLCD_WrCmdData(0xD208, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD209, 0x99);
    EIE3810_TFTLCD_WrCmdData(0xD20A, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD20B, 0xBB);
    EIE3810_TFTLCD_WrCmdData(0xD20C, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD20D, 0xDC);

    EIE3810_TFTLCD_WrCmdData(0xD20E, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD20F, 0x04);
    EIE3810_TFTLCD_WrCmdData(0xD210, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD211, 0x25);
    EIE3810_TFTLCD_WrCmdData(0xD212, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD213, 0x59);
    EIE3810_TFTLCD_WrCmdData(0xD214, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD215, 0x82);
    EIE3810_TFTLCD_WrCmdData(0xD216, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD217, 0xC6);
    EIE3810_TFTLCD_WrCmdData(0xD218, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD219, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD21A, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD21B, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD21C, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD21D, 0x39);
    EIE3810_TFTLCD_WrCmdData(0xD21E, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD21F, 0x79);
    EIE3810_TFTLCD_WrCmdData(0xD220, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD221, 0xA1);
    EIE3810_TFTLCD_WrCmdData(0xD222, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD223, 0xD9);
    EIE3810_TFTLCD_WrCmdData(0xD224, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD225, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD226, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD227, 0x38);
    EIE3810_TFTLCD_WrCmdData(0xD228, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD229, 0x67);
    EIE3810_TFTLCD_WrCmdData(0xD22A, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD22B, 0x8F);
    EIE3810_TFTLCD_WrCmdData(0xD22C, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD22D, 0xCD);
    EIE3810_TFTLCD_WrCmdData(0xD22E, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD22F, 0xFD);
    EIE3810_TFTLCD_WrCmdData(0xD230, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD231, 0xFE);
    EIE3810_TFTLCD_WrCmdData(0xD232, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD233, 0xFF);

    EIE3810_TFTLCD_WrCmdData(0xD300, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD301, 0x05);
    EIE3810_TFTLCD_WrCmdData(0xD302, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD303, 0x40);
    EIE3810_TFTLCD_WrCmdData(0xD304, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD305, 0x6D);
    EIE3810_TFTLCD_WrCmdData(0xD306, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD307, 0x90);
    EIE3810_TFTLCD_WrCmdData(0xD308, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD309, 0x99);
    EIE3810_TFTLCD_WrCmdData(0xD30A, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD30B, 0xBB);
    EIE3810_TFTLCD_WrCmdData(0xD30C, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD30D, 0xDC);

    EIE3810_TFTLCD_WrCmdData(0xD30E, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD30F, 0x04);
    EIE3810_TFTLCD_WrCmdData(0xD310, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD311, 0x25);
    EIE3810_TFTLCD_WrCmdData(0xD312, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD313, 0x59);
    EIE3810_TFTLCD_WrCmdData(0xD314, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD315, 0x82);
    EIE3810_TFTLCD_WrCmdData(0xD316, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD317, 0xC6);

    EIE3810_TFTLCD_WrCmdData(0xD318, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD319, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD31A, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD31B, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD31C, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD31D, 0x39);
    EIE3810_TFTLCD_WrCmdData(0xD31E, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD31F, 0x79);
    EIE3810_TFTLCD_WrCmdData(0xD320, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD321, 0xA1);
    EIE3810_TFTLCD_WrCmdData(0xD322, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD323, 0xD9);
    EIE3810_TFTLCD_WrCmdData(0xD324, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD325, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD326, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD327, 0x38);
    EIE3810_TFTLCD_WrCmdData(0xD328, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD329, 0x67);
    EIE3810_TFTLCD_WrCmdData(0xD32A, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD32B, 0x8F);
    EIE3810_TFTLCD_WrCmdData(0xD32C, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD32D, 0xCD);
    EIE3810_TFTLCD_WrCmdData(0xD32E, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD32F, 0xFD);
    EIE3810_TFTLCD_WrCmdData(0xD330, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD331, 0xFE);
    EIE3810_TFTLCD_WrCmdData(0xD332, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD333, 0xFF);
    EIE3810_TFTLCD_WrCmdData(0xD400, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD401, 0x05);
    EIE3810_TFTLCD_WrCmdData(0xD402, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD403, 0x40);
    EIE3810_TFTLCD_WrCmdData(0xD404, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD405, 0x6D);
    EIE3810_TFTLCD_WrCmdData(0xD406, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD407, 0x90);
    EIE3810_TFTLCD_WrCmdData(0xD408, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD409, 0x99);
    EIE3810_TFTLCD_WrCmdData(0xD40A, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD40B, 0xBB);
    EIE3810_TFTLCD_WrCmdData(0xD40C, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD40D, 0xDC);

    EIE3810_TFTLCD_WrCmdData(0xD40E, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD40F, 0x04);
    EIE3810_TFTLCD_WrCmdData(0xD410, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD411, 0x25);
    EIE3810_TFTLCD_WrCmdData(0xD412, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD413, 0x59);
    EIE3810_TFTLCD_WrCmdData(0xD414, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD415, 0x82);
    EIE3810_TFTLCD_WrCmdData(0xD416, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD417, 0xC6);

    EIE3810_TFTLCD_WrCmdData(0xD418, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD419, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD41A, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD41B, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD41C, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD41D, 0x39);
    EIE3810_TFTLCD_WrCmdData(0xD41E, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD41F, 0x79);
    EIE3810_TFTLCD_WrCmdData(0xD420, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD421, 0xA1);
    EIE3810_TFTLCD_WrCmdData(0xD422, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD423, 0xD9);
    EIE3810_TFTLCD_WrCmdData(0xD424, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD425, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD426, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD427, 0x38);
    EIE3810_TFTLCD_WrCmdData(0xD428, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD429, 0x67);
    EIE3810_TFTLCD_WrCmdData(0xD42A, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD42B, 0x8F);
    EIE3810_TFTLCD_WrCmdData(0xD42C, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD42D, 0xCD);
    EIE3810_TFTLCD_WrCmdData(0xD42E, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD42F, 0xFD);
    EIE3810_TFTLCD_WrCmdData(0xD430, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD431, 0xFE);
    EIE3810_TFTLCD_WrCmdData(0xD432, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD433, 0xFF);

    EIE3810_TFTLCD_WrCmdData(0xD500, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD501, 0x05);
    EIE3810_TFTLCD_WrCmdData(0xD502, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD503, 0x40);
    EIE3810_TFTLCD_WrCmdData(0xD504, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD505, 0x6D);
    EIE3810_TFTLCD_WrCmdData(0xD506, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD507, 0x90);
    EIE3810_TFTLCD_WrCmdData(0xD508, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD509, 0x99);
    EIE3810_TFTLCD_WrCmdData(0xD50A, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD50B, 0xBB);
    EIE3810_TFTLCD_WrCmdData(0xD50C, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD50D, 0xDC);

    EIE3810_TFTLCD_WrCmdData(0xD50E, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD50F, 0x04);
    EIE3810_TFTLCD_WrCmdData(0xD510, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD511, 0x25);
    EIE3810_TFTLCD_WrCmdData(0xD512, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD513, 0x59);
    EIE3810_TFTLCD_WrCmdData(0xD514, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD515, 0x82);
    EIE3810_TFTLCD_WrCmdData(0xD516, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD517, 0xC6);
    EIE3810_TFTLCD_WrCmdData(0xD518, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD519, 0x01);

    EIE3810_TFTLCD_WrCmdData(0xD51A, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD51B, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD51C, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD51D, 0x39);
    EIE3810_TFTLCD_WrCmdData(0xD51E, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD51F, 0x79);
    EIE3810_TFTLCD_WrCmdData(0xD520, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD521, 0xA1);
    EIE3810_TFTLCD_WrCmdData(0xD522, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD523, 0xD9);
    EIE3810_TFTLCD_WrCmdData(0xD524, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD525, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD526, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD527, 0x38);
    EIE3810_TFTLCD_WrCmdData(0xD528, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD529, 0x67);
    EIE3810_TFTLCD_WrCmdData(0xD52A, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD52B, 0x8F);
    EIE3810_TFTLCD_WrCmdData(0xD52C, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD52D, 0xCD);
    EIE3810_TFTLCD_WrCmdData(0xD52E, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD52F, 0xFD);
    EIE3810_TFTLCD_WrCmdData(0xD530, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD531, 0xFE);
    EIE3810_TFTLCD_WrCmdData(0xD532, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD533, 0xFF);

    EIE3810_TFTLCD_WrCmdData(0xD600, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD601, 0x05);
    EIE3810_TFTLCD_WrCmdData(0xD602, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD603, 0x40);
    EIE3810_TFTLCD_WrCmdData(0xD604, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD605, 0x6D);
    EIE3810_TFTLCD_WrCmdData(0xD606, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD607, 0x90);
    EIE3810_TFTLCD_WrCmdData(0xD608, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD609, 0x99);
    EIE3810_TFTLCD_WrCmdData(0xD60A, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD60B, 0xBB);
    EIE3810_TFTLCD_WrCmdData(0xD60C, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD60D, 0xDC);

    EIE3810_TFTLCD_WrCmdData(0xD60E, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD60F, 0x04);
    EIE3810_TFTLCD_WrCmdData(0xD610, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD611, 0x25);
    EIE3810_TFTLCD_WrCmdData(0xD612, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD613, 0x59);
    EIE3810_TFTLCD_WrCmdData(0xD614, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD615, 0x82);

    EIE3810_TFTLCD_WrCmdData(0xD616, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD617, 0xC6);
    EIE3810_TFTLCD_WrCmdData(0xD618, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD619, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xD61A, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD61B, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD61C, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD61D, 0x39);
    EIE3810_TFTLCD_WrCmdData(0xD61E, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD61F, 0x79);
    EIE3810_TFTLCD_WrCmdData(0xD620, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD621, 0xA1);

    EIE3810_TFTLCD_WrCmdData(0xD622, 0x02);
    EIE3810_TFTLCD_WrCmdData(0xD623, 0xD9);
    EIE3810_TFTLCD_WrCmdData(0xD624, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD625, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xD626, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD627, 0x38);
    EIE3810_TFTLCD_WrCmdData(0xD628, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD629, 0x67);
    EIE3810_TFTLCD_WrCmdData(0xD62A, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD62B, 0x8F);
    EIE3810_TFTLCD_WrCmdData(0xD62C, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD62D, 0xCD);
    EIE3810_TFTLCD_WrCmdData(0xD62E, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD62F, 0xFD);
    EIE3810_TFTLCD_WrCmdData(0xD630, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD631, 0xFE);
    EIE3810_TFTLCD_WrCmdData(0xD632, 0x03);
    EIE3810_TFTLCD_WrCmdData(0xD633, 0xFF);

    // LV2 Page 0 enable
    EIE3810_TFTLCD_WrCmdData(0xF000, 0x55);
    EIE3810_TFTLCD_WrCmdData(0xF001, 0xAA);
    EIE3810_TFTLCD_WrCmdData(0xF002, 0x52);
    EIE3810_TFTLCD_WrCmdData(0xF003, 0x08);
    EIE3810_TFTLCD_WrCmdData(0xF004, 0x00);
    // Display control
    EIE3810_TFTLCD_WrCmdData(0xB100, 0xCC);
    EIE3810_TFTLCD_WrCmdData(0xB101, 0x00);
    // Source hold time
    EIE3810_TFTLCD_WrCmdData(0xB600, 0x05);
    // Gate EQ control
    EIE3810_TFTLCD_WrCmdData(0xB700, 0x70);
    EIE3810_TFTLCD_WrCmdData(0xB701, 0x70);
    // Source EQ control (Mode 2)
    EIE3810_TFTLCD_WrCmdData(0xB800, 0x01);
    EIE3810_TFTLCD_WrCmdData(0xB801, 0x05);
    EIE3810_TFTLCD_WrCmdData(0xB802, 0x05);
    EIE3810_TFTLCD_WrCmdData(0xB803, 0x05);
    // Inversion mode (2-dot)
    EIE3810_TFTLCD_WrCmdData(0xBC00, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xBC01, 0x00);
    EIE3810_TFTLCD_WrCmdData(0xBC02, 0x00);
    // Timing control 4H w/ 4-delay
    EIE3810_TFTLCD_WrCmdData(0xC900, 0xD0);
    EIE3810_TFTLCD_WrCmdData(0xC901, 0x82);
    EIE3810_TFTLCD_WrCmdData(0xC902, 0x50);
    EIE3810_TFTLCD_WrCmdData(0xC903, 0x50);
    EIE3810_TFTLCD_WrCmdData(0xC904, 0x50);

    EIE3810_TFTLCD_WrCmdData(0x3A00, 0x55); // 16-bit/pixel
    EIE3810_TFTLCD_WrCmd(0x1100);
    Delay(120000);

    EIE3810_TFTLCD_WrCmd(0x2900);
}
