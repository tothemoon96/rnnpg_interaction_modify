/*
 * StringBuffer.h
 *
 *  Created on: 2014年2月16日
 *      Author: xing
 */

#ifndef STRINGBUFFER_H_
#define STRINGBUFFER_H_

#include <vector>
using namespace std;

class StringBuffer
{
public:
	/**
	 * @brief
	 * 向buffer中添加一个str字符串，以\0作为间隔
	 * @param str
	 * @return int 新添加的str在vector中的位置，下标从0开始
	 */
	static int add(const char* str)
	{
		int start = buffer.size();
		for(int i = 0; str[i] != '\0'; i ++)
			buffer.push_back(str[i]);
		buffer.push_back(0);

		return start;
	}
	/**
	 * @brief
	 * 获得buffer[offset]的真实地址
	 * @param offset
	 * @return char
	 */
	static char *getRealAddr(int offset)
	{
		return &buffer[offset];
	}
private:
	static vector<char> buffer;
};

#endif /* STRINGBUFFER_H_ */
