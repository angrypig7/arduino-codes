/*****************************************************************************\
|*
|*  VERSION CONTROL:    $Version$   $Date$
|*
|*  IN PACKAGE:
|*
|*  COPYRIGHT:          Copyright (c) 2007, Altium
|*
|*  DESCRIPTION:
|*
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

#include <audio.h>

#include "wave.h"
#include "form1.h"


#define WAV_RIFF        0x46464952          // "RIFF"
#define WAV_WAVE        0x45564157          // "WAVE"
#define WAV_FMT         0x20746D66          // "fmt "
#define WAV_DATA        0x61746164          // "data"


// copied from util_endian.h in the software platform

/*
 * Functions to convert from a little endian entity to host format or vice versa
 */

inline uint16_t little16( uint16_t val )
{
#ifdef __BIG_ENDIAN__
    return (val << 8) | (val >> 8);
#else
    return val;
#endif
}


inline uint32_t little32( uint32_t val )
{
#ifdef __BIG_ENDIAN__
    return (( (uint32_t)little16( (uint16_t)val )) << 16 ) | ( (uint32_t)little16( val >> 16 ));
#else
    return val;
#endif
}


/*
 * Functions to read a little endian value from an 8-bit aligned address
 */

inline uint8_t read_little8from8( const void * ptr )
{
    return *(uint8_t *)ptr;
}


inline uint16_t read_little16from8( const void * ptr )
{
    return read_little8from8( ptr ) | ( (uint16_t)read_little8from8( (uint8_t *)ptr+1 ) << 8 );
}


inline uint32_t read_little32from8( const void * ptr )
{
    return read_little16from8( ptr ) | ( (uint32_t)read_little16from8( (uint8_t *)ptr+2 ) << 16 );
}


// fix up endianness and play buffer through the audio service
static void decode_and_play( audio_t * restrict audio, wav_header_t info, void *buffer, unsigned size )
{
    uint8_t * buf8;
    uint16_t * buf16;
    uint32_t * buf32;
    unsigned i;

    if ( info.samplesize == 8 )
    {
        buf8 = buffer;
        for( i = 0; i < size; i++ )
        {
            // Convert sample to signed int
            buf8[i] = (((int)buf8[i]) - 0x80) & 0xFF;
        }
        while( size )
        {
            i = audio_play( audio, buf8, size );
            buf8 += i;
            size -= i;
        }
    }
    else if ( info.samplesize == 16 )
    {
        size /= 2;
        buf16 = (uint16_t*)buffer;
        for( i = 0; i < size; i++ )
        {
            // Convert to native endianess. Do we need to convert from unsigned to signed?
            buf16[i] = little16( buf16[i] );
        }
        while( size )
        {
            i = audio_play( audio, buf16, size );
            buf16 += i;
            size -= i;
        }
    }
    else if ( info.samplesize == 32 )
    {
        size /= 4;
        buf32 = (uint32_t*)buffer;
        for( i = 0; i < size; i++ )
        {
            // Convert to native endianess. Do we need to convert from unsigned to signed?
            buf32[i] = little32( buf32[i] );
        }
        while( size )
        {
            i = audio_play( audio, buf32, size );
            buf32 += i;
            size -= i;
        }
    }
}


/**********************************************************************
|*
|*  FUNCTION    : wav_playfile
|*
|*  PARAMETERS  : info = wave file information structure
|*                filename = name of file to be played
|*
|*  RETURNS     : None
|*
|*  DESCRIPTION : Play a wav file using the generic audio service
 */
int wav_playfile( audio_t * audio, void *buf, int size, int handle, bool start )
{
    int bufsize = size;

    int minsize;
    static wav_header_t info = { 0 } ;
    uintptr_t bufstart = (uintptr_t)buf;
    static uint32_t  chunksize = 0;
    static uint32_t  chunktype;
    static bool valid_header;
    static int done = 0;

    if (start)
    {
        chunksize = 0;
        chunktype = 0;
        done = size;
        info = wav_parseheader( bufstart, bufstart + size );
        info.size -= size;

        audio_set_format( audio, info.samplerate, info.channels, info.samplesize );

        minsize = 3 * sizeof(uint32_t); // size of header
        if ( size < minsize ) return -1;

        if ( read_little32from8( (void*)bufstart ) == WAV_RIFF  && read_little32from8( (void*)(bufstart + 8) ) == WAV_WAVE )
        {
            valid_header = true;
        }
        else
        {
            valid_header = false;
        }
        bufstart += minsize;
        size  -= minsize;
    }

    if (valid_header)
    {
        done += size;
        while (size)
        {
            if (chunksize == 0)
            {

                minsize = 2 * sizeof(uint32_t); // size of chunk

                if ( size < minsize )
                {
                    memcpy(buf, (void*)bufstart, size);
                    size += read(handle, (char*)buf + size, bufsize - size);
                    done += size;
                    if ( size < minsize)
                    {
                        return done * 100 / info.size;
                    }
                }

                chunktype = read_little32from8( (void*)bufstart );
                chunksize = read_little32from8( (void*)(bufstart + 4) );

                bufstart += minsize;
                size  -= minsize;

                if ( chunktype == WAV_DATA )
                {
                    printf( "Playing %d samples...\n\n", chunksize * 8 / info.samplesize );
                    if ( size < chunksize )
                    {
                        decode_and_play( audio, info, (void*)bufstart, size );
                        chunksize -= size;
                        size = 0;
                    }
                    else
                    {
                        decode_and_play( audio, info, (void*)bufstart, chunksize );
                        size -= chunksize;
                        bufstart += chunksize;
                        chunksize = 0;
                    }
                }
            }
            else if (chunktype == WAV_DATA)
            {
                if ( size < chunksize )
                {
                    decode_and_play( audio, info, (void*)bufstart, size );
                    chunksize -= size;
                    size = 0;
                }
                else
                {
                    decode_and_play( audio, info, (void*)bufstart, chunksize );
                    size -= chunksize;
                    bufstart += chunksize;
                    chunksize = 0;
                }
            }
            else
            {
                if ( size < chunksize )
                {
                    chunksize -= size;
                    size = 0;
                }
                else
                {
                    size -= chunksize;
                    bufstart += chunksize;
                    chunksize = 0;
                }
            }
        }
    }
    return done * 100 / info.size;
}


