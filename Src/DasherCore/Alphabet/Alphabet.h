// Alphabet.h
//
/////////////////////////////////////////////////////////////////////////////
// Alphabet.h
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2001-2002 David Ward
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __DASHER_ALPHABET_H__
#define __DASHER_ALPHABET_H__

#include "../DasherTypes.h"
#include "GroupInfo.h"

#include <cstdlib>
#include <vector>
#include "AlphabetMap.h"
#include "AlphIO.h"

namespace Dasher {

  ///
  /// \defgroup Alphabet Alphabet information
  /// @{

  class CAlphabet {
  public:


    CAlphabet();

    CAlphabet(const CAlphIO::AlphInfo & AlphInfo);

     ~CAlphabet() {
    };

    // Return size of alphabet, including control symbols
    int GetNumberSymbols() const {
      return m_Characters.size();
    }                           // return size of alphabet
    // Return number of text symbols 
    int GetNumberTextSymbols() const {
      return m_Characters.size() - 1;
    } 

    Opts::ScreenOrientations GetOrientation() {
      return m_Orientation;
    } 

    Opts::AlphabetTypes GetType() {
      return m_DefaultEncoding;
    }

    std::string & GetTrainingFile() {
      return m_TrainingFile;
    }
    std::string GetGameModeFile() {
      return m_GameModeFile;
    }
    std::string & GetPalette() {
      return m_DefaultPalette;
    }

    symbol GetParagraphSymbol() const;
    symbol GetSpaceSymbol() const;
    symbol GetControlSymbol() const;
    //-- Added for Kanji Conversion 13 July 2005 by T.Kaburagi
    symbol GetStartConversionSymbol() const;
    symbol GetEndConversionSymbol() const;

    const std::string & GetDisplayText(symbol i) const {
      return m_Display[i];
    }
    // return display string for i'th symbol

    const std::string & GetText(symbol i) const {
      return m_Characters[i];
    } 
    // return string for i'th symbol

    int GetColour(symbol i) const {
      return m_Colours[i];
    } 
    // return the colour for i'th symbol

/*     int GetGroupColour(int i) const { */
/*       return m_GroupColour[i]; */
/*     }  */
    // return the colour for i'th group 

/*     std::string GetGroupLabel(int i)const { */
/*       return m_GroupLabel[i]; */
/*     } */

    int GetTextColour(symbol i);      // return the foreground colour for i'th symbol
    const std::string & GetForeground(symbol i) const {
      return m_Foreground[i];
    } // return the foreground colour for i'th symbol

/*     int GetGroupCount() const { */
/*       return m_iGroups; */
/*     } int GetGroupStart(int i) const { */
/*       return m_GroupStart[i]; */
/*     } int GetGroupEnd(int i) const { */
/*       return m_GroupEnd[i]; */
/*     } */

    //int get_group(symbol i) const {return m_Group[i];}                
    // return group membership of i'th symbol
    // Fills Symbols with the symbols corresponding to Input. {{{ Note that this
    // is not necessarily reversible by repeated use of GetText. Some text
    // may not be recognised and so discarded. If IsMore is true then Input
    // is truncated to any final characters that were not used due to ambiguous
    // continuation. If IsMore is false Input is assumed to be all the available
    // text and so a symbol will be returned for a final "a" even if "ae" is
    // defined as its own symbol. }}}
    void GetSymbols(std::vector<symbol> *Symbols, std::string * Input, bool IsMore) const;

    void Trace() const;         // diagnostic

    // Add the characters that can appear in Nodes
    void AddChar(std::string NewCharacter, std::string Display, int Colour, std::string Foreground);    // add single char to the alphabet

    // Alphabet language parameters
    void AddParagraphSymbol(std::string NewCharacter, std::string Display, int Colour, std::string Foreground);
    void AddSpaceSymbol(std::string NewCharacter, std::string Display, int Colour, std::string Foreground);
    void AddControlSymbol(std::string NewCharacter, std::string Display, int Colour, std::string Foreground);
    //-- Added for Kanji Conversion 13 July 2005 by T.Kaburagi
    void AddStartConversionSymbol(std::string NewCharacter, std::string Display, int Colour, std::string Foreground);
    void AddEndConversionSymbol(std::string NewCharacter, std::string Display, int Colour, std::string Foreground);

    void SetOrientation(Opts::ScreenOrientations Orientation) {
      m_Orientation = Orientation;
    }
    void SetLanguage(Opts::AlphabetTypes Group) {
      m_DefaultEncoding = Group;
    }
    void SetTrainingFile(std::string TrainingFile) {
      m_TrainingFile = TrainingFile;
    }
    void SetGameModeFile(std::string GameModeFile) {
      m_GameModeFile = GameModeFile;
    }
    void SetPalette(std::string Palette) {
      m_DefaultPalette = Palette;
    }

    const alphabet_map GetAlphabetMap() const {
      return TextMap;
    } 

    const std::string &GetDefaultContext() const {
      return m_strDefaultContext;
    }

    SGroupInfo *m_pBaseGroup;
    
    
  private:

    Opts::AlphabetTypes m_DefaultEncoding;
    Opts::ScreenOrientations m_Orientation;
    symbol m_ParagraphSymbol;
    symbol m_SpaceSymbol;
    symbol m_ControlSymbol;
    //-- Added for Kanji Conversion 13 July 2005 by T.Kaburagi
    symbol m_StartConversionSymbol;
    symbol m_EndConversionSymbol;

    std::string m_TrainingFile;
    std::string m_GameModeFile;
    std::string m_DefaultPalette;

    std::vector < std::string > m_Characters;   // stores the characters
    std::vector < std::string > m_Display;      // stores how the characters are visually represented in the Dasher nodes
    std::vector < int >m_Colours;       // stores the colour of the characters
    std::vector < std::string > m_Foreground;   // stores the colour of the character foreground

    //    int m_iGroups;              // number of groups

/*     std::vector < int >m_GroupStart;    // stores the group start index */
/*     std::vector < int >m_GroupEnd;      // stores the group end index (1 past the last) */
/*     std::vector < int >m_GroupColour;   // stores the colour of the group */
/*     std::vector < std::string > m_GroupLabel; */

    SGroupInfo *pFirstGroup;

    alphabet_map TextMap;

    std::string m_strDefaultContext;

    //    friend class CGroupAdder;
  };

  /// @}

  inline symbol CAlphabet::GetParagraphSymbol() const {
    return m_ParagraphSymbol;
  }

  inline symbol CAlphabet::GetSpaceSymbol() const {
    return m_SpaceSymbol;
  }

  inline symbol CAlphabet::GetControlSymbol() const {
    return m_ControlSymbol;
  }

  inline symbol CAlphabet::GetStartConversionSymbol() const {
    return m_StartConversionSymbol;
  }

  inline symbol CAlphabet::GetEndConversionSymbol() const {
    return m_EndConversionSymbol;
  }
}                              // end namespace dasher

#endif                          // ifndef __DASHER_ALPHABET_H__