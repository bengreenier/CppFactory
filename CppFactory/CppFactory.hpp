#pragma once

#include <map>
#include <functional>
#include <memory>

/// <summary>
/// Modern c++ object factory implementation in <200 lines
/// </summary>
/// <remarks>
/// Version 0.1.0
/// </remarks>
namespace CppFactory
{
	/// <summary>
	/// Represents the lifecycle of an instance
	/// </summary>
	/// <remarks>
	/// We use this value to influence how internal state is tracked
	/// for object re-use (Global) and knowing when to destruct instances
	enum ObjectLifecycle
	{
		/// <summary>
		/// The object should be untracked, subsequent calls
		/// to <see cref="Object{TObject, TObjLifecycle}.Get(int zone)"/> will result in a new instance
		/// </summary>
		Untracked,

		/// <summary>
		/// The object should be tracked and tied to the global lifecycle
		/// subsequent calls to <see cref="Object{TObject, TObjLifecycle}.Get(int zone)"/> will reuse the instance
		/// </summary>
		Global
	};

	/// <summary>
	/// Represents an object that is created via a "factory" and (optionally) is long-lived
	/// </summary>
	/// <param name="TObject">The type of object</param>
	/// <param name="TObjLifecycle">The lifecycle of the object</param>
	template <class TObject, ObjectLifecycle TObjLifecycle = ObjectLifecycle::Untracked>
	class Object
	{
	public:
		/// <summary>
		/// Registers logic capable of allocating (and optionally deallocating) an object of type
		/// <c>TObject</c> optionally within a particular zone
		/// </summary>
		/// <param name="alloc">Function that allocates an object</param>
		/// <param name="zone">Zone allocator applies to</param>
		/// <example>
		/// Object&lt;TObject&gt;::RegisterAllocator([] { return std::make_shared<TObject>(); });
		/// </example>
		static void RegisterAllocator(const std::function<std::shared_ptr<TObject>()>& alloc, int zone = 0)
		{
			m_allocFunc[zone] = alloc;
		}

		/// <summary>
		/// Unregisters all allocators for all zones for type <c>TObject</c>
		/// </summary>
		/// <example>
		/// Object&lt;TObject&gt;::UnregisterAllocator();
		/// </example>
		static void UnregisterAllocator()
		{
			m_allocFunc.clear();
		}
		
		/// <summary>
		/// Unregisters all allocators for a particular zone for type <c>TObject</c>
		/// </summary>
		/// <param name="zone">The zone to unregister</param>
		/// <example>
		/// Object&lt;TObject&gt;::UnregisterAllocator(10);
		/// </example>
		static void UnregisterAllocator(int zone)
		{
			m_allocFunc.erase(zone);
		}

		/// <summary>
		/// Removes a stored global value from cache for a particular zone for type <c>TObject</c>,
		/// destroying the object if no references are left
		/// </summary>
		/// <param name="zone">The zone to remove the value from</param>
		/// <remarks>
		/// This is only valid when <c>TObjLifecycle</c> is <c>ObjectLifecycle::Global</c>
		/// </remarks>
		/// <example>
		/// Object&lt;TObject&gt;::RemoveGlobal(10);
		/// </example>
		static void RemoveGlobal(int zone)
		{
			if (std::conditional<TObjLifecycle == ObjectLifecycle::Global, std::true_type, std::false_type>::type())
			{
				m_allocObj.erase(zone);
			}
		}

		/// <summary>
		/// Removes a stored global value from cache for all zones for type <c>TObject</c>,
		/// destroying the object if no references are left
		/// </summary>
		/// <remarks>
		/// This is only valid when <c>TObjLifecycle</c> is <c>ObjectLifecycle::Global</c>
		/// </remarks>
		/// <example>
		/// Object&lt;TObject&gt;::RemoveGlobal();
		/// </example>
		static void RemoveGlobal()
		{
			if (std::conditional<TObjLifecycle == ObjectLifecycle::Global, std::true_type, std::false_type>::type())
			{
				m_allocObj.clear();
			}
		}

		/// <summary>
		/// Gets (and allocates, if needed) an object (optionally from a particular zone) for type <c>TObject</c>
		/// </summary>
		/// <param name="zone">The zone to get from</param>
		/// <remarks>
		/// This is always an allocation when <c>TObjLifecycle</c> is <c>ObjectLifecycle::Untracked</c>
		/// </remarks>
		/// <returns>The object</returns>
		/// <example>
		/// Object<TObject>::Get();
		/// </example>
		static std::shared_ptr<TObject> Get(int zone = 0)
		{
			// if we're using global, check for an existing value in the zone
			if (std::conditional<TObjLifecycle == ObjectLifecycle::Global, std::true_type, std::false_type>::type() &&
				m_allocObj.find(zone) != m_allocObj.end())
			{
				return m_allocObj[zone];
			}

			std::shared_ptr<TObject> obj;

			// if we have a custom allocator use it
			if (m_allocFunc.find(zone) == m_allocFunc.end())
			{
				// TODO(bengreenier): support not default ctors
				//
				// If compilation is failing here, you may have a ctor with parameters or a non-public ctor
				// Non-public: add `friend Object<TObject>;`
				// Parameters: not supported yet
				obj = std::shared_ptr<TObject>(new TObject());
			}
			else
			{
				obj = m_allocFunc[zone]();
			}

			// if we're using global, store the value in the zone
			if (std::conditional<TObjLifecycle == ObjectLifecycle::Global, std::true_type, std::false_type>::type())
			{
				m_allocObj[zone] = obj;
			}

			return obj;
		}

	private:
		/// <summary>
		/// The type of the allocator function map
		/// </summary>
		typedef std::map<int, std::function<std::shared_ptr<TObject>()>> AllocFuncMapType;

		/// <summary>
		/// The type of the allocated object map
		/// </summary>
		typedef std::map<int, std::shared_ptr<TObject>> AllocObjMapType;

		/// <summary>
		/// The allocator function map
		/// </summary>
		static AllocFuncMapType m_allocFunc;

		/// <summary>
		/// The allocated object map
		/// </summary>
		static AllocObjMapType m_allocObj;
	};

	template <class TObject, ObjectLifecycle TObjLifecycle>
	typename Object<TObject, TObjLifecycle>::AllocFuncMapType Object<TObject, TObjLifecycle>::m_allocFunc = Object<TObject, TObjLifecycle>::AllocFuncMapType();

	template <class TObject, ObjectLifecycle TObjLifecycle>
	typename Object<TObject, TObjLifecycle>::AllocObjMapType Object<TObject, TObjLifecycle>::m_allocObj = Object<TObject, TObjLifecycle>::AllocObjMapType();
}