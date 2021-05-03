/*
This file is a derivative of the Boost thread library, whose license text is as follows:
==================================================================
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba
==================================================================
see <boost/thread/lock_algorithms.hpp>

This is inlined into AZStd mainly because adding a boost dependency to AZStd would be annoying
*/

#pragma once

#include <AzCore/std/iterator.h>
#include <AzCore/std/parallel/lock.h>

namespace AZStd
{
    namespace detail
    {
        template <typename Iterator>
        void lock_range_impl(Iterator begin, Iterator end);

        template <typename Iterator>
        struct range_lock_guard
        {
            Iterator begin;
            Iterator end;

            range_lock_guard(Iterator begin_, Iterator end_) :
                begin(begin_), end(end_)
            {
                lock_range(begin, end);
            }

            void release()
            {
                begin = end;
            }

            ~range_lock_guard()
            {
                for (; begin != end; ++begin)
                {
                    begin->unlock();
                }
            }
        };

        template <typename Iterator>
        Iterator try_lock_range_impl(Iterator begin, Iterator end)
        {
            if (begin == end)
            {
                return end;
            }
            typedef typename AZStd::iterator_traits<Iterator>::value_type lock_type;
            unique_lock<lock_type> guard(*begin, try_to_lock);

            if (!guard.owns_lock())
            {
                return begin;
            }
            Iterator const failed = try_lock_range_impl(++begin, end);
            if (failed == end)
            {
                guard.release();
            }

            return failed;
        }

        template <typename Iterator>
        void lock_range_impl(Iterator begin, Iterator end)
        {
            typedef typename AZStd::iterator_traits<Iterator>::value_type lock_type;

            if (begin == end)
            {
                return;
            }
            bool start_with_begin = true;
            Iterator second = begin;
            ++second;
            Iterator next = second;

            for (;;)
            {
                unique_lock<lock_type> begin_lock(*begin, defer_lock);
                if (start_with_begin)
                {
                    begin_lock.lock();
                    Iterator const failed_lock = try_lock_range_impl(next, end);
                    if (failed_lock == end)
                    {
                        begin_lock.release();
                        return;
                    }
                    start_with_begin = false;
                    next = failed_lock;
                }
                else
                {
                    detail::range_lock_guard<Iterator> guard(next, end);
                    if (begin_lock.try_lock())
                    {
                        Iterator const failed_lock = try_lock_range_impl(second, next);
                        if (failed_lock == next)
                        {
                            begin_lock.release();
                            guard.release();
                            return;
                        }
                        start_with_begin = false;
                        next = failed_lock;
                    }
                    else
                    {
                        start_with_begin = true;
                        next = second;
                    }
                }
            }
        }
    }

    template <typename Iterator>
    AZ_FORCE_INLINE Iterator try_lock_range(Iterator begin, Iterator end)
    {
        return detail::try_lock_range_impl(begin, end);
    }
    
    template <typename Iterator>
    AZ_FORCE_INLINE void lock_range(Iterator begin, Iterator end)
    {
        return detail::lock_range_impl(begin, end);
    }
}