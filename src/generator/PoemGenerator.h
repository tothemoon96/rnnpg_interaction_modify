#ifndef POEMGENERATOR_H
#define POEMGENERATOR_H
#include<vector>
#include<string>

class PoemGenerator
{
private:
	vector<string> prevSents;
	int senLen;
public:
	PoemGenerator();
	void generateFirstSentence(const vector<string> &keywords);
	void generateNextSentence(vector<string> &topNextSents);
	void setSenLen(int senLen);
	int getSenLen();
	void reset();
};

#endif // POEMGENERATOR_H
