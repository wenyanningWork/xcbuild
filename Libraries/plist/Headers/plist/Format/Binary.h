/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef __plist_Format_Binary_h
#define __plist_Format_Binary_h

#include <plist/Format/Format.h>
#include <plist/Format/Type.h>

namespace plist {
namespace Format {

class Binary : public Format<Binary> {
private:
    Binary();

public:
    static Type FormatType();

public:
    static Binary Create();
};

}
}

#endif  // !__plist_Format_Binary_h
