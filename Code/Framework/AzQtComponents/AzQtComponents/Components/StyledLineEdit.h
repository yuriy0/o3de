/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QLineEdit>
#endif

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API StyledLineEdit
        : public QLineEdit
    {
        Q_OBJECT
        Q_PROPERTY(Flavor flavor READ flavor WRITE setFlavor NOTIFY flavorChanged)

    public:

        enum Flavor
        {
            Plain = 0,
            Information,
            Question,
            Invalid,
            Valid,

            FlavorCount
        };
        Q_ENUM(Flavor)

        explicit StyledLineEdit(QWidget* parent = nullptr);
        ~StyledLineEdit();
        Flavor flavor() const;
        void setFlavor(Flavor);

    protected:
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;

    signals:
        void flavorChanged();
        void onFocus(); // Required for focus dependent custom widgets, e.g. ConfigStringLineEditCtrl.
        void onFocusOut();

        // A combination of 'QLineEdit::textChanged' and 'QLineEdit::editingFinished'
        // Only emitted when editing is definitely finished, i.e. this widget looses focus,
        // and the text has definitely changed, i.e. it is different than when editing began
        void editingFinishedTextChanged(const QString& text);

    private:
        void validateEntry();

		// APC TODO: still needed?
        QString m_valueAtEditingStart;
        bool m_editing;
        Flavor m_flavor;
    };
} // namespace AzQtComponents

