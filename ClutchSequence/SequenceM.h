#pragma once
/*
BASIC DRAFT

have a larger contiguous array of T elements on the heap.
same as base with size refering to the valid elements.
iterators also refer to one off the end of valid and begin. 
the difference is instead of immediately destroying object, the "undesireble" objects are moved past the end point,
but obviously stay within capacity. 
there the elements would stay dormant.
if a new element is created, if would first check if there are valid elements in the "junk section"
and if yes it would simply move into them. saving creation deletion cycles.
there would also be extra utility public API to manually clean up the junk pile,
maybe even extra functionality like a car and have manual/automatic switches where it would auto clean up a large junk tail 
but not all of it (leave the clutch amount so to speak)

CAN NOT BE CONST T
MUST BE MOVABLE
PUSH BACK/INSERT IDEALLY PREFER NON CONST T& TO SQUEEZE OUT MOVE
USE UNIQUE_PTR FOR ARRAY INSTEAD OF RAW ARRAY (UNFORTUNATELY I DON'T THINK IT'S QUITE SO POSSIBLE, or at least fucky as fuck)
*/

#include <assert.h>
#include <stdexcept>
#include <memory>
#include <concepts>

namespace lmnop {

	template <typename T>
	concept fully_defined_T = std::default_initializable<T> &&
		                      std::copyable<T> &&
		                      std::is_nothrow_move_constructible_v<T> &&
		                      std::is_nothrow_move_assignable_v<T> &&
		                      std::destructible<T> && 
		                      !std::is_const_v<T>;

	template<typename T>
	requires fully_defined_T<T>
	class SMAllocator {

		using value_type = T;
		using pointer = T*;
		using size_type = std::size_t;

	public:

		pointer alloc(size_type count) {
			return static_cast<pointer>(::operator new(count * sizeof(value_type)));
		}
		constexpr void destroy(pointer begin, pointer end) {
			std::destroy(begin, end);
		}
		void free(pointer mem) {
			::operator delete(mem);
		}
		pointer reallocate(pointer begin, pointer end, size_type newSize) {
			pointer destination = alloc(newSize);
			pointer initialized = destination;
			try {
				initialized = std::uninitialized_move(begin, end, destination);
			}
			catch (...) {
				destroy(destination, initialized);
				free(destination);
				throw;
			}

			return destination;
		}
		pointer copyConstruct(pointer begin, pointer end, size_type newSize) {
			pointer destination = alloc(newSize);
			pointer initialized = destination;
			try {
				initialized = std::uninitialized_copy(begin, end, destination);
			}
			catch (...) {
				destroy(destination, initialized);
				free(destination);
				throw;
			}
			return destination;
		}
	};

	template<typename T>
    requires fully_defined_T<T>
	class SequenceM {
	private:
		class Iterator;
		class Const_Iterator;
	public:
		using type            = SequenceM<T>;
		using value_type      = T;
		using pointer         = T*;
		using const_pointer   = const T*;
		using reference       = T&;
		using const_reference = const T&;
		using size_type       = std::size_t;
		using difference_type = std::ptrdiff_t;

		using iterator = Iterator;
		using const_iterator = Const_Iterator;
	public:
		//CONSTRUCTORS
		constexpr SequenceM()noexcept = default;
		SequenceM(std::initializer_list<value_type> init) requires std::copyable<value_type> {
			copyConstruct(init.begin(), init.end(), init.size());
		}
		SequenceM(const SequenceM<value_type>& rhs) requires std::copyable<value_type> {
			copyConstruct(rhs.raw_begin(), rhs.raw_end(), rhs.size());
		}


		//###################################################################
		//GETTERS
		constexpr bool is_empty()const noexcept               { return mPublicSize == ZERO__SM; }
		constexpr bool is_empty_buffer() const noexcept       { return mBufferSize == ZERO__SM; }
		constexpr bool is_sequence_allocated() const noexcept { return mArray != nullptr; }

		constexpr size_type size()const noexcept              { return mPublicSize; }
		constexpr size_type size_buffer() const noexcept      { return mBufferSize; }
		constexpr size_type size_total()const noexcept        { return mTotalSize; }

		constexpr iterator       begin()                      { return  mArray ; }
		constexpr iterator       end()                        { return  mArray + mPublicSize; }
		constexpr const_iterator begin()const                 { return  mArray; }
		constexpr const_iterator end()const                   { return  mArray + mPublicSize; }
		constexpr const_iterator cbegin()const                { return  mArray; }
		constexpr const_iterator cend()const                  { return  mArray + mPublicSize; }

		constexpr reference       front() {
			assert(mPublicSize > ZERO__SM);
			return mArray[ZERO__SM];
		}
		constexpr const_reference front()const {
			assert(mPublicSize > ZERO__SM);
			return mArray[ZERO__SM];
		}
		constexpr reference       back() {
			assert(mPublicSize > ZERO__SM);
			return mArray[mPublicSize - 1];
		}
		constexpr const_reference back()const {
			assert(mPublicSize > ZERO__SM);
			return mArray[mPublicSize - 1];
		}
		constexpr reference       operator[](size_type index) {
			assert(index < mPublicSize);
			return mArray[index];
		}
		constexpr const_reference operator[](size_type index)const {
			assert(index < mPublicSize);
			return mArray[index];
		}
		constexpr reference       at(size_type pos) {
			if (pos >= mPublicSize)
				throw std::out_of_range("position out of range");
			return mArray[pos];
		}
		constexpr const_reference at(size_type pos)const {
			if (pos >= mPublicSize)
				throw std::out_of_range("position out of range");
			return mArray[pos];
		}
		//###################################################################

