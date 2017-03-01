/*
 * Creat on:2017-3-1
 * Author:韩玮光
 */
/*
 *本程序根据用户输入的关键词来生成一首诗，运行时需要指定
 * <configFile> 和模型有关的一些参数的设置
 */
/*
 * version 0.1:实现最初级的功能，把first-generator和rnnpg-generator整合到一起
 */
#include <iostream>
#include "util/XConfig.h"
using namespace std;
void cmdLineGenerator(int argc, char **argv){
	cout<<"诗歌生成0.1版"<<endl;
	if(argc!=2){
		cerr<<"PoemGenerator <configFile>"<<endl;
	}
	XConfig::load(argv[1]);
	//TODO:添加生成的步骤
}



int main(int argc, char **argv){
	cmdLineGenerator(argc,argv);
	return 0;
}

