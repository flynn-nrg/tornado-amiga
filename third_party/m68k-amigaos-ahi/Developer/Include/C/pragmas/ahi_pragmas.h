/* Automatically generated header (sfdc 1.4)! Do not edit! */
#ifndef PRAGMAS_AHI_PRAGMAS_H
#define PRAGMAS_AHI_PRAGMAS_H

/*
**	$VER: ahi_pragmas.h 5.3.2.2 (02.02.2005)
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
 #pragma libcall AHIBase AHI_AllocAudioA 2a 901
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x2a, AHI_AllocAudioA(a1))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_TAGCALL
 #ifdef __CLIB_PRAGMA_LIBCALL
  #pragma tagcall AHIBase AHI_AllocAudio 2a 901
 #endif /* __CLIB_PRAGMA_LIBCALL */
 #ifdef __CLIB_PRAGMA_AMICALL
  #pragma tagcall(AHIBase, 0x2a, AHI_AllocAudio(a1))
 #endif /* __CLIB_PRAGMA_AMICALL */
#endif /* __CLIB_PRAGMA_TAGCALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_FreeAudio 30 a01
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x30, AHI_FreeAudio(a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_ControlAudioA 3c 9a02
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x3c, AHI_ControlAudioA(a2,a1))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_TAGCALL
 #ifdef __CLIB_PRAGMA_LIBCALL
  #pragma tagcall AHIBase AHI_ControlAudio 3c 9a02
 #endif /* __CLIB_PRAGMA_LIBCALL */
 #ifdef __CLIB_PRAGMA_AMICALL
  #pragma tagcall(AHIBase, 0x3c, AHI_ControlAudio(a2,a1))
 #endif /* __CLIB_PRAGMA_AMICALL */
#endif /* __CLIB_PRAGMA_TAGCALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_SetVol 42 3a21005
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x42, AHI_SetVol(d0,d1,d2,a2,d3))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_SetFreq 48 2a1004
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x48, AHI_SetFreq(d0,d1,a2,d2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_SetSound 4e 4a321006
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x4e, AHI_SetSound(d0,d1,d2,d3,a2,d4))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_SetEffect 54 a802
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x54, AHI_SetEffect(a0,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_LoadSound 5a a81004
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x5a, AHI_LoadSound(d0,d1,a0,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_UnloadSound 60 a002
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x60, AHI_UnloadSound(d0,a2))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_NextAudioID 66 001
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x66, AHI_NextAudioID(d0))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_GetAudioAttrsA 6c 9a003
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x6c, AHI_GetAudioAttrsA(d0,a2,a1))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_TAGCALL
 #ifdef __CLIB_PRAGMA_LIBCALL
  #pragma tagcall AHIBase AHI_GetAudioAttrs 6c 9a003
 #endif /* __CLIB_PRAGMA_LIBCALL */
 #ifdef __CLIB_PRAGMA_AMICALL
  #pragma tagcall(AHIBase, 0x6c, AHI_GetAudioAttrs(d0,a2,a1))
 #endif /* __CLIB_PRAGMA_AMICALL */
#endif /* __CLIB_PRAGMA_TAGCALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_BestAudioIDA 72 901
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x72, AHI_BestAudioIDA(a1))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_TAGCALL
 #ifdef __CLIB_PRAGMA_LIBCALL
  #pragma tagcall AHIBase AHI_BestAudioID 72 901
 #endif /* __CLIB_PRAGMA_LIBCALL */
 #ifdef __CLIB_PRAGMA_AMICALL
  #pragma tagcall(AHIBase, 0x72, AHI_BestAudioID(a1))
 #endif /* __CLIB_PRAGMA_AMICALL */
#endif /* __CLIB_PRAGMA_TAGCALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_AllocAudioRequestA 78 801
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x78, AHI_AllocAudioRequestA(a0))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_TAGCALL
 #ifdef __CLIB_PRAGMA_LIBCALL
  #pragma tagcall AHIBase AHI_AllocAudioRequest 78 801
 #endif /* __CLIB_PRAGMA_LIBCALL */
 #ifdef __CLIB_PRAGMA_AMICALL
  #pragma tagcall(AHIBase, 0x78, AHI_AllocAudioRequest(a0))
 #endif /* __CLIB_PRAGMA_AMICALL */
#endif /* __CLIB_PRAGMA_TAGCALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_AudioRequestA 7e 9802
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x7e, AHI_AudioRequestA(a0,a1))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_TAGCALL
 #ifdef __CLIB_PRAGMA_LIBCALL
  #pragma tagcall AHIBase AHI_AudioRequest 7e 9802
 #endif /* __CLIB_PRAGMA_LIBCALL */
 #ifdef __CLIB_PRAGMA_AMICALL
  #pragma tagcall(AHIBase, 0x7e, AHI_AudioRequest(a0,a1))
 #endif /* __CLIB_PRAGMA_AMICALL */
#endif /* __CLIB_PRAGMA_TAGCALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_FreeAudioRequest 84 801
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x84, AHI_FreeAudioRequest(a0))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_PlayA 8a 9a02
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x8a, AHI_PlayA(a2,a1))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_TAGCALL
 #ifdef __CLIB_PRAGMA_LIBCALL
  #pragma tagcall AHIBase AHI_Play 8a 9a02
 #endif /* __CLIB_PRAGMA_LIBCALL */
 #ifdef __CLIB_PRAGMA_AMICALL
  #pragma tagcall(AHIBase, 0x8a, AHI_Play(a2,a1))
 #endif /* __CLIB_PRAGMA_AMICALL */
#endif /* __CLIB_PRAGMA_TAGCALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_SampleFrameSize 90 001
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x90, AHI_SampleFrameSize(d0))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_AddAudioMode 96 801
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x96, AHI_AddAudioMode(a0))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_RemoveAudioMode 9c 001
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0x9c, AHI_RemoveAudioMode(d0))
#endif /* __CLIB_PRAGMA_AMICALL */
#ifdef __CLIB_PRAGMA_LIBCALL
 #pragma libcall AHIBase AHI_LoadModeFile a2 801
#endif /* __CLIB_PRAGMA_LIBCALL */
#ifdef __CLIB_PRAGMA_AMICALL
 #pragma amicall(AHIBase, 0xa2, AHI_LoadModeFile(a0))
#endif /* __CLIB_PRAGMA_AMICALL */

#endif /* PRAGMAS_AHI_PRAGMAS_H */
