// PPMLanguageModel.h
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999-2005 David Ward
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PPMLanguageModel_h__
#define __PPMLanguageModel_h__

#include "../../Common/NoClones.h"
#include "../../Common/Allocators/PooledAlloc.h"

#include "LanguageModel.h"

#include <vector>
#include <fstream>
#include <set>

namespace Dasher {

  ///
  /// \ingroup LM
  /// @{

  ///
  /// PPM language model
  ///

  class CPPMLanguageModel:public CLanguageModel, private NoClones {
  public:
    CPPMLanguageModel(Dasher::CEventHandler * pEventHandler, CSettingsStore * pSettingsStore, const CSymbolAlphabet & alph);

    virtual ~ CPPMLanguageModel();

    Context CreateEmptyContext();
    void ReleaseContext(Context context);
    Context CloneContext(Context context);

    virtual void EnterSymbol(Context context, int Symbol);
    virtual void LearnSymbol(Context context, int Symbol);

    virtual void GetProbs(Context context, std::vector < unsigned int >&Probs, int norm, int iUniform) const;

    void dump();

    virtual int GetMemory() {
      return NodesAllocated;
    }

    class CPPMnode {
    public:
      CPPMnode * find_symbol(int sym)const;
      CPPMnode *child;
      CPPMnode *next;
      CPPMnode *vine;
      unsigned short int count;
      short int symbol;
      CPPMnode(int sym);
      CPPMnode();
    };


    virtual bool WriteToFile(std::string strFilename);
    virtual bool ReadFromFile(std::string strFilename);
    bool RecursiveWrite(CPPMnode *pNode, std::map<CPPMnode *, int> *pmapIdx, int *pNextIdx, std::ofstream *pOutputFile);
    int GetIndex(CPPMnode *pAddr, std::map<CPPMnode *, int> *pmapIdx, int *pNextIdx);
    CPPMnode *GetAddress(int iIndex, std::map<int, CPPMnode*> *pMap);

    class CPPMContext {
    public:
      CPPMContext(CPPMContext const &input) {
        head = input.head;
        order = input.order;
      } CPPMContext(CPPMnode * _head = 0, int _order = 0):head(_head), order(_order) {
      };
      ~CPPMContext() {
      };
      void dump();
      CPPMnode *head;
      int order;
    };

    CPPMnode *AddSymbolToNode(CPPMnode * pNode, int sym, int *update);

    virtual void AddSymbol(CPPMContext & context, int sym);
    void dumpSymbol(int sym);
    void dumpString(char *str, int pos, int len);
    void dumpTrie(CPPMnode * t, int d);

    CPPMContext *m_pRootContext;
    CPPMnode *m_pRoot;

    int m_iMaxOrder;
    double m_dBackOffConstat;

    int NodesAllocated;

    bool bUpdateExclusion;

    mutable CSimplePooledAlloc < CPPMnode > m_NodeAlloc;
    CPooledAlloc < CPPMContext > m_ContextAlloc;

    std::set<const CPPMContext *> m_setContexts;
  };

  /// @}

  inline Dasher::CPPMLanguageModel::CPPMnode::CPPMnode(int sym):symbol(sym) {
    child = next = vine = 0;
    count = 1;
  }

  inline CPPMLanguageModel::CPPMnode::CPPMnode() {
    child = next = vine = 0;
    count = 1;
  }

  inline CLanguageModel::Context CPPMLanguageModel::CreateEmptyContext() {
    CPPMContext *pCont = m_ContextAlloc.Alloc();
    *pCont = *m_pRootContext;

    m_setContexts.insert(pCont);

    return (Context) pCont;
  }

  inline CLanguageModel::Context CPPMLanguageModel::CloneContext(Context Copy) {
    CPPMContext *pCont = m_ContextAlloc.Alloc();
    CPPMContext *pCopy = (CPPMContext *) Copy;
    *pCont = *pCopy;

    m_setContexts.insert(pCont);

    return (Context) pCont;
  }

  inline void CPPMLanguageModel::ReleaseContext(Context release) {

    m_setContexts.erase(m_setContexts.find((CPPMContext *) release));

    m_ContextAlloc.Free((CPPMContext *) release);
  }
}                               // end namespace Dasher

#endif // __LanguageModelling__PPMLanguageModel_h__