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
		is_attached_ = false;
	}

	// Deleted copy assignment operator to prevent copying
	UniqueHandle<HandleInfo, Factory> &operator=(UniqueHandle<HandleInfo, Factory> &other) = delete;

	// Move assignment operator: Transfers ownership from one handle to another
	UniqueHandle<HandleInfo, Factory> &operator=(UniqueHandle<HandleInfo, Factory> &&other) noexcept
	{
		if (is_attached_)
		{
			this->info_.reset();
		}
		this->is_attached_ = other.is_attached_;
		other.is_attached_ = false;
		this->info_        = other.info_;
		return *this;
	}

	// Move constructor: Transfers ownership from one handle to another
	UniqueHandle(UniqueHandle<HandleInfo, Factory> &&other) noexcept
	{
		is_attached_       = other.is_attached_;
		other.is_attached_ = false;
		this->info_        = other.info_;
	}

	// Destructor: Automatically cleans up the resource if handle is attached
	~UniqueHandle()
	{
		if (is_attached_)
		{
			info_.reset();
		}
	}

	// Detach: Releases ownership of the resource without destroying it
	void detach()
	{
		is_attached_ = false;
	}

	// Reset: Detaches and resets the handle information
	void reset()
	{
		detach();
		info_.reset();
	}

	// IsAttached: Returns whether the handle is attached to a resource
	bool is_attached() const
	{
		return is_attached_;
	}

	// Get: Returns the underlying handle information
	const HandleInfo &get() const
	{
		return info_;
	}

	// Arrow operator: Provides access to handle info members
	const HandleInfo *operator->() const
	{
		return &info_;
	}

  private:
	// Private constructor used by Factory to create attached handles
	// Parameters:
	// - info: Handle information
	// - isAttached: Whether the handle is attached to a resource
	UniqueHandle(const HandleInfo &info, const bool is_attached = true) :
	    info_(info),
	    is_attached_(is_attached)
	{
	}

	friend Factory;
	bool       is_attached_;        // Whether the handle is attached to a resource
	HandleInfo info_;               // The underlying handle information
};
}        // namespace lz
