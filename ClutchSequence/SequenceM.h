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

		template<typename constructorPredicate>
		requires std::predicate<constructorPredicate, pointer, size_type>
		pointer allocConstruct(constructorPredicate constructor, size_type size) {
			pointer destination = alloc(size);
			pointer initialized = destination;
			try {
				initialized = constructor(destination, size);
			}
			catch (...) {
				destroy(destination, initialized);
				free(destination);
				throw;
			}
			return destination;
		}

		void constructAdditional(pointer source, size_type count)
		requires std::default_initializable<value_type>
		{
			pointer initalized = source;
			try {
				initalized = std::uninitialized_value_construct_n(source, count);
			}
			catch (...) {
				destroy(source, initalized);
				throw;
			}
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
			if (count > ZERO__SM) {
				construct(allocator.allocConstruct([](pointer dest, size_type n) {
						return std::uninitialized_value_construct_n(dest, n);
						},
					count), count);
			}
		}
		SequenceM(size_type count, const_reference value) {
			if (count > ZERO__SM) {
				construct(allocator.allocConstruct([value&](pointer dest, size_type n) {
					return std::uninitialized_fill_n(dest, n, value);
					},
					count), count);
			}
		}
		SequenceM(std::initializer_list<value_type> init) {
			size_type size = init.size();
			if (size > ZERO__SM) {
				construct(allocator.allocConstruct([init](pointer dest, size_type n) {
					return std::uninitialized_copy(init.begin(), init.end(), dest);
					},
					size), size);
			}
		}
		SequenceM(const SequenceM<value_type>& rhs) {
			size_type size = rhs.size();
			if (size > ZERO__SM) {
				construct(allocator.allocConstruct([&rhs](pointer dest, size_type n) {
					return uninitialized_copy(rhs.begin(), rhs.end(), dest);
					},
					size), size);
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
		
		//ADD/REMOVE
		template <typename U>
		void element_assign(U&& value)
		requires std::convertible_to<U, value_type> && std::constructible_from<value_type, U&&> {
			if (mTotalSize == mCapacity)
				reallocate(pBegin(), pRealEnd(), growthFactor(mCapacity));
			
			//since old objects are preserved, need to handle either move assignment or move construction
			pointer slot = pValidEnd();
			if (has_reserve()) {
				*slot = std::forward<U>(value);
			}
			else {// validEnd == realEnd
				std::construct_at(slot, std::forward<U>(value));
				++mTotalSize;
			}

			++mValidSize;
		}
		template<typename... Args>
		void element_create(Args&&... args)
		requires std::constructible_from<value_type, Args&&...> {
			if (mTotalSize == mCapacity)
				reallocate(pBegin(), pRealEnd(), growthFactor(mCapacity));
			pointer slot = pValidEnd();

			if (has_reserve()) {
				*slot = value_type(std::forward<Args>(args)...);
			}
			else {//validEnd == realEnd
				std::construct_at(slot, std::forward<Args>(args)...);
				++mTotalSize;
			}

			++mValidSize;
		}
		void element_disable_back()noexcept {
			if (mValidSize > ZERO__SM) {
				--mValidSize;
			}
		}
		iterator element_disable(iterator pos) {
			pointer target = pos.base();
			pointer begin = pBegin();
			pointer end = pValidEnd();

			assert(target >= begin && target <= end);
			if (target == end) return end;

			std::move(target + 1, end, target);
			--mValidSize;

			return (target == pValidEnd()) ? pValidEnd() : target;
		}
		iterator element_disable_range(iterator first, iterator last) {
			pointer target_begin = first.base();
			pointer target_end = last.base();
			pointer array_begin = pBegin();
			pointer array_end = pValidEnd();

			assert(target_begin >= array_begin && target_begin <= array_end && target_end >= array_begin && target_end <= array_end && target_end >= target_begin);
			if (target_begin == target_end || target_begin == array_end) return array_end;


			if (target_end != array_end) {
				std::move(target_end, array_end, target_begin);
			}
			mValidSize -= (target_end - target_begin);

			return (target_begin == pValidEnd()) ? pValidEnd() : target_begin;
		}



		//RESERVE/CAPACITY
		void set_capacity(size_type new_cap) {
			if (new_cap > mCapacity) {
				reallocate(pBegin(), pRealEnd(), new_cap);//when reserving, implies need to copy over everything
			}
		}
		void set_reserve_size(size_type reserve_size) {
			size_type current_reserve = mTotalSize - mValidSize;

			if (reserve_size < current_reserve) return;

			size_type difference_count = reserve_size - current_reserve;

			if (mTotalSize + difference_count > mCapacity)
				reallocate(pBegin(), pRealEnd(), growthFactor(mTotalSize + difference_count));

			allocator.constructAdditional(pRealEnd(), difference_count);
			mTotalSize += difference_count;
		}
		void shrink_to_fit() {
			if (mCapacity > mValidSize) {
				reallocate(pBegin(), pValidEnd(), mValidSize);//when shrinking, implies need to shrink to only valid count
				mTotalSize = mValidSize;
			}
		}
		void shrink_to(size_type shrink_to)noexcept {
			if (shrink_to >= mTotalSize) return;
			//when resizing down, it is allowed to cut off buffered (off the end elems),
			//but logically is not allowed to cut off past that (shrinking, not growing)

			allocator.destroy(pBegin() + shrink_to, pRealEnd());
			mTotalSize = shrink_to;
			mValidSize = (shrink_to < mValidSize) ? shrink_to : mValidSize;
		}
		/*
		* PROBABLY DOESN'T ACTUALLY MAKE SENSE TO HAVE, SUBEJCT TO FUTURE CHANGE IF AT ALL
		void resize_grow(size_type grow_to, value_type value) {
			if (grow_to <= mValidSize) return;
			//when growing, it implies we want to grow in the valid range direction

			//taking an argument by value type enables rvalue function calling ("my string"),
			//but is a heads up if a named type is inserted
			
			//buffer zone values are INITALIZED, meaning can't just plug them in, at least not atm in this API
			//should use assignment operator on them

			//otherwise use uninitalized fill_n
		}
		*/

		//UTILITY
		constexpr bool empty_valid()const noexcept              { return mValidSize == ZERO__SM; }
		constexpr bool empty_total()const noexcept              { return mTotalSize == ZERO__SM; }
		constexpr bool is_sequence_allocated() const noexcept   { return mArray != nullptr; }
															   
		constexpr size_type size()const noexcept                { return mValidSize; }
		constexpr size_type size_total()const noexcept          { return mTotalSize; }
		constexpr size_type capacity()const noexcept            { return mCapacity; }
		constexpr bool      has_reserve()const noexcept         { return (mTotalSize - mValidSize) > ZERO__SM; }
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
		constexpr pointer pValidEnd()            { return mArray + mValidSize; }
		constexpr pointer pRealEnd()             { return mArray + mTotalSize; }

		void reallocate(pointer from, pointer to, size_type newSize) {
			//when reallocating, the range is not always known, thus iterate from -> to, but cleaning up old is always known
			pointer temp = allocator.allocConstruct([from, to](pointer dest, size_type n) {
				return std::uninitialized_move(from, to, dest);
				},
				newSize
			);
			allocator.destroy(pBegin(), pRealEnd());
			allocator.free(data());

			mArray = temp;
			mCapacity = newSize;
			//size does not change on reallocation
		}
		constexpr void construct(pointer data, size_type size) noexcept {
			mArray     = data;
			mValidSize = size;
			mTotalSize = size;
			mCapacity  = size;
		}
		constexpr size_type growthFactor(size_type seed)const { return seed + (seed / mGrowthResistor) + 1; }
	private:
		//main variables
		pointer     mArray          = nullptr;
		size_type   mValidSize      = 0;
		size_type   mTotalSize      = 0;
		size_type   mCapacity       = 0;
		float       mGrowthResistor = GROWTH_INTERMEDIATE;
		SMAllocator<value_type> allocator;
		//bs
		/*TODO: add setters later*/
		static constexpr size_type ZERO__SM = 0;
		static constexpr float GROWTH_LIGHT = 3.0f;
		static constexpr float GROWTH_INTERMEDIATE = 2.0f;
		static constexpr float GROWTH_AGGRESSIVE = 1.0f;
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