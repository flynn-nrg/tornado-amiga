/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_AHI_SUB_H
#define _INLINE_AHI_SUB_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef AHI_SUB_BASE_NAME
#define AHI_SUB_BASE_NAME AHIsubBase
#endif /* !AHI_SUB_BASE_NAME */

#define AHIsub_AllocAudio(___tagList, ___AudioCtrl) \
	LP2(0x1e, ULONG, AHIsub_AllocAudio , struct TagItem *, ___tagList, a1, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_FreeAudio(___AudioCtrl) \
	LP1NR(0x24, AHIsub_FreeAudio , struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_Disable(___AudioCtrl) \
	LP1NR(0x2a, AHIsub_Disable , struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_Enable(___AudioCtrl) \
	LP1NR(0x30, AHIsub_Enable , struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_Start(___Flags, ___AudioCtrl) \
	LP2(0x36, ULONG, AHIsub_Start , ULONG, ___Flags, d0, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_Update(___Flags, ___AudioCtrl) \
	LP2(0x3c, ULONG, AHIsub_Update , ULONG, ___Flags, d0, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_Stop(___Flags, ___AudioCtrl) \
	LP2(0x42, ULONG, AHIsub_Stop , ULONG, ___Flags, d0, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_SetVol(___Channel, ___Volume, ___Pan, ___AudioCtrl, ___Flags) \
	LP5(0x48, ULONG, AHIsub_SetVol , UWORD, ___Channel, d0, Fixed, ___Volume, d1, sposition, ___Pan, d2, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2, ULONG, ___Flags, d3,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_SetFreq(___Channel, ___Freq, ___AudioCtrl, ___Flags) \
	LP4(0x4e, ULONG, AHIsub_SetFreq , UWORD, ___Channel, d0, ULONG, ___Freq, d1, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2, ULONG, ___Flags, d2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_SetSound(___Channel, ___Sound, ___Offset, ___Length, ___AudioCtrl, ___Flags) \
	LP6(0x54, ULONG, AHIsub_SetSound , UWORD, ___Channel, d0, UWORD, ___Sound, d1, ULONG, ___Offset, d2, LONG, ___Length, d3, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2, ULONG, ___Flags, d4,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_SetEffect(___Effect, ___AudioCtrl) \
	LP2(0x5a, ULONG, AHIsub_SetEffect , APTR, ___Effect, a0, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_LoadSound(___Sound, ___Type, ___Info, ___AudioCtrl) \
	LP4(0x60, ULONG, AHIsub_LoadSound , UWORD, ___Sound, d0, ULONG, ___Type, d1, APTR, ___Info, a0, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_UnloadSound(___Sound, ___Audioctrl) \
	LP2(0x66, ULONG, AHIsub_UnloadSound , UWORD, ___Sound, d0, struct AHIAudioCtrlDrv *, ___Audioctrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_GetAttr(___Attribute, ___Argument, ___DefValue, ___tagList, ___AudioCtrl) \
	LP5(0x6c, LONG, AHIsub_GetAttr , ULONG, ___Attribute, d0, LONG, ___Argument, d1, LONG, ___DefValue, d2, struct TagItem *, ___tagList, a1, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#define AHIsub_HardwareControl(___Attribute, ___Argument, ___AudioCtrl) \
	LP3(0x72, LONG, AHIsub_HardwareControl , ULONG, ___Attribute, d0, LONG, ___Argument, d1, struct AHIAudioCtrlDrv *, ___AudioCtrl, a2,\
	, AHI_SUB_BASE_NAME)

#endif /* !_INLINE_AHI_SUB_H */
