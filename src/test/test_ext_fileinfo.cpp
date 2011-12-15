/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010- Facebook, Inc. (http://www.facebook.com)         |
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

#include <test/test_ext_fileinfo.h>
#include <runtime/ext/ext_fileinfo.h>

IMPLEMENT_SEP_EXTENSION_TEST(Fileinfo);
///////////////////////////////////////////////////////////////////////////////

bool TestExtFileinfo::RunTests(const std::string &which) {
  bool ret = true;

  RUN_TEST(test_finfo_open);
  RUN_TEST(test_finfo_close);
  RUN_TEST(test_finfo_set_flags);
  RUN_TEST(test_finfo_file);
  RUN_TEST(test_finfo_buffer);
  RUN_TEST(test_finfo);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

bool TestExtFileinfo::test_finfo_open() {
  VERIFY(f_finfo_open(k_FILEINFO_MIME, null).isResource());
  VERIFY(f_finfo_open(k_FILEINFO_MIME, "").isResource());
  VS(f_finfo_open(k_FILEINFO_MIME, 123), false);
  VS(f_finfo_open(k_FILEINFO_MIME, 1.0), false);
  VS(f_finfo_open(k_FILEINFO_MIME, "/foo/bar/inexistent"), false);
  VS(f_finfo_open(-1), false);
  return Count(true);
}

bool TestExtFileinfo::test_finfo_close() {
  Object resource = f_finfo_open();
  VS(f_finfo_close(resource), true);
  return Count(true);
}

bool TestExtFileinfo::test_finfo_set_flags() {
  Object resource = f_finfo_open(k_FILEINFO_MIME);
  VS(f_finfo_set_flags(resource, k_FILEINFO_NONE), true);
  VS(f_finfo_set_flags(resource, k_FILEINFO_SYMLINK), true);
  VS(f_finfo_set_flags(resource, -1), false);
  return Count(true);
}

bool TestExtFileinfo::test_finfo_file() {
  Object resource = f_finfo_open();
  VS(f_finfo_file(resource, ""), false);
  VS(f_finfo_file(resource, null), false);
  VS(f_finfo_file(resource, "."), "directory");
  VS(f_finfo_file(resource, "&"), false);

  f_finfo_set_flags(resource, k_FILEINFO_MIME_TYPE);
  VS(f_finfo_file(resource, "test/images/246x247.png"), "image/png");
  VS(f_finfo_file(resource, "test/images/php.gif"), "image/gif");
  VS(f_finfo_file(resource, "test/images/simpletext.jpg"), "image/jpeg");
  VS(f_finfo_file(resource, "test/test_ext_zlib.gz"), "application/x-gzip");
  VS(f_finfo_file(resource, "test/tahoma.ttf"), "application/x-font-ttf");

  VS(f_finfo_file(resource, "test/tahoma.ttf", k_FILEINFO_SYMLINK),
     "TrueType font data");
  return Count(true);
}

bool TestExtFileinfo::test_finfo_buffer() {
  Array buffers(ArrayInit(5)
                .set("Regular string here")
                .set("\177ELF")
                .set("<html>\n<head><title>Bogdan-Cristian Tătăroiu</title>"
                     "</head>\n<body></body>\n</html>")
                .set("\x55\x7A\x6E\x61")
                .set("id=ImageMagick")
                .set("RIFFüîò^BAVI LISTv"));
  Array resultsDefault(ArrayInit(6)
                       .set("ASCII text, with no line terminators")
                       .set("ELF")
                       .set("HTML document, UTF-8 Unicode text")
                       .set("xo65 object,")
                       .set("MIFF image data")
                       .set("RIFF (little-endian) data"));
  Array resultsMime(ArrayInit(6)
                    .set("text/plain; charset=us-ascii")
                    .set("text/plain; charset=ebcdic")
                    .set("text/html; charset=utf-8")
                    .set("text/plain; charset=us-ascii")
                    .set("text/plain; charset=us-ascii")
                    .set("text/plain; charset=utf-8"));

  Object resource = f_finfo_open(k_FILEINFO_NONE);
  for (int k = 0; k < 6; k++) {
    VS(f_finfo_buffer(resource, buffers[k]), resultsDefault[k]);
  }
  f_finfo_set_flags(resource, k_FILEINFO_MIME);
  for (int k = 0; k < 6; k++) {
    VS(f_finfo_buffer(resource, buffers[k]), resultsMime[k]);
  }
  return Count(true);
}

bool TestExtFileinfo::test_finfo() {
  {
    p_finfo res(NEWOBJ(c_finfo)());
    res->t___construct(-1);
    VS(res->t_buffer("<?php"), false);
  }

  {
    p_finfo res(NEWOBJ(c_finfo)());
    res->t___construct(k_FILEINFO_MIME);
    res->t_set_flags(k_FILEINFO_MIME_TYPE);
    VS(res->t_buffer("<?php"), "text/x-php");
    VS(res->t_file("test/tahoma.ttf"), "application/x-font-ttf");
  }
  return Count(true);
}
