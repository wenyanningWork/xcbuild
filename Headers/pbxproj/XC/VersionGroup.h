// Copyright 2013-present Facebook. All Rights Reserved.

#ifndef __pbxproj_XC_VersionGroup_h
#define __pbxproj_XC_VersionGroup_h

#include <pbxproj/PBX/BaseGroup.h>
#include <pbxproj/Context.h>

namespace pbxproj { namespace XC {

class VersionGroup : public PBX::BaseGroup {
public:
    typedef std::shared_ptr <VersionGroup> shared_ptr;

private:
    PBX::GroupItem::shared_ptr _currentVersion;
    std::string                _versionGroupType;

public:
    VersionGroup();

public:
    inline PBX::GroupItem::shared_ptr const &currentVersion() const
    { return _currentVersion; }
    inline PBX::GroupItem::shared_ptr &currentVersion()
    { return _currentVersion; }

public:
    inline std::string const &versionGroupType() const
    { return _versionGroupType; }

public:
    bool parse(Context &context, plist::Dictionary const *dict) override;

public:
    static inline char const *Isa()
    { return ISA::XCVersionGroup; }
};

} }

#endif  // !__pbxproj_XC_VersionGroup_h