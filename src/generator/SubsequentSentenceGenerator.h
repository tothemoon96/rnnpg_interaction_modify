/*
 * SubsequentSentenceGenerator.h
 *
 *  Created on: 19 Mar 2014
 *      Author: s1270921
 */

#ifndef SUBSEQUENTSENTENCEGENERATOR_H_
#define SUBSEQUENTSENTENCEGENERATOR_H_

#include "../util/xutil.h"
#include <iostream>
#include <fstream>
#include <set>
#include <stdlib.h>
using namespace std;
#include "../util/XConfig.h"
#include "Decoder.h"

#include "../tone_helper/Constraints.h"

/*
topK = 20
model = 
transTable = 
stackSize = 300
channelOption = 1 
weightFile = 
ngramLM = 
ngramFeatOn = 1
pingshuiyunDir =
*/

class SubsequentSentenceGenerator
{
public:
	SubsequentSentenceGenerator()
	{
		init();
	}
	~SubsequentSentenceGenerator()
	{
		int i;
		if(rnnpg) delete rnnpg;
		// if(transTable) delete transTable;
		for(i = 0; i < DECODER_NUMBER; i ++)
			delete transTables[i];
		delete []transTables;
		if(model) delete model;
		if(kenlm) delete kenlm;
		// if(decoder) delete decoder;
		for(i = 0; i < DECODER_NUMBER; i ++)
			delete decoders[i];
		delete []decoders;
	}
	void init()
	{
		srand(7);

		int i;
		topK = XConfig::getInt("topK");
		cout << "topK = " << topK << endl;
		rnnpg = NULL;
		decoders = new Decoder*[3];
		for(i = 0; i < DECODER_NUMBER; i ++)
			decoders[i] = NULL;

		kenlm = NULL;
		transTables = new TranslationTable*[3];
		for(i = 0; i < DECODER_NUMBER; i ++)
			transTables[i] = NULL;

		initDecoder();
	}
	void initDecoder()
	{
		int i;
		const int PATH_LENGTH = 1024;
		char modelPath[PATH_LENGTH]; modelPath[0] = 0;

		char transTablePath[3][PATH_LENGTH];
		for(i = 0; i < DECODER_NUMBER; i ++)
			transTablePath[i][0] = 0;

		char weightPath[3][PATH_LENGTH]; // weightPath[0] = 0;
		for(i = 0; i < DECODER_NUMBER; i ++)
			weightPath[i][0] = 0;

		char ngramLMPath[PATH_LENGTH]; ngramLMPath[0] = 0;

		char pingshuiyunDir[PATH_LENGTH]; pingshuiyunDir[0] = 0;

		stackSize = 300;
		int channelOption = 1;
		int rerank = 0;
		int disableRNN = 0;
		double posProbCut5 = 1;
		double posProbCut7 = 1;
		double interpolateWeight = 0;
		int ngramFeatOn = 0;

		/*
		topK = 20
		model =
		transTable =
		stackSize = 300
		channelOption = 1
		weightFile =
		ngramLM =
		ngramFeatOn = 1
		pingshuiyunDir =
		*/

		xstrcpy(modelPath, sizeof(modelPath), XConfig::getStr("model"));
		if(modelPath[0] != 0)
		{
			rnnpg = new RNNPG();
			rnnpg->loadNet(modelPath);
		}

		xstrcpy(transTablePath[0], sizeof(transTablePath[0]), XConfig::getStr("transTable12"));
		xstrcpy(transTablePath[1], sizeof(transTablePath[1]), XConfig::getStr("transTable23"));
		xstrcpy(transTablePath[2], sizeof(transTablePath[2]), XConfig::getStr("transTable34"));

		for(i = 0; i < DECODER_NUMBER; i ++)
			if(transTablePath[i][0] != 0)
			{
				transTables[i] = new TranslationTable();
				transTables[i]->load(transTablePath[i]);
				cout << "load translation table " << transTablePath[i] << " done" << endl << endl;
			}

		stackSize = XConfig::getInt("stackSize");
		channelOption = XConfig::getInt("channelOption");
//		xstrcpy(weightPath[0], sizeof(weightPath[0]), XConfig::getStr("weightFile12"));
//		xstrcpy(weightPath[1], sizeof(weightPath[1]), XConfig::getStr("weightFile23"));
//		xstrcpy(weightPath[2], sizeof(weightPath[2]), XConfig::getStr("weightFile34"));
		weightFilePaths[0][0] = XConfig::getStr("weightFile12-5");
		weightFilePaths[0][1] = XConfig::getStr("weightFile12-7");
		weightFilePaths[1][0] = XConfig::getStr("weightFile23-5");
		weightFilePaths[1][1] = XConfig::getStr("weightFile23-7");
		weightFilePaths[2][0] = XConfig::getStr("weightFile34-5");
		weightFilePaths[2][1] = XConfig::getStr("weightFile34-7");

		xstrcpy(ngramLMPath, sizeof(ngramLMPath), XConfig::getStr("ngramLM"));
		if(ngramLMPath[0] != 0)
		{
			model = new Model(ngramLMPath);
			kenlm = new KenLMM(model);
		}
		ngramFeatOn = XConfig::getInt("ngramFeatOn");

		xstrcpy(pingshuiyunDir, sizeof(pingshuiyunDir), XConfig::getStr("pingshuiyunDir"));

		for(i = 0; i < DECODER_NUMBER; i ++)
		{
			decoders[i] = new Decoder(rnnpg, transTables[i], kenlm);
//			if(weightPath[i][0] != 0)
//				decoders[i]->loadWeights(weightPath[i]);
			assert(weightFilePaths[i][0].length() != 0);
			assert(weightFilePaths[i][1].length() != 0);

			decoders[i]->setChannelOption(channelOption);
			decoders[i]->setRerank(rerank);
			decoders[i]->setDisableRNN(disableRNN);
			decoders[i]->setPosProbCut5(posProbCut5);
			decoders[i]->setPosProbCut7(posProbCut7);
			decoders[i]->setInterpolateWeight(interpolateWeight);
			decoders[i]->setNgramFeatureOn(ngramFeatOn);

			if(pingshuiyunDir[0] != 0)
				decoders[i]->loadToneRhythm(pingshuiyunDir);
		}
	}
	void generatePoems(const char *firstSentFile, const char *outFile)
	{
		FILE *fin = xfopen(firstSentFile, "r", "open file first sentence file %s failed!");
		FILE *fout = xfopen(outFile, "w");
		char buf[1024];
		int cnt = 0;//一组关键词对应的第一句中的第几项
		bool isEnd = false;
		string keywords;
		vector<string> firstSentLines;
		int i, j, k;
		while(fgets(buf,sizeof(buf),fin))
		{
			string line = buf;
			trim(line);
			//如果读取到了诗的一组关键词
			if(buf[0] == '<')
			{
				cnt = 0;
				fprintf(fout, "%s", buf);
				keywords = buf;
				trim(keywords);
				isEnd = false;
				firstSentLines.clear();
			}//如果读到了５言诗或者７言诗的一组关键词结束了
			else if(buf[0] == '\n')
			{
				if(!isEnd)
				{
					cout << keywords << endl;
					vector<vector<string> > allTopSents;
					vector<string> poem;
					for(i = 0; i < (int)firstSentLines.size(); i ++)
					{
						vector<string> fields;
						split(firstSentLines[i], "|||", fields);
						trim(fields[0]);
						string firstSent = fields[0];//如＂游 春 车 马 客＂
						int tpIndex = atoi(fields[5].c_str());//使用的韵律格式的索引下标
						if(generatePoem(firstSent, tpIndex, allTopSents, poem))
							//一首诗生成完毕，就break
							break;
						else
							cout << firstSent << ", failed to generate a poem" << endl;
					}
					if(i != (int)firstSentLines.size())
					{

						for(j = 0; j < (int)allTopSents.size(); j ++)
						{
							fprintf(fout, "\n");
							for(k = 0; k < (int)allTopSents[j].size(); k ++)
								fprintf(fout, "%s\n", allTopSents[j][k].c_str());
						}
						fprintf(fout, "\n");
						for(j = 0; j < (int)poem.size(); j ++)
							fprintf(fout, "%s\n", poem[j].c_str());
						fprintf(fout, "\n\n");
					}
					else
						cout << "all the first sentences failed to generate a poem" << endl;

					isEnd = true;
				}
			}
			else
			{
				if(cnt == 0)
				{
					// extract the sentence and the tonalPatterIndex, do generation
					vector<string> fields;
					split(line, "|||", fields);
					trim(fields[0]);
					int tpIndex = atoi(fields[5].c_str());
					cout << fields[0] << " " << tpIndex << endl;
				}
				if(cnt < topK)
				{
					firstSentLines.push_back(line);
					fprintf(fout, "%s", buf);
				}
				cnt ++;
			}
		}
		fclose(fin);
		fclose(fout);
	}
private:
	int topK;		// topK sentences should be printed in the output file
	int stackSize;
	Decoder **decoders;
	enum {DECODER_NUMBER = 3};
	RNNPG *rnnpg;
	KenLMM *kenlm;
	TranslationTable **transTables;
	Model *model;
	string weightFilePaths[3][2];		// i, j... i for 12, 23, 34; j for 5 and 7 character sentence

