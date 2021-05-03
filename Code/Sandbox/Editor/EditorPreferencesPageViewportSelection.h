#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/RTTI/RTTI.h>

struct ViewportSelectionSettings { 
    AZ_TYPE_INFO(ViewportSelectionSettings, "{1FA88F7D-6378-414B-9F9C-7469688638CE}");

    enum SliceSelectionPassthroughMode { 
        SSPM_None,          // Not enabled at all
        SSPM_OnByDefault,   // Enabled and on by default (selecting without modifiers uses SSP)
        SSPM_OffByDefault   // Enabled and off by default (selecting using SSP requires modifier)
    };

    SliceSelectionPassthroughMode sliceSelectionPassthroughMode = SSPM_OnByDefault;
};

namespace AZ { class ReflectContext; }

class EditorPreferencesPage_ViewportSelection
    : public IPreferencesPage
{
public:
    AZ_RTTI(EditorPreferencesPage_ViewportSelection, "{EDDE5651-7D48-4DEF-AEAE-6786C8B2F116}", IPreferencesPage);

    static void Reflect(AZ::ReflectContext* serialize);

    EditorPreferencesPage_ViewportSelection();
    virtual ~EditorPreferencesPage_ViewportSelection() = default;

    virtual const char* GetCategory() override { return "Viewport"; }
    virtual const char* GetTitle() override { return "Selection"; }
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }

private:
    void InitializeSettings();

    ViewportSelectionSettings m_settings;
};

