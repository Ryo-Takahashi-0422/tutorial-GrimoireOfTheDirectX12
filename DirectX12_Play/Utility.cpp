#include <stdafx.h>
#include <Utility.h>


Utility& Utility::Instance()
{
	static Utility instance;
	return instance;
};

//アライメントにそろえたサイズを返す
//@param size 元のサイズ
//@param alignment アライメントサイズ
//@return アライメントを揃えたサイズ
size_t
Utility::AlignmentSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
}