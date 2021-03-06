// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: IM.Other.proto

#ifndef PROTOBUF_IM_2eOther_2eproto__INCLUDED
#define PROTOBUF_IM_2eOther_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3000000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3000000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
// @@protoc_insertion_point(includes)

namespace IM {
namespace Other {

// Internal implementation detail -- do not call these.
void protobuf_AddDesc_IM_2eOther_2eproto();
void protobuf_AssignDesc_IM_2eOther_2eproto();
void protobuf_ShutdownFile_IM_2eOther_2eproto();

class IMHeartBeat;

// ===================================================================

class IMHeartBeat : public ::google::protobuf::MessageLite /* @@protoc_insertion_point(class_definition:IM.Other.IMHeartBeat) */ {
 public:
  IMHeartBeat();
  virtual ~IMHeartBeat();

  IMHeartBeat(const IMHeartBeat& from);

  inline IMHeartBeat& operator=(const IMHeartBeat& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::std::string& unknown_fields() const {
    return _unknown_fields_.GetNoArena(
        &::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }

  inline ::std::string* mutable_unknown_fields() {
    return _unknown_fields_.MutableNoArena(
        &::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }

  static const IMHeartBeat& default_instance();

  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  // Returns the internal default instance pointer. This function can
  // return NULL thus should not be used by the user. This is intended
  // for Protobuf internal code. Please use default_instance() declared
  // above instead.
  static inline const IMHeartBeat* internal_default_instance() {
    return default_instance_;
  }
  #endif

  void Swap(IMHeartBeat* other);

  // implements Message ----------------------------------------------

  inline IMHeartBeat* New() const { return New(NULL); }

  IMHeartBeat* New(::google::protobuf::Arena* arena) const;
  void CheckTypeAndMergeFrom(const ::google::protobuf::MessageLite& from);
  void CopyFrom(const IMHeartBeat& from);
  void MergeFrom(const IMHeartBeat& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  void DiscardUnknownFields();
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(IMHeartBeat* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _arena_ptr_;
  }
  inline ::google::protobuf::Arena* MaybeArenaPtr() const {
    return _arena_ptr_;
  }
  public:

  ::std::string GetTypeName() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // @@protoc_insertion_point(class_scope:IM.Other.IMHeartBeat)
 private:

  ::google::protobuf::internal::ArenaStringPtr _unknown_fields_;
  ::google::protobuf::Arena* _arena_ptr_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  #ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
  friend void  protobuf_AddDesc_IM_2eOther_2eproto_impl();
  #else
  friend void  protobuf_AddDesc_IM_2eOther_2eproto();
  #endif
  friend void protobuf_AssignDesc_IM_2eOther_2eproto();
  friend void protobuf_ShutdownFile_IM_2eOther_2eproto();

  void InitAsDefaultInstance();
  static IMHeartBeat* default_instance_;
};
// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
// IMHeartBeat

#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS

// @@protoc_insertion_point(namespace_scope)

}  // namespace Other
}  // namespace IM

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_IM_2eOther_2eproto__INCLUDED
