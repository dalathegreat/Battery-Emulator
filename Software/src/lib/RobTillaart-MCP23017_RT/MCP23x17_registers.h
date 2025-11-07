#pragma once
//
//    FILE: MCP23x17_registers.h
//  AUTHOR: Rob Tillaart
// PURPOSE: MCP23x17 register file
//     URL: https://github.com/RobTillaart/MCP23017_RT
//     URL: https://github.com/RobTillaart/MCP23S17

//  REGISTERS                           //  description                 datasheet
#define MCP23x17_DDR_A          0x00    //  Data Direction Register A     P18
#define MCP23x17_DDR_B          0x01    //  Data Direction Register B     P18
#define MCP23x17_POL_A          0x02    //  Input Polarity A              P18
#define MCP23x17_POL_B          0x03    //  Input Polarity B              P18
#define MCP23x17_GPINTEN_A      0x04    //  Interrupt enable              P19
#define MCP23x17_GPINTEN_B      0x05    //  Interrupt enable              P19
#define MCP23x17_DEFVAL_A       0x06    //  Interrupt default             P19
#define MCP23x17_DEFVAL_B       0x07    //  Interrupt default             P19
#define MCP23x17_INTCON_A       0x08    //  Interrupt control register    P20
#define MCP23x17_INTCON_B       0x09    //  Interrupt control register    P20
#define MCP23x17_IOCR           0x0A    //  IO control register           P20
#define MCP23x17_PUR_A          0x0C    //  Pull Up Resistors A           P22
#define MCP23x17_PUR_B          0x0D    //  Pull Up Resistors A           P22
#define MCP23x17_INTF_A         0x0E    //  Interrupt flag register       P22
#define MCP23x17_INTF_B         0x0F    //  Interrupt flag register       P22
#define MCP23x17_INTCAP_A       0x10    //  Interrupt capture register    P23
#define MCP23x17_INTCAP_B       0x11    //  Interrupt capture register    P23
#define MCP23x17_GPIO_A         0x12    //  General Purpose IO A          P23
#define MCP23x17_GPIO_B         0x13    //  General Purpose IO B          P23
#define MCP23x17_OLAT_A         0x14    //  NOT USED output latch         P24
#define MCP23x17_OLAT_B         0x15    //  NOT USED


//  IOCR = IO CONTROL REGISTER bit masks   - details datasheet P20
#define MCP23x17_IOCR_BANK      0x80    //  Controls how the registers are addressed.
#define MCP23x17_IOCR_MIRROR    0x40    //  INT Pins Mirror bit.
#define MCP23x17_IOCR_SEQOP     0x20    //  Sequential Operation mode bit.
#define MCP23x17_IOCR_DISSLW    0x10    //  Slew Rate control bit for SDA output.
#define MCP23x17_IOCR_HAEN      0x08    //  Hardware Address Enable bit (MCP23S17 only).
#define MCP23x17_IOCR_ODR       0x04    //  Configures the INT pin as an open-drain output.
#define MCP23x17_IOCR_INTPOL    0x02    //  This bit sets the polarity of the INT output pin.
#define MCP23x17_IOCR_NI        0x01    //  Not implemented.


//  -- END OF FILE --
