/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <pbxbuild/Tool/InterfaceBuilderResolver.h>
#include <pbxbuild/Tool/InterfaceBuilderCommon.h>
#include <pbxbuild/Tool/Environment.h>
#include <pbxbuild/Tool/OptionsResult.h>
#include <pbxbuild/Tool/Tokens.h>
#include <pbxbuild/Tool/Context.h>
#include <pbxsetting/Environment.h>
#include <pbxsetting/Level.h>
#include <pbxsetting/Setting.h>
#include <pbxsetting/Type.h>

namespace Tool = pbxbuild::Tool;
namespace Phase = pbxbuild::Phase;

Tool::InterfaceBuilderResolver::
InterfaceBuilderResolver(pbxspec::PBX::Compiler::shared_ptr const &tool) :
    _tool(tool)
{
}

void Tool::InterfaceBuilderResolver::
resolve(
    Tool::Context *toolContext,
    pbxsetting::Environment const &baseEnvironment,
    std::vector<Phase::File> const &inputs) const
{
    /*
     * Filter arguments as either a real input or a localization-specific strings file.
     */
    std::vector<Phase::File> primaryInputs;
    std::vector<std::string> localizationStringsFiles;
    for (Phase::File const &input : inputs) {
        if (input.fileType()->identifier() == "text.plist.strings") {
            /* The format here is as expected by ibtool. */
            localizationStringsFiles.push_back(input.localization() + ":" + input.path());
        } else {
            primaryInputs.push_back(input);
        }
    }

    /*
     * Create the custom environment with the needed options.
     */
    pbxsetting::Level level = pbxsetting::Level({
        InterfaceBuilderCommon::TargetedDeviceSetting(baseEnvironment),
        pbxsetting::Setting::Create("IBC_REGIONS_AND_STRINGS_FILES", pbxsetting::Type::FormatList(localizationStringsFiles)),
    });
    pbxsetting::Environment interfaceBuilderEnvironment = pbxsetting::Environment(baseEnvironment);
    interfaceBuilderEnvironment.insertFront(level, false);

    /*
     * Resolve the tool options.
     */
    Tool::Environment toolEnvironment = Tool::Environment::Create(_tool, interfaceBuilderEnvironment, toolContext->workingDirectory(), primaryInputs);
    Tool::OptionsResult options = Tool::OptionsResult::Create(toolEnvironment, toolContext->workingDirectory(), nullptr);
    Tool::Tokens::ToolExpansions tokens = Tool::Tokens::ExpandTool(toolEnvironment, options);

    pbxsetting::Environment const &environment = toolEnvironment.environment();

    /*
     * Add custom arguments to the end.
     */
    std::vector<std::string> arguments = tokens.arguments();
    std::vector<std::string> deploymentTargetArguments = InterfaceBuilderCommon::DeploymentTargetArguments(environment);
    arguments.insert(arguments.end(), deploymentTargetArguments.begin(), deploymentTargetArguments.end());

    // TODO(grp): Invocations must emit all their outputs for now, but ibtool can emit both general
    // and device-specific (e.g. ~iphone, ~ipad) variants. For now, assume all files are not variant.
    std::vector<std::string> outputs = toolEnvironment.outputs();
    if (_tool->mightNotEmitAllOutputs() && !outputs.empty()) {
        outputs = { outputs.front() };
    }

    // TODO(grp): These should be handled generically for all tools.
    std::unordered_map<std::string, std::string> environmentVariables = options.environment();
    if (_tool->environmentVariables()) {
        for (auto const &variable : *_tool->environmentVariables()) {
            environmentVariables.insert({ variable.first, environment.expand(variable.second) });
        }
    }

    // TODO(grp): This should be handled generically for all tools.
    if (_tool->generatedInfoPlistContentFilePath()) {
        std::string infoPlistContent = environment.expand(*_tool->generatedInfoPlistContentFilePath());
        toolContext->additionalInfoPlistContents().push_back(infoPlistContent);
        outputs.push_back(infoPlistContent);
    }

    /*
     * Create the invocation.
     */
    Tool::Invocation invocation;
    invocation.executable() = Tool::Invocation::Executable::Determine(tokens.executable());
    invocation.arguments() = arguments;
    invocation.environment() = environmentVariables;
    invocation.workingDirectory() = toolContext->workingDirectory();
    invocation.inputs() = toolEnvironment.inputs(toolContext->workingDirectory());
    invocation.outputs() = outputs;
    invocation.logMessage() = tokens.logMessage();
    toolContext->invocations().push_back(invocation);
}

std::unique_ptr<Tool::InterfaceBuilderResolver> Tool::InterfaceBuilderResolver::
Create(Phase::Environment const &phaseEnvironment, std::string const &toolIdentifier)
{
    Build::Environment const &buildEnvironment = phaseEnvironment.buildEnvironment();
    Target::Environment const &targetEnvironment = phaseEnvironment.targetEnvironment();

    pbxspec::PBX::Compiler::shared_ptr interfaceBuilderTool = buildEnvironment.specManager()->compiler(toolIdentifier, targetEnvironment.specDomains());
    if (interfaceBuilderTool == nullptr) {
        fprintf(stderr, "warning: could not find interface builder tool\n");
        return nullptr;
    }

    return std::unique_ptr<Tool::InterfaceBuilderResolver>(new Tool::InterfaceBuilderResolver(interfaceBuilderTool));
}

