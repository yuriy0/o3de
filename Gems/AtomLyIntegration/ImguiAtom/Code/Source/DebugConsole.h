/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Contexts/InputContext.h>
#include <AzFramework/Input/Events/InputChannelEventFilter.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Debug/TraceMessagesDrillerBus.h>
#include <AzCore/Console/IConsole.h>

#include <AzCore/std/containers/deque.h>
#include <AzCore/std/string/string.h>

#include <Atom/RPI.Public/ViewportContextBus.h>

struct ImGuiInputTextCallbackData;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
#if !defined(IMGUI_ENABLED)
    class DebugConsole {};
#else
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! A debug console used to enter debug console commands and display debug log messages.
    //!
    //! Toggled using any of the following:
    //! - The '~' key on a keyboard.
    //! - Both the 'L3+R3' buttons on a gamepad.
    //! - The fourth finger press on a touch screen.
    class DebugConsole : public AzFramework::InputChannelEventListener
                       , public AZ::RPI::ViewportContextNotificationBus::Handler
        , private AZ::Debug::TraceMessageDrillerBus::Handler
    {
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The default maximum number of entries to display in the debug log.
        static constexpr int DefaultMaxEntriesToDisplay = 1028;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The default maximum number of input history items to retain.
        static constexpr int DefaultMaxInputHistorySize = 512;

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(DebugConsole, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] maxEntriesToDisplay The maximum number of entries to display in the debug log.
        //! \param[in] maxInputHistorySize The maximum number of text input history items to retain.
        DebugConsole(int maxEntriesToDisplay = DefaultMaxEntriesToDisplay,
                     int maxInputHistorySize = DefaultMaxInputHistorySize);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(DebugConsole);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~DebugConsole() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::RPI::ViewportContextRequestsInterface
        void OnRenderTick() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelEventListener::OnInputChannelEventFiltered
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! TraceMessageDrillerBus
        void OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
        void OnException(const char* /*message*/) override;
        void OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
        void OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
        void OnPrintf(const char* /*window*/, const char* /*message*/) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add a string to the debug log display.
        //! \param[in] debugLogString The string to add to the debug log display.
        //! \param[in] color The color in which to display the above string.

        struct MessageWithTracing
        {
            AZStd::string_view message;
            AZStd::optional<AZStd::tuple<const char*, int, const char*>> sourceLocation;
            bool callstack = false;
            AZStd::string_view tracePrefix;
            AZStd::string_view window;
            AZ::LogLevel level;
        };
        void AddLogMessageWithTracing(MessageWithTracing);

        template<class Str>
        void AddLogMessage(Str&& debugLogString, AZ::LogLevel level);

        template<class Str>
        void AddSystemLogMessage(Str&& debugLogString, const AZ::Color& color);

        void RenderLogEntries();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Clears the debug log display.
        void ClearDebugLog();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Attempt to auto complete a command using this input text callback data.
        //! \param[in] data The input text callback data used try and auto complete.
        void AutoCompleteCommand(ImGuiInputTextCallbackData* data);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Attempt to browse the input history using this input text callback data.
        //! \param[in] data The input text callback data used to try and browse history.
        void BrowseInputHistory(ImGuiInputTextCallbackData* data);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when the user enters text input.
        //! \param[in] inputText The text input that was entered.
        void OnTextInputEntered(const char* inputText);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Draw the debug console.
        //! \return True if we should continue showing the debug console, false otherwise.
        bool DrawDebugConsole();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Toggle whether the debug console is showing or not.
        void ToggleIsShowing();

    private:
        struct TextLogEntry
        {
            AZStd::string entryText;

            void ImGuiRender() const;
        };

        struct ColoredTextLogEntry
        {
            AZStd::string entryText;
            AZ::Color textColor = AZ::Colors::White;

            void ImGuiRender() const;
        };

        struct SameLineLogEntry
        {
            void ImGuiRender() const;
        };

        struct SeperatorLogEntry
        {
            void ImGuiRender() const;
        };

        struct DebugLogEntry
        {
            AZStd::variant<TextLogEntry, ColoredTextLogEntry, SameLineLogEntry, SeperatorLogEntry> logEntry;
            AZStd::optional<AZ::LogLevel> logLevel;
        };

        void AddLogEntry(DebugLogEntry entry);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::deque<DebugLogEntry> m_debugLogEntires; //!< All debug logs.

        AZStd::deque<AZStd::string> m_textInputHistory; //!< History of input that has been entered.
        AZ::ILogger::LogEvent::Handler m_logHandler; //!< Handler that receives log events to display.
        AzFramework::InputContext m_inputContext; //!< Input context used to open/close the console.
        char m_inputBuffer[1028] = {}; //!< The character buffer used to accept text input.
        AzFramework::SystemCursorState m_previousSystemCursorState; //! The last system cursor state.
        int m_currentHistoryIndex = -1; //!< The current index into the input history when browsing.
        int m_maxEntriesToDisplay = DefaultMaxEntriesToDisplay; //!< The maximum entries to display.
        int m_maxInputHistorySize = DefaultMaxInputHistorySize; //!< The maximum input history size.
        bool m_isShowing = false; //!< Is the debug console currently being displayed?
        bool m_autoScroll = true; //!< Should we auto-scroll as new entries are added?
        bool m_forceScroll = false; //!< Do we need to force scroll after input entered?
        AZ::DispatchCommandNotFoundEvent::Handler m_commandNotFoundHandler;
    };
#endif // defined(IMGUI_ENABLED)
} // namespace AZ
