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

#include <runtime/ext/ext_fileinfo.h>

namespace HPHP {
IMPLEMENT_DEFAULT_EXTENSION(fileinfo);
///////////////////////////////////////////////////////////////////////////////
// Constants

const int64 k_FILEINFO_NONE           = MAGIC_NONE;
const int64 k_FILEINFO_SYMLINK        = MAGIC_SYMLINK;
const int64 k_FILEINFO_MIME_TYPE      = MAGIC_MIME_TYPE;
const int64 k_FILEINFO_MIME_ENCODING  = MAGIC_MIME_ENCODING;
const int64 k_FILEINFO_MIME           = MAGIC_MIME;
// FILEINFO_COMPRESS is disabled as of php 5.3.8, because it "does fork now"
#ifdef ENABLE_FILEINFO_COMPRESS
const int64 k_FILEINFO_COMPRESS       = MAGIC_COMPRESS;
#else
const int64 k_FILEINFO_COMPRESS       = -1;
#endif
const int64 k_FILEINFO_DEVICES        = MAGIC_DEVICES;
const int64 k_FILEINFO_CONTINUE       = MAGIC_CONTINUE;
#ifdef MAGIC_PRESERVE_ATIME
const int64 k_FILEINFO_PRESERVE_ATIME = MAGIC_PRESERVE_ATIME;
#else
const int64 k_FILEINFO_PRESERVE_ATIME = -1;
#endif
#ifdef MAGIC_RAW
const int64 k_FILEINFO_RAW            = MAGIC_RAW;
#else
const int64 k_FILEINFO_RAW            = -1;
#endif

static const int64 k_FILEINFO_ALL =
    k_FILEINFO_SYMLINK | k_FILEINFO_MIME |
    k_FILEINFO_DEVICES | k_FILEINFO_CONTINUE |
#ifdef ENABLE_FILEINFO_COMPRESS
    k_FILEINFO_COMPRESS |
#endif
#ifdef MAGIC_PRESERVE_ATIME
    k_FILEINFO_PRESERVE_ATIME |
#endif
#ifdef MAGIC_RAW
    k_FILEINFO_RAW |
#endif
    k_FILEINFO_NONE;

// Required since stock libmagic is not thread-safe
static Mutex s_mutex;

///////////////////////////////////////////////////////////////////////////////
// FinfoResource class

class FinfoResource : public SweepableResourceData {
  DECLARE_OBJECT_ALLOCATION(FinfoResource)
public:
  static StaticString s_class_name;
  virtual CStrRef o_getClassNameHook() const { return s_class_name; }

  FinfoResource() : m_magic_cookie(NULL), m_options(-1) {
  }

  ~FinfoResource() {
    close();
  }

  bool isOpen() {
    return m_magic_cookie != NULL;
  }

  bool open(int64 options = k_FILEINFO_NONE, CStrRef magic_file = null_string) {
    if (isOpen()) close();

    m_magic_cookie = magic_open(k_FILEINFO_NONE);
    if (m_magic_cookie == NULL) {
      raise_warning("Unable to obtain libmagic cookie");
      return false;
    }
    if (!setOptions(options)) {
      magic_close(m_magic_cookie);
      m_magic_cookie = NULL;
      return false;
    }

    const char* magic_db = NULL;
    String magic_file_safe;
    if (!magic_file.empty()) {
      // Take care of SafeFileAccess restrictions
      magic_file_safe = File::TranslatePath(magic_file);
      if (magic_file_safe.empty()) {
        raise_warning(
          "Access to %s not allowed by SafeFileAccess configuration options",
          magic_file.data());
      } else {
        magic_db = magic_file_safe.data();
      }
    }
    {
      Lock lock(s_mutex);
      if (magic_load(m_magic_cookie, magic_db) == -1) {
        raise_warning("Failed to load magic database: (%d) %s",
                      magic_errno(m_magic_cookie), magic_error(m_magic_cookie));
        magic_close(m_magic_cookie);
        m_magic_cookie = NULL;
        return false;
      }
    }

    return true;
  }

  bool close() {
    if (isOpen()) {
      magic_close(m_magic_cookie);
      m_magic_cookie = NULL;
    }
    return true;
  }

  bool setOptions(int64 options) {
    if (!isOpen()) {
      raise_warning("Uninitialized finfo object");
      return false;
    }

    // options == -1 if the user specified an unsupported flag
    // options & (~k_FILEINFO_ALL) if the user specified extra flags not defined
    // in the public constants
    if (options == -1 || options & (~k_FILEINFO_ALL) ||
        magic_setflags(m_magic_cookie, options) == -1) {
      raise_warning("Invalid or unsupported finfo flags '%d'", options);
      return false;
    }
    m_options = options;
    return true;
  }

