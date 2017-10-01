#include <chrono>
#include <thread>
#include <CppUnitTest.h>

#include "CppFactory.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace CppFactory;

namespace CppFactoryUnitTests
{
	struct Data
	{
	public:
		int Value;
		int Value2;

		Data() : Value(10), Value2(20) {}
	};

	enum TestZones
	{
		ZoneOne,
		ZoneTwo
	};

	TEST_CLASS(ObjectTests)
	{
	public:

		TEST_CLASS_INITIALIZE(InitC)
		{
			//std::this_thread::sleep_for(std::chrono::seconds::duration(20));
		}

		TEST_METHOD_INITIALIZE(Init)
		{
			// removes all custom allocators
			Object<Data>::UnregisterAllocator();
			Object<Data, ObjectLifecycle::Global>::UnregisterAllocator();

			// removes all globals
			Object<Data, ObjectLifecycle::Global>::RemoveGlobal();
		}

		TEST_METHOD(Default_Success)
		{
			Assert::AreEqual<int>(10, Object<Data>::Get()->Value);
			Assert::AreEqual<int>(20, Object<Data>::Get()->Value2);
		}

		TEST_METHOD(Alloc_Success)
		{
			Object<Data>::RegisterAllocator([] {
				return std::make_shared<Data>();
			});

			Assert::AreEqual<int>(10, Object<Data>::Get()->Value);
			Assert::AreEqual<int>(20, Object<Data>::Get()->Value2);
		}

		TEST_METHOD(CustomAlloc_Success)
		{
			Object<Data>::RegisterAllocator([] {
				// custom alloc
				auto ptr = (Data*)malloc(sizeof(Data));
				ptr->Value = 0;
				ptr->Value2 = 0;

				// custom dealloc
				return std::shared_ptr<Data>(ptr, [](Data* data) { free(data); });
			});

			Assert::AreEqual<int>(0, Object<Data>::Get()->Value);
			Assert::AreEqual<int>(0, Object<Data>::Get()->Value2);
		}
		
		TEST_METHOD(ArbitraryZone_Success)
		{
			Assert::AreEqual<int>(10, Object<Data>::Get(12345)->Value);
			Assert::AreEqual<int>(20, Object<Data>::Get(12345)->Value2);
			Assert::AreEqual<int>(10, Object<Data>::Get(TestZones::ZoneOne)->Value);
			Assert::AreEqual<int>(20, Object<Data>::Get(TestZones::ZoneOne)->Value2);
			Assert::AreEqual<int>(10, Object<Data>::Get(TestZones::ZoneTwo)->Value);
			Assert::AreEqual<int>(20, Object<Data>::Get(TestZones::ZoneTwo)->Value2);
		}

		TEST_METHOD(PerZoneAlloc_Success)
		{
			// write an allocator for zone 10
			Object<Data>::RegisterAllocator([] {
				// custom alloc
				auto ptr = (Data*)malloc(sizeof(Data));
				ptr->Value = 0;
				ptr->Value2 = 0;

				// custom dealloc
				return std::shared_ptr<Data>(ptr, [](Data* data) { free(data); });
			}, 10);

			// assert default zone
			Assert::AreEqual<int>(10, Object<Data>::Get()->Value);
			Assert::AreEqual<int>(20, Object<Data>::Get()->Value2);

			// assert zone 10
			Assert::AreEqual<int>(0, Object<Data>::Get(10)->Value);
			Assert::AreEqual<int>(0, Object<Data>::Get(10)->Value2);
		}

		TEST_METHOD(GlobalLifecycle_Success)
		{
			// note: these could be simplified to just `Global`
			Assert::AreEqual<int>(10, Object<Data, ObjectLifecycle::Global>::Get()->Value);
			Assert::AreEqual<int>(20, Object<Data, ObjectLifecycle::Global>::Get()->Value2);

			Object<Data, ObjectLifecycle::Global>::Get()->Value = 100;
			Object<Data, ObjectLifecycle::Global>::Get()->Value2 = 200;

			Assert::AreEqual<int>(100, Object<Data, ObjectLifecycle::Global>::Get()->Value);
			Assert::AreEqual<int>(200, Object<Data, ObjectLifecycle::Global>::Get()->Value2);
		}

		TEST_METHOD(RefCount_Verify)
		{
			// should be just us for untracked
			Assert::AreEqual<long>(1, Object<Data, ObjectLifecycle::Untracked>::Get().use_count());

			// should be us and the object manager for global
			Assert::AreEqual<long>(2, Object<Data, ObjectLifecycle::Global>::Get().use_count());
		}

		TEST_METHOD(Destructor_Verify)
		{
			// block scope for Untracked
			{
				bool dealloc = false;
				Object<Data>::RegisterAllocator([&] {
					// custom alloc
					auto ptr = (Data*)malloc(sizeof(Data));
					ptr->Value = 0;
					ptr->Value2 = 0;

					// custom dealloc
					return std::shared_ptr<Data>(ptr, [&](Data* data) { free(data); dealloc = true; });
				});

				Assert::IsFalse(dealloc);

				Object<Data>::Get().reset();

				Assert::IsTrue(dealloc);
			}

			// block scope for Global
			{
				bool dealloc = false;
				Object<Data, ObjectLifecycle::Global>::RegisterAllocator([&] {
					// custom alloc
					auto ptr = (Data*)malloc(sizeof(Data));
					ptr->Value = 0;
					ptr->Value2 = 0;

					// custom dealloc
					return std::shared_ptr<Data>(ptr, [&](Data* data) { free(data); dealloc = true; });
				});

				// retrieval scope
				{
					Object<Data, ObjectLifecycle::Global>::Get();
				}

				Assert::IsFalse(dealloc);

				Object<Data, ObjectLifecycle::Global>::RemoveGlobal();

				Assert::IsTrue(dealloc);
			}
		}

		BEGIN_TEST_METHOD_ATTRIBUTE(Timings)
			TEST_IGNORE()
		END_TEST_METHOD_ATTRIBUTE()

		TEST_METHOD(Timings)
		{
			int iterations = 10 * 1000;

			// block scope for no re-use
			{
				auto start = std::chrono::system_clock::now();
				for (auto i = 0; i < iterations; ++i)
				{
					Object<Data>::Get();
				}
				auto end = std::chrono::system_clock::now();

				auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
				Logger::WriteMessage((L"Untracked, normal alloc: " + std::to_wstring(ms) + L"ms (" + std::to_wstring(ms / (float)iterations) + L"ms / iteration)").c_str());
			}

			Logger::WriteMessage(L"\r\n");

			// block scope for re-use
			{
				auto start = std::chrono::system_clock::now();

				for (auto i = 0; i < iterations; ++i)
				{
					Object<Data, ObjectLifecycle::Global>::Get();
				}
				Object<Data, ObjectLifecycle::Global>::RemoveGlobal();

				auto end = std::chrono::system_clock::now();

				auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
				Logger::WriteMessage((L"Global, normal alloc: " + std::to_wstring(ms) + L"ms (" + std::to_wstring(ms / (float)iterations) + L"ms / iteration)").c_str());
			}
			
			Logger::WriteMessage(L"\r\n");

			// block scope for slow allocator, no re-use
			{
				Object<Data>::RegisterAllocator([] {
					// custom alloc
					auto ptr = (Data*)malloc(sizeof(Data));
					ptr->Value = 0;
					ptr->Value2 = 0;

					// slow down
					std::this_thread::sleep_for(std::chrono::milliseconds::duration(10));

					// custom dealloc
					return std::shared_ptr<Data>(ptr, [](Data* data) { free(data); });
				});

				auto start = std::chrono::system_clock::now();
				for (auto i = 0; i < iterations; ++i)
				{
					Object<Data>::Get();
				}
				auto end = std::chrono::system_clock::now();

				auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
				Logger::WriteMessage((L"Untracked, slow alloc: " + std::to_wstring(ms) + L"ms (" + std::to_wstring(ms / (float)iterations) + L"ms / iteration)").c_str());
			}

			Logger::WriteMessage(L"\r\n");

			// block scope for slow allocator, re-use
			{
				Object<Data, ObjectLifecycle::Global>::RegisterAllocator([] {
					// custom alloc
					auto ptr = (Data*)malloc(sizeof(Data));
					ptr->Value = 0;
					ptr->Value2 = 0;

					// slow down
					std::this_thread::sleep_for(std::chrono::milliseconds::duration(10));

					// custom dealloc
					return std::shared_ptr<Data>(ptr, [](Data* data) { free(data); });
				});

				auto start = std::chrono::system_clock::now();
				for (auto i = 0; i < iterations; ++i)
				{
					Object<Data, ObjectLifecycle::Global>::Get();
				}
				Object<Data, ObjectLifecycle::Global>::RemoveGlobal();

				auto end = std::chrono::system_clock::now();

				auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
				Logger::WriteMessage((L"Global, slow alloc: " + std::to_wstring(ms) + L"ms (" + std::to_wstring(ms / (float)iterations) + L"ms / iteration)").c_str());
			}
		}
	};
}