#pragma once

#include <stdexcept>
#include <memory>
#include "badUtility.h"

namespace badEngine {
	template<typename T>
	class badAllocator {

		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		using size_type = std::size_t;

	public:

		pointer alloc(size_type count) {
			return static_cast<pointer>(::operator new(count * sizeof(value_type)));
		}
		void free(pointer mem) {
			::operator delete(mem);
		}
		constexpr void destroy(pointer begin, pointer end) requires std::destructible<value_type> {
			std::destroy(begin, end);
		}

		template<typename constructorPredicate>
			requires std::predicate<constructorPredicate, pointer, size_type>
		pointer alloc_and_construct(constructorPredicate constructor, size_type size) {
			pointer destination = alloc(size);
			pointer initialized = destination;

			try {
				initialized = constructor(destination, size);
			}
			catch (...) {
				destroy(destination, initialized);
				free(destination);
				throw std::runtime_error("POSSIBLE ERRORS: faulty predicate or T object constructors");
			}

			return destination;
		}

		void construct_additional(pointer source, size_type count)
			requires std::default_initializable<value_type>
		{
			pointer initalized = source;
			try {
				initalized = std::uninitialized_value_construct_n(source, count);
			}
			catch (...) {
				destroy(source, initalized);
				throw std::runtime_error("POSSIBLE ERRORS: T object constructor missing default constructor");
			}
		}
	};

	template<typename T>
		requires IS_RULE_OF_FIVE_CLASS_T<T>
	class SequenceM {
	private:
		class Iterator;
		class Const_Iterator;
	public:
		using type = SequenceM<T>;
		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		using iterator = Iterator;
		using const_iterator = Const_Iterator;
	public:
		//CONSTRUCTORS
		constexpr SequenceM()noexcept = default;
		SequenceM(size_type count) {
			if (count > EMPTY_GUARD) {
				construct(allocator.alloc_and_construct([](pointer dest, size_type n) {
					return std::uninitialized_value_construct_n(dest, n);
					},
					count), count);
			}
		}
		SequenceM(size_type count, const_reference value) {
			if (count > EMPTY_GUARD) {
				construct(allocator.alloc_and_construct([&value](pointer dest, size_type n) {
					return std::uninitialized_fill_n(dest, n, value);
					},
					count), count);
			}
		}
		SequenceM(std::initializer_list<value_type> init) {
			size_type size = init.size();
			if (size > EMPTY_GUARD) {
				construct(allocator.alloc_and_construct([init](pointer dest, size_type n) {
					return std::uninitialized_copy(init.begin(), init.end(), dest);
					},
					size), size);
			}
		}
		SequenceM(const SequenceM<value_type>& rhs) {
			size_type size = rhs.size_in_use();
			if (size > EMPTY_GUARD) {
				construct(allocator.alloc_and_construct([&rhs](pointer dest, size_type n) {
					return std::uninitialized_copy(rhs.begin(), rhs.end(), dest);
					},
					size), size);
			}
		}
		constexpr SequenceM(SequenceM<value_type>&& rhs)noexcept {
			mArray = std::exchange(rhs.mArray, nullptr);
			mValidSize = std::exchange(rhs.mValidSize, 0);
			mTotalSize = std::exchange(rhs.mTotalSize, 0);
			mCapacity = std::exchange(rhs.mCapacity, 0);
			mGrowthResistor = std::exchange(rhs.mGrowthResistor, 0);
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
			if (!empty_total()) {
				allocator.destroy(pBegin(), pRealEnd());
				mTotalSize = 0;
				mValidSize = 0;
			}
			if (!empty_capacity()) {
				allocator.free(data());
				mCapacity = 0;
				mArray = nullptr;
			}
		}
		constexpr void swap(SequenceM<value_type>& rhs)noexcept {
			std::swap(mArray, rhs.mArray);
			std::swap(mValidSize, rhs.mValidSize);
			std::swap(mTotalSize, rhs.mTotalSize);
			std::swap(mCapacity, rhs.mCapacity);
			std::swap(mGrowthResistor, rhs.mGrowthResistor);
		}

