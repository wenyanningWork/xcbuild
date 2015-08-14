// Copyright 2013-present Facebook. All Rights Reserved.

#include <pbxspec/PBX/PackageType.h>

using pbxspec::PBX::PackageType;

PackageType::PackageType(bool isDefault) :
    Specification        (ISA::PBXPackageType, isDefault),
    _defaultBuildSettings(nullptr)
{
}

PackageType::~PackageType()
{
    if (_defaultBuildSettings != nullptr) {
        _defaultBuildSettings->release();
    }
}

PackageType::shared_ptr PackageType::
Parse(plist::Dictionary const *dict)
{
    auto T = dict->value <plist::String> ("Type");
    if (T == nullptr || T->value() != Type())
        return false;

    PackageType::shared_ptr result;
    auto C = dict->value <plist::String> ("Class");
    if (C == nullptr) {
        result.reset(new PackageType(true));
    } else {
        fprintf(stderr, "warning: package type class '%s' not recognized\n",
                C->value().c_str());
        result.reset(new PackageType(true));
    }
    if (!result->parse(dict))
        return nullptr;

    return result;
}

bool PackageType::
parse(plist::Dictionary const *dict)
{
    plist::WarnUnhandledKeys(dict, "PackageType",
        // Specification
        plist::MakeKey <plist::String> ("Class"),
        plist::MakeKey <plist::String> ("Type"),
        plist::MakeKey <plist::String> ("Identifier"),
        plist::MakeKey <plist::String> ("BasedOn"),
        plist::MakeKey <plist::String> ("Name"),
        plist::MakeKey <plist::String> ("Description"),
        plist::MakeKey <plist::String> ("Vendor"),
        plist::MakeKey <plist::String> ("Version"),
        // PackageType
        plist::MakeKey <plist::Dictionary> ("ProductReference"),
        plist::MakeKey <plist::Dictionary> ("DefaultBuildSettings"));

    if (!Specification::parse(dict))
        return false;

    auto PR  = dict->value <plist::Dictionary> ("ProductReference");
    auto DBS = dict->value <plist::Dictionary> ("DefaultBuildSettings");

    if (PR != nullptr) {
        _productReference.reset(new ProductReference);
        if (!_productReference->parse(PR)) {
            _productReference.reset();
        }
    }

    if (DBS != nullptr) {
        if (_defaultBuildSettings != nullptr) {
            _defaultBuildSettings->release();
        }

        _defaultBuildSettings = plist::Copy(DBS);
    }

    return true;
}

bool PackageType::
inherit(Specification::shared_ptr const &base)
{
    if (!base->isa(PackageType::Isa()))
        return false;

    return inherit(reinterpret_cast <PackageType::shared_ptr const &> (base));
}

bool PackageType::
inherit(PackageType::shared_ptr const &b)
{
    if (!Specification::inherit(b))
        return false;

    auto base = this->base();

    _productReference     = base->productReference();
    _defaultBuildSettings = plist::Copy(base->defaultBuildSettings());

    return true;
}

PackageType::ProductReference::ProductReference() :
    _isLaunchable(false)
{
}

bool PackageType::ProductReference::
parse(plist::Dictionary const *dict)
{
    plist::WarnUnhandledKeys(dict, "ProductReference",
        plist::MakeKey <plist::String> ("Name"),
        plist::MakeKey <plist::String> ("FileType"),
        plist::MakeKey <plist::Boolean> ("IsLaunchable"));

    auto N  = dict->value <plist::String> ("Name");
    auto FT = dict->value <plist::String> ("FileType");
    auto IL = dict->value <plist::Boolean> ("IsLaunchable");

    if (N != nullptr) {
        _name = N->value();
    }

    if (FT != nullptr) {
        _fileType = FT->value();
    }

    if (IL != nullptr) {
        _isLaunchable = IL->value();
    }

    return true;
}