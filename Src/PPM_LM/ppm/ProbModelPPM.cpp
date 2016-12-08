
#include "ProbModelPPM.h"
#include "../DasherCore/WinOptions.h"
#include "../DasherCore/Eventhandler.h"
#include <algorithm>

using namespace std;

// Default constructor
ProbModelPPM::ProbModelPPM() : m_ppm(NULL), m_pAlphabet(NULL), m_pTrainer(NULL), m_AlphIO(NULL), m_context(0)
{
}

// Destructor
ProbModelPPM::~ProbModelPPM()
{
    delete m_pSettingsStore;
    m_pSettingsStore = NULL;
    delete m_AlphIO;
    m_AlphIO = NULL;
    delete m_pAlphabet;
    m_pAlphabet = NULL;

    if(m_context != 0) m_ppm->ReleaseContext(m_context);

    delete m_ppm;
    m_ppm = NULL;
}

// loads the alphabet, creates and trains the PPM model
void ProbModelPPM::Init()
{
    m_context = 0;
    m_strPrev = "";

    delete m_pSettingsStore;
    m_pSettingsStore = new CWinOptions("Project78", "PPM", m_pEventHandler);

    SetupPaths();

    vector<string> vAlphabetFiles;

    delete m_AlphIO;
    m_AlphIO = new CAlphIO(GetStringParameter(SP_SYSTEM_LOC), GetStringParameter(SP_USER_LOC), vAlphabetFiles);

    const Dasher::CAlphIO::AlphInfo &oAlphInfo(m_AlphIO->GetInfo("Default"));
    delete m_pAlphabet;
    m_pAlphabet = new CAlphabet(oAlphInfo);

    m_pSettingsStore->SetStringParameter(SP_TRAIN_FILE, m_pAlphabet->GetTrainingFile());

    Dasher::CSymbolAlphabet alphabet(m_pAlphabet->GetNumberTextSymbols());
    alphabet.SetSpaceSymbol(m_pAlphabet->GetSpaceSymbol());
    alphabet.SetAlphabetPointer(m_pAlphabet);

    delete m_ppm;
    m_ppm = new CPPMLanguageModel(m_pEventHandler, m_pSettingsStore, alphabet);

    delete m_pTrainer;
    m_pTrainer = new CTrainer(m_ppm, m_pAlphabet);

    if (!m_pAlphabet->GetTrainingFile().empty())
    {
        m_pTrainer->LoadFile(GetStringParameter(SP_SYSTEM_LOC) + m_pAlphabet->GetTrainingFile());
        m_pTrainer->LoadFile(GetStringParameter(SP_USER_LOC) + m_pAlphabet->GetTrainingFile());
    }
}

void ProbModelPPM::OutputInfoEveryStep(const string& context)
{
    for( int len = 0; len <= context.length(); ++len )
    {
        string temp_str = context.substr(0, len);
        std::cout << "Context: " << temp_str << std::endl;
        GetCharProb(temp_str);
    }
}

// Returns the probability of each character given the current previous context.
void ProbModelPPM::GetCharProb(const string& strContext)
{
    CLanguageModel::Context context = GetContext(strContext);

    double result = 0.0;
    vector<unsigned int> vectorProbs;
    unsigned int norm = PPM_NORM_VALUE;
    m_ppm->GetProbs(context, vectorProbs, norm, 0);

    size_t num_of_zero = 0;
    size_t i = 0;
    for (i = 1 /*care*/; i < vectorProbs.size(); i++)
    {
        unsigned int count = (unsigned int) vectorProbs[i];
        if(count == 0)
        {
            num_of_zero ++;
            count = 1;
        }
        result = (double) count / (double) norm;
        std::cout << m_pAlphabet->GetText(i) << ": " << result << "\n";
    }
}

////////////////////////////////////////// private methods ////////////////////////////////////////////////

// Given a string, go down the trie from the root based on each character's alphabet symbol.
CLanguageModel::Context ProbModelPPM::GetContext(const string& str)
{
    if (m_ppm == NULL) return 0;
    if (m_pAlphabet == NULL) return 0;

    vector<symbol> vectorSymbols;
    if(m_context != 0 && m_strPrev.length() <= str.length() && str.find(m_strPrev) == 0)
    {
        m_pAlphabet->GetSymbols(vectorSymbols, str.substr(m_strPrev.length()));
        for(size_t i = 0; i < vectorSymbols.size(); ++i)
            m_ppm->EnterSymbol(m_context, vectorSymbols[i]);
    }
    else
    {
        if(m_context != 0) m_ppm->ReleaseContext(m_context);
        m_context = m_ppm->CreateEmptyContext();
        vector<symbol> vectorSymbols;
        m_pAlphabet->GetSymbols(vectorSymbols, string(". ") + str); // take care .<SPACE>!!!!!

        for(size_t i = 0; i < vectorSymbols.size(); ++i)
            m_ppm->EnterSymbol(m_context, vectorSymbols[i]);
    }
    m_strPrev = str;
    return m_context;
}

void ProbModelPPM::SetupPaths()
{
  using namespace WinHelper;
  using namespace WinUTF8;

  Tstring UserData, AppData;
  std::string UserData2, AppData2;
  GetUserDirectory(&UserData);
  GetAppDirectory(&AppData);
  UserData += TEXT("dasher.rc\\");
  AppData += TEXT("system.rc\\");
  CreateDirectory(UserData.c_str(), NULL);      // Try and create folders. Doesn't seem
  CreateDirectory(AppData.c_str(), NULL);       // to do any harm if they already exist.
  wstring_to_UTF8string(UserData, UserData2);   // TODO: I don't know if special characters will work.
  wstring_to_UTF8string(AppData, AppData2);     // ASCII-only filenames are safest. Being English doesn't help debug this...
  SetStringParameter(SP_SYSTEM_LOC, AppData2);
  SetStringParameter(SP_USER_LOC, UserData2);
}

void ProbModelPPM::ScanDirectory(const Tstring &strMask, std::vector<std::string> &vFileList)
{
  using namespace WinUTF8;

  std::string filename;
  WIN32_FIND_DATA find;
  HANDLE handle;

  handle = FindFirstFile(strMask.c_str(), &find);
  if(handle != INVALID_HANDLE_VALUE) {
    wstring_to_UTF8string(wstring(find.cFileName), filename);
    vFileList.push_back(filename);
    while(FindNextFile(handle, &find) != false) {
      wstring_to_UTF8string(wstring(find.cFileName), filename);
      vFileList.push_back(filename);
    }
    FindClose(handle);
  }
}
