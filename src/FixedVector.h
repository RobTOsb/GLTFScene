#pragma once

#include <new>

#include "ArenaAllocator.h"

namespace DSA
{
	template<typename T>
	class FixedVector
	{
	public:
		FixedVector() = default;

		FixedVector(Allocator::Arena &arena, size_t count) : capacity_(count), size_(count)
		{
			data_ = static_cast<T *>(arena.Alloc(sizeof(T), alignof(T), count));

			for (size_t i = 0; i < count; ++i)
				new (data_ + i) T();
		}

		FixedVector(Allocator::Arena &arena, T *data, size_t count) : capacity_(count), size_(count)
		{
			data_ = static_cast<T *>(arena.Alloc(sizeof(T), alignof(T), count));

			for (size_t i = 0; i < count; ++i)
				new (data_ + i) T(std::move(data[i]));
		}

		template<typename Factory>
		FixedVector(Allocator::Arena &arena, size_t count, Factory factory) : capacity_(count), size_(count)
		{
			data_ = static_cast<T *>(arena.Alloc(sizeof(T), alignof(T), count));

			for (size_t i = 0; i < count; ++i)
				new (data_ + i) T(factory(i));
		}

		FixedVector(FixedVector &&other) : data_(other.data_), capacity_(other.capacity_), size_(other.size_)
		{
			other.data_ = nullptr;
			other.capacity_ = 0;
			other.size_ = 0;
		}

		FixedVector &operator=(FixedVector &&other)
		{
			if (this != &other)
			{
				data_ = other.data_;
				capacity_ = other.capacity_;
				size_ = other.size_;

				other.data_ = nullptr;
				other.capacity_ = 0;
				other.size_ = 0;
			}
			return *this;
		}

		FixedVector(const FixedVector &) = delete;
		FixedVector &operator=(const FixedVector &) = delete;

		~FixedVector()
		{
			for (size_t i = 0; i < size_; ++i)
				data_[i].~T();
		}

		T *begin()
		{
			return data_;
		}
		const T *begin() const
		{
			return data_;
		}

		T *end()
		{
			return data_ + size_;
		}
		const T *end() const
		{
			return data_ + size_;
		}

		T *data()
		{
			return data_;
		}

		const T *data() const
		{
			return data_;
		}

		T &operator[](size_t index)
		{
			return data_[index];
		}

		const T &operator[](size_t index) const
		{
			return data_[index];
		}

		size_t size() const
		{
			return size_;
		}

		bool empty() const
		{
			return size_ == 0;
		}


	private:
		T *data_;
		size_t capacity_;
		size_t size_;
	};
} // namespace DSA
