#pragma once

#include <stdint.h>
#include <cassert>
#include <stack>

#ifndef __MINGW32__
#pragma warning(push)

#pragma warning(disable : 4061) // enum is not explicitly handled by a case label
#pragma warning(disable : 4365) // signed/unsigned mismatch
#pragma warning(disable : 4464) // relative include path contains
#pragma warning(disable : 4668) // is not defined as a preprocessor macro
#pragma warning(disable : 6313) // Incorrect operator
#endif                          // __MINGW32__

// #include "rapidjson/document.h"
// #include "rapidjson/stringbuffer.h"
// #include "rapidjson/writer.h"
#include "../Radar/Deps/json11/json11.hpp"

#ifndef __MINGW32__
#pragma warning(pop)
#endif // __MINGW32__

// if only there was a standard library function for this
template <size_t Len>
inline size_t StringCopy(char (&dest)[Len], const char* src)
{
	if (!src || !Len) {
		return 0;
	}
	size_t copied;
	char* out = dest;
	for (copied = 1; *src && copied < Len; ++copied) {
		*out++ = *src++;
	}
	*out = 0;
	return copied - 1;
}

size_t JsonWriteHandshakeObj(char* dest, size_t maxLen, int version, const char* applicationId);

// Commands
struct DiscordRichPresence;
size_t JsonWriteRichPresenceObj(char* dest,
								size_t maxLen,
								int nonce,
								int pid,
								const DiscordRichPresence* presence);
size_t JsonWriteSubscribeCommand(char* dest, size_t maxLen, int nonce, const char* evtName);

size_t JsonWriteUnsubscribeCommand(char* dest, size_t maxLen, int nonce, const char* evtName);

size_t JsonWriteJoinReply(char* dest, size_t maxLen, const char* userId, int reply, int nonce);

// I want to use as few allocations as I can get away with, and to do that with RapidJson, you need
// to supply some of your own allocators for stuff rather than use the defaults

class LinearAllocator {
public:
	char* buffer_;
	char* end_;
	LinearAllocator()
	{
		assert(0); // needed for some default case in rapidjson, should not use
	}
	LinearAllocator(char* buffer, size_t size)
	  : buffer_(buffer)
	  , end_(buffer + size)
	{
	}
	static const bool kNeedFree = false;
	void* Malloc(size_t size)
	{
		char* res = buffer_;
		buffer_ += size;
		if (buffer_ > end_) {
			buffer_ = res;
			return nullptr;
		}
		return res;
	}
	void* Realloc(void* originalPtr, size_t originalSize, size_t newSize)
	{
		if (newSize == 0) {
			return nullptr;
		}
		// allocate how much you need in the first place
		assert(!originalPtr && !originalSize);
		// unused parameter warning
		(void)(originalPtr);
		(void)(originalSize);
		return Malloc(newSize);
	}
	static void Free(void* ptr)
	{
		/* shrug */
		(void)ptr;
	}
};

template <size_t Size>
class FixedLinearAllocator : public LinearAllocator {
public:
	char fixedBuffer_[Size];
	FixedLinearAllocator()
	  : LinearAllocator(fixedBuffer_, Size)
	{
	}
	static const bool kNeedFree = false;
};

// wonder why this isn't a thing already, maybe I missed it
class DirectStringBuffer {
public:
	using Ch = char;
	char* buffer_;
	char* end_;
	char* current_;

	DirectStringBuffer(char* buffer, size_t maxLen)
	  : buffer_(buffer)
	  , end_(buffer + maxLen)
	  , current_(buffer)
	{
	}

	void Put(char c)
	{
		if (current_ < end_) {
			*current_++ = c;
		}
	}
	void Flush() {}
	size_t GetSize() const { return (size_t)(current_ - buffer_); }
};