		//ITERATORS
		constexpr pointer        data() { return  mArray; }
		constexpr iterator       begin() { return  mArray; }
		constexpr iterator       end() { return  mArray + mValidSize; }
		constexpr const_iterator begin()const { return  mArray; }
		constexpr const_iterator end()const { return  mArray + mValidSize; }
		constexpr const_iterator cbegin()const { return  mArray; }
		constexpr const_iterator cend()const { return  mArray + mValidSize; }

		//ADD/REMOVE ELEMENTS
		template <typename U>
		void element_assign(U&& value)
			requires std::convertible_to<U, value_type>&& std::constructible_from<value_type, U&&> {
			if (mTotalSize == mCapacity)
				reallocate(pBegin(), pRealEnd(), growthFactor(mCapacity));

			//since old objects are preserved, need to handle either move assignment or move construction
			pointer slot = pValidEnd();
			if (!empty_reserve()) {
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

			if (!empty_reserve()) {
				*slot = value_type(std::forward<Args>(args)...);
			}
			else {//validEnd == realEnd
				std::construct_at(slot, std::forward<Args>(args)...);
				++mTotalSize;
			}

			++mValidSize;
		}
		constexpr void depricate_back()noexcept {
			if (!empty_in_use()) {
				--mValidSize;
			}
		}
		void depricate_ordered(iterator pos) {
			pointer target = pos.base();
			pointer begin = pBegin();
			pointer end = pValidEnd();

			if (target < begin || target >= end)
				throw std::out_of_range("position out of range");

			std::move(target + 1, end, target);//from, till, into
			--mValidSize;
		}
		void depricate_ordered(iterator first, iterator last) {
			pointer targetBegin = first.base();
			pointer targetEnd = last.base();
			pointer arrayBegin = pBegin();
			pointer arrayEnd = pValidEnd();

			if (targetBegin == targetEnd) return;
			if (targetBegin < arrayBegin || targetEnd > arrayEnd || targetEnd < targetBegin)
				throw std::out_of_range("position out of range or invalid ordering");
			//if target end is not arrays end then must move all elements because the range is somewhere in the middle
			//otherwise and in any case, reduce size by they range amount
			if (targetEnd != arrayEnd)
				std::move(targetEnd, arrayEnd, targetBegin);//from, till, into
			mValidSize -= (targetEnd - targetBegin);
		}
		void depricate_unordered(iterator pos) {
			pointer target = pos.base();
			pointer begin = pBegin();
			pointer end = pValidEnd();

			if (target < begin || target >= end)
				throw std::out_of_range("position out of range");

			--end;
			*target = std::move(*end);
			--mValidSize;
		}
		template <typename Condition>
		void depricate_unordered(Condition condition) requires std::predicate<Condition, const_reference> {

			if (empty_in_use()) return;

			pointer current = pBegin();
			pointer validLast = pValidEnd() - 1;
			size_type decreaseSize = 0;

			while (current <= validLast) {
				if (condition(*current)) {

					if (current != validLast)
						*current = std::move(*validLast);

					--validLast;
					++decreaseSize;
				}
				else {
					++current;
				}
			}
			mValidSize -= decreaseSize;
		}

		//CAPACITY/SIZE META MANIPULATION
		void set_capacity(size_type new_cap) {
			if (new_cap > mCapacity)
				reallocate(pBegin(), pRealEnd(), new_cap);//when reserving, implies need to copy over everything
		}
		void set_reserve_size(size_type reserveSize) {
			size_type currentReserve = mTotalSize - mValidSize;

			if (reserveSize <= currentReserve) return;

			size_type differenceCount = reserveSize - currentReserve;

			if (mTotalSize + differenceCount > mCapacity)
				reallocate(pBegin(), pRealEnd(), growthFactor(mTotalSize + differenceCount));//is this a lot potentially?

			allocator.construct_additional(pRealEnd(), differenceCount);
			mTotalSize += differenceCount;
		}
		void shrink_to_fit() {
			if (mCapacity > mValidSize) {
				reallocate(pBegin(), pValidEnd(), mValidSize);//will cut off and destroy any elements past valid end
				mTotalSize = mValidSize;
			}
		}
		void shrink_to(size_type shrinkTo)noexcept {
			if (shrinkTo >= mTotalSize) return;
			//will destroy elements from begin+shrinkTo to real end not valid end, shrink
			allocator.destroy(pBegin() + shrinkTo, pRealEnd());
			mTotalSize = shrinkTo;
			mValidSize = (shrinkTo < mValidSize) ? shrinkTo : mValidSize;
		}
		void clear() {
			if (!empty_total()) {
				allocator.destroy(pBegin(), pRealEnd());
				mTotalSize = mValidSize = 0;
			}
		}

		//INFO/SETTERS/GETTERS
		constexpr bool empty_in_use()const noexcept { return mValidSize == EMPTY_GUARD; }
		constexpr bool empty_total()const noexcept { return mTotalSize == EMPTY_GUARD; }
		constexpr bool empty_reserve()const noexcept { return (mTotalSize - mValidSize) == EMPTY_GUARD; }
		constexpr bool empty_capacity()const noexcept { return mCapacity == EMPTY_GUARD; }
		constexpr bool is_sequence_allocated() const noexcept { return mArray != nullptr; }

		constexpr size_type size_in_use()const noexcept { return mValidSize; }
		constexpr size_type size_total()const noexcept { return mTotalSize; }
		constexpr size_type size_reserve()const noexcept { return mTotalSize - mValidSize; }
		constexpr size_type size_capacity()const noexcept { return mCapacity; }

		constexpr void set_growth_resist_high()noexcept { mGrowthResistor = GROWTH_HIGH_RESIST; }
		constexpr void set_growth_resist_medium()noexcept { mGrowthResistor = GROWTH_MEDIUM_RESIST; }
		constexpr void set_growth_resist_low()noexcept { mGrowthResistor = GROWTH_LOW_RESIST; }

		constexpr reference       front() {
			empty_array_error();
			return mArray[0];
		}
		constexpr const_reference front()const {
			empty_array_error();
			return mArray[0];
		}
		constexpr reference       back() {
			empty_array_error();
			return mArray[mValidSize - 1];
		}
		constexpr const_reference back()const {
			empty_array_error();
			return mArray[mValidSize - 1];
		}
		constexpr reference       operator[](size_type index) {
			return mArray[index];
		}
		constexpr const_reference operator[](size_type index)const {
			return mArray[index];
		}
		constexpr reference       at(size_type index) {
			out_of_range_access(index);
			return mArray[index];
		}
		constexpr const_reference at(size_type index)const {
			out_of_range_access(index);
			return mArray[index];
		}

	private:
		//private access
		constexpr pointer pBegin() { return mArray; }
		constexpr pointer pValidEnd() { return mArray + mValidSize; }
		constexpr pointer pRealEnd() { return mArray + mTotalSize; }
		//the things that actually make it work
		void reallocate(pointer from, pointer to, size_type newSize) {
			//when reallocating, the range is not always known, thus iterate from -> to, but cleaning up old is always known
			pointer temp = allocator.alloc_and_construct([from, to](pointer dest, size_type n) {
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
			mArray = data;
			mValidSize = size;
			mTotalSize = size;
			mCapacity = size;
			mGrowthResistor = GROWTH_MEDIUM_RESIST;
		}
		//growth math
		constexpr size_type growthFactor(size_type seed)const { return size_type(seed + (seed / mGrowthResistor) + 1); }
		//error consolidation
		constexpr inline void empty_array_error()const {
			if (empty_in_use())
				throw std::out_of_range("position out of range");
			//assert(mValidSize > EMPTY_GUARD); if check is slow in testing, opt for assert instead
		}
		constexpr inline void out_of_range_access(size_type index)const {
			if (index >= mValidSize)
				throw std::out_of_range("position out of range");
			//assert(mValidSize > EMPTY_GUARD && index < mValidSize); if check is slow in testing, opt for assert instead
		}

	private:
		//member variables
		pointer                  mArray = nullptr;
		size_type                mValidSize = 0;
		size_type                mTotalSize = 0;
		size_type                mCapacity = 0;
		float                    mGrowthResistor = GROWTH_MEDIUM_RESIST;
		badAllocator<value_type> allocator;
		//universal variables
		static constexpr size_type EMPTY_GUARD = 0;
		static constexpr float     GROWTH_HIGH_RESIST = 4.0f;
		static constexpr float     GROWTH_MEDIUM_RESIST = 2.0f;
		static constexpr float     GROWTH_LOW_RESIST = 1.0f;
	};

	//iterators
	template<typename T>
		requires IS_RULE_OF_FIVE_CLASS_T<T>
	class SequenceM<T>::Iterator {
	public:
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using self_type = Iterator;

		constexpr reference operator*()noexcept {
			return *ptr;
		}
		constexpr pointer   operator->()noexcept {
			return ptr;
		}
		constexpr reference operator[](difference_type n)noexcept {
			return ptr[n];
		}

		constexpr const reference operator*() const noexcept {
			return *ptr;
		}
		constexpr const pointer   operator->() const noexcept {
			return ptr;
		}
		constexpr const reference operator[](difference_type n) const noexcept {
			return ptr[n];
		}

		constexpr self_type& operator++()noexcept {
			++ptr;
			return *this;
		}
		constexpr self_type  operator++(int)noexcept {
			self_type temp = *this;
			++ptr;
			return temp;
		}
		constexpr self_type& operator--()noexcept {
			--ptr;
			return *this;
		}
		constexpr self_type  operator--(int)noexcept {
			self_type temp = *this;
			--ptr;
			return temp;
		}

		constexpr self_type& operator+=(difference_type n)noexcept {
			ptr += n;
			return *this;
		}
		constexpr self_type& operator-=(difference_type n)noexcept {
			ptr -= n;
			return *this;
		}
		constexpr self_type  operator+(difference_type n)const noexcept {
			return ptr + n;
		}
		constexpr self_type  operator-(difference_type n)const noexcept {
			return ptr - n;
		}
		constexpr difference_type operator-(const self_type& rhs)const noexcept {
			return ptr - rhs.ptr;
		}

		constexpr bool operator==(const self_type& rhs)const noexcept {
			return ptr == rhs.ptr;
		}
		constexpr std::strong_ordering operator<=>(const self_type& rhs)const noexcept {
			return ptr <=> rhs.ptr;
		}

		constexpr Iterator() = default;
		constexpr Iterator(pointer p) :ptr(p) {}
		constexpr pointer base()const noexcept {
			return ptr;
		}
	private:
		pointer ptr = nullptr;
	};

	template<typename T>
		requires IS_RULE_OF_FIVE_CLASS_T<T>
	class SequenceM<T>::Const_Iterator {
	public:
		using value_type = const T;
		using pointer = const T*;
		using reference = const T&;
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using self_type = Const_Iterator;

		constexpr reference operator*()const noexcept {
			return *ptr;
		}
		constexpr pointer   operator->()const noexcept {
			return ptr;
		}
		constexpr reference operator[](difference_type n)const noexcept {
			return ptr[n];
		}

		constexpr self_type& operator++() noexcept {
			++ptr;
			return *this;
		}
		constexpr self_type  operator++(int) noexcept {
			self_type temp = *this;
			++ptr;
			return temp;
		}
		constexpr self_type& operator--() noexcept {
			--ptr;
			return *this;
		}
		constexpr self_type  operator--(int) noexcept {
			self_type temp = *this;
			--ptr;
			return temp;
		}

		constexpr self_type& operator+=(difference_type n) noexcept {
			ptr += n;
			return *this;
		}
		constexpr self_type& operator-=(difference_type n) noexcept {
			ptr -= n;
			return *this;
		}
		constexpr self_type  operator+(difference_type n)const noexcept {
			return ptr + n;
		}
		constexpr self_type  operator-(difference_type n)const noexcept {
			return ptr - n;
		}
		constexpr difference_type operator-(const self_type& rhs)const noexcept {
			return ptr - rhs.ptr;
		}

		constexpr bool operator==(const self_type& rhs)const noexcept {
			return ptr == rhs.ptr;
		}
		constexpr std::strong_ordering operator<=>(const self_type& rhs)const noexcept {
			return ptr <=> rhs.ptr;
		}

		constexpr Const_Iterator() = default;
		constexpr Const_Iterator(pointer p) :ptr(p) {}
		constexpr Const_Iterator(const Iterator& rp) : ptr(rp.base()) {}
		constexpr pointer base()const noexcept {
			return ptr;
		}
	private:
		pointer ptr = nullptr;
	};
}