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
#include <iomanip>
#include "generator/PoemGenerator.h"
using namespace std;
void cmdLineGenerator(int argc, char **argv){
	cout<<"诗歌生成0.1版"<<endl;
	if(argc!=2){
		cerr<<"PoemGenerator <configFile>"<<endl;
	}
	XConfig::load(argv[1]);
	PoemGenerator pg;
	while(1){
		pg.reset();
		string input;
		cout<<"请输入关键词："<<endl;
		cout<<"举例：＂才 女,美 人,中 秋 月＂，分割的标点符号为英语标点符号"<<endl;
		getline(cin,input);
		vector<string> keywords;
		split(input,",",keywords);
		cout<<"请输入诗句长度："<<endl;
		int senLen;
		cin>>senLen;
		pg.setSenLen(senLen);
		pg.generateFirstSentence(keywords);

		for(int len=0;len<4;len++){
			vector<string> topSents;
			topSents=pg.getLastTopSents();
			for(int i=0;i<topSents.size();i++){
				cout<<left<<setw(2)<<i<<":"<<topSents[i]<<endl;
			}
			int index;
			cout<<"请输入要选择的诗句的下标：";
			cin>>index;
			if(len<3){
				pg.seletSentInLastTopSents(index);
				pg.generateNextSentence();
				vector<string> poem;
				poem=pg.getPoem();
				cout<<"当前生成的诗句为："<<endl;
				for(int i=0;i<poem.size();i++){
					cout<<poem[i]<<endl;
				}
				cout<<endl;
			}
			else{
				pg.seletSentInLastTopSents(index);
				vector<string> poem;
				poem=pg.getPoem();
				cout<<"整首诗为："<<endl;
				for(int i=0;i<poem.size();i++){
					cout<<poem[i]<<endl;
				}
				cout<<endl;
			}
		}
		cin.get();
	}
}

int main(int argc, char **argv){
	cmdLineGenerator(argc,argv);
	return 0;
}

