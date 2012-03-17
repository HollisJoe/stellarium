/*
Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef LIBINTL_INTERNAL_UTIL_HPP_
#define LIBINTL_INTERNAL_UTIL_HPP_

#if defined(WIN32) || defined(WINCE)
typedef unsigned int uint32_t;
#else
#include <stdint.h>
#endif

namespace libintllite
{

namespace internal
{

// Helper functions for handling numbers and char array conversions:

static inline bool isBigEndian()
{
	uint32_t checkNumber = 0x1100;
	return (*reinterpret_cast<const char*>(&checkNumber) != 0);
}

static inline uint32_t swapUInt32Bytes(uint32_t number)
{
	const char* numberAsCharArray = reinterpret_cast<const char*>(&number);

	uint32_t swappedNumber;
	char* swappedNumberAsCharArray = reinterpret_cast<char*>(&swappedNumber);
	swappedNumberAsCharArray[0] = numberAsCharArray[3];
	swappedNumberAsCharArray[1] = numberAsCharArray[2];
	swappedNumberAsCharArray[2] = numberAsCharArray[1];
	swappedNumberAsCharArray[3] = numberAsCharArray[0];
	return swappedNumber;
}

static inline uint32_t charArrayToUInt32(const char uint32CharArray[4])
{
	return *reinterpret_cast<const uint32_t*>(uint32CharArray);
}

static inline bool readUIn32FromFile(FILE* fileHandle, bool needsBeToLeConversion, uint32_t& outReadUInt32)
{
	char uint32CharArray[4];
	if ((fread(uint32CharArray, 1, 4, fileHandle)) != 4)
	{
		return false;
	}

	if (needsBeToLeConversion)
	{
		outReadUInt32 = swapUInt32Bytes(charArrayToUInt32(uint32CharArray));
		return true;
	}
	else
	{
		outReadUInt32 = charArrayToUInt32(uint32CharArray);
		return true;
	}
}

// RAII classes:

template <class T>
class ArrayGurard
{
private:
	ArrayGurard(const ArrayGurard&);
	ArrayGurard& operator=(const ArrayGurard&);

	T*& arrayRef;
	bool released;

public:
	explicit ArrayGurard(T*& arrayRef) :
			arrayRef(arrayRef),
			released(false)
	{
	}

	~ArrayGurard()
	{
		if (!this->released)
		{
			delete[] this->arrayRef;
		}
	}

	const T* release()
	{
		this->released = true;
		return this->arrayRef;
	}
};

class CloseFileHandleGuard
{
private:
	CloseFileHandleGuard(const CloseFileHandleGuard&);
	CloseFileHandleGuard& operator=(const CloseFileHandleGuard&);

	FILE*& fileHandleRef;

public:
	explicit CloseFileHandleGuard(FILE*& fileHandleRef) :
			fileHandleRef(fileHandleRef)
	{
	}

	~CloseFileHandleGuard()
	{
		if (this->fileHandleRef)
		{
			fclose(this->fileHandleRef);
		}
	}
};

// Helper function to load strings from a .mo file and stores them in a given array

static bool loadMoFileStringsToArray(FILE* moFile,
		uint32_t numberOfStrings,
		uint32_t stringsTableOffsetFromFileBegin,
		bool needsBeToLeConversion,
		std::string* outStringsFromMoFileArray)
{
	if (fseek(moFile, stringsTableOffsetFromFileBegin, SEEK_SET) != 0) return false;

	uint32_t* stringsLengthsArray = NULL;
	ArrayGurard<uint32_t> stringsLengthsArrayGuard(stringsLengthsArray);
	stringsLengthsArray = new uint32_t[numberOfStrings];
	if (!stringsLengthsArray)
	{
		return false;
	}

	uint32_t firstStringOffset = 0;
	uint32_t lastStringOffset = 0;
	{
		uint32_t currentStringLength;
		uint32_t currentStringOffset;
		for (uint32_t i = 0; i < numberOfStrings; i++)
		{
			if (!readUIn32FromFile(moFile, needsBeToLeConversion, currentStringLength)) return false;
			if (!readUIn32FromFile(moFile, needsBeToLeConversion, currentStringOffset)) return false;

			stringsLengthsArray[i] = currentStringLength;

			if (i == 0)
			{
				firstStringOffset = currentStringOffset;
			}
			
			if (i == (numberOfStrings - 1))
			{
				lastStringOffset = currentStringOffset;
			}
		}
	}

	{
		char* stringCharsArray = NULL;
		ArrayGurard<char> stringCharsArrayGuard(stringCharsArray);

		uint32_t stringCharsArraySize = lastStringOffset + stringsLengthsArray[numberOfStrings - 1] + 1 - firstStringOffset;
		if (stringCharsArraySize == 0)
		{
			return false;
		}

		if (fseek(moFile, firstStringOffset, SEEK_SET) != 0) return false;
		stringCharsArray = new char[stringCharsArraySize];
		if (!stringCharsArray)
		{
			return false;
		}
		if (fread(stringCharsArray, 1, stringCharsArraySize, moFile) != stringCharsArraySize) return false;

		const char* stringsCharsArrayIter = stringCharsArray;
		for (uint32_t i = 0; i < numberOfStrings; i++)
		{
			const char* currentStrEndIter = stringsCharsArrayIter + stringsLengthsArray[i] + 1;
			outStringsFromMoFileArray[i] = std::string(stringsCharsArrayIter, currentStrEndIter);
			stringsCharsArrayIter = currentStrEndIter;
		}
	}

	return true;
}

} // namespace internal

} // namespace libintllite

#endif // LIBINTL_INTERNAL_UTIL_HPP_
