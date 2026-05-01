#pragma once
#include "Types.h"
#include "Allocator.h"
#include <array>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
using namespace std;

/*-----------------------------
	Pool 기반 컨테이너 (기본)
	- 수명이 긴 데이터에 사용
-----------------------------*/

template<typename Type, uint32 Size>
using Array = array<Type, Size>;

template<typename Type>
using Vector = vector<Type, StlAllocator<Type>>;

template<typename Type>
using List = list<Type, StlAllocator<Type>>;

template<typename Key, typename Type, typename Pred = less<Key>>
using Map = map<Key, Type, Pred, StlAllocator<pair<const Key, Type>>>;

template<typename Key, typename Pred = less<Key>>
using Set = set<Key, Pred, StlAllocator<Key>>;

template<typename Type>
using Deque = deque<Type, StlAllocator<Type>>;

template<typename Type, typename Container = Deque<Type>>
using Queue = queue<Type, Container>;

template<typename Type, typename Container = Deque<Type>>
using Stack = stack<Type, Container>;

template<typename Type, typename Container = Vector<Type>, typename Pred = less<typename Container::value_type>>
using PriorityQueue = priority_queue<Type, Container, Pred>;

using String = basic_string<wchar_t, char_traits<wchar_t>, StlAllocator<wchar_t>>;

template<typename Key, typename Type, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
using HashMap = unordered_map<Key, Type, Hasher, KeyEq, StlAllocator<pair<const Key, Type>>>;

template<typename Key, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
using HashSet = unordered_set<Key, Hasher, KeyEq, StlAllocator<Key>>;

/*----------------------------------
	Frame 기반 컨테이너 (틱 임시)
	- 매 틱 생성/소멸되는 임시 데이터
	- 틱 끝에 LFrameAllocator->Clear()로 일괄 해제
----------------------------------*/

template<typename Type>
using FrameVector = vector<Type, StlAllocator<Type, AllocType::Frame>>;

template<typename Type>
using FrameList = list<Type, StlAllocator<Type, AllocType::Frame>>;

template<typename Key, typename Type, typename Pred = less<Key>>
using FrameMap = map<Key, Type, Pred, StlAllocator<pair<const Key, Type>, AllocType::Frame>>;

template<typename Key, typename Pred = less<Key>>
using FrameSet = set<Key, Pred, StlAllocator<Key, AllocType::Frame>>;

template<typename Key, typename Type, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
using FrameHashMap = unordered_map<Key, Type, Hasher, KeyEq, StlAllocator<pair<const Key, Type>, AllocType::Frame>>;

using FrameString = basic_string<wchar_t, char_traits<wchar_t>, StlAllocator<wchar_t, AllocType::Frame>>;
