// Copyright 2015-2020 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "PsData.h"
#include "PsDataCore.h"
#include "PsDataEvent.h"
#include "PsDataField.h"
#include "PsDataHardObjectPtr.h"
#include "PsDataTraits.h"

#include "CoreMinimal.h"

namespace FDataReflectionTools
{
template <typename T>
struct FArrayChangeBehavior
{
	static void Add(UPsData* Instance, const TSharedPtr<const FDataField>& Field, int32 Index, const T& Value, TFunction<void()> AddAction)
	{
		AddAction();
		FPsDataFriend::Changed(Instance, Field);
	}

	static void Remove(UPsData* Instance, const TSharedPtr<const FDataField>& Field, const T& Value, TFunction<void()> RemoveAction)
	{
		RemoveAction();
		FPsDataFriend::Changed(Instance, Field);
	}

	static void Replace(UPsData* Instance, const TSharedPtr<const FDataField>& Field, const T& OldValue, const T& NewValue, TFunction<void()> ReplaceAction)
	{
		ReplaceAction();
		FPsDataFriend::Changed(Instance, Field);
	}
};

template <typename T>
struct FArrayChangeBehavior<T*>
{
	static_assert(FDataReflectionTools::TIsPsData<T>::Value, "Pointer must be only UPsData");

	static void Add(UPsData* Instance, const TSharedPtr<const FDataField>& Field, int32 Index, T* Value, TFunction<void()> AddAction)
	{
		AddAction();
		FPsDataFriend::ChangeDataName(Value, FString::FromInt(Index), Field->Name);
		FPsDataFriend::AddChild(Instance, Value);
		FPsDataFriend::Changed(Instance, Field);
	}

	static void Remove(UPsData* Instance, const TSharedPtr<const FDataField>& Field, T* Value, TFunction<void()> RemoveAction)
	{
		RemoveAction();
		FPsDataFriend::RemoveChild(Instance, Value);
		FPsDataFriend::Changed(Instance, Field);
	}

	static void Replace(UPsData* Instance, const TSharedPtr<const FDataField>& Field, int32 Index, T* OldValue, T* NewValue, TFunction<void()> ReplaceAction)
	{
		ReplaceAction();
		FPsDataFriend::RemoveChild(Instance, OldValue);
		FPsDataFriend::ChangeDataName(NewValue, FString::FromInt(Index), Field->Name);
		FPsDataFriend::AddChild(Instance, NewValue);
		FPsDataFriend::Changed(Instance, Field);
	}
};
} // namespace FDataReflectionTools

template <typename T, bool bConst>
struct FPsDataBaseArrayProxy
{
private:
	friend struct FPsDataBaseArrayProxy<T, true>;
	friend struct FPsDataBaseArrayProxy<T, false>;

	THardObjectPtr<UPsData> Instance;
	TSharedPtr<const FDataField> Field;

	TArray<T>& Get() const
	{
		check(IsValid());
		TArray<T>* Output = nullptr;
		FDataReflectionTools::GetByField(Instance.Get(), Field, Output);
		check(Output);
		return *Output;
	}

protected:
	friend class UPsDataBlueprintArrayProxy;

	FPsDataBaseArrayProxy()
	{
	}

public:
	FPsDataBaseArrayProxy(UPsData* InInstance, TSharedPtr<const FDataField> InField)
		: Instance(InInstance)
		, Field(InField)
	{
		check(IsValid());
	}

	FPsDataBaseArrayProxy(UPsData* InInstance, int32 Hash)
		: Instance(InInstance)
		, Field(FDataReflection::GetFieldByHash(InInstance->GetClass(), Hash))
	{
		check(IsValid());
	}

	template <bool bOtherConst>
	FPsDataBaseArrayProxy(const FPsDataBaseArrayProxy<T, bOtherConst>& ArrayProxy)
		: Instance(ArrayProxy.Instance)
		, Field(ArrayProxy.Field)
	{
		static_assert(bConst == bOtherConst || (bConst && !bOtherConst), "Can't create FPsDataArrayProxy from FPsDataConstArrayProxy");
		check(IsValid());
	}

	template <bool bOtherConst>
	FPsDataBaseArrayProxy(FPsDataBaseArrayProxy<T, bOtherConst>&& ArrayProxy)
		: Instance(std::move(ArrayProxy.Instance))
		, Field(std::move(ArrayProxy.Field))
	{
		static_assert(bConst == bOtherConst || (bConst && !bOtherConst), "Can't create FPsDataArrayProxy from FPsDataConstArrayProxy");
		check(IsValid());
		ArrayProxy.Instance = nullptr;
		ArrayProxy.Field = nullptr;
	}

