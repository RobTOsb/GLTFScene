#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SCENE_ALLOC(capacity) malloc(capacity)
#define SCENE_FREE(ptr) free(ptr)

namespace Allocator
{
	class Arena
	{
	public:
		Arena(ptrdiff_t capacity)
		{
			block = static_cast<char *>(SCENE_ALLOC(capacity));
			begin = block;
			end = block ? block + capacity : nullptr;
		}

		~Arena()
		{
			SCENE_FREE(block);
		}

		Arena(const Arena &) = delete;
		Arena &operator=(const Arena &) = delete;

		void *Alloc(ptrdiff_t size, ptrdiff_t align, ptrdiff_t count)
		{
			ptrdiff_t padding = -(uintptr_t) begin & (align - 1);
			ptrdiff_t available = end - begin - padding;

			if (!block || available < 0 || count > available / size)
			{
				ptrdiff_t capacity = end - block;
				ptrdiff_t used = begin - block;
				fprintf(stderr,
						"Arena::Alloc: out of memory (requested %lld bytes, %lld/%lld used)\n",
						(long long) (count * size),
						(long long) used,
						(long long) capacity);
				abort();
			}

			void *p = begin + padding;
			begin += padding + count * size;
			return memset(p, 0, count * size);
		}

	private:
		char *block;
		char *begin;
		char *end;
	};
}; // namespace Allocator