// using MallocAllocator = rapidjson::CrtAllocator;
// using PoolAllocator = rapidjson::MemoryPoolAllocator<MallocAllocator>;
// using UTF8 = rapidjson::UTF8<char>;
// // Writer appears to need about 16 bytes per nested object level (with 64bit size_t)
// using StackAllocator = FixedLinearAllocator<2048>;
// constexpr size_t WriterNestingLevels = 2048 / (2 * sizeof(size_t));
// using JsonWriterBase =
//   rapidjson::Writer<DirectStringBuffer, UTF8, UTF8, StackAllocator, rapidjson::kWriteNoFlags>;
// class JsonWriter : public JsonWriterBase {
// public:
//     DirectStringBuffer stringBuffer_;
//     StackAllocator stackAlloc_;

//     JsonWriter(char* dest, size_t maxLen)
//       : JsonWriterBase(stringBuffer_, &stackAlloc_, WriterNestingLevels)
//       , stringBuffer_(dest, maxLen)
//       , stackAlloc_()
//     {
//     }

//     size_t Size() const { return stringBuffer_.GetSize(); }
// };

// using JsonDocumentBase = rapidjson::GenericDocument<UTF8, PoolAllocator, StackAllocator>;
// class JsonDocument : public JsonDocumentBase {
// public:
//     static const int kDefaultChunkCapacity = 32 * 1024;
//     // json parser will use this buffer first, then allocate more if needed; I seriously doubt we
//     // send any messages that would use all of this, though.
//     char parseBuffer_[32 * 1024];
//     MallocAllocator mallocAllocator_;
//     PoolAllocator poolAllocator_;
//     StackAllocator stackAllocator_;
//     JsonDocument()
//       : JsonDocumentBase(rapidjson::kObjectType,
//                          &poolAllocator_,
//                          sizeof(stackAllocator_.fixedBuffer_),
//                          &stackAllocator_)
//       , poolAllocator_(parseBuffer_, sizeof(parseBuffer_), kDefaultChunkCapacity, &mallocAllocator_)
//       , stackAllocator_()
//     {
//     }
// };

// a polyfill for rapidjson JsonWriter using a modified json11
// can only create objects
class JsonWriter{
public:
	json11::Json root;
	json11::Json* currentObject;
	bool isCurrentlyArray = false;
	size_t currentArrayIndex = 0;
	bool hasExplicitlyDefinedRootObject = false;
	std::string currentKey;
	std::stack<json11::Json*> stack;
	char* dest;
	size_t maxLen;
	
	inline JsonWriter(char* dest, size_t maxLen){
		this->dest = dest;
		this->maxLen = maxLen;
		root = json11::CreateJsonObject();
		currentObject = &root;
	}
	
	// write data to dist buffer
	inline size_t Write(){
		// json11::Json test = json11::CreateJsonArray();
		// json11::Json test = json11::CreateJsonObject();
		// json11::Json test = json11::Json(json11::Json::object({{std::string("key1"),json11::Json(1)}}));
		// test.set("key2", 2);
		// test.set(0, 3);
		// test.set(1, 4);
		// fprintf(stderr, "test %s\n", test.dump().c_str());
		
		
		std::string jsonData = root.dump();
		// fprintf(stderr, "%s\n", jsonData.c_str());
		if(maxLen > 0){
			size_t outputLength = std::min(maxLen - 1, jsonData.size());
			if(outputLength < jsonData.size()){
				fprintf(stderr, "Could not fit json into output buffer");
			}
			memcpy(dest, jsonData.c_str(), outputLength);
			dest[outputLength] = 0;
			return outputLength;
		}else{
			return 0;
		}
	}
	
	// implicitly writes data out
	inline size_t Size(){
		return Write();
	}
	
	inline void Key(const char* name, size_t size){
		currentKey = std::string(name, size);
	}
	
	inline void Key(const char* name){
		currentKey = std::string(name);
	}
	
