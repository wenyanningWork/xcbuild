/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <gtest/gtest.h>
#include <builtin/copy/Options.h>
#include <builtin/copy/Driver.h>
#include <libutil/Filesystem.h>
#include <libutil/MemoryFilesystem.h>
#include <process/Context.h>
#include <process/MemoryContext.h>
#include <plist/Format/Encoding.h>

using builtin::copy::Driver;
using builtin::copy::Options;
using libutil::Filesystem;
using libutil::MemoryFilesystem;

static std::vector<uint8_t>
Contents(std::string const &string)
{
    return std::vector<uint8_t>(string.begin(), string.end());
}

static process::MemoryContext
Process(std::string const &executable, std::vector<std::string> const &arguments)
{
    return process::MemoryContext(
        executable,
        "/",
        arguments,
        std::unordered_map<std::string, std::string>());
}

TEST(copy, Name)
{
    Driver driver;
    EXPECT_EQ(driver.name(), "builtin-copy");
}

TEST(copy, CopyFiles)
{
    std::vector<uint8_t> contents;
    MemoryFilesystem filesystem = MemoryFilesystem({
        MemoryFilesystem::Entry::File("in1", Contents("one")),
        MemoryFilesystem::Entry::File("in2", Contents("two")),
        MemoryFilesystem::Entry::Directory("output", { }),
    });

    Driver driver;
    auto process = Process(driver.name(), { "in1", "in2", "output", });
    EXPECT_EQ(0, driver.run(&process, &filesystem));

    contents.clear();
    EXPECT_TRUE(filesystem.read(&contents, "/output/in1"));
    EXPECT_EQ(contents, Contents("one"));

    contents.clear();
    EXPECT_TRUE(filesystem.read(&contents, "/output/in2"));
    EXPECT_EQ(contents, Contents("two"));
}

TEST(copy, CopyDirectory)
{
    std::vector<uint8_t> contents;
    MemoryFilesystem filesystem = MemoryFilesystem({
        MemoryFilesystem::Entry::Directory("input", {
            MemoryFilesystem::Entry::File("in1", Contents("one")),
            MemoryFilesystem::Entry::File("in2", Contents("two")),
        }),
        MemoryFilesystem::Entry::Directory("output", { }),
    });

    Driver driver;
    auto process = Process(driver.name(), { "input", "output", });
    EXPECT_EQ(0, driver.run(&process, &filesystem));
    EXPECT_EQ(filesystem.type("/output"), Filesystem::Type::Directory);

    contents.clear();
    EXPECT_TRUE(filesystem.read(&contents, "/output/input/in1"));
    EXPECT_EQ(contents, Contents("one"));

    contents.clear();
    EXPECT_TRUE(filesystem.read(&contents, "/output/input/in2"));
    EXPECT_EQ(contents, Contents("two"));
}

TEST(copy, IgnoreMissingOutput)
{
    std::vector<uint8_t> contents;
    MemoryFilesystem filesystem = MemoryFilesystem({
        MemoryFilesystem::Entry::File("exists", Contents("")),
        MemoryFilesystem::Entry::Directory("output", { }),
    });

    Driver driver;

    /* Missing input should fail if not allowed. */
    auto fail = Process(driver.name(), { "exists", "missing", "output", });
    EXPECT_NE(0, driver.run(&fail, &filesystem));

    /* Missing input should succeed when allowed. */
    auto succeed = Process(driver.name(), { "-ignore-missing-inputs", "exists", "missing", "output", });
    EXPECT_EQ(0, driver.run(&succeed, &filesystem));
}

