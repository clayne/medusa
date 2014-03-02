#include "medusa/label.hpp"
#include <algorithm>
#include <sstream>
#include <boost/format.hpp>

MEDUSA_NAMESPACE_BEGIN

  Label::Label(Address const& rAddress, u16 Type)
  : m_spName(nullptr)
  , m_NameLength()
  , m_Type(Type | AutoGenerated)
  , m_Version()
{
  std::string Prefix;
  switch (Type & CellMask)
  {
  default:
  case Code:     Prefix = "lbl_"; break;
  case Function: Prefix = "fcn_"; break;
  case Data:     Prefix = "dat_"; break;
  case String:   Prefix = "str_"; break;
  }
  std::string LblName = Prefix + rAddress.ToString();
  std::replace(std::begin(LblName), std::end(LblName), ':', '_');
  m_NameLength = LblName.length() + 1;
  m_spName.reset(new char[m_NameLength], [](char *pName) { delete [] pName; });
  memcpy(m_spName.get(), LblName.c_str(), m_NameLength);
}

Label::Label(std::string const& rName, u16 Type, u32 Version)
  : m_spName(nullptr)
  , m_NameLength(static_cast<u16>(rName.length() + 1))
  , m_Type(Type)
  , m_Version(Version)
{
  m_spName.reset(new char[m_NameLength], [](char *pName) { delete [] pName; });
  memcpy(m_spName.get(), rName.c_str(), m_NameLength);
}

Label::~Label(void)
{
}

std::string Label::Dump(void) const
{
  std::ostringstream oss;
  oss << std::hex << std::showbase;

  char TypeBuf[4];
  switch (m_Type & CellMask)
  {
  case Data:     TypeBuf[0] = 'd'; break;
  case Code:     TypeBuf[0] = 'c'; break;
  case String:   TypeBuf[0] = 's'; break;
  default:       TypeBuf[0] = '-'; break;
  }
  switch (m_Type & AccessMask)
  {
  case Imported: TypeBuf[1] = 'i'; break;
  case Exported: TypeBuf[1] = 'e'; break;
  case Global:   TypeBuf[1] = 'g'; break;
  case Local:    TypeBuf[1] = 'l'; break;
  default:       TypeBuf[1] = '-'; break;
  }
  TypeBuf[2] = (m_Type & AutoGenerated) ? 'a' : '-';
  TypeBuf[3] = '\0';

  oss
    << "lbl(" << m_spName
    << " "   << m_NameLength
    << " "   << TypeBuf
    << " "   << m_Version
    << ")";
  return oss.str();
}

std::string Label::GetLabel(void) const
{
  if (m_NameLength == 0x0)
    return "";

  std::string Result;

  if ((m_Type & CellMask) == String)
  {
    size_t Limit = 0x10;
    bool Maj = true;
    for (auto pRawName = m_spName.get(); *pRawName; ++pRawName)
    {
      char CurChr = ConvertToLabel(*pRawName);
      if (CurChr == '\0')
      {
        Maj = true;
        continue;
      }
      CurChr = tolower(CurChr);
      if (Maj)
      {
        Maj = false;
        CurChr = toupper(CurChr);
      }
      Result += CurChr;

      if (!--Limit)
        break;
    }
  }
  else
    for (auto pRawName = m_spName.get(); *pRawName; ++pRawName)
    {
      char CurChr = ConvertToLabel(*pRawName);
      if (CurChr == '\0')
        continue;
      Result += CurChr;
    }

  if (Result.empty())
    Result = "uname";

  if (m_Version == 0)
    return Result;

  return (boost::format("%s.%d") % Result % m_Version).str();
}

void Label::IncrementVersion(void)
{
  ++m_Version;
}

bool Label::IsAutoGenerated(void) const
{
  return (m_Type & AutoGenerated) ? true : false;
}

bool Label::operator<(Label const& rLabel) const
{
  int StrDiff = strcmp(m_spName.get(), rLabel.m_spName.get());

  if (StrDiff == 0)
    return m_Version < rLabel.m_Version;

  return StrDiff < 0;
}

bool Label::operator==(Label const& rLabel) const
{
  return !strcmp(m_spName.get(), rLabel.m_spName.get()) && m_Type == rLabel.m_Type && m_Version == rLabel.m_Version;
}

char Label::ConvertToLabel(char c)
{
  /*
  In VC debug, isalnum can assert...
  src: http://msdn.microsoft.com/en-us/library/k84c0490(v=vs.71).aspx
  When used with a debug CRT library, isalnum will display a CRT assert if passed a parameter that isn't EOF or in the range of 0 through 0xFF.
  When used with a debug CRT library, isalnum will use the parameter as an index into an array, with undefined results if the parameter isn't EOF or in the range of 0 through 0xFF.
  */
  int n = c;
  if (n < 0 || n > 0xff)
    return '\0';
  if (!isalnum(c) && c != '!' && c != ':' && c != '_' && c != '@')
    return '\0';
  return c;
}

MEDUSA_NAMESPACE_END
