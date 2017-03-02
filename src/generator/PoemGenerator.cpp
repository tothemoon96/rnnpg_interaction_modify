#include "PoemGenerator.h"
#include "../language_model/rnnlmlib.h"
#include "../util/XConfig.h"
const int PATH_LENGTH = 1024;
PoemGenerator::PoemGenerator()
{
	topK=XConfig::getInt("topK");
	stackSize=XConfig::getInt("stackSize");
	initFirstSentenceGenerator();
	initSubsequentSentenceGenerator();
}

PoemGenerator::~PoemGenerator(){
	if(rnnlm!=NULL)
		delete rnnlm;
	if(model!=NULL)
		delete model;
	if(kenlm!=NULL)
		delete kenlm;
	if(fsg!=NULL)
		delete fsg;
	if(ssg!=NULL)
		delete ssg;
}

void PoemGenerator::initFirstSentenceGenerator(){
	char rnnlmModelPath[PATH_LENGTH];
	char kenlmModelPath[PATH_LENGTH];
	char shixuehanyingPath[PATH_LENGTH];
	char pingshuiyunPath[PATH_LENGTH];
	double rnnlmWeight=XConfig::getDouble("rnnlmWeight");
	xstrcpy(rnnlmModelPath, sizeof(rnnlmModelPath), XConfig::getStr("rnnlm"));
	xstrcpy(kenlmModelPath, sizeof(kenlmModelPath), XConfig::getStr("ngramLM"));
	xstrcpy(shixuehanyingPath, sizeof(shixuehanyingPath), XConfig::getStr("shixuehanying"));
	xstrcpy(pingshuiyunPath, sizeof(pingshuiyunPath), XConfig::getStr("pingshuiyunDir"));

	rnnlm=new CRnnLM;
	rnnlm->setRnnLMFile(rnnlmModelPath);
	rnnlm->restoreNet();
	kenlm=new KenLMM;
	model=new Model(kenlmModelPath);
	kenlm->setModel(model);

	fsg=new FirstSentenceGenerator;
	fsg->setRNNLM(rnnlm);
	fsg->setKenLM(kenlm);
	fsg->loadShixuehanying(shixuehanyingPath);
	fsg->loadPingShuiYun(pingshuiyunPath);
	fsg->setRNNLMWeight(rnnlmWeight);
}

void PoemGenerator::initSubsequentSentenceGenerator(){
	ssg=new SubsequentSentenceGenerator;
}

void PoemGenerator::setSenLen(int senLen){
	this->senLen=senLen;
}

int PoemGenerator::getSenLen(){
	return this->senLen;
}

/**
 * @brief
 *
 * @param keywords 格式有要求，比如［＂空　山＂，＂新　雨＂，＂明　月＂］
 */
bool PoemGenerator::generateFirstSentence(const vector<string> &keywords){
	lastTopSents.clear();
	fsg->getFirstSentence(keywords,topK,senLen,stackSize,lastTopSents);
	if(lastTopSents.size()==0)
		return false;
	return true;
}

bool PoemGenerator::generateNextSentence(){
	lastTopSents.clear();
	return ssg->generateNextSents(prevSents,tpIndex,lastTopSents);
}

vector<string> PoemGenerator::getLastTopSents(){
	return lastTopSents;
}

vector<string> PoemGenerator::getPoem(){
	return prevSents;
}

void PoemGenerator::seletSentInLastTopSents(int index){
	vector<string> fields;
	trim(lastTopSents[index]);
	split(lastTopSents[index], "|||", fields);
	prevSents.push_back(fields[0]);
	//如果是第一句，那么设定整首诗的韵律
	trim(fields[5]);
	if(prevSents.size()==1)
		tpIndex = atoi(fields[5].c_str());
}

void PoemGenerator::reset(){
	prevSents.clear();
	lastTopSents.clear();
}
