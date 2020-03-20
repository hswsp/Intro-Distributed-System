#include "stringTokenizer.h"
#include <iostream>

using namespace std;

int CStringTokenizer::GetSize()
{
	return m_tokenList.size();
}

bool CStringTokenizer::HasMoreTokens()
{
	return m_index != m_tokenList.end();
}

string CStringTokenizer::GetNext()
{
	if(m_index!= m_tokenList.end()) 
	{
		return *(m_index++);
	}
	else
	{
		return "";
	}
}

string CStringTokenizer::GetCurrent()
{
	if(m_index!= m_tokenList.end())
	{
		return *(m_index);
	}
	else
	{
		return "";
	}
}

void CStringTokenizer::Split(const string strInput, const string strDelimiter)
{
	//Empty Previous tokens.
	m_tokenList.clear();
	string::size_type lastPos = strInput.find_first_not_of(strDelimiter, 0); //Find the not seperator
	string::size_type pos = strInput.find_first_of(strDelimiter, lastPos); //Find the seperator

	string strToken;

	while(string::npos != pos || string::npos!=lastPos)
	{
		strToken = strInput.substr(lastPos, pos-lastPos);

		m_tokenList.push_back(strToken);
		lastPos = strInput.find_first_not_of(strDelimiter, pos); //Find again
		pos = strInput.find_first_of(strDelimiter, lastPos); //Find again
	}

	m_index = m_tokenList.begin();
}
