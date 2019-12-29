/* Automatically generated header (sfdc 1.4)! Do not edit! */
#ifndef PRAGMAS_AHI_SUB_PRAGMAS_H
#define PRAGMAS_AHI_SUB_PRAGMAS_H

/*
**	$VER: ahi_sub_pragmas.h 5.2.2.2 (02.02.2005)
**
**	Direct ROM interface (pragma) definitions.
**
**	Copyright © 1994-2005 Martin Blom
**	    All Rights Reserved
*/

#if defined(LATTICE) || defined(__SASC) || defined(_DCC)
#ifndef __CLIB_PRAGMA_LIBCALL
#define __CLIB_PRAGMA_LIBCALL
#endif /* __CLIB_PRAGMA_LIBCALL */
#else /* __MAXON__, __STORM__ or AZTEC_C */
#ifndef __CLIB_PRAGMA_AMICALL
#define __CLIB_PRAGMA_AMICALL
#endif /* __CLIB_PRAGMA_AMICALL */
#endif /* */

#if defined(__SASC_60) || defined(__STORM__)
#ifndef __CLIB_PRAGMA_TAGCALL
#define __CLIB_PRAGMA_TAGCALL
#endif /* __CLIB_PRAGMA_TAGCALL */
#endif /* __MAXON__, __STORM__ or AZTEC_C */

#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_AllocAudio 1e a902
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x1e, AHIsub_AllocAudio(a1,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_FreeAudio 24 a01
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x24, AHIsub_FreeAudio(a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_Disable 2a a01
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x2a, AHIsub_Disable(a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_Enable 30 a01
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x30, AHIsub_Enable(a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_Start 36 a002
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x36, AHIsub_Start(d0,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_Update 3c a002
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x3c, AHIsub_Update(d0,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_Stop 42 a002
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x42, AHIsub_Stop(d0,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_SetVol 48 3a21005
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x48, AHIsub_SetVol(d0,d1,d2,a2,d3))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_SetFreq 4e 2a1004
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x4e, AHIsub_SetFreq(d0,d1,a2,d2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_SetSound 54 4a321006
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x54, AHIsub_SetSound(d0,d1,d2,d3,a2,d4))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_SetEffect 5a a802
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x5a, AHIsub_SetEffect(a0,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_LoadSound 60 a81004
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x60, AHIsub_LoadSound(d0,d1,a0,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_UnloadSound 66 a002
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x66, AHIsub_UnloadSound(d0,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_GetAttr 6c a921005
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x6c, AHIsub_GetAttr(d0,d1,d2,a1,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIsubBase AHIsub_HardwareControl 72 a1003
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIsubBase, 0x72, AHIsub_HardwareControl(d0,d1,a2))
#endif /* __CLIB_PRAGMA_AMICALL */

#endif /* PRAGMAS_AHI_SUB_PRAGMAS_H */
