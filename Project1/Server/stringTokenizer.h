#include <vector>
#include <string>

using namespace std;

class CStringTokenizer
{
public:
	CStringTokenizer(){};
	~CStringTokenizer(){};

private:
	vector<string>					m_tokenList;
	vector<string>::iterator		m_index;
		
public:
	int								GetSize();					// token couont
	bool							HasMoreTokens();		// check next token 
	string							GetNext();				// GetNext token
	string 							GetCurrent();
	void							GoFirst(){m_index = m_tokenList.begin();}
	void							Split(const string strInput, const string strDelimiter);				// split string and store in vector
};
