/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <pbxbuild/Tool/ToolResolver.h>
#include <pbxbuild/Tool/Environment.h>
#include <pbxbuild/Tool/Tokens.h>
#include <pbxbuild/Tool/OptionsResult.h>
#include <pbxbuild/Tool/Context.h>
#include <libutil/FSUtil.h>

namespace Tool = pbxbuild::Tool;
namespace Phase = pbxbuild::Phase;
using libutil::FSUtil;

Tool::ToolResolver::
ToolResolver(pbxspec::PBX::Tool::shared_ptr const &tool) :
    _tool(tool)
{
}

Tool::ToolResolver::
~ToolResolver()
{
}

void Tool::ToolResolver::
resolve(
    Tool::Context *toolContext,
    pbxsetting::Environment const &environment,
    std::vector<Phase::File> const &inputs,
    std::string const &outputDirectory,
    std::string const &logMessage) const
{
    Tool::Environment toolEnvironment = Tool::Environment::Create(_tool, environment, toolContext->workingDirectory(), inputs);
    Tool::OptionsResult options = Tool::OptionsResult::Create(toolEnvironment, toolContext->workingDirectory(), nullptr);
    Tool::Tokens::ToolExpansions tokens = Tool::Tokens::ExpandTool(toolEnvironment, options);
    std::string const &resolvedLogMessage = (!logMessage.empty() ? logMessage : tokens.logMessage());

    std::vector<Tool::Invocation::DependencyInfo> dependencyInfo;
    if (_tool->deeplyStatInputDirectories()) {
        for (Phase::File const &input : inputs) {
            /* Create a dependency info file to track the input directory contents. */
            auto info = Tool::Invocation::DependencyInfo(dependency::DependencyInfoFormat::Directory, input.path());
            dependencyInfo.push_back(info);
        }
    }

    Tool::Invocation invocation;
    invocation.executable() = Tool::Invocation::Executable::Determine(tokens.executable());
    invocation.arguments() = tokens.arguments();
    invocation.environment() = options.environment();
    invocation.workingDirectory() = toolContext->workingDirectory();
    invocation.inputs() = toolEnvironment.inputs(toolContext->workingDirectory());
    invocation.outputs() = toolEnvironment.outputs(toolContext->workingDirectory());
    invocation.dependencyInfo() = dependencyInfo;
    invocation.logMessage() = resolvedLogMessage;
    toolContext->invocations().push_back(invocation);
}

void Tool::ToolResolver::
resolve(
    Tool::Context *toolContext,
    pbxsetting::Environment const &environment,
    std::vector<std::string> const &inputs,
    std::vector<std::string> const &outputs,
    std::string const &logMessage) const
{
    Tool::Environment toolEnvironment = Tool::Environment::Create(_tool, environment, toolContext->workingDirectory(), inputs, outputs);
    Tool::OptionsResult options = Tool::OptionsResult::Create(toolEnvironment, toolContext->workingDirectory(), nullptr);
    Tool::Tokens::ToolExpansions tokens = Tool::Tokens::ExpandTool(toolEnvironment, options);
    std::string const &resolvedLogMessage = (!logMessage.empty() ? logMessage : tokens.logMessage());

    Tool::Invocation invocation;
    invocation.executable() = Tool::Invocation::Executable::Determine(tokens.executable());
    invocation.arguments() = tokens.arguments();
    invocation.environment() = options.environment();
    invocation.workingDirectory() = toolContext->workingDirectory();
    invocation.inputs() = toolEnvironment.inputs(toolContext->workingDirectory());
    invocation.outputs() = toolEnvironment.outputs(toolContext->workingDirectory());
    invocation.logMessage() = resolvedLogMessage;
    toolContext->invocations().push_back(invocation);
}

std::unique_ptr<Tool::ToolResolver> Tool::ToolResolver::
Create(Phase::Environment const &phaseEnvironment, std::string const &identifier)
{
    Build::Environment const &buildEnvironment = phaseEnvironment.buildEnvironment();
    Target::Environment const &targetEnvironment = phaseEnvironment.targetEnvironment();

    pbxspec::PBX::Tool::shared_ptr tool = nullptr;
    if (pbxspec::PBX::Tool::shared_ptr tool_ = buildEnvironment.specManager()->tool(identifier, targetEnvironment.specDomains())) {
        tool = tool_;
    } else if (pbxspec::PBX::Compiler::shared_ptr compiler = buildEnvironment.specManager()->compiler(identifier, targetEnvironment.specDomains())) {
        tool = std::static_pointer_cast<pbxspec::PBX::Tool>(compiler);
    } else if (pbxspec::PBX::Linker::shared_ptr linker = buildEnvironment.specManager()->linker(identifier, targetEnvironment.specDomains())) {
        tool = std::static_pointer_cast<pbxspec::PBX::Tool>(linker);
    } else {
        fprintf(stderr, "warning: could not find tool %s\n", identifier.c_str());
        return nullptr;
    }

    return std::unique_ptr<Tool::ToolResolver>(new Tool::ToolResolver(tool));
}