		constexpr swap(SequenceM<value_type>& rhs)noexcept {
			std::swap(mArray,      rhs.mArray);
			std::swap(mPublicSize, rhs.mPublicSize);
			std::swap(mTotalSize,  rhs.mTotalSize);
			std::swap(mBufferSize, rhs.mBufferSize);
			std::swap(mCapacity,   rhs.mCapacity);
			std::swap(mAutoSize,   rhs.mAutoSize);
			std::swap(isAuto,      rhs.isAuto);
		}

	private:
		constexpr pointer raw_begin()    { return mArray; }
		constexpr pointer raw_end()      { return mArray + mPublicSize; }
		constexpr pointer raw_real_end() { return mArray + mTotalSize; }

		void grow(size_type newSize) {
			pointer temp = allocator.reallocate(raw_begin(), raw_real_end(), newSize);
			allocator.destroy(raw_begin(), raw_real_end());
			allocator.free(raw_begin());

			mArray = temp;
			cap = newSize;
			//size does not change on reallocation
		}
		void copyConstruct(pointer begin, pointer end, size_type size) {
			if (size > 0) {
				pointer temp = allocator.copyConstruct(begin, end, size);

				mArray = temp;
				mPublicSize = size;
				mTotalSize = size;
				mCapacity = size;
				//other vairables related to class behavior are set manually...
			}
		}

	private:
		//main variables
		pointer     mArray = nullptr;
		size_type   mPublicSize    = 0;
		size_type   mTotalSize     = 0;
		size_type   mCapacity      = 0;
		SMAllocator<value_type> allocator;
		//the clutch variables
		size_type   mBufferSize    = 0;
		size_type   mAutoSize      = 0;
		bool isAuto = false;
		//bs
		static constexpr size_type ZERO__SM = 0;
	};


	template<typename T>
	class SequenceM<T>::Iterator {
	public:
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using self_type = Iterator;

		constexpr reference operator*()noexcept { return *ptr; }
		constexpr pointer operator->()noexcept { return ptr; }
		constexpr reference operator[](difference_type n)noexcept { return ptr[n]; }
		constexpr const reference operator*() const noexcept { return *ptr; }
		constexpr const pointer operator->() const noexcept { return ptr; }
		constexpr const reference operator[](difference_type n) const noexcept { return ptr[n]; }

		constexpr self_type& operator++()noexcept { ++ptr; return *this; }
		constexpr self_type operator++(int)noexcept { self_type temp = *this; ++ptr; return temp; }
		constexpr self_type& operator--()noexcept { --ptr; return *this; }
		constexpr self_type operator--(int)noexcept { self_type temp = *this; --ptr; return temp; }

		constexpr self_type& operator+=(difference_type n)noexcept { ptr += n; return *this; }
		constexpr self_type& operator-=(difference_type n)noexcept { ptr -= n; return *this; }
		constexpr self_type operator+(difference_type n)const noexcept { return self_type(ptr + n); }
		constexpr self_type operator-(difference_type n)const noexcept { return self_type(ptr - n); }
		constexpr difference_type operator-(const self_type& rhs)const noexcept { return ptr - rhs.ptr; }

		constexpr bool operator==(const self_type& rhs)const noexcept { return ptr == rhs.ptr; }
		constexpr std::strong_ordering operator<=>(const self_type& rhs)const noexcept { return ptr <=> rhs.ptr; }

		constexpr Iterator(pointer p) :ptr(p) { assert(p != nullptr && "Iterator constructed from nullptr!"); }
		constexpr pointer base()const noexcept { return ptr; }
	private:
		pointer ptr = nullptr;
	};

	template<typename T>
	class SequenceM<T>::Const_Iterator {
	public:
		using value_type = const T;
		using pointer = const T*;
		using reference = const T&;
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using self_type = Const_Iterator;

		constexpr reference operator*()const noexcept { return *ptr; }
		constexpr pointer operator->()const noexcept { return ptr; }
		constexpr reference operator[](difference_type n)const noexcept { return ptr[n]; }

		constexpr self_type& operator++() noexcept { ++ptr; return *this; }
		constexpr self_type operator++(int) noexcept { self_type temp = *this; ++ptr; return temp; }
		constexpr self_type& operator--() noexcept { --ptr; return *this; }
		constexpr self_type operator--(int) noexcept { self_type temp = *this; --ptr; return temp; }

		constexpr self_type& operator+=(difference_type n) noexcept { ptr += n; return *this; }
		constexpr self_type& operator-=(difference_type n) noexcept { ptr -= n; return *this; }
		constexpr self_type operator+(difference_type n)const noexcept { return self_type(ptr + n); }
		constexpr self_type operator-(difference_type n)const noexcept { return self_type(ptr - n); }
		constexpr difference_type operator-(const self_type& rhs)const noexcept { return ptr - rhs.ptr; }

		constexpr bool operator==(const self_type& rhs)const noexcept { return ptr == rhs.ptr; }
		constexpr std::strong_ordering operator<=>(const self_type& rhs)const noexcept { return ptr <=> rhs.ptr; }

		constexpr Const_Iterator(pointer p) :ptr(p) { assert(ptr != nullptr && "Iterator constructed from nullptr!"); }
		constexpr Const_Iterator(const Iterator& rp) : ptr(rp.base()) { assert(ptr != nullptr && "Iterator constructed from nullptr!"); }
		constexpr pointer base()const noexcept { return ptr; }
	private:
		pointer ptr = nullptr;
	};
}