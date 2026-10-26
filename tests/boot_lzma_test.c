/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio)
 * Portions Copyright (C) 2026 Charles Vestal */
/* boot_lzma.h against a stream Python's lzma made (tests/boot_audio_test.py): boot_lzma_test PACKED RAW SIZE */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../firmware/src/boot_lzma.h"
int main(int argc,char **argv)
{
    assert(argc==4);uint32_t size=(uint32_t)strtoul(argv[3],0,10);
    uint16_t *probs=malloc(BOOT_LZMA_PROBS*sizeof *probs);assert(probs);
    FILE *f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);
    uint8_t *in=malloc((size_t)n+1u),*out=malloc(size+16u);assert(in&&out);assert(fread(in,1,(size_t)n,f)==(size_t)n);fclose(f);
    memset(out,0x5a,size+16u);assert(boot_lzma(out,size,in,(uint32_t)n,probs));
    f=fopen(argv[2],"rb");assert(f);uint8_t *expected=malloc(size?size:1u);assert(expected);assert(fread(expected,1,size,f)==size);fclose(f);assert(!memcmp(out,expected,size));
    for(uint32_t i=size;i<size+16u;i++)assert(out[i]==0x5a);
    if(n)assert(!boot_lzma(out,size,in,(uint32_t)n-1u,probs));            /* cut short */
    if(size)assert(!boot_lzma(out,size-1u,in,(uint32_t)n,probs));         /* more than the room */
    assert(!boot_lzma(out,size+1u,in,(uint32_t)n,probs));                 /* less than asked */
    in[n]=0;assert(!boot_lzma(out,size,in,(uint32_t)n+1u,probs));         /* a byte after the end */
    /* Deterministic malformed streams and mutated valid streams, bounded output.
     * ASan/UBSan checks both ends; explicit guards check the destination bound. */
    uint32_t seed=90125u;uint8_t fuzz[512],bounded[4096+32];
    for(uint32_t trial=0;trial<10000u;trial++){
        seed=seed*1664525u+1013904223u;uint32_t len=seed%sizeof fuzz;
        seed=seed*1664525u+1013904223u;uint32_t cap=seed%4097u;
        for(uint32_t i=0;i<len;i++){
            seed=seed*1664525u+1013904223u;
            fuzz[i]=(trial&1u) && i<(uint32_t)n?in[i]:(uint8_t)(seed>>24);
        }
        if(len){seed=seed*1664525u+1013904223u;fuzz[seed%len]^=(uint8_t)(seed>>16);}
        memset(bounded,0xa7,sizeof bounded);
        (void)boot_lzma(bounded+16u,cap,fuzz,len,probs);
        for(uint32_t i=0;i<16u;i++)assert(bounded[i]==0xa7);
        for(uint32_t i=16u+cap;i<sizeof bounded;i++)assert(bounded[i]==0xa7);
    }
    free(in);free(out);free(expected);free(probs);return 0;
}