  Variant file(CStrRef file_name = null_string,
               int64 options = k_FILEINFO_NONE,
               CObjRef context = null_object) {
    if (file_name.empty()) {
      raise_warning("Empty filename or path");
      return false;
    }

    String file_name_safe = File::TranslatePath(file_name);
    if (file_name_safe.empty()) {
      raise_warning(
        "Access to %s not allowed by SafeFileAccess configuration options",
        file_name.data());
      return false;
    }

    return query(true, file_name, options, context);
  }

  Variant buffer(CStrRef string = null_string,
                 int64 options = k_FILEINFO_NONE,
                 CObjRef context = null_object) {
    return query(false, string, options, context);
  }

private:
  Variant query(bool file, CStrRef arg = null_string,
                int64 options = k_FILEINFO_NONE,
                CObjRef context = null_object) {
    if (!isOpen()) {
      raise_warning("Uninitialized finfo object");
      return false;
    }

    // HPHP doesn't implement the full stream protocol
    if (!context.isNull()) {
      throw NotSupportedException(__func__,
                                  "Stream contexts are not yet supported");
    }

    // Override default options... oh god why
    int old_options = m_options;
    if (options != k_FILEINFO_NONE) {
      if (!setOptions(options)) {
        return false;
      }
    }

    String desc;
    {
      Lock lock(s_mutex);
      const char *output;
      if (file) {
        output = magic_file(m_magic_cookie, arg.data());
      } else {
        output = magic_buffer(m_magic_cookie, arg.data(), arg.length());
      }
      if (output == NULL) {
        raise_warning("Failed to get description: (%d) %s",
                      magic_errno(m_magic_cookie), magic_error(m_magic_cookie));
        return false;
      } else {
        desc = String(output, CopyString);
      }
    }

    // Revert to default options
    if (options != k_FILEINFO_NONE) {
      setOptions(old_options);
    }
    return desc;
  }

private:
  magic_t m_magic_cookie;
  int64 m_options;
};
StaticString FinfoResource::s_class_name("file_info");
IMPLEMENT_OBJECT_ALLOCATION(FinfoResource);

///////////////////////////////////////////////////////////////////////////////
// Methods

#define CHECK_FINFO_RESOURCE(res)                                       \
  FinfoResource *res = finfo.getTyped<FinfoResource>(true, true);       \
  if (res == NULL) {                                                    \
    raise_warning("finfo resource has unexpected type or is invalid");  \
    return false;                                                       \
  }                                                                     \

Variant f_finfo_open(int64 options /* = k_FILEINFO_NONE */,
                     CStrRef magic_file /* = null_string */) {
  FinfoResource *res = NEWOBJ(FinfoResource)();
  Object handle(res);
  if (!res->open(options, magic_file)) {
    return false;
  }
  return handle;
}

bool f_finfo_close(CObjRef finfo) {
  CHECK_FINFO_RESOURCE(res);
  return res->close();
}

bool f_finfo_set_flags(CObjRef finfo, int64 options) {
  CHECK_FINFO_RESOURCE(res);
  return res->setOptions(options);
}

Variant f_finfo_file(CObjRef finfo, CStrRef file_name /* = null_string */,
                     int64 options /* = k_FILEINFO_NONE */,
                     CObjRef context /* = null_object */) {
  CHECK_FINFO_RESOURCE(res);
  return res->file(file_name, options, context);
}

Variant f_finfo_buffer(CObjRef finfo, CStrRef string /* = null_string */,
                       int64 options /* = k_FILEINFO_NONE */,
                       CObjRef context /* = null_object */) {
  CHECK_FINFO_RESOURCE(res);
  return res->buffer(string, options, context);
}

///////////////////////////////////////////////////////////////////////////////
// Object finfo API

c_finfo::c_finfo(const ObjectStaticCallbacks *cb) : ExtObjectData(cb),
                                                    m_res(false) {
}

c_finfo::~c_finfo() {
}

void c_finfo::t___construct(int64 options /* = k_FILEINFO_NONE */,
                            CStrRef magic_file /* = null_string */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(finfo, finfo::__construct);
  m_res = f_finfo_open(options, magic_file);
}

Variant c_finfo::t___destruct() {
  INSTANCE_METHOD_INJECTION_BUILTIN(finfo, finfo::__destruct);
  f_finfo_close(m_res);
  return null;
}

bool c_finfo::t_set_flags(int64 options) {
  INSTANCE_METHOD_INJECTION_BUILTIN(finfo, finfo::set_flags);
  return f_finfo_set_flags(m_res, options);
}

Variant c_finfo::t_file(CStrRef file_name /* = null_string */,
                        int64 options /* = k_FILEINFO_NONE */,
                        CObjRef context /* = null_object */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(finfo, finfo::file);
  return f_finfo_file(m_res, file_name, options, context);
}

Variant c_finfo::t_buffer(CStrRef string /* = null_string */,
                          int64 options /* = k_FILEINFO_NONE */,
                          CObjRef context /* = null_object */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(finfo, finfo::buffer);
  return f_finfo_buffer(m_res, string, options, context);
}

///////////////////////////////////////////////////////////////////////////////
}
