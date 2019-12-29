/* Automatically generated function table (sfdc 1.4)! Do not edit! */

#ifndef AHI_INTERFACE_DEF_H
#define AHI_INTERFACE_DEF_H

#include <exec/types.h>
#include <exec/devices.h>
#include <utility/tagitem.h>
#include <devices/ahi.h>

struct AHIInterfaceData {
  struct Library * LibBase;
};

struct AHIIFace
{
  struct AHIInterfaceData Data;

  static struct AHIIFace* CreateIFace(struct Library * _AHIBase) {
    struct AHIIFace* _iface = new struct AHIIFace();
    _iface->Data.LibBase = _AHIBase;
    return _iface;
  }

  static void DestroyIFace(struct AHIIFace* _iface) {
    delete _iface;
  }

  struct AHIAudioCtrl * AHI_AllocAudioA(struct TagItem * tagList);
  struct AHIAudioCtrl * AHI_AllocAudio(Tag tag1, ...);
  void AHI_FreeAudio(struct AHIAudioCtrl * AudioCtrl);
  void AHI_KillAudio();
  ULONG AHI_ControlAudioA(struct AHIAudioCtrl * AudioCtrl, struct TagItem * tagList);
  ULONG AHI_ControlAudio(struct AHIAudioCtrl * AudioCtrl, Tag tag1, ...);
  void AHI_SetVol(UWORD Channel, Fixed Volume, sposition Pan, struct AHIAudioCtrl * AudioCtrl, ULONG Flags);
  void AHI_SetFreq(UWORD Channel, ULONG Freq, struct AHIAudioCtrl * AudioCtrl, ULONG Flags);
  void AHI_SetSound(UWORD Channel, UWORD Sound, ULONG Offset, LONG Length, struct AHIAudioCtrl * AudioCtrl, ULONG Flags);
  ULONG AHI_SetEffect(APTR Effect, struct AHIAudioCtrl * AudioCtrl);
  ULONG AHI_LoadSound(UWORD Sound, ULONG Type, APTR Info, struct AHIAudioCtrl * AudioCtrl);
  void AHI_UnloadSound(UWORD Sound, struct AHIAudioCtrl * Audioctrl);
  ULONG AHI_NextAudioID(ULONG Last_ID);
  BOOL AHI_GetAudioAttrsA(ULONG ID, struct AHIAudioCtrl * Audioctrl, struct TagItem * tagList);
  BOOL AHI_GetAudioAttrs(ULONG ID, struct AHIAudioCtrl * Audioctrl, Tag tag1, ...);
  ULONG AHI_BestAudioIDA(struct TagItem * tagList);
  ULONG AHI_BestAudioID(Tag tag1, ...);
  struct AHIAudioModeRequester * AHI_AllocAudioRequestA(struct TagItem * tagList);
  struct AHIAudioModeRequester * AHI_AllocAudioRequest(Tag tag1, ...);
  BOOL AHI_AudioRequestA(struct AHIAudioModeRequester * Requester, struct TagItem * tagList);
  BOOL AHI_AudioRequest(struct AHIAudioModeRequester * Requester, Tag tag1, ...);
  void AHI_FreeAudioRequest(struct AHIAudioModeRequester * Requester);
  void AHI_PlayA(struct AHIAudioCtrl * Audioctrl, struct TagItem * tagList);
  void AHI_Play(struct AHIAudioCtrl * Audioctrl, Tag tag1, ...);
  ULONG AHI_SampleFrameSize(ULONG SampleType);
  ULONG AHI_AddAudioMode(struct TagItem * AHIPrivate);
  ULONG AHI_RemoveAudioMode(ULONG AHIPrivate);
  ULONG AHI_LoadModeFile(STRPTR AHIPrivate);
};

#endif /* AHI_INTERFACE_DEF_H */
