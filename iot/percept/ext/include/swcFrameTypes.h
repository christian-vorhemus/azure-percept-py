///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Frametypes used in Myriad common code
///

/*************************************************************************************************/
/*                          RGB format stored in memory                                          */
/*-----------------------------------------------------------------------------------------------*/
/* |  RGB888p(planar)  |        RGB888i(interleaved)       |                                     */
/* |-------------------|-----------------------------------|                                     */
/* |   | R0 R1 R2 |    |    R0 G0 B0  R1 G1 B1  R2 G2 B2   |                                     */
/* |   | R3 R4 R5 |    |    R3 G3 B3  R4 G4 B4  R5 G5 B5   |                                     */
/* |   | R6 R7 R8 |    |    R6 G6 B6  R7 G7 B7  R8 G8 B8   |, where Ri, Gi, Bi are 8bit elements */
/* |     ---------     |                                                                         */
/* |   | G0 G1 G2 |    |                                                                         */
/* |   | G3 G4 G5 |    |                                                                         */
/* |   | G6 G7 G8 |    |                                                                         */
/* |    ---------      |                                                                         */
/* |   | B0 B1 B2 |    |                                                                         */
/* |   | B3 B4 B5 |    |                                                                         */
/* |   | B6 B7 B8 |    |                                                                         */
/*                                                                                               */
/*************************************************************************************************/

#ifndef _SWC_FRAME_TYPES_H_
#define _SWC_FRAME_TYPES_H_

typedef enum frameTypes
{
     YUV422i,   // interleaved 8 bit
     YUV444p,   // planar 4:4:4 format
     YUV420p,   // planar 4:2:0 format
     YUV422p,   // planar 8 bit
     YUV400p,   // 8-bit greyscale
     RGBA8888,  // RGBA interleaved stored in 32 bit word
     RGB161616, // Planar 16 bit RGB data
     RGB888,    // Planar 8 bit RGB data
     RGB888p,   // Planar 8 bit RGB data = RGB888
     BGR888p,   // Planar 8 bit BGR data
     RGB888i,   // Interleaved 8 bit RGB data
     BGR888i,   // Interleaved 8 bit BGR data
     LUT2,      // 1 bit  per pixel, Lookup table (used for graphics layers)
     LUT4,      // 2 bits per pixel, Lookup table (used for graphics layers)
     LUT16,     // 4 bits per pixel, Lookup table (used for graphics layers)
     RAW16,     // save any raw type (8, 10, 12bit) on 16 bits
     RAW14,     // 14bit value in 16bit storage
     RAW12,     // 12bit value in 16bit storage
     RAW10,     // 10bit value in 16bit storage
     RAW8,
     PACK10,    // SIPP 10bit packed format
     PACK12,    // SIPP 12bit packed format
     YUV444i,
     NV12,
     NV21,
     BITSTREAM, // used for video encoder bitstream
     HDR,
     NONE
}frameType;

typedef struct frameSpecs
{
     frameType      type;
     unsigned int   height;    // width in pixels
     unsigned int   width;     // width in pixels
     unsigned int   stride;    // defined as distance in bytes from pix(y,x) to pix(y+1,x)
     unsigned int   bytesPP;   // bytes per pixel (for LUT types set this to 1)
}frameSpec;

typedef struct frameElements
{
     frameSpec spec;
     unsigned char* p1;  // Pointer to first image plane
     unsigned char* p2;  // Pointer to second image plane (if used)
     unsigned char* p3;  // Pointer to third image plane  (if used)
} frameBuffer;

#endif // _SWC_FRAME_TYPES_H_
