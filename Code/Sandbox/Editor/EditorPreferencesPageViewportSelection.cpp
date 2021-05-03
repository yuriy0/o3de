#include "StdAfx.h"
#include "EditorPreferencesPageViewportSelection.h"

void EditorPreferencesPage_ViewportSelection::Reflect(AZ::ReflectContext * context) {
    if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context)) {

        serialize->Class<ViewportSelectionSettings>()
            ->Version(1)
            ->Field("SliceSelectionPassthroughMode", &ViewportSelectionSettings::sliceSelectionPassthroughMode);

        serialize->Class<EditorPreferencesPage_ViewportSelection>()
            ->Version(1)
            ->Field("Settings", &EditorPreferencesPage_ViewportSelection::m_settings);

        AZ::EditContext* editContext = serialize->GetEditContext();
        if (editContext) {
            editContext->Class<ViewportSelectionSettings>("ViewportSelectionSettings", "")
                ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ViewportSelectionSettings::sliceSelectionPassthroughMode,
                              "Slice Selection Passthrough Mode",
                              "When enabled, slice selection passthrough causes selecting an entity which is a part of a slice in the viewport to select the slice root entity instead."
                              "\nNone = Slice selection passthrough disabled"
                              "\nOn by default = Slice selection passthrough enabled and modifier key temporarily disables it"
                              "\nOff by default = Slice selection passthrough enabled but requires modifier key to use"
                )
                ->EnumAttribute(ViewportSelectionSettings::SSPM_None, "None")
                ->EnumAttribute(ViewportSelectionSettings::SSPM_OnByDefault, "On by default")
                ->EnumAttribute(ViewportSelectionSettings::SSPM_OffByDefault, "Off by default")
                ;

            editContext->Class<EditorPreferencesPage_ViewportSelection>("Viewport Selection Preferences", "")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPreferencesPage_ViewportSelection::m_settings, "Settings", "Viewport Selection Settings");
        }
    }
}

EditorPreferencesPage_ViewportSelection::EditorPreferencesPage_ViewportSelection() {
    InitializeSettings();
}

void EditorPreferencesPage_ViewportSelection::OnApply() {
    gSettings.viewportSelectionSettings = m_settings;
}

void EditorPreferencesPage_ViewportSelection::InitializeSettings() {
    m_settings = gSettings.viewportSelectionSettings;
}
