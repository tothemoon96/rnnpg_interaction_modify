//============================================================================
// Name        : FirstSentenceGenerator.cpp
// Author      : Xingxing Zhang
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================


#include "FirstSentenceGenerator.h"
/**
 * @brief
 * 将诗学含英的内容映射成一个map<string,set<string>>的map，map的内容举例如下
 * {"不 寝":("衾 寒","灯 残",...,"惊 鹊 未 安 枝"),...}
 * @param infile 诗学含英的文件路径
 */
void FirstSentenceGenerator::loadShixuehanying(const char* infile)
{
	FILE *fin = xfopen(infile, "r", "load shixuehanying...");
	char buf[1024*5];
	while(fgets(buf,sizeof(buf),fin))
	{
		if(buf[0] == '<')// 如果载入的一行是标识符，那么跳过，载入下一行
			continue;
		vector<string> fields;
		//分割字符串，效果如下
		//（＂1＂，＂不 寝＂，＂衾 寒*灯 残*虫 鸣*夜 永*枕 冷*月 落*雁 叫*愁 多*俏 俏*漏 月*寒 月 尽*催 诗 急*饥 鼠 眠 床*不 成 蝶 化*残 砧 数 家 月*空 壁 闻 虫 唧*耿 耿*曼 夜*晓 漏 迟*聒 耳 明*惊 乌 啼 月*怕 听 虫 鸣*疏 柳 几 枝 烟*高 楼 过 雁 声*自 知 吟 更 苦*鳏 鱼 不 闭 目*独 有 坐 如 神*惊 鹊 未 安 枝＂）
		split(buf, "\t\r\n", fields);
		if(fields.size() != 3) continue;//如果不是如上标准格式，则跳过
		set<string> s;
		//在上面的例子里，fields[1]是＂不 寝＂
		shixuehanyingDict[fields[1]] = s;
		map<string,set<string> >::iterator iter = shixuehanyingDict.find(fields[1]);
		vector<string> words;
		split(fields[2], "*", words);
		for(size_t i = 0; i < words.size(); i ++)
			iter->second.insert(words[i]);
	}
	fclose(fin);

	cout << "load shixuehanying done" << endl;
	// printShixuehanying();
}

void FirstSentenceGenerator::printShixuehanying()
{
	map<string,set<string> >::iterator iter;
	int i = 0;
	for(iter = shixuehanyingDict.begin(); iter != shixuehanyingDict.end(); ++ iter)
	{
		cout << "cluster name " << ++ i << " -- " << iter->first << endl;
		set<string>::iterator sIter;
		for(sIter = iter->second.begin(); sIter != iter->second.end(); ++ sIter)
			cout << *sIter << " - ";
		cout << endl;
	}
}

/**
 * @brief
 * 使用keywords中的每个词在shixuehanyingDict中查找相应的键,将它填入到candiPhrase中
 * @param keywords 关键词向量
 * @param candiPhrase 整合了keywords中每个词的查询结果,全部存入了candiPhrase中
 */
void FirstSentenceGenerator::getCandidatePhrase(const vector<string> &keywords, vector<string> &candiPhrase)
{
	set<string> s;
	size_t i;
	for(i = 0; i < keywords.size(); i ++)
	{
		map<string,set<string> >::iterator iter = shixuehanyingDict.find(keywords[i]);
		s.insert(iter->second.begin(), iter->second.end());
	}
	candiPhrase.clear();
	candiPhrase.insert(candiPhrase.end(), s.begin(), s.end());
}

/**
 * @brief
 * 根据关键词生成第一句诗，生成topK句存放在topSents中
 * @param keywords　存储关键词的向量，如（＂空　山＂，＂新　雨＂，＂后＂）
 * @param topK 生成生成概率最大的前topK个句子
 * @param senLen 生成的句子的长度
 * @param stackSize
 * @param topSents 存储生成出的句子的向量
 */
