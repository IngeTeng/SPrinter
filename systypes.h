/******************************************************************************
**
**  COPYRIGHT (C) 2000 Intel Corporation
**
**  FILENAME:      systypes.h
**
**  PURPOSE:       Contains common type definitions used in this project
**
**  $Modtime: 7/17/03 1:01p $
**
******************************************************************************/

#ifndef _SYSTYPES_H
#define _SYSTYPES_H

/*
*******************************************************************************

    Portable Integral Type Aliases and associated pointer types
    - Must be verified for all compiler ports.

    The underlying size of one of these data types may be larger than implied
    by its name.  The underlying types here assume the ARM ADS 1.01 compiler,
    and in that case the sizes ares exactly as implied.

    INT64 and UINT64 types are permitted but not standardized by ANSI C.
    Their existence and behavior are implementation-dependent.

    Some information relative to UINT64 or INT64 types in the ARM compilers:
    "The following restrictions apply to long long:
     ·long long enumerators are not available.
     ·The controlling expression of a switch statement can not have
        (unsigned) long long type.  Consequently case labels must
        also have values that can be contained in a variable of
        type unsigned long."

*******************************************************************************
*/

typedef enum {FALSE=0, TRUE=1} BOOL;
typedef enum {CLEAR, SET} SetClearT;
typedef enum {DISABLE, ENABLE} EnableT;

typedef void(*FnPVOID)(void);

typedef unsigned int        UINT,     *PUINT;    // The size is not important
typedef unsigned long long  UINT64,   *PUINT64;
typedef unsigned int        UINT32,   *PUINT32;
typedef unsigned short      UINT16,   *PUINT16;
typedef unsigned char       UINT8,    *PUINT8;
typedef unsigned char       UCHAR,BYTE,*PUCHAR;

typedef int                 INT,      *PINT;    // The size is not important
typedef long long           INT64,    *PINT64;
typedef int                 INT32,    *PINT32;
typedef short               INT16,    *PINT16;
typedef char                INT8,     *PINT8;
typedef char                CHAR,     *PCHAR;
typedef void                VOID,     *PVOID;

typedef volatile  UINT      VUINT,    *PVUINT;    // The size is not important
typedef volatile  UINT64    VUINT64,  *PVUINT64;
typedef volatile  UINT32    VUINT32,  *PVUINT32;
typedef volatile  UINT16    VUINT16,  *PVUINT16;
typedef volatile  UINT8     VUINT8,   *PVUINT8;
typedef volatile  UCHAR     VUCHAR,   *PVUCHAR;

typedef volatile  INT       VINT,     *PVINT;    // The size is not important
typedef volatile  INT64     VINT64,   *PVINT64;
typedef volatile  INT32     VINT32,   *PVINT32;
typedef volatile  INT16     VINT16,   *PVINT16;
typedef volatile  INT8      VINT8,    *PVINT8;
typedef volatile  CHAR      VCHAR,    *PVCHAR;

typedef enum {
    BIT0    =   ( 0x1U << 0),
    BIT1    =   ( 0x1U << 1),
    BIT2    =   ( 0x1U << 2),
    BIT3    =   ( 0x1U << 3),
    BIT4    =   ( 0x1U << 4),
    BIT5    =   ( 0x1U << 5),
    BIT6    =   ( 0x1U << 6),
    BIT7    =   ( 0x1U << 7),
    BIT8    =   ( 0x1U << 8),
    BIT9    =   ( 0x1U << 9),
    BIT10   =   ( 0x1U << 10),
    BIT11   =   ( 0x1U << 11),
    BIT12   =   ( 0x1U << 12),
    BIT13   =   ( 0x1U << 13),
    BIT14   =   ( 0x1U << 14),
    BIT15   =   ( 0x1U << 15),
    BIT16   =   ( 0x1U << 16),
    BIT17   =   ( 0x1U << 17),
    BIT18   =   ( 0x1U << 18),
    BIT19   =   ( 0x1U << 19),
    BIT20   =   ( 0x1U << 20),
    BIT21   =   ( 0x1U << 21),
    BIT22   =   ( 0x1U << 22),
    BIT23   =   ( 0x1U << 23),
    BIT24   =   ( 0x1U << 24),
    BIT25   =   ( 0x1U << 25),
    BIT26   =   ( 0x1U << 26),
    BIT27   =   ( 0x1U << 27),
    BIT28   =   ( 0x1U << 28),
    BIT29   =   ( 0x1U << 29),
    BIT30   =   ( 0x1U << 30),
    BIT31   =   (INT)( 0x1U << 31)
} BitT;
#define	U8	UINT8
#define	U16	UINT16
#define	U32	UINT32
#endif // #ifndef _SYSTYPES_H