	TSharedPtr<const FDataField> GetField() const
	{
		return Field;
	}

	bool IsValid() const
	{
		return Instance.IsValid() && Field.IsValid();
	}

	int32 Add(typename FDataReflectionTools::TConstRef<T>::Type Element)
	{
		static_assert(!bConst, "Unsupported method for FPsDataConstArrayProxy, use FPsDataArrayProxy");

		auto& Array = Get();
		const int32 Index = Array.Num();
		FDataReflectionTools::FArrayChangeBehavior<T>::Add(Instance.Get(), Field, Index, Element, [&Array, &Element]() {
			Array.Add(Element);
		});

		return Index;
	}

	void Insert(typename FDataReflectionTools::TConstRef<T>::Type Element, int32 Index)
	{
		static_assert(!bConst, "Unsupported method for FPsDataConstArrayProxy, use FPsDataArrayProxy");

		auto& Array = Get();
		FDataReflectionTools::FArrayChangeBehavior<T>::Add(Instance.Get(), Field, Index, Element, [&Array, &Element, Index]() {
			Array.Insert(Element, Index);
		});
	}

	void RemoveAt(int32 Index, bool bAllowShrinking = false)
	{
		static_assert(!bConst, "Unsupported method for FPsDataConstArrayProxy, use FPsDataArrayProxy");

		auto& Array = Get();
		FDataReflectionTools::FArrayChangeBehavior<T>::Remove(Instance.Get(), Field, Array[Index], [&Array, Index, bAllowShrinking]() {
			Array.RemoveAt(Index, 1, bAllowShrinking);
		});
	}

	int32 Remove(typename FDataReflectionTools::TConstRef<T>::Type Element, bool bAllowShrinking = false)
	{
		static_assert(!bConst, "Unsupported method for FPsDataConstArrayProxy, use FPsDataArrayProxy");

		auto& Array = Get();
		const int32 Index = Array.Find(Element);
		if (Index == INDEX_NONE)
		{
			return INDEX_NONE;
		}

		FDataReflectionTools::FArrayChangeBehavior<T>::Remove(Instance.Get(), Field, Element, [&Array, Index, bAllowShrinking]() {
			Array.RemoveAt(Index, 1, bAllowShrinking);
		});

		return Index;
	}

	template <typename PredicateType>
	int32 RemoveAll(const PredicateType& Predicate)
	{
		static_assert(!bConst, "Unsupported method for FPsDataConstArrayProxy, use FPsDataArrayProxy");

		int32 RemovedElements = 0;
		for (auto It = CreateIterator(); It; ++It)
		{
			if (Predicate(It.GetConstValue()))
			{
				It.RemoveCurrent();
				++RemovedElements;
			}
		}

		return RemovedElements;
	}

	typename FDataReflectionTools::TConstRef<T, bConst>::Type Set(typename FDataReflectionTools::TConstRef<T>::Type Element, int32 Index)
	{
		static_assert(!bConst, "Unsupported method for FPsDataConstArrayProxy, use FPsDataArrayProxy");

		auto& Array = Get();
		auto& OldElement = Array[Index];
		FDataReflectionTools::FArrayChangeBehavior<T>::Replace(Instance.Get(), Field, Index, OldElement, Element, [&Array, &Element, Index]() {
			Array[Index] = Element;
		});

		return OldElement;
	}

	int32 Find(typename FDataReflectionTools::TConstRef<T>::Type Element) const
	{
		return Get().Find(Element);
	}

	template <typename PredicateType>
	typename FDataReflectionTools::TConstRef<T*, bConst>::Type FindByPredicate(const PredicateType& Predicate) const
	{
		for (auto It = CreateIterator(); It; ++It)
		{
			if (Predicate(It.GetConstValue()))
			{
				return &(*It.Iterator);
			}
		}

		return nullptr;
	}

	typename FDataReflectionTools::TConstRef<T, bConst>::Type Get(int32 Index) const
	{
		return Get()[Index];
	}

	int32 Num() const
	{
		return Get().Num();
	}

	void Reserve(int32 Number)
	{
		Get().Reserve(Number);
	}

	void Empty()
	{
		static_assert(!bConst, "Unsupported method for FPsDataConstArrayProxy, use FPsDataArrayProxy");

		for (auto It = CreateIterator(); It; ++It)
		{
			It.RemoveCurrent();
		}
	}

	bool IsEmpty() const
	{
		return Num() == 0;
	}

	bool IsValidIndex(int32 Index) const
	{
		return Get().IsValidIndex(Index);
	}

