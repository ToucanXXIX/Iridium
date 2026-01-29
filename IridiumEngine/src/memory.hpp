#include <cstdint>
#include <limits>

namespace Iridium {
	namespace MemoryExperimental {
		using id_t = uint16_t;

		template<typename T>
		concept reference_count_table_c = requires {
			T::init();
			T::cleanup();
			T::claimId();
			T::releaseId(id_t());

			T::increment(id_t());
			T::decrement(id_t());
			T::check(id_t());
			
			T::incrementWeak(id_t());
			T::decrementWeak(id_t());
			T::checkWeak(id_t());
		};

		namespace utils {
			template<reference_count_table_c T>
			inline id_t claimAndIncrement() {
				id_t id = T::claimId();
				T::increment(id);
				return id;
			}

			template<typename T>
			inline uintptr_t combinePtrId(id_t id, T* ptr) {
				uintptr_t combined = reinterpret_cast<uintptr_t>(ptr);
				combined |= uintptr_t(id) << 48;
				return combined;
			}
	}

		struct reftable_type {
			enum {
				MAX_ID = std::numeric_limits<id_t>::max()
			};

			union node {
				node* next = nullptr;
				struct {
					uint32_t strong;
					uint32_t weak;
				} references;
			};
		
			inline static node* refTable = nullptr;		
			inline static node* nextFree = nullptr;

			reftable_type() = delete;
			reftable_type(reftable_type&&) = delete;

			static void init();
			static void cleanup();

			[[nodiscard]] static id_t claimId();
			static void releaseId(id_t id);

			static uint32_t increment(id_t id);
			static uint32_t decrement(id_t id);
			static uint32_t check(id_t id);
			static uint32_t incrementWeak(id_t id);
			static uint32_t decrementWeak(id_t id);
			static uint32_t checkWeak(id_t id);
		};

		template<typename T, reference_count_table_c RefcountT>
		class shared_ptr_impl {
			template <typename f_T, reference_count_table_c f_refcount_t> friend class weak_ptr_impl;
		private:
			enum : uintptr_t {
				ID_MASK  = 0xffff000000000000,
				PTR_MASK = 0x0000ffffffffffff
			};

			uintptr_t m_ptr;

			inline id_t getId() {
				return (m_ptr & ID_MASK) >> 48;
			}

			inline T* getPtr() {
				return reinterpret_cast<T*>(m_ptr & PTR_MASK);
			}

			void onDecrement() {
				id_t id = getId();
				uint32_t strongRefs = RefcountT::decrement(id);
				if(strongRefs != 0)
					return;

				uint32_t weakRefs = RefcountT::checkWeak(id);
				if(weakRefs == 0)			
					RefcountT::releaseId(id);

				::delete getPtr();
			}
		public:
			shared_ptr_impl()
			:m_ptr(0) {};

			shared_ptr_impl(shared_ptr_impl &other)
			: m_ptr(other.m_ptr) {
				RefcountT::increment(getId());
        	}

        	shared_ptr_impl(shared_ptr_impl&& other)
				:m_ptr(other.m_ptr) {
			other.m_ptr = 0;
			}

			shared_ptr_impl(std::nullptr_t)
				:m_ptr(0) {}

			shared_ptr_impl(T* ptr) {
				if(ptr == nullptr)
					return;

				uintptr_t id = utils::claimAndIncrement<RefcountT>();
				m_ptr = utils::combinePtrId(id, ptr);
			}

			~shared_ptr_impl() {
				if(!getPtr())
					return;

				onDecrement();
			}

			T* get() const {
				return getPtr();
			}

			T* get() {
				return getPtr();
			}

			T& operator*() const {
				return *getPtr();
			}

			T& operator*() {
				return *getPtr();
			}

			T* operator->() const {
				return getPtr();
			}

			T* operator->() {
				return getPtr();
			}

			shared_ptr_impl& operator=(const shared_ptr_impl& other) {
				if(getPtr())
					onDecrement();
				
				m_ptr = other.m_ptr;
				RefcountT::increment(getId());

				return *this;
			}

			shared_ptr_impl& operator=(shared_ptr_impl& other) {
				if(getPtr())
					onDecrement();

				m_ptr = other.m_ptr;
				other.m_ptr = 0;
				return *this;
			}

			operator bool() const {
				return getPtr();
			}

			operator bool() {
				return getPtr();
			}
		};

		template<typename Type>
		using shared_ptr = shared_ptr_impl<Type, reftable_type>;
	}
}