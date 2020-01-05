/* Automatically generated function table (sfdc 1.4)! Do not edit! */

#ifndef AHI_SUB_INTERFACE_DEF_H
#define AHI_SUB_INTERFACE_DEF_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <libraries/ahi_sub.h>

struct AHIsubInterfaceData {
  struct Library * LibBase;
};

struct AHIsubIFace
{
  struct AHIsubInterfaceData Data;

  static struct AHIsubIFace* CreateIFace(struct Library * _AHIsubBase) {
    struct AHIsubIFace* _iface = new struct AHIsubIFace();
    _iface->Data.LibBase = _AHIsubBase;
    return _iface;
  }

  static void DestroyIFace(struct AHIsubIFace* _iface) {
    delete _iface;
  }

  ULONG AHIsub_AllocAudio(struct TagItem * tagList, struct AHIAudioCtrlDrv * AudioCtrl);
  void AHIsub_FreeAudio(struct AHIAudioCtrlDrv * AudioCtrl);
  void AHIsub_Disable(struct AHIAudioCtrlDrv * AudioCtrl);
  void AHIsub_Enable(struct AHIAudioCtrlDrv * AudioCtrl);
  ULONG AHIsub_Start(ULONG Flags, struct AHIAudioCtrlDrv * AudioCtrl);
  ULONG AHIsub_Update(ULONG Flags, struct AHIAudioCtrlDrv * AudioCtrl);
  ULONG AHIsub_Stop(ULONG Flags, struct AHIAudioCtrlDrv * AudioCtrl);
  ULONG AHIsub_SetVol(UWORD Channel, Fixed Volume, sposition Pan, struct AHIAudioCtrlDrv * AudioCtrl, ULONG Flags);
  ULONG AHIsub_SetFreq(UWORD Channel, ULONG Freq, struct AHIAudioCtrlDrv * AudioCtrl, ULONG Flags);
  ULONG AHIsub_SetSound(UWORD Channel, UWORD Sound, ULONG Offset, LONG Length, struct AHIAudioCtrlDrv * AudioCtrl, ULONG Flags);
  ULONG AHIsub_SetEffect(APTR Effect, struct AHIAudioCtrlDrv * AudioCtrl);
  ULONG AHIsub_LoadSound(UWORD Sound, ULONG Type, APTR Info, struct AHIAudioCtrlDrv * AudioCtrl);
  ULONG AHIsub_UnloadSound(UWORD Sound, struct AHIAudioCtrlDrv * Audioctrl);
  LONG AHIsub_GetAttr(ULONG Attribute, LONG Argument, LONG DefValue, struct TagItem * tagList, struct AHIAudioCtrlDrv * AudioCtrl);
  LONG AHIsub_HardwareControl(ULONG Attribute, LONG Argument, struct AHIAudioCtrlDrv * AudioCtrl);
};

#endif /* AHI_SUB_INTERFACE_DEF_H */
