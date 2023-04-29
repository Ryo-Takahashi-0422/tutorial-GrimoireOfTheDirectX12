#include <stdafx.h>
#include <Utility.h>


Utility& Utility::Instance()
{
	static Utility instance;
	return instance;
};

//�A���C�����g�ɂ��낦���T�C�Y��Ԃ�
//@param size ���̃T�C�Y
//@param alignment �A���C�����g�T�C�Y
//@return �A���C�����g�𑵂����T�C�Y
size_t
Utility::AlignmentSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
}