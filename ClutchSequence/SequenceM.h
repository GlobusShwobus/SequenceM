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
	class SMAllocator {

		using value_type      = T;
		using pointer         = T*;
		using const_pointer   = const T*;
		using reference       = T&;
		using const_reference = const T&;
		using size_type       = std::size_t;

	public:

		pointer alloc(size_type count) {
			return static_cast<pointer>(::operator new(count * sizeof(value_type)));
		}
		void free(pointer mem) {
			::operator delete(mem);
		}
		constexpr void destroy(pointer begin, pointer end)
			requires std::destructible<value_type> {
			std::destroy(begin, end);
		}
		pointer reallocate(pointer begin, pointer end, size_type newSize)
			requires std::is_nothrow_move_constructible_v<value_type>{
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
		pointer copyConstruct(const_pointer begin, const_pointer end, size_type newSize)
			requires std::copyable<value_type> {
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
		pointer defaultConstructN(size_type newSize)
			requires std::default_initializable<value_type> {
			pointer destination = alloc(newSize);
			pointer initialized = destination;
			try {
				initialized = std::uninitialized_default_construct_n(destination, newSize);
			}
			catch (...) {
				destroy(destination, initialized);
				free(destination);
			}
			return destination;
		}
		pointer fillConstructN(size_type newSize, const_reference value)
			requires std::copyable<value_type> {
			pointer destination = alloc(newSize);
			pointer initialized = destination;
			try {
				initialized = std::uninitialized_fill_n(destination, newSize, value);
			}
			catch (...) {
				destroy(destination, initialized);
				free(destination);
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

		using iterator        = Iterator;
		using const_iterator  = Const_Iterator;
	public:
		//CONSTRUCTORS
		constexpr SequenceM()noexcept = default;
		SequenceM(size_type count) {
			if (count > 0) {
				construct(allocator.defaultConstructN(count),count);
			}
		}
		SequenceM(size_type count, const_reference value) {
			if (count > 0) {//if we pass a reference object then how tf can it be 0? idk but the check is fine
				construct(allocator.fillConstructN(count, value), count);
			}
		}
		SequenceM(std::initializer_list<value_type> init) {
			size_type size = init.size();
			if (size > 0) {
				construct(
					allocator.copyConstruct(init.begin(), init.end(), size),
					size
				);
			}
		}
		SequenceM(const SequenceM<value_type>& rhs) {
			size_type size = rhs.size();
			if (size > 0) {
				construct(
					allocator.copyConstruct(rhs.cpBegin(), rhs.cpEnd(), size),
					size
				);
			}
		}
		constexpr SequenceM(SequenceM<value_type>&& rhs)noexcept {
			mArray     = std::exchange(rhs.mArray, nullptr);
			mValidSize = std::exchange(rhs.mValidSize, 0);
			mTotalSize = std::exchange(rhs.mTotalSize, 0);
			mCapacity  = std::exchange(rhs.mCapacity, 0);
		}
		SequenceM& operator=(SequenceM<value_type> rhs)noexcept {
			rhs.swap(*this);
			return *this;
		}
		SequenceM& operator=(std::initializer_list<value_type> ilist) {
			SequenceM<value_type> temp = ilist;
			temp.swap(*this);
			return *this;
		}
		~SequenceM()noexcept {
			if (mTotalSize > 0) {
				allocator.destroy(pBegin(), pRealEnd());
				mTotalSize = 0;
				mValidSize = 0;
			}
			if (mCapacity > 0) {
				allocator.free(data());
				mCapacity = 0;
				mArray = nullptr;
			}
		}

		//GETTERS
		constexpr bool empty_valid()const noexcept               { return mValidSize == ZERO__SM; }
		constexpr bool empty_total()const noexcept               { return mTotalSize == ZERO__SM; }
		constexpr bool is_sequence_allocated() const noexcept    { return mArray != nullptr; }

		constexpr size_type size()const noexcept                 { return mValidSize; }
		constexpr size_type size_total()const noexcept           { return mTotalSize; }
		constexpr size_type capacity()const noexcept             { return mCapacity; }

		constexpr iterator       begin()                         { return  mArray ; }
		constexpr iterator       end()                           { return  mArray + mValidSize; }
		constexpr const_iterator begin()const                    { return  mArray; }
		constexpr const_iterator end()const                      { return  mArray + mValidSize; }
		constexpr const_iterator cbegin()const                   { return  mArray; }
		constexpr const_iterator cend()const                     { return  mArray + mValidSize; }

		/* CHANGE THEM TO TAKE FUNCTIONS size() later... */
		constexpr reference       front() {
			assert(mValidSize > ZERO__SM);
			return mArray[ZERO__SM];
		}
		constexpr const_reference front()const {
			assert(mValidSize > ZERO__SM);
			return mArray[ZERO__SM];
		}
		constexpr reference       back() {
			assert(mValidSize > ZERO__SM);
			return mArray[mValidSize - 1];
		}
		constexpr const_reference back()const {
			assert(mValidSize > ZERO__SM);
			return mArray[mValidSize - 1];
		}
		constexpr reference       operator[](size_type index) {
			assert(index < mValidSize);
			return mArray[index];
		}
		constexpr const_reference operator[](size_type index)const {
			assert(index < mValidSize);
			return mArray[index];
		}
		constexpr reference       at(size_type pos) {
			if (pos >= mValidSize)
				throw std::out_of_range("position out of range");
			return mArray[pos];
		}
		constexpr const_reference at(size_type pos)const {
			if (pos >= mValidSize)
				throw std::out_of_range("position out of range");
			return mArray[pos];
		}
		
		//UTILITY
		constexpr void swap(SequenceM<value_type>& rhs)noexcept {
			pointer tempA = mArray;
			mArray = rhs.mArray;
			rhs.mArray = tempA;

			size_type tempB = mValidSize;
			mValidSize = rhs.mValidSize;
			rhs.mValidSize = tempB;

			size_type tempC = mTotalSize;
			mTotalSize = rhs.mTotalSize;
			rhs.mTotalSize = tempC;

			size_type tempD = mCapacity;
			mCapacity = rhs.mCapacity;
			rhs.mCapacity = tempD;
		}

	private:
		constexpr pointer data()                 { return mArray; }
		constexpr pointer pBegin()               { return mArray; }
		constexpr pointer pEnd()                 { return mArray + mValidSize; }
		constexpr pointer pRealEnd()             { return mArray + mTotalSize; }
		constexpr const_pointer cpBegin()const   { return mArray; }
		constexpr const_pointer cpEnd()const     { return mArray + mValidSize; }
		constexpr const_pointer cpRealEnd()const { return mArray + mTotalSize; }

		void grow(size_type newSize) {
			pointer temp = allocator.reallocate(pBegin(), pRealEnd(), newSize);
			allocator.destroy(pBegin(), pRealEnd());
			allocator.free(data());

			mArray = temp;
			mCapacity = newSize;
			//size does not change on reallocation
		}
		void construct(pointer data, size_type size) {
			mArray     = data;
			mValidSize = size;
			mTotalSize = size;
			mCapacity  = size;
		}

	private:
		//main variables
		pointer     mArray         = nullptr;
		size_type   mValidSize     = 0;
		size_type   mTotalSize     = 0;
		size_type   mCapacity      = 0;
		SMAllocator<value_type> allocator;
		//bs
		static constexpr size_type ZERO__SM = 0;
	};

	//iterators
	template<typename T>
	requires fully_defined_T<T>
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
	requires fully_defined_T<T>
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