#pragma once
namespace lz
{
	// UniqueHandle: A template class for managing ownership of Vulkan resources
	// - Provides RAII (Resource Acquisition Is Initialization) semantics for Vulkan handles
	// - Automatically cleans up resources when handle goes out of scope
	// - Implements move semantics but prevents copying
	// Parameters:
	// - HandleInfo: Type containing handle information and reset logic
	// - Factory: Type that creates instances of UniqueHandle
	template <typename HandleInfo, typename Factory>
	struct UniqueHandle
	{
		// Default constructor: Creates a handle that isn't attached to any resource
		UniqueHandle()
		{
			isAttached = false;
		}

		// Deleted copy assignment operator to prevent copying
		UniqueHandle<HandleInfo, Factory>& operator =(UniqueHandle<HandleInfo, Factory>& other) = delete;

		// Move assignment operator: Transfers ownership from one handle to another
		UniqueHandle<HandleInfo, Factory>& operator =(UniqueHandle<HandleInfo, Factory>&& other)
		{
			if (isAttached)
			{
				this->info.Reset();
			}
			this->isAttached = other.isAttached;
			other.isAttached = false;
			this->info = other.info;
			return *this;
		}

		// Move constructor: Transfers ownership from one handle to another
		UniqueHandle(UniqueHandle<HandleInfo, Factory>&& other)
		{
			isAttached = other.isAttached;
			other.isAttached = false;
			this->info = other.info;
		}

		// Destructor: Automatically cleans up the resource if handle is attached
		~UniqueHandle()
		{
			if (isAttached)
			{
				info.Reset();
			}
		}

		// Detach: Releases ownership of the resource without destroying it
		void Detach()
		{
			isAttached = false;
		}

		// Reset: Detaches and resets the handle information
		void Reset()
		{
			Detach();
			info.Reset();
		}

		// IsAttached: Returns whether the handle is attached to a resource
		bool IsAttached()
		{
			return isAttached;
		}

		// Get: Returns the underlying handle information
		const HandleInfo& Get() const
		{
			return info;
		}

		// Arrow operator: Provides access to handle info members
		const HandleInfo* operator->() const
		{
			return &info;
		}

	private:
		// Private constructor used by Factory to create attached handles
		// Parameters:
		// - info: Handle information
		// - isAttached: Whether the handle is attached to a resource
		UniqueHandle(const HandleInfo& info, bool isAttached = true) :
			info(info),
			isAttached(isAttached)
		{
		}

		friend Factory;
		bool isAttached; // Whether the handle is attached to a resource
		HandleInfo info; // The underlying handle information
	};
}
