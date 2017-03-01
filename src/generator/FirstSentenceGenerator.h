/*
 * FirstSentenceGenerator.h
 *
 *  Created on: 10 Mar 2014
 *      Author: s1270921
 */

#ifndef FIRSTSENTENCEGENERATOR_H_
#define FIRSTSENTENCEGENERATOR_H_

#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <map>
#include <set>
using namespace std;
#include "../language_model/rnnlmlib.h"
#include "../language_model/KenLMM.h"
#include "../util/xutil.h"
#include "../tone_helper/ToneRhythm.h"
#include "../tone_helper/TonalPattern.h"
#include "../tone_helper/Constraints.h"

class FirstSentenceGenerator
{
public:
	FirstSentenceGenerator()
	{
		rnnlm = NULL;
		kenlm = NULL;
		hiddenSize = 0;
		cout << "interpolate weights " << endl;
		for(int i = 0; i < FEATURE_SIZE; i ++)
		{
			interpolateWeights[i] = (double)1 / FEATURE_SIZE;
			cout << interpolateWeights[i] << endl;
		}
	}
	void setRNNLM(CRnnLM *_rnnlm) { rnnlm = _rnnlm; hiddenSize = rnnlm->getHiddenSize(); }
	void setKenLM(KenLMM *_kenlm) { kenlm = _kenlm; }
	void setRNNLMWeight(double rnnWeight)
	{
		interpolateWeights[0] = rnnWeight;
		interpolateWeights[1] = 1 - interpolateWeights[0];
	}
	void loadShixuehanying(const char *infile);
	void loadPingShuiYun(const char *infile);
	void getCandidatePhrase(const vector<string> &keywords, vector<string> &candiPhrase);
	void printShixuehanying();
	void getFirstSentence(const vector<string> &keywords, int topK, int senLen, int MAX_STACK_SIZE, vector<string> &topSents);
private:
	CRnnLM *rnnlm;
	KenLMM *kenlm;
	int hiddenSize;

	//这个map的数据结构：{"不 寝":("衾 寒","灯 残",...,"惊 鹊 未 安 枝"),...}
	map<string,set<string> > shixuehanyingDict;

	enum FSIZE{FEATURE_SIZE = 2};
	double interpolateWeights[FEATURE_SIZE];

	ToneRhythm tr;
	TonalPattern tp;
//	vector<SenTP> firstSenPTs;

	Constraints constraints;

	bool badRepeat(const string &trans, const string &word)
	{

		vector<string> twords, words;
		split(trans, " ", twords);
		split(word, " ", words);
		int begin = twords.size();
		twords.reserve(twords.size() + words.size());
		twords.insert(twords.end(), words.begin(), words.end());

		for(int i = begin; i < (int)twords.size(); i ++)
		{
			int cnt = 0;
			for(int j = 0; j < i; j ++)
				if(twords[j] == twords[i])
					cnt ++;
			if(cnt > 1)
				return true;
			if(cnt == 1 && twords[i] != twords[i-1])
				return true;
		}

		return false;
	}

	double getLMLogProb(string curTrans, vector<string> &curWords, vector<double> &probs);

	static bool strlencmp(const string& s1, const string& s2)
	{
		return s1.length() < s2.length();
	}

	/**
	 * @brief
	 * 如果s1的长度小于s2，返回1,反之返回0
	 * @param s1 字符串
	 * @param s2 字符串
	 * @return bool 返回值
	 */
	static bool phrlencmp(const string& s1, const string& s2)
	{
		return getPhraseLen(s1) < getPhraseLen(s2);
	}

	static int getPhraseLen(const string &s)
	{
		int len = 0;
		for(int i = 0; i < (int)s.length(); i ++)
			if(s[i] == ' ')
				len ++;
		return len + 1;
	}

	/**
	 * remove the </s> symbol in string 's'
	 * e.g. '</s> 空 山 新 雨 后 </s>'  ==> '空 山 新 雨 后'
	 */
	string removeS(string s)
	{
		int left = s.find('>');
		int right = s.rfind( '<' );

		return s.substr(left + 2, right - left - 3);
	}

