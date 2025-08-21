#pragma once

#	include <ppl.h>
#	include <concurrent_unordered_map.h>
#	include <concurrent_vector.h>

#include <atomic>

// Atomic types
using xr_atomic_u8  = std::atomic_uint8_t;
using xr_atomic_u32  = std::atomic_uint32_t;
using xr_atomic_s32  = std::atomic_int;
using xr_atomic_bool = std::atomic_bool;
using xr_atomic_float = std::atomic<float>;

// Tasks Redefinition

using xr_task_group = concurrency::task_group;

template <typename T, typename U>
using xr_concurrent_unordered_map = concurrency::concurrent_unordered_map<T, U>;

template <typename T>
using xr_concurrent_vector = concurrency::concurrent_vector<T>;

template<typename BlockRangeType, typename Body>
inline void xr_parallel_for(BlockRangeType Begin, BlockRangeType End, Body Functor)
{
	concurrency::parallel_for(Begin, End, Functor);
}

inline const size_t xr_max_concurrency()
{
	return Concurrency::CurrentScheduler::Get()->GetNumberOfVirtualProcessors();
}

template<typename BlockRangeType, typename Body>
inline void xr_parallel_for(BlockRangeType Begin, BlockRangeType End, BlockRangeType Grain, Body Functor)
{
	concurrency::parallel_for(Begin, End, Grain, Functor);
}

template<typename Index, typename Body>
inline void xr_parallel_foreach(Index Begin, Index End, Body Functor)
{
	concurrency::parallel_for_each(Begin, End, Functor);
}