/**********************************************************************
|*
|*  FUNCTION    : wav_readheader
|*
|*  PARAMETERS  : filename = name of file to be checked
|*
|*  RETURNS     : WAVE information
|*
|*  DESCRIPTION : Get WAVE information of said file
 */
bool wav_readheader(char *filename)
{
    int handle;
    size_t size;
    char buf[512];

    handle = open(filename, O_RDONLY);
    size = read( handle, buf, sizeof(buf));
    close( handle);

    return wav_testsupported( wav_parseheader( (uintptr_t)&buf[0], (uintptr_t)&buf[size - 1] ) );
}


/**********************************************************************
|*
|*  FUNCTION    : wav_parseheader
|*
|*  PARAMETERS  : filename = name of file to be checked
|*
|*  RETURNS     : WAVE information
|*
|*  DESCRIPTION : Get WAVE information of said file
 */
extern wav_header_t wav_parseheader( uintptr_t bufstart, uintptr_t bufend )
{
    wav_header_t wavinfo = { 0 } ;
    uint32_t chunksize;
    uint32_t chunktype;
    uint32_t bufsize;
    uint32_t minsize;

    bufsize = 1 + bufend - bufstart;
    minsize = 3 * sizeof(uint32_t); // // uint32_t hdr[3]

    if ( bufsize < minsize ) return wavinfo;

    if ( read_little32from8( (void*)bufstart ) == WAV_RIFF  && read_little32from8( (void*)(bufstart + 8) ) == WAV_WAVE )
    {
        wavinfo.size = 8 + read_little32from8( (void*)(bufstart + 4) );

        bufstart += minsize;
        bufsize  -= minsize;

        minsize = 2 * sizeof(uint32_t); // uint32_t chunk[2]

        while (bufstart < bufend)
        {
            if ( bufsize < minsize ) break;

            chunktype = read_little32from8( (void*)bufstart );
            chunksize = read_little32from8( (void*)(bufstart + 4) );

            bufstart += minsize;
            bufsize  -= minsize;

            if ( chunktype == WAV_FMT )
            {
                minsize = 7 * sizeof(uint16_t); // uint16_t fmt[7]
                if ( bufsize < minsize ) break;

                wavinfo.format     = read_little16from8( (void*)(bufstart + 0) ); // fmt[0]
                wavinfo.channels   = read_little16from8( (void*)(bufstart + 2) ); // fmt[1]
                wavinfo.samplerate = (uint16_t)read_little32from8( (void*)(bufstart + 4) ); // fmt[2]
                wavinfo.samplesize = read_little16from8( (void*)(bufstart + 12) ) * 8 / wavinfo.channels; // fmt[6]
                break;
            }
            else
            {
                bufstart += chunksize;
                bufsize  -= chunksize;
            }
        }
    }
    if ( wavinfo.channels && wavinfo.samplerate && wavinfo.samplesize )
    {
        wavinfo.playtime = wavinfo.size / (wavinfo.channels * wavinfo.samplerate * wavinfo.samplesize / 8);
    }
    else
    {
        wavinfo.playtime = 0;
    }
    return wavinfo;
}

/**********************************************************************
|*
|*  FUNCTION    : wav_testsupported
|*
|*  PARAMETERS  : header = wave file information structure
|*
|*  RETURNS     : False if not supported, true otherwise
|*
|*  DESCRIPTION : Check if this is a file we support (PCM encoded WAV, mono or stereo, 8 or 16 bits per sample only)
 */
bool wav_testsupported( wav_header_t header )
{
    bool supported = true;
    if ( header.format != WAVE_FORMAT_PCM ) supported = false;
    if ( header.channels != 1 && header.channels != 2 ) supported = false;
    if ( header.samplesize != 8 && header.samplesize != 16 ) supported = false;
    return supported;
}