void FirstSentenceGenerator::getFirstSentence(const vector<string> &keywords, int topK, int senLen, int stackSize, vector<string> &topSents)
{
	cout << "interploate weight" << endl;
	cout << interpolateWeights[0] << " -- " << interpolateWeights[1] << endl;
//	cout << "get first sentence " << endl;
	topSents.clear();
	//candiPhrase中存储的是形如("衾 寒","灯 残",...,"惊 鹊 未 安 枝")这样的短语
	vector<string> candiPhrase;
	int i, j, k;
	getCandidatePhrase(keywords, candiPhrase);
	// 按照字符串的长度由小到大进行排序
	sort(candiPhrase.begin(), candiPhrase.end(), phrlencmp);
//	cout << "candi phrase size " << candiPhrase.size() << endl;
//	for(i = 0; i < candiPhrase.size(); i ++)
//		cout << candiPhrase[i] << endl;
	neuron *newHiddenNeu = new neuron[hiddenSize];
	// stacks是一个指向指针数组的指针，这个指针数组的长度为senLen + 1
	Stack **stacks = new Stack*[senLen + 1];//stacks[i],i指示的是已经生成了i个字
	for(i = 0; i < senLen; i ++)
		// stacks中每个元素的构造
		stacks[i] = new Stack(stackSize, hiddenSize);
	//最后一个元素构造的与众不同
	stacks[senLen] = new Stack(topK > stackSize ? topK : stackSize, hiddenSize);

	StackItem *sitem = NULL;
	sitem = new StackItem(hiddenSize);
	sitem->posInSent = 0;
	sitem->curTrans = sitem->word = "</s>";
	//其实第stacks[0]里就存了一个Stack，没有存其他的东西
	stacks[0]->push(sitem);

	// 控制韵律,if you want to use tonal pattern constraints
	vector<SenTP> firstSenPTs;//存储第一句诗要遵守的一些格律要求
	tp.getFirstSenTPs(senLen, firstSenPTs);

	//对一首诗里第一句话里的每个字的位置
	for(i = 0; i < senLen; i ++)
	{
		Stack *nxStack = NULL;
		//对Stack中的每一个StackItem
		for(j = 0; j < stacks[i]->size(); j ++)
		{
			StackItem *curItem = stacks[i]->get(j);
			//对每一个候选短语
			for(k = 0; k < (int)candiPhrase.size(); k ++)
			{
				string phrase = candiPhrase[k];
				vector<string> curWords;//存储candiPhrase的每一个字的vector
				split(phrase, " ", curWords);//把一个短语分割成字符
				//如果生成的当前位置的下标＋curWords.size()超过了一句话的长度，就选择下一个StackItem
				if(i + (int)curWords.size() > senLen)
					break;

				// before doing anything, check the tonal pattern first!
				//在初始时刻，从</s>开始,tonalPattern,validPos,curTPIdx都是0
				int tonalPattern = curItem->tonalPattern;
				int validPos = curItem->validPos;
				int curTPIdx = curItem->curTPIdx;

				//下面循环的功能的举例说明:若当前正在处理的curWords的平仄为P,Z，已经处理到五言诗的第３个词
				//如--PZ?，则
				//validPos    :01100
				//tonalPattern:01000
				//对候选短语中的每一个词
				for(int cIdx = 0; cIdx < (int)curWords.size(); cIdx ++)
				{
					string ch = curWords[cIdx];
					int tone = tr.getTone(ch);

					int base = 1 << (i+cIdx);
					if(tone == PING)
						validPos = validPos | base;
					else if(tone == ZE)
					{
						validPos = validPos | base;
						tonalPattern = tonalPattern | base;
					}
				}

				//为整个第一句诗寻找一个合适的韵律,作为当前的curTPIdx
				for( ; curTPIdx < (int)firstSenPTs.size(); curTPIdx ++)
				{
					SenTP senTP = firstSenPTs[curTPIdx];
					if(senTP.isValidPattern(tonalPattern, validPos))
						break;
				}
				// no suitable tonal pattern
				if(curTPIdx == (int)firstSenPTs.size())
					continue;

				StackItem *nxItem = new StackItem(hiddenSize);

				nxItem->tonalPattern = tonalPattern;
				nxItem->validPos = validPos;
				nxItem->curTPIdx = curTPIdx;

				vector<double> rnnprobs, kn3probs;
				double rnnLogProb = rnnlm->computeNetPhrase(curItem->word.c_str(), curWords,
						curItem->hiddenNeu, newHiddenNeu, rnnprobs);//计算使用rnnlm时词的生成对数概率
				double kenLogProb = getLMLogProb(curItem->curTrans, curWords, kn3probs);//计算使用kenlm时词的生成对数概率


				nxItem->featVals[0] = curItem->featVals[0] + rnnLogProb;
				nxItem->featVals[1] = curItem->featVals[1] + kenLogProb;

				nxItem->interpolate(rnnprobs, kn3probs, interpolateWeights, curItem->cost);

				nxItem->renewHiddenNeu(newHiddenNeu);
				nxItem->posInSent = curItem->posInSent + curWords.size();
				nxItem->word = curWords[curWords.size() - 1];//取curWords的最后一个字作为新状态的word

				// .. record used phrase ..
				nxItem->words = curItem->words;
				nxItem->words.push_back(phrase);

				nxItem->curTrans = curItem->curTrans;
				for(int ii = 0; ii < (int)curWords.size(); ii ++)
					nxItem->curTrans += " " + curWords[ii];

				nxStack = stacks[i + curWords.size()];

				// it is possible to have the same string during decoding, so recombination is needed
				if(nxStack->recombine(nxItem))
					continue;

				// when the stack is full, we need to prune the items，Stack满和不满有不同的处理逻辑
				if(nxStack->isFull())
				{
					if(!nxStack->prune(nxItem))
					{
						delete nxItem;
						// when the current value is less than the smallest element in the stack,
						// then other hyposes should be small as they are sorted by cost
						break;
					}
				}
				else
					nxStack->push(nxItem);
			}
		}
	}

	for(i = 0; i < stacks[senLen]->size(); i ++)
	{
		StackItem *curItem = stacks[senLen]->get(i);
		vector<string> curWords;
		curWords.push_back("</s>");
		//RNNLM检查这一行诗完结的概率
		vector<double> rnnprobs;		
		double rnnLogProb = rnnlm->computeNetPhrase(curItem->word.c_str(), curWords, curItem->hiddenNeu, newHiddenNeu, rnnprobs);
		//KENLM检查这一行诗完结的概率
		vector<double> kn3probs;
		double kenLogProb = getLMLogProb(curItem->curTrans, curWords, kn3probs);

		curItem->featVals[0] = curItem->featVals[0] + rnnLogProb;
		curItem->featVals[1] = curItem->featVals[1] + kenLogProb;
		curItem->interpolate(rnnprobs, kn3probs, interpolateWeights, curItem->cost);
		curItem->curTrans = curItem->curTrans + " </s>";
		curItem->renewHiddenNeu(newHiddenNeu);
	}
	stacks[senLen]->sortByCost();

	topSents.clear();
	char strBuf[1024];
	int cnt = 0;//当前处理的备选句子的个数
	for(i = 0; i < stacks[senLen]->size(); i ++)
	{
		StackItem *sitem = stacks[senLen]->get(i);

//		cout << "before ";
//		printsvec(sitem->words);
		if( this->constraints.isSegmentOK(sitem->words, senLen) )
		{
//			cout << "after ";
//			printsvec(sitem->words);

			string pureSent = removeS(sitem->curTrans);//去除首尾的</s>
			string featValStr;
			// sitem->getFeatValString(featValStr);
			snprintf(strBuf, sizeof(strBuf), " ||| %s |||%s", pureSent.c_str(), sitem->toFirstSentString(senLen, firstSenPTs).c_str());
			topSents.push_back(strBuf);

			cnt ++;
			if(cnt >= topK)
				break;
		}
	}

	delete []newHiddenNeu;
	for(i = 0; i <= senLen; i ++)
		delete stacks[i];
	delete []stacks;
}

double FirstSentenceGenerator::getLMLogProb(string curTrans, vector<string> &curWords, vector<double> &probs)
{
	int ngram = 3;
	vector<string> transWords;
	split(curTrans, " ", transWords);
	vector<string> contexts;
	int i = transWords.size() - ngram + 1;
	i = i >= 0 ? i : 0;
	for( ; i < (int)transWords.size(); i ++)
		contexts.push_back(transWords[i]);
	int pos = contexts.size();
	for(i = 0; i < (int)curWords.size(); i ++)
		contexts.push_back(curWords[i]);

	return kenlm->getProbs(contexts, pos, probs);
}

void FirstSentenceGenerator::loadPingShuiYun(const char* infile)
{
	tr.loadAll(infile);
}