	inline void String(const char* value){
		if(isCurrentlyArray){
			currentObject->set(currentArrayIndex++, json11::Json(value));
		}else{
			currentObject->set(currentKey, json11::Json(value));
		}
	}
	inline void Int(int value){
		if(isCurrentlyArray){
			currentObject->set(currentArrayIndex++, json11::Json(value));
		}else{
			currentObject->set(currentKey, json11::Json(value));
		}
	}
	inline void Int64(int64_t value){
		if(isCurrentlyArray){
			currentObject->set(currentArrayIndex++, json11::Json((double)value));
		}else{
			currentObject->set(currentKey, json11::Json((double)value));
		}
	}
	inline void Bool(bool value){
		if(isCurrentlyArray){
			currentObject->set(currentArrayIndex++, json11::Json(value));
		}else{
			currentObject->set(currentKey, json11::Json(value));
		}
	}
	
	inline void StartObject(){
		if(!hasExplicitlyDefinedRootObject && currentKey.size() == 0 && stack.size() == 0){
			// ignore first call because the currentObject is already created
			hasExplicitlyDefinedRootObject = true;
			stack.push(currentObject);
			return;
		}
		stack.push(currentObject);
		if(!isCurrentlyArray && currentObject->is_object()){
			currentObject->set(currentKey, json11::CreateJsonObject());
			// ignore const
			currentObject = (json11::Json*)&(*currentObject)[currentKey];
			isCurrentlyArray = false;
		}else if(isCurrentlyArray){
			currentObject->set(currentArrayIndex, json11::CreateJsonObject());
			// ignore const
			currentObject = (json11::Json*)&(*currentObject)[currentArrayIndex++];
			isCurrentlyArray = false;
		}else{
			fprintf(stderr, "JsonWriter::StartObject: currentObject is not an object %s\n", currentObject->dump().c_str());
		}
		// fprintf(stderr, "JsonWriter::StartObject: created %s\n", currentObject->dump().c_str());
	}
	
	inline void EndObject(){
		PopHierarchy();
	}
	
	inline void StartArray(){
		stack.push(currentObject);
		if(currentObject->is_object()){
			currentObject->set(currentKey, json11::CreateJsonArray());
			// ignore const
			currentObject = (json11::Json*)&(*currentObject)[currentKey];
			isCurrentlyArray = true;
			currentArrayIndex = 0;
		}else if(isCurrentlyArray){
			currentObject->set(currentArrayIndex, json11::CreateJsonArray());
			// ignore const
			currentObject = (json11::Json*)&(*currentObject)[currentArrayIndex++];
			isCurrentlyArray = true;
			currentArrayIndex = 0;
		}else{
			fprintf(stderr, "JsonWriter::StartArray: currentObject is not an object\n");
		}
		// fprintf(stderr, "JsonWriter::StartArray: created %s\n", currentObject->dump().c_str());
	}
	
	inline void EndArray(){
		PopHierarchy();
	}
	
	
	inline void PopHierarchy(){
		if(stack.empty()){
			fprintf(stderr, "JsonWriter::PopHierarchy: stack empty\n");
		}else{
			currentObject = stack.top();
			stack.pop();
			isCurrentlyArray = currentObject->is_array();
			if(isCurrentlyArray){
				currentArrayIndex = currentObject->array_items().size();
			}
		}
	}
};

// using JsonValue = rapidjson::GenericValue<UTF8, PoolAllocator>;

inline const json11::Json* GetObjMember(const json11::Json* obj, const char* name)
{
	if(obj){
		return &((*obj)[std::string(name)]);
	}
	return nullptr;
}

inline int GetIntMember(const json11::Json* obj, const char* name, int notFoundDefault = 0)
{
	if (obj) {
		json11::Json value = (*obj)[std::string(name)];
		if(value.is_number()){
			return value.int_value();
		}
	}
	return notFoundDefault;
}

inline const char* GetStrMember(const json11::Json* obj,
								const char* name,
								const char* notFoundDefault = nullptr)
{
	if (obj) {
		json11::Json value = (*obj)[std::string(name)];
		if(value.is_string()){
			return value.string_value().c_str();
		}
	}
	return notFoundDefault;
}
