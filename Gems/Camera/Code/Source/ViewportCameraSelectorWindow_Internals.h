/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QAbstractItemModel>
#include <QListView>
#include <AzFramework/Components/CameraBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/EditorCameraBus.h>

namespace Camera
{
    namespace Internal
    {
        // Each item in the list holds the camera's entityId and name
        struct CameraListItem
            : public AZ::EntityBus::Handler
        {
        public:
            CameraListItem(const AZ::EntityId& cameraId);
            ~CameraListItem();

            void OnEntityNameChanged(const AZStd::string& name) override { m_cameraName = name; }

            bool operator<(const CameraListItem& rhs);

            AZ::EntityId m_cameraId;
            AZStd::string m_cameraName;
        };

        struct ViewportCameraSelectorWindow;

        // holds a list of camera items
        struct CameraListModel
            : public QAbstractListModel
            , public CameraNotificationBus::Handler
        {
        public:
            explicit CameraListModel(QWidget* myParent);
            ~CameraListModel();

            // QAbstractItemModel interface
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

            // CameraNotificationBus interface
            void OnCameraAdded(const AZ::EntityId& cameraId) override;
            void OnCameraRemoved(const AZ::EntityId& cameraId) override;

            QModelIndex GetIndexForEntityId(const AZ::EntityId entityId);

        private:
            AZStd::vector<CameraListItem> m_cameraItems;
            AZ::EntityId m_sequenceCameraEntityId;
        };

        struct ViewportCameraSelectorWindow
            : public QListView
            , public EditorCameraNotificationBus::Handler
            , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
        {
        public:
            ViewportCameraSelectorWindow(QWidget* parent = nullptr);
            ~ViewportCameraSelectorWindow();

            void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;

            //////////////////////////////////////////////////////////////////////////
            /// EditorCameraNotificationBus::Handler
            void OnViewportViewEntityChanged(const AZ::EntityId& newViewId) override;

            //////////////////////////////////////////////////////////////////////////
            /// AzToolsFramework::EditorEntityContextRequestBus::Handler
            // make sure we can only use this window while in Edit mode
            void OnStartPlayInEditor() override;
            void OnStopPlayInEditor() override;

            void mouseMoveEvent(QMouseEvent*) override;
            void mouseDoubleClickEvent(QMouseEvent* event) override;
            QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
            QModelIndex GetPreviousIndex() const;
            QModelIndex GetNextIndex() const;

        private:
            CameraListModel* m_cameraList;
            bool m_ignoreViewportViewEntityChanged;
        };

        // wrapper for the ViewportCameraSelectorWindow so that we can add some descriptive helpful text
        struct ViewportSelectorHolder
            : public QWidget
        {
            explicit ViewportSelectorHolder(QWidget* parent = nullptr);
        };

        // simple factory method
        ViewportSelectorHolder* CreateNewSelectionWindow(QWidget* parent = nullptr);
    } // namespace Camera::Internal
} // namespace Camera
