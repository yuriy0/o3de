/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZ_SCRIPT_COMPONENT_H
#define AZ_SCRIPT_COMPONENT_H

#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

namespace AZ
{
    class ScriptProperty;
}

namespace AzToolsFramework
{
    namespace Components
    {
        class ScriptEditorComponent;
    }
}

namespace AzFramework
{
    struct ScriptCompileRequest;

    using WriteFunction = AZStd::function< AZ::Outcome<void, AZStd::string>(const ScriptCompileRequest&, AZ::IO::GenericStream& in, AZ::IO::GenericStream& out) >;

    struct ScriptCompileRequest
    {
        AZStd::string_view m_errorWindow;
        AZStd::string_view m_sourceFile;
        AZStd::string_view m_fullPath;
        AZStd::string_view m_fileName;
        AZStd::string_view m_tempDirPath;        
        AZ::IO::GenericStream* m_input = nullptr;
        AZ::IO::GenericStream* m_output = nullptr;
        WriteFunction m_prewriteCallback;
        WriteFunction m_postwriteCallback;

        AZStd::string m_destFileName;
        AZStd::string m_destPath;
    };

    void ConstructScriptAssetPaths(ScriptCompileRequest& request);
    AZ::Outcome<void, AZStd::string> CompileScript(ScriptCompileRequest& request);
    AZ::Outcome<void, AZStd::string> CompileScriptAndAsset(ScriptCompileRequest& request);
    AZ::Outcome<void, AZStd::string> CompileScript(ScriptCompileRequest& request, AZ::ScriptContext& context);
    AZ::Outcome<AZStd::string, AZStd::string> CompileScriptAndSaveAsset(ScriptCompileRequest& request, bool writeAssetInfo = true);

    struct ScriptPropertyGroup
    {
        AZ_TYPE_INFO(ScriptPropertyGroup, "{79682522-2f81-4b36-9fc2-a091c7504f7f}");
        AZStd::string                       m_name;
        AZStd::vector<AZ::ScriptProperty*>  m_properties;
        AZStd::vector<ScriptPropertyGroup>  m_groups;

        // Get the pointer to the specified group in m_groups. Returns nullptr if not found.
        ScriptPropertyGroup* GetGroup(const char* groupName);
        // Get the pointer to the specified property in m_properties. Returns nullptr if not found.
        AZ::ScriptProperty* GetProperty(const char* propertyName);
        // Remove all properties and groups
        void Clear();

        ScriptPropertyGroup() = default;
        ~ScriptPropertyGroup();

        ScriptPropertyGroup(const ScriptPropertyGroup& rhs) = delete;
        ScriptPropertyGroup& operator=(ScriptPropertyGroup&) = delete;
    public:
        ScriptPropertyGroup(ScriptPropertyGroup&& rhs) { *this = AZStd::move(rhs); }
        ScriptPropertyGroup& operator=(ScriptPropertyGroup&& rhs);
    };

    class ScriptComponent
        : public AZ::Component
        , private AZ::Data::AssetBus::Handler
    {
        friend class AzToolsFramework::Components::ScriptEditorComponent;        

    public:
        static const char* DefaultFieldName;
        
        AZ_COMPONENT(AzFramework::ScriptComponent, "{8D1BC97E-C55D-4D34-A460-E63C57CD0D4B}", AZ::Component);
        
        void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided) const;
        void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent) const;
        void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required) const;
        void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible) const;

        /// \red ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* reflection);        

        ScriptComponent();
        ~ScriptComponent();

        AZ::ScriptContext* GetScriptContext() const            { return m_context; }
        void                  SetScriptContext(AZ::ScriptContext* context);

        const AZ::Data::Asset<AZ::ScriptAsset>& GetScript() const       { return m_script; }
        void                                    SetScript(const AZ::Data::Asset<AZ::ScriptAsset>& script);

        // Methods used for unit tests
        AZ::ScriptProperty* GetScriptProperty(const char* propertyName);

    protected:
        ScriptComponent(const ScriptComponent&) = delete;
        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        /// Load script (unless already by other instances) and creates the script instance into the VM
        void LoadScript();
        /// Removes the script instance and unloads the script (unless needed by other instances)
        void UnloadScript();

        /// Loads the script into the context/VM, \returns true if the script is loaded
        bool LoadInContext();

        /// Attempts to load services from the Lua script
        void LoadServices();
        void ClearServices();

        // Create script instance table.
        void CreateEntityTable();
        void DestroyEntityTable();

        void CreatePropertyGroup(const ScriptPropertyGroup& group, int propertyGroupTableIndex, int parentIndex, int metatableIndex, bool isRoot);

        AZ::ScriptContext*               m_context;              ///< Context in which the script will be running
        AZ::ScriptContextId                 m_contextId;            ///< Id of the script context.
        AZ::Data::Asset<AZ::ScriptAsset>    m_script;               ///< Reference to the script asset used for this component.
        int                                 m_table;                ///< Cached table index
        ScriptPropertyGroup                 m_properties;           ///< List with all properties that were tweaked in the editor and should override values in the m_sourceScriptName class inside m_script.

        // Services
        using ServicesSet = AZStd::unordered_set<AZ::Crc32>;
        ServicesSet m_providedServices;
        ServicesSet m_dependentServices;
        ServicesSet m_requiredServices;
        ServicesSet m_incompatibleServices;
    };        
}   // namespace AZ

#endif  // AZ_SCRIPT_COMPONENTH_
#pragma once