	FPsDataBind Bind(const FString& Type, const FPsDataDynamicDelegate& Delegate) const
	{
		return Instance->BindInternal(Type, Delegate, Field);
	}

	FPsDataBind Bind(const FString& Type, const FPsDataDelegate& Delegate) const
	{
		return Instance->BindInternal(Type, Delegate, Field);
	}

	void Unbind(const FString& Type, const FPsDataDynamicDelegate& Delegate) const
	{
		Instance->UnbindInternal(Type, Delegate, Field);
	}

	void Unbind(const FString& Type, const FPsDataDelegate& Delegate) const
	{
		Instance->UnbindInternal(Type, Delegate, Field);
	}

	const TArray<typename FDataReflectionTools::TConstRef<T, bConst>::Type>& GetRef()
	{
		return Get();
	}

	typename FDataReflectionTools::TConstRef<T, bConst>::Type operator[](int32 Index) const
	{
		return Get()[Index];
	}

	void operator=(const FPsDataBaseArrayProxy& Proxy)
	{
		Instance = Proxy.Instance;
		Field = Proxy.Field;
		check(IsValid());
	}

	/***********************************
	 * TProxyIterator
	 ***********************************/

public:
	template <bool bIteratorConst>
	struct TProxyIterator
	{
	private:
		friend struct FPsDataBaseArrayProxy;

		typedef typename FDataReflectionTools::TConstRef<FPsDataBaseArrayProxy, bIteratorConst>::Type ProxyType;
		typedef typename FDataReflectionTools::TSelector<typename TArray<T>::TConstIterator, typename TArray<T>::TIterator, bIteratorConst>::Value IteratorType;

		ProxyType& Proxy;
		IteratorType Iterator;

		TProxyIterator(ProxyType& InProxy, bool bEnd = false)
			: Proxy(InProxy)
			, Iterator(InProxy.Get())
		{
			if (bEnd)
			{
				while (Iterator) //TODO: call std::end
				{
					++Iterator;
				}
			}
		}

	public:
		void RemoveCurrent()
		{
			static_assert(!bIteratorConst, "Unsupported method for FPsDataConstArrayProxy::TProxyIterator, use FPsDataArrayProxy::TProxyIterator");

			FDataReflectionTools::FArrayChangeBehavior<T>::Remove(Proxy.Instance.Get(), Proxy.Field, *Iterator, [this]() {
				Iterator.RemoveCurrent();
			});
		}

		typename FDataReflectionTools::TConstRef<T, bIteratorConst>::Type GetValue() const
		{
			return *Iterator;
		}

		typename FDataReflectionTools::TConstRef<T, true>::Type GetConstValue() const
		{
			return *Iterator;
		}

		TProxyIterator& operator++()
		{
			++Iterator;
			return *this;
		}

		explicit operator bool() const
		{
			return bool(Iterator);
		}

		bool operator!() const
		{
			return !bool(Iterator);
		}

		friend bool operator==(const TProxyIterator& Lhs, const TProxyIterator& Rhs)
		{
			return Lhs.Iterator == Rhs.Iterator;
		}

		friend bool operator!=(const TProxyIterator& Lhs, const TProxyIterator& Rhs)
		{
			return Lhs.Iterator != Rhs.Iterator;
		}

		typename FDataReflectionTools::TConstRef<T, bIteratorConst>::Type operator*() const
		{
			return *Iterator;
		}
	};

	TProxyIterator<bConst> CreateIterator() const
	{
		return TProxyIterator<bConst>(*this);
	}

	TProxyIterator<true> CreateConstIterator() const
	{
		return TProxyIterator<true>(*this);
	}

	TProxyIterator<bConst> begin() { return TProxyIterator<bConst>(*this); }
	TProxyIterator<bConst> end() { return TProxyIterator<bConst>(*this, true); }
	TProxyIterator<true> begin() const { return TProxyIterator<true>(*this); }
	TProxyIterator<true> end() const { return TProxyIterator<true>(*this, true); }
};

template <class T>
using FPsDataArrayProxy = FPsDataBaseArrayProxy<T, false>;
template <class T>
using FPsDataConstArrayProxy = FPsDataBaseArrayProxy<T, true>;

template <typename T, bool bConst>
struct FPsDataBaseArrayProxy<TArray<T>, bConst>
{
	static_assert(FDataReflectionTools::TAlwaysFalse<T>::value, "Unsupported type");
};

template <typename T, bool bConst>
struct FPsDataBaseArrayProxy<TMap<FString, T>, bConst>
{
	static_assert(FDataReflectionTools::TAlwaysFalse<T>::value, "Unsupported type");
};