	Constraints constraints;

	/**
	 * @brief
	 * 使用firstSent生成一首诗，生成的结果存放在poem里，allTopSents存放生成每一句里概率最大的几个句子
	 * @param firstSent 如＂游 春 车 马 客＂
	 * @param tpIndex
	 * @param allTopSents
	 * @param poem
	 * @return bool
	 */
	bool generatePoem(string firstSent, int tpIndex, vector<vector<string> > &allTopSents, vector<string> &poem)
	{
		int i, j;
		allTopSents.clear();
		poem.clear();
		vector<string> prevSents, topSents;
		prevSents.push_back(firstSent);
		vector<string> words;
		split(firstSent, " ", words);//如(＂游＂，＂春＂，＂车＂，＂马＂，＂客＂)
		assert(words.size() == 5 || words.size() == 7);
		int pathIndex = words.size() == 5 ? 0 : 1;//决定是５言诗还是7言诗
		//这个循环会去生成一首诗的后3句诗
		for(i = 0; i < DECODER_NUMBER; i ++)
		{
			// load weight first ...
			decoders[i]->loadWeights(weightFilePaths[i][pathIndex].c_str(), false);
			decoders[i]->decodeWithConstraits(prevSents,
					stackSize, topK, tpIndex, topSents);
			if(topSents.size() == 0)
				return false;
			allTopSents.push_back(topSents);

			for(j = 0; j < (int)topSents.size(); j ++)
				trim(topSents[j]);

			bool push = false;
			int minCost = 1000;
			vector<string> goodSents;

			//在这个循环中prevSents只会向其中添加一项或者一项都不添加
			for(j = 0; j < (int)topSents.size(); j ++)
			{
				vector<string> fields;
				split(topSents[j], "|||", fields);
				trim(fields[0]);

				// estimate cost, if there is no good sentence
				//cost越小，一行中字重复的程度越低，诗句越好，下面这一段是要寻找cost最小的诗句
				int cost = this->constraints.containsRep(fields[0]);
				if(cost < minCost)
				{
					minCost = cost;
					goodSents.clear();
					goodSents.push_back(fields[0]);
				}
				else if(cost == minCost)//0b1000
					goodSents.push_back(fields[0]);

				// try
				prevSents.push_back(fields[0]);
				if(this->constraints.isRepetitionOK(prevSents))
				{
					push = true;
					break;
				}
				else
					prevSents.pop_back();
			}

			//如果在之前的循环里一项都没有添加，那么在这里添加一项
			if(!push)
			{
				// int index = rand() % goodSents.size();
				prevSents.push_back(goodSents[0]);
//				vector<string> fields;
//				split(topSents[0], "|||", fields);
//				trim(fields[0]);
				// prevSents.push_back(fields[0]);
			}
		}

		poem = prevSents;

		return true;
	}

public:
	bool generateNextSents(vector<string> prevSents,int tpIndex,vector<string> &allTopSents){
		allTopSents.clear();
		vector<string> words;
		trim(prevSents[0]);
		split(prevSents[0]," ",words);
		assert(words.size()==5||words.size()==7);
		int pathIndex=words.size()==5?0:1;
		int senIndex=prevSents.size()-1;
		decoders[senIndex]->loadWeights(weightFilePaths[senIndex][pathIndex].c_str(), false);
		decoders[senIndex]->decodeWithConstraits(prevSents,stackSize, topK, tpIndex, allTopSents);
		if(allTopSents.size() == 0)
			return false;
		return true;
	}
};


#endif /* SUBSEQUENTSENTENCEGENERATOR_H_ */
