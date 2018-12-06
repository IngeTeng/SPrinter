#ifndef PCL_H_INCLUDED
#define PCL_H_INCLUDED

#define PCL_ESCAPTE                 0x1b
#define PCL_UNKNOWN             0x00
#define PCL_JOB_START             0x01
#define PCL_RASTER_DATA       0x02
#define PCL_RASTER_START      0x03
#define PCL_RASTER_END          0x04
#define PCL_RESET                       0x05
#define PCL_COLOR_MODE         0x06
#define PCL_RASTER_HEIGHT     0x07
#define PCL_RASTER_WIDTH      0x08
#define PCL_RESOLUTION           0x09
#define PCL_COPIES                      0x0A
#define PCL_PAGE_SIZE                0x0B
#define PCL_EXIT                           0x0C
#define PCL_PAPER_SRC               0x0D
#define PCL_PAPER_COPY            0x0E
#define PCL_PAPER_CMBMODE   0x0F
#define PCL_SINGLE_DOUBLE     0x10

typedef struct
{
    int len;			//used to jump to next data
    int cmd;
    int value;
}PCL;

#endif // PCL_H_INCLUDED
