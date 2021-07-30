/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>

#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>

#include <QSharedPointer>
#include <QTimer>
#include <QCollator>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        //AssetBrowserFilterModel
        AssetBrowserFilterModel::AssetBrowserFilterModel(QObject* parent)
            : QSortFilterProxyModel(parent)
        {
            m_showColumn.insert(AssetBrowserModel::m_column);
            m_collator.setNumericMode(true);
            AssetBrowserComponentNotificationBus::Handler::BusConnect();
        }

        AssetBrowserFilterModel::~AssetBrowserFilterModel()
        {
            AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
        }

        void AssetBrowserFilterModel::SetFilter(FilterConstType filter)
        {
            connect(filter.data(), &AssetBrowserEntryFilter::updatedSignal, this, &AssetBrowserFilterModel::filterUpdatedSlot);
            m_filter = filter;
            m_invalidateFilter = true;
            // asset browser entries are not guaranteed to have populated when the filter is set, delay filtering until they are
            bool isAssetBrowserComponentReady = false;
            AssetBrowserComponentRequestBus::BroadcastResult(isAssetBrowserComponentReady, &AssetBrowserComponentRequests::AreEntriesReady);
            if (isAssetBrowserComponentReady)
            {
                OnAssetBrowserComponentReady();
            }
        }

        void AssetBrowserFilterModel::OnAssetBrowserComponentReady()
        {
            if (m_invalidateFilter)
            {
                invalidateFilter();
                m_invalidateFilter = false;
            }
        }

        bool AssetBrowserFilterModel::filterAcceptsRowImpl(int source_row, const QModelIndex& source_parent) const
        {
            //get the source idx, if invalid early out
            QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
            if (!idx.isValid())
            {
                return false;
            }
            // no filter present, every entry is visible
            if (!m_filter)
            {
                return true;
            }

            //the entry is the internal pointer of the index
            auto entry = static_cast<AssetBrowserEntry*>(idx.internalPointer());

            // root should return true even if its not displayed in the treeview
            if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Root)
            {
                return true;
            }
            return m_filter->Match(entry);
        }

        AZ_CVAR(
            int,
            editor_assetBrowserIncrementalFilterRowsPerUpdate,
            100,
            nullptr,
            AZ::ConsoleFunctorFlags::Null,
            ""
        );

        AZStd::optional<bool> AssetBrowserFilterModel::filterAcceptsRowCached(int source_row, const QModelIndex& source_parent) const
        {
            const auto& cache = m_invalidateFilterIncrementally.m_filterCache;
            QModelIndex ix = sourceModel()->index(source_row, 0, source_parent);
            if (auto it = cache.find(FilterKey(ix)); it != cache.end())
            {
                return { it->second };
            }
            else
            {
                return {};
            }
        }

        bool AssetBrowserFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
        {
            if (m_invalidateFilterIncrementally.m_active)
            {
                if (m_invalidateFilterIncrementally.m_remainingThisUpdate <= 0)
                {
                    // We're done this step of the incremental update; reject ALL remaining rows
                    return false;
                }
                else
                {
                    if (auto res = filterAcceptsRowCached(source_row, source_parent))
                    {
                        // First check cache for results
                        return *res;
                    }
                    else
                    {
                        // Performing a check, increment counter
                        m_invalidateFilterIncrementally.m_remainingThisUpdate--;

                        // Compute result and store in cache
                        auto& cache = m_invalidateFilterIncrementally.m_filterCache;
                        QModelIndex ix = sourceModel()->index(source_row, 0, source_parent);
                        return cache.emplace(FilterKey(ix), filterAcceptsRowImpl(source_row, source_parent)).first->second;
                    }
                }
            }
            else
            {
                // Not currently incrementally updating, do a cache lookup, which should always succeed
                // but fall back to actually checking the filter if it doesn't
                if (auto res = filterAcceptsRowCached(source_row, source_parent))
                {
                    return *res;
                }
                else
                {
                    return filterAcceptsRowImpl(source_row, source_parent);
                }
            }
        }

        void AssetBrowserFilterModel::invalidateFilter()
        {
            // Reset cache
            m_invalidateFilterIncrementally.m_filterCache.clear();
            m_invalidateFilterIncrementally.m_active = true;

            continueIncrementalFilterRebuild();
        }

        void AssetBrowserFilterModel::continueIncrementalFilterRebuild()
        {
            if (!m_invalidateFilterIncrementally.m_active)
            {
                // shouldn't happen anyways
                return;
            }

            // Store number of rows to update this time
            m_invalidateFilterIncrementally.m_remainingThisUpdate = AZ::GetMax((int)editor_assetBrowserIncrementalFilterRowsPerUpdate, 100);

            // Call base class method, which (synchronously) starts to recompute filter data
            QSortFilterProxyModel::invalidateFilter();

            // Check if a further incremental update is needed
            const bool active = m_invalidateFilterIncrementally.m_active = m_invalidateFilterIncrementally.m_remainingThisUpdate <= 0;
            if (active)
            {
                // Schedule to resume the filter rebuild some short time later
                QTimer::singleShot(10, this, [this]()
                {
                    continueIncrementalFilterRebuild();
                });
            }

            // Emit events
            Q_EMIT filterChanged();
        }

        bool AssetBrowserFilterModel::filterAcceptsColumn(int source_column, const QModelIndex&) const
        {
            //if the column is in the set we want to show it
            return m_showColumn.find(source_column) != m_showColumn.end();
        }

        bool AssetBrowserFilterModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
        {
            if (source_left.column() == source_right.column())
            {
                QVariant leftData = sourceModel()->data(source_left, AssetBrowserModel::Roles::EntryRole);
                QVariant rightData = sourceModel()->data(source_right, AssetBrowserModel::Roles::EntryRole);
                if (leftData.canConvert<const AssetBrowserEntry*>() && rightData.canConvert<const AssetBrowserEntry*>())
                {
                    auto leftEntry = qvariant_cast<const AssetBrowserEntry*>(leftData);
                    auto rightEntry = qvariant_cast<const AssetBrowserEntry*>(rightData);

                    // folders should always come first
                    if (azrtti_istypeof<const FolderAssetBrowserEntry*>(leftEntry) && azrtti_istypeof<const SourceAssetBrowserEntry*>(rightEntry))
                    {
                        return false;
                    }
                    if (azrtti_istypeof<const SourceAssetBrowserEntry*>(leftEntry) && azrtti_istypeof<const FolderAssetBrowserEntry*>(rightEntry))
                    {
                        return true;
                    }

                    // if both entries are of same type, sort alphabetically
                    return m_collator.compare(leftEntry->GetDisplayName(), rightEntry->GetDisplayName()) > 0;
                }
            }
            return QSortFilterProxyModel::lessThan(source_left, source_right);
        }

        void AssetBrowserFilterModel::FilterUpdatedSlotImmediate()
        {
            auto compFilter = qobject_cast<QSharedPointer<const CompositeFilter> >(m_filter);
            if (compFilter)
            {
                auto& subFilters = compFilter->GetSubFilters();
                auto it = AZStd::find_if(subFilters.begin(), subFilters.end(), [subFilters](FilterConstType filter) -> bool
                {
                    auto assetTypeFilter = qobject_cast<QSharedPointer<const CompositeFilter> >(filter);
                    return !assetTypeFilter.isNull();
                });
                if (it != subFilters.end())
                {
                    m_assetTypeFilter = qobject_cast<QSharedPointer<const CompositeFilter> >(*it);
                }
                it = AZStd::find_if(subFilters.begin(), subFilters.end(), [subFilters](FilterConstType filter) -> bool
                {
                    auto stringFilter = qobject_cast<QSharedPointer<const StringFilter> >(filter);
                    return !stringFilter.isNull();
                });
                if (it != subFilters.end())
                {
                    m_stringFilter = qobject_cast<QSharedPointer<const StringFilter> >(*it);
                }
            }

            const auto t0 = AZStd::chrono::system_clock::now();

            invalidateFilter();

            const auto t1 = AZStd::chrono::system_clock::now();
            const auto dt = AZStd::chrono::duration<float>(t1 - t0).count();

            if (dt > 0.1)
            {
                AZ_Warning("Editor", false, "Updating asset browser search filter took %f seconds", dt);
            }
            else
            {
                AZ_Printf("Editor", "Updating asset browser search filter took %f seconds", dt);
            }
        }

        void AssetBrowserFilterModel::filterUpdatedSlot()
        {
            if (!m_alreadyRecomputingFilters)
            {
                m_alreadyRecomputingFilters = true;
                // de-bounce it, since we may get many filter updates all at once.
                QTimer::singleShot(0, this, [this]()
                {
                    m_alreadyRecomputingFilters = false;
                    FilterUpdatedSlotImmediate();
                }
                );
            }
        }

        AssetBrowserFilterModel::FilterKey::FilterKey(const QModelIndex& ix)
            : m_ix(ix)
        {
        }

        AssetBrowserFilterModel::FilterKey::operator size_t() const
        {
            return qHash(m_ix);
        }

        bool AssetBrowserFilterModel::FilterKey::operator==(const FilterKey& other) const
        {
            return m_ix == other.m_ix;
        }

        bool AssetBrowserFilterModel::FilterKey::operator!=(const FilterKey& other) const
        {
            return !(*this == other);
        }

        bool AssetBrowserFilterModel::FilterKey::operator<(const FilterKey& other) const
        {
            return m_ix < other.m_ix;
        }

} // namespace AssetBrowser
} // namespace AzToolsFramework// namespace AssetBrowser

#include "AssetBrowser/moc_AssetBrowserFilterModel.cpp"