	class StackItem
	{
	public:
		StackItem(int hsize) : cost(0), hiddenSize(hsize), posInSent(0),
		tonalPattern(0), validPos(0), curTPIdx(0)
		{
			hiddenNeu = new neuron[hiddenSize];
			for(int i = 0; i < FEATURE_SIZE; i ++)
				featVals[i] = 0;
		}
		void ecopy(StackItem &sitem)
		{
			cost = sitem.cost;
			neuron* tmp = hiddenNeu;
			hiddenNeu = sitem.hiddenNeu;
			sitem.hiddenNeu = tmp;
			hiddenSize = sitem.hiddenSize;
			posInSent = sitem.posInSent;
			curTrans = sitem.curTrans;
		}
		void renewHiddenNeu(neuron *newHiddenNeu)
		{
			for(int i = 0; i < hiddenSize; i ++)
				hiddenNeu[i].ac = newHiddenNeu[i].ac;
		}
		/**
		 * @brief
		 * 将StackItem的各个属性分别输出
		 * @param senLen
		 * @param firstSenPTs
		 * @return string
		 */
		string toFirstSentString(int senLen, vector<SenTP> &firstSenPTs)
		{
			string str;
			char buf[128];
			for(int i = 0; i < FEATURE_SIZE; i ++)
			{
				sprintf(buf, " %f", featVals[i]);
				str.append(buf);
			}
			str.append( " ||| " );
			sprintf(buf, "%f", cost);
			str.append(buf);
			str.append( " ||| " + SenTP::inttp2strtp(tonalPattern, validPos, senLen)
			+ " ||| " + firstSenPTs[curTPIdx].toString() + " ||| " );
			sprintf(buf, "%d", curTPIdx);
			str.append(buf);
		/*
			return curSen + " ||| " + curCost + " ||| " + TonalPattern.inttp2strtp(tonalPattern, validPos, curSen.length())
						+ " ||| " + firstSenPTs.get(curTPIdx) + " ||| " + this.curTPIdx;
						*/
			return str;
		}
		void getFeatValString(string &str)
		{
			str.clear();
			char buf[128];
			for(int i = 0; i < FEATURE_SIZE; i ++)
			{
				sprintf(buf, " %f", featVals[i]);
				str.append(buf);
			}
			str.append( " ||| " );
			sprintf(buf, "%f", cost);
			str.append(buf);
		}
		double max2(double a, double b)
		{
			return a > b ? a : b;
		}
		/**
		 * @brief
		 * 使用rnnlm和kenlm对词的生成概率进行综合性的评价，修改cost变量
		 * @param rnnprobs
		 * @param kn3probs
		 * @param interpolateWeights
		 * @param oldCost
		 */
		void interpolate(vector<double> rnnprobs, vector<double> kn3probs, double *interpolateWeights, double oldCost)
		{
			double logProb = 0;
			double zeroProb = 1e-24;
			assert(rnnprobs.size() == kn3probs.size());
			for(size_t i = 0; i < rnnprobs.size(); i ++)
			{
				double rnnprob = max2(rnnprobs[i], zeroProb);
				double kn3prob = max2(kn3probs[i], zeroProb);
				logProb += log(interpolateWeights[0] * rnnprob + interpolateWeights[1] * kn3prob);
			}

			cost = oldCost + logProb;
		}
		void updateCost()
		{
			cost = 0;
			for(int i = 0; i < FEATURE_SIZE; i ++)
				cost += featVals[i];
		}
		void updateCost(double *featWeights)
		{
			cost = 0;
			for(int i = 0; i < FEATURE_SIZE; i ++)
			{
				cost += featVals[i] * featWeights[i];
			}
		}
		~StackItem()
		{
			delete []hiddenNeu;
		}
		static bool stack_item_cmp(StackItem *firstItem, StackItem *secondItem)
		{
			return firstItem->cost > secondItem->cost;
		}
		static bool stack_item_rerank_cmp(StackItem *firstItem, StackItem *secondItem)
		{
			return firstItem->featVals[0] > secondItem->featVals[0];
		}
	// private:
		double cost;//当前sitem的生成概率
		neuron *hiddenNeu;
		int hiddenSize;
		int posInSent;//在一句诗中的位置，从１开始编号
		string curTrans;//保存当前状态下目前生成的内容的字符串形式,包含</s>，如＂</s> 空 上 新 雨 后 </s>＂
		string word;//存储目前生成内容的最后一个字
		vector<string> words;//保存当前状态下目前生成的内容,如（＂空　山＂，＂新　雨＂，＂后＂）不包含首尾的</s>

		int tonalPattern;
		int validPos;
		int curTPIdx;

	//	 add more features: 0. rnnpg_cost; 1. P(Si|Fi) inverted phrase prob; 2. P(Si|Fi) inverted lexical prob
	//	 3. P(Fi|Si) phrase prob; 4. P(Fi|Si) lexical prob;
	//	 5. P(pos|ch) : the probability of a char 'ch' appearing at the position 'pos'
	//	 6. KN3 Language Model feature

		// there is only two features: RNNLM feature and KN3 LM feature
		enum FSIZE{FEATURE_SIZE = 2};
		double featVals[FEATURE_SIZE];
	};

	//bool stack_item_cmp(StackItem *firstItem, StackItem *secondItem);

	class Stack
	{
	public:
		Stack(int msize = 200, int hsize = 200) : maxSize(msize), curSize(0)
		{
			arr = new StackItem*[maxSize];
			for(int i = 0; i < maxSize; i ++)
				arr[i] = NULL;
		}
		// do recombine before push into stack
		void push(StackItem *sitem)
		{
			pq.push(make_pair(sitem->cost, curSize));
			transIndex[sitem->curTrans] = curSize;
			arr[curSize++] = sitem;
		}
		/**
		 * @brief
		 * 检查当前生成的一个片段在这个Stack中是否存在相同的
		 * 如果之前已经存在了相同的片段，则保留概率较大的那一个，返回true
		 * 如果之前不存在相同的片段，则返回false
		 * @param sitem
		 * @return bool
		 */
		bool recombine(StackItem *sitem)
		{
			map<string,int>::iterator iter = transIndex.find(sitem->curTrans);
			if(iter != transIndex.end())
			{
				int index = iter->second;
				if(sitem->cost > arr[index]->cost)
				{
					delete arr[index];
					arr[index] = sitem;
					pq.push(make_pair(sitem->cost, index));
				}
				else
					delete sitem;

				return true;
			}
			return false;
		}
		/**
		 * @brief
		 * 对Stack中的项目进行裁剪,删除掉pq中和arr中不一致的项目
		 * 若Stack中存在生成概率小于sitem的项目，则删除一项Stack生成概率小于sitem的项目，用Sitem将其替换，返回True
		 * 若不存在上述条件的项目或者pq为空，返回False
		 * @param sitem
		 * @return bool
		 */
		bool prune(StackItem *sitem)
		{
			while(!pq.empty())
			{
				P top = pq.top();//从小到大出队
				double cost = top.first;
				int index = top.second;
				//去除实际的cost和pq中cost的不一致性
				if(cost != arr[index]->cost)
				{
					pq.pop();
					continue;
				}
				//如果从pq优先队列里出队的片段生成的概率要小于sitem
				//那么将该项目其替换成sitem
				if(sitem->cost > cost)
				{
					map<string,int>::iterator iter = transIndex.find(arr[index]->curTrans);
					if(iter != transIndex.end())
						transIndex.erase(iter);

					delete arr[index];
					arr[index] = sitem;
					pq.push(make_pair(sitem->cost, index));
					transIndex[sitem->curTrans] = index;

					return true;
				}

				return false;
			}
			// this should not be executed
			cout << "warnning : only when the stack is full that you can use this function" << endl;

			return false;
		}
		/**
		 * @brief
		 * 检查Stack的容量是否已满
		 * @return bool
		 */
		bool isFull() { return curSize >= maxSize; }
		StackItem *pop()
		{
			return arr[--curSize];
		}
		StackItem *get(int i)
		{
			return arr[i];
		}
		int size()
		{
			return curSize;
		}
		/**
		 * @brief
		 * 按照cost由大到小排序
		 */
		void sortByCost()
		{
			if(curSize > 1)
				sort(arr, arr + curSize, StackItem::stack_item_cmp);
		}
		void rerankByRNNPG()
		{
			if(curSize > 1)
				sort(arr, arr + curSize, StackItem::stack_item_rerank_cmp);
		}
		~Stack()
		{
			for(int i = 0; i < curSize; i ++)
				if(arr[i] != NULL) delete arr[i];
			delete []arr;
		}
	private:
		StackItem **arr;//pq这个优先队列是和arr一起维护的
		int maxSize;//StackItem指针的数目，或者说arr这个指针数组里StackItem指针的数目
		int curSize;//保存arr的大小

		typedef pair<double,int> P;//第一个是cost，第二个是index
		priority_queue<P, vector<P>, greater<P> > pq;//由小到大出队列，是一个小根堆
		map<string,int> transIndex;//存储生成某个curTrans时Stack的curSize
	};
};


#endif /* FIRSTSENTENCEGENERATOR_H_ */
