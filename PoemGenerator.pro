HEADERS += \
    src/generator/Decoder.h \
    src/generator/FirstSentenceGenerator.h \
    src/generator/StringBuffer.h \
    src/generator/SubsequentSentenceGenerator.h \
    src/generator/TranslationTable.h \
    src/language_model/KenLMM.h \
    src/language_model/QuatrainPositionProb.h \
    src/language_model/rnnlmlib.h \
    src/language_model/RNNPG.h \
    src/language_model/Vocab.h \
    src/language_model/WordEmbedding.h \
    src/tone_helper/AncChar.h \
    src/tone_helper/Constraints.h \
    src/tone_helper/TonalPattern.h \
    src/tone_helper/ToneRhythm.h \
    src/util/XConfig.h \
    src/util/xutil.h \
    src/generator/PoemGenerator.h

SOURCES += \
    src/generator/Decoder.cpp \
    src/generator/FirstSentenceGenerator.cpp \
    src/generator/StringBuffer.cpp \
    src/generator/SubsequentSentenceGenerator.cpp \
    src/language_model/rnnlmlib.cpp \
    src/language_model/RNNPG.cpp \
    src/language_model/Vocab.cpp \
    src/language_model/WordEmbedding.cpp \
    src/tone_helper/ToneRhythm.cpp \
    src/util/XConfig.cpp \
    src/util/xutil.cpp \
    src/generator/PoemGenerator.cpp \
    src/main.cpp

DISTFILES += \
	.gitignore \
    model/PoemGenerator.conf

INCLUDEPATH +=\
	kenlm

LIBS +=\
	-L/home/tothemoon/Project/PoemGenerator/PoemGenerator/kenlm/lib -lkenlm

QMAKE_CXXFLAGS += -Og -g3 -Wall -Winline -pipe -DKENLM_MAX_ORDER=6
