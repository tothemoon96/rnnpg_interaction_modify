#ifndef POEMGENERATOR_H
#define POEMGENERATOR_H
#include<vector>
#include<string>
#include"../language_model/rnnlmlib.h"
#include"../language_model/KenLMM.h"
#include"FirstSentenceGenerator.h"
#include"SubsequentSentenceGenerator.h"

using namespace std;

class PoemGenerator
{
private:
	vector<string> prevSents;
	vector<string> lastTopSents;
	int senLen;
	int tpIndex;//由生成的第一句诗确定的全诗的韵律

	int topK;
	int stackSize;

	CRnnLM *rnnlm;
	Model *model;
	KenLMM *kenlm;
	FirstSentenceGenerator *fsg;

	SubsequentSentenceGenerator *ssg;

	void initFirstSentenceGenerator();
	void initSubsequentSentenceGenerator();
public:
	PoemGenerator();
	~PoemGenerator();
	bool generateFirstSentence(const vector<string> &keywords);
	bool generateNextSentence();
	void seletSentInLastTopSents(int index);
	vector<string> getLastTopSents();
	vector<string> getPoem();
	void setSenLen(int senLen);
	int getSenLen();
	void reset();
};

#endif // POEMGENERATOR_H
