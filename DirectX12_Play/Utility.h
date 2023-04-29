#pragma once

class Utility 
{
public:
	Utility* utility;
	static Utility& Instance();
 	//Utility* Init();
	size_t AlignmentSize(size_t size, size_t alignment);
};