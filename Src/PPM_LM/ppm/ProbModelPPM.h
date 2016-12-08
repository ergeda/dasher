// Probabilistic model that uses the Dasher's classic PPM model.
//

#ifndef __PROB_MODEL_PPM_H__
#define __PROB_MODEL_PPM_H__

#include <string>
#include <vector>
#include <math.h>

#include "../DasherCore/DasherInterfaceBase.h"
#include "../DasherCore/LanguageModelling/PPMLanguageModel.h"
#include "../DasherCore/Alphabet/AlphIO.h"
#include "../DasherCore/Alphabet/Alphabet.h"
#include "../DasherCore/wincommon.h"
#include "../DasherCore/Trainer.h"

using std::vector;
using std::string;
using Dasher::CPPMLanguageModel;
using Dasher::CAlphIO;
using Dasher::CAlphabet;
using Dasher::CDasherInterfaceBase;
using Dasher::CTrainer;
using Dasher::CLanguageModel;
using Dasher::symbol;

// Want we use to normalize our PPM calculations (should be big otherwise we might get zero counts for low probability events!)
#define PPM_NORM_VALUE (1 << 16)

class ProbModelPPM : public CDasherInterfaceBase 
{
public:
    ProbModelPPM();
    ~ProbModelPPM();

    void Init();
    void GetCharProb(const string& strContext);
    void OutputInfoEveryStep(const string& context);

private:
    // useful virtual function
    virtual void SetupPaths();
    virtual void ScanAlphabetFiles(std::vector<std::string> &vFileList) {}
    // implement non-use virtual funciton
    virtual void CreateSettingsStore() {}
    virtual int  GetFileSize(const std::string &strFileName) { return 0; }
    virtual void StartTimer() {}
    virtual void ShutdownTimer() {}
    virtual void ScanColourFiles(std::vector<std::string> &vFileList) {}
    virtual void SetupUI() {}
    // private member function
    CLanguageModel::Context GetContext(const string& str);
    void ScanDirectory(const Tstring &strMask, std::vector<std::string> &vFileList);

    // language model
    CPPMLanguageModel* m_ppm;
    CAlphIO* m_AlphIO;
    CAlphabet* m_pAlphabet;
    CTrainer* m_pTrainer;

    // saved context for trie traversal
    CLanguageModel::Context m_context;
    // previous string context
    string m_strPrev;
};

#endif
