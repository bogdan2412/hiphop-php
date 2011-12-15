/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010- Facebook, Inc. (http://www.facebook.com)         |
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#ifndef __EXT_FILEINFO_H__
#define __EXT_FILEINFO_H__

// >>>>>> Generated by idl.php. Do NOT modify. <<<<<<

#include <runtime/base/base_includes.h>
#include <magic.h>

namespace HPHP {
extern const int64 k_FILEINFO_NONE;
}

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

Variant f_finfo_open(int64 options = k_FILEINFO_NONE, CStrRef magic_file = null_string);
bool f_finfo_close(CObjRef finfo);
bool f_finfo_set_flags(CObjRef finfo, int64 options);
Variant f_finfo_file(CObjRef finfo, CStrRef file_name = null_string, int64 options = k_FILEINFO_NONE, CObjRef context = null_object);
Variant f_finfo_buffer(CObjRef finfo, CStrRef string = null_string, int64 options = k_FILEINFO_NONE, CObjRef context = null_object);
extern const int64 k_FILEINFO_NONE;
extern const int64 k_FILEINFO_SYMLINK;
extern const int64 k_FILEINFO_MIME_TYPE;
extern const int64 k_FILEINFO_MIME_ENCODING;
extern const int64 k_FILEINFO_MIME;
extern const int64 k_FILEINFO_COMPRESS;
extern const int64 k_FILEINFO_DEVICES;
extern const int64 k_FILEINFO_CONTINUE;
extern const int64 k_FILEINFO_PRESERVE_ATIME;
extern const int64 k_FILEINFO_RAW;

///////////////////////////////////////////////////////////////////////////////
// class finfo

FORWARD_DECLARE_CLASS_BUILTIN(finfo);
class c_finfo : public ExtObjectData {
 public:
  DECLARE_CLASS(finfo, finfo, ObjectData)

  // need to implement
  public: c_finfo(const ObjectStaticCallbacks *cb = &cw_finfo);
  public: ~c_finfo();
  public: void t___construct(int64 options = k_FILEINFO_NONE, CStrRef magic_file = null_string);
  DECLARE_METHOD_INVOKE_HELPERS(__construct);
  public: Variant t___destruct();
  DECLARE_METHOD_INVOKE_HELPERS(__destruct);
  public: bool t_set_flags(int64 options);
  DECLARE_METHOD_INVOKE_HELPERS(set_flags);
  public: Variant t_file(CStrRef file_name = null_string, int64 options = k_FILEINFO_NONE, CObjRef context = null_object);
  DECLARE_METHOD_INVOKE_HELPERS(file);
  public: Variant t_buffer(CStrRef string = null_string, int64 options = k_FILEINFO_NONE, CObjRef context = null_object);
  DECLARE_METHOD_INVOKE_HELPERS(buffer);

  // implemented by HPHP
  public: c_finfo *create(int64 options = k_FILEINFO_NONE, String magic_file = null_string);

  private:
    Variant m_res;
};

///////////////////////////////////////////////////////////////////////////////
}

#endif // __EXT_FILEINFO_H__
