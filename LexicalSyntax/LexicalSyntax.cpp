#include "p2lex.h"
#include <string>
#include <istream>
#include <fstream>
#include <sstream>
#include <map>
using namespace std;

/// Token Wrapper
Token saved;
bool isSaved = false;
std::map<string, string>Variables;
map<string, string>::iterator it;

Token GetAToken(istream *in) {
	if (isSaved) {
		isSaved = false;
		return saved;
	}

	return getToken(in);
}

void PushbackToken(Token& t) {
	if (isSaved) {
		cerr << "Can't push back more than one token!!!";
		exit(0);
	}

	saved = t;
	isSaved = true;
}

int linenum = 0;
int globalErrorCount = 0;

/// error handler
void error(string msg, bool showline = true)
{
	if (showline)
		cout << linenum << ": ";
	cout << msg << endl;
	++globalErrorCount;
	exit(EXIT_FAILURE);
}
enum Type{ INTEGER, STRING, MULTISTRING, VARIABLE, ERROR };

class Value {
private:
	Type	t;
	std::string	lexeme;
	int i;

public:
	Value() : t(ERROR), lexeme("") {}
	Value(Type t, std::string s) : t(t), lexeme(s) {}
	Value(Type t, int s) : t(t), i(s){}

	Type	getType() const		{ return t; }
	std::string	getLexeme() const	{ return lexeme; }


};
/////////
//// this class can be used to represent the parse result
//// the various different items that the parser might recognize can be
//// subclasses of ParseTree
/// Expression = Term {+Term}
/// Term = Primary {*Primary}
class ParseTree {
private:
	ParseTree *leftChild;
	ParseTree *rightChild;

	int	whichLine;

public:
	ParseTree(ParseTree *left = 0, ParseTree *right = 0) : leftChild(left), rightChild(right) {
		whichLine = linenum;
	}

	int onWhichLine() { return whichLine; }

	int traverseAndCount(int (ParseTree::*f)()) {
		int cnt = 0;
		if (leftChild) cnt += leftChild->traverseAndCount(f);
		if (rightChild) cnt += rightChild->traverseAndCount(f);
		return cnt + (this->*f)();
	}

	int countUseBeforeSet(map<string, int>& symbols) {
		int cnt = 0;
		if (leftChild) cnt += leftChild->countUseBeforeSet(symbols);
		if (rightChild) cnt += rightChild->countUseBeforeSet(symbols);
		return cnt + this->checkUseBeforeSet(symbols);
	}

	virtual int checkUseBeforeSet(map<string, int>& symbols) {
		return 0;
	}

	virtual int isPlus() { return 0; }
	virtual int isStar() { return 0; }
	virtual int isBrack() { return 0; }
	virtual int isEmptyString() { return 0; }
	virtual string getInteger() { return "0"; }
	virtual bool isInteger() { return false; }
	virtual bool isString() { return false; }
	virtual bool isIdentifier() { return false; }
	virtual string getString() { return ""; }
	virtual bool isSet() { return false; }
	virtual bool isPrint() { return false; }
	virtual bool isStatementList() { return false; }
	virtual bool isStatement() { return false; }
	virtual string getType() { return "none"; }
	virtual string getIdentifier() { return ""; }
	virtual string getIdentifierSet() { return "none"; }
	virtual string getSubString() { return "none"; }
	int StringtoInt(string s)
	{
		return stoi(s);
	}

	string InttoString(int s)
	{
		return std::to_string(static_cast<long long int>(s));
	}
	string RemoveQuotes(string s)
	{
		string str = s;

		for (int  i = 0; i < str.size(); i++)
		{
			if (str[i] == '\"')
			{
				str.erase(i,1);
			}
		}

		return str;
	}
	bool IntegerCheck(string& s)
	{
		std::string::const_iterator it = s.begin();
		while (it != s.end() && isdigit(*it)) ++it;
		return !s.empty() && it == s.end();
	}

	//========================================
	int traverse(ParseTree *in)
	{
		if (leftChild->isStatement())
		{
			cout << "Sending the Statement" << endl;
			leftChild->evaluate(in);
			
		}
		if (rightChild->isStatementList())
		{
			cout << "Found more statements" << endl; 
			rightChild->traverse(in);
			rightChild->evaluate(in);
		}
		return 0;
	}
	string evaluate(ParseTree *in)
	{
		string leftvalue, rightvalue, resolvedvalue, varvalue,stringsub;
		int leftint, rightint, result;
		string leftvar;
		Type lefttype, righttype;
		//cout << "Type is: " << this->getType() << endl;
		//cout << "LeftType is: " << leftChild->getType() << endl;
		//cout << "RightType is: " << rightChild->getType() << endl;
		
		if (this->isSet())
		{
			//cout << "============================" << endl << "SET STATEMENT" << endl;
			leftvar = this->getIdentifierSet();
			//cout << "leftvar is: " << leftvar << endl;
			//=================Problem in next line
			varvalue = leftChild->evaluate(in); //Send it back to solve the right side of the tree
			//cout << "Set Found: " << leftvar << endl;
			//cout << "Variable equals " << varvalue << endl;
			Variables.insert(::pair<string, string>(leftvar, varvalue));
			//cout << "::::::::::::::::Inserted::::::::::::::" << leftvar << ":::::::::::::" << varvalue << endl;

			it = Variables.find(leftvar);
			if (it != Variables.end())
			{
				//cout << "Found existing variable" << endl;
				it->second = varvalue;
			}

			/*for (it = Variables.begin(); it != Variables.end(); it++)
			{
				cout << "In loop: " << endl;
				cout << it->first << "================" << it->second << endl;
			}*/
		}
		else if (this->isPrint())
		{
			resolvedvalue = leftChild->evaluate(in);
			resolvedvalue = RemoveQuotes(resolvedvalue);


			cout <<resolvedvalue<<endl;
		}
		
		

		

			if (leftChild)
			{
				//cout << "IN LEFT CHILD" << endl;
				leftvalue = leftChild->evaluate(in);
				//cout << "Left Value retrieved " << leftvalue << endl;
				if (IntegerCheck(leftvalue)) //If is integer
				{
					lefttype = INTEGER;
					//cout << "LEFTTYPE IS INTEGER" << endl;
				}
				else if (!IntegerCheck(leftvalue)) //If its a string
				{
					//cout << "LEFTTYPE IS STRING" << endl;
					leftvalue = RemoveQuotes(leftvalue);
					lefttype = STRING;
				}

			}
			if (rightChild)
			{
				rightvalue = rightChild->evaluate(in);
				//cout << "Right Value retrieved " << rightvalue << endl;

				if (IntegerCheck(rightvalue))
				{
					righttype = INTEGER;
					//cout << "RIGHTTYPE IS INTEGER" << endl;
				}
				else if (!IntegerCheck(rightvalue))
				{
					//cout << "RIGHTTYPE IS STRING" << endl;
					rightvalue = RemoveQuotes(rightvalue);
					righttype = STRING;
				}

			}
		
			if (this->isPlus())
			{
				//cout << "===============In Plus==============" << endl;
				if (lefttype == INTEGER)
				{
					if (righttype == INTEGER)
					{
						leftint = StringtoInt(leftvalue);
						rightint = StringtoInt(rightvalue);
						//cout << endl << "LEFTINT: " << leftint << endl;
						//cout << "RIGHTINT: " << rightint << endl;
						result = leftint + rightint;

						//cout << "Result: " << result << endl;
						resolvedvalue = InttoString(result);

					}
					else
					{
						error("Not same type");
						
					}
				}

				else if (lefttype == STRING)
				{
					if (righttype == STRING)
					{
						resolvedvalue = leftvalue + rightvalue;
						//resolvedvalue = RemoveQuotes(resolvedvalue);
					}
					else
					{
						error("Not same type");
						
					}

				}
				//cout << "==============================" << endl;
			}
			else if (this->isStar())
			{
				if (lefttype == INTEGER)
				{
					if (righttype == INTEGER)
					{
						leftint = StringtoInt(leftvalue);
						rightint = StringtoInt(rightvalue);
						result = leftint * rightint;
						resolvedvalue = InttoString(result);
					}
					else if (righttype == STRING)
					{
						leftint = StringtoInt(leftvalue);
						for (int i = 0; i < leftint; i++)
						{
							resolvedvalue += rightvalue;
						}
					}
					else
					{
						error("Type not supported");
					
					}
				}
				if (lefttype == STRING)
				{
					if (righttype == INTEGER)
					{
						rightint = StringtoInt(rightvalue);
						for (int i = 0; i < rightint; i++)
						{
							resolvedvalue += leftvalue;
						}
					}
					else
					{
						error("Type Multiplication not supported");
					}
				}
			}
			else if (this->isBrack())
			{
				stringsub = this->getSubString();
				stringsub = RemoveQuotes(stringsub);
				leftvalue = leftChild->evaluate(in);

				if (rightChild)
				{
					rightvalue = rightChild->evaluate(in);
					rightint = StringtoInt(rightvalue);
				}
				leftint = StringtoInt(leftvalue);
				

				if (leftint < 0 || leftint > stringsub.length())
				{
					error("Subscript out of range");
				}
				if (rightint<0 || rightint > stringsub.length())
				{
					error("Subscript out of range");
				}
				if (rightint > leftint)
				{
					error("Subscript cannot go backwords");
				}

				if (rightint == 0)
				{
					stringsub = RemoveQuotes(stringsub);
					stringsub = stringsub[leftint];
					resolvedvalue = stringsub;
				}
				else
				{
					stringsub = RemoveQuotes(stringsub);

					stringsub = stringsub.substr(leftint, rightint);
					resolvedvalue = stringsub;
				}
			}
			else if (this->isInteger())
			{
				//cout << "Just a Value ";
				resolvedvalue = this->getInteger();
				//cout << resolvedvalue << endl;
			}
			else if (this->isString())
			{
				resolvedvalue = this->getString();
				resolvedvalue = RemoveQuotes(resolvedvalue);
			}

			else if (this->isIdentifier())
			{
				//cout << "==============IN IDENTIFIER===============" << endl;
				resolvedvalue = this->getIdentifier();
				//cout << "VARIABLE IS: " << resolvedvalue << endl;

				
				//cout << "FINDING VALUE::::::" << Variables.find(resolvedvalue)->second;
				resolvedvalue = Variables.find(resolvedvalue)->second;
				//cout << "VALUE IS: " << resolvedvalue << endl;
				//cout << "===========================================" << endl;
			}

			

			//cout << "Result: " << resolvedvalue << endl;
			return resolvedvalue;
		
		
}


};

class Slist : public ParseTree {
public:
	Slist(ParseTree *left, ParseTree *right) : ParseTree(left, right) {}
	bool isStatementList()
	{
		return true;
	}
	string getType() { return "StatementList"; }
};

class PrintStmt : public ParseTree {
public:
	PrintStmt(ParseTree *expr) : ParseTree(expr) {}

	bool isPrint()
	{
		return true;
	}
	bool isStatement()
	{
		return true;
	}
	string getType() { return "PrintStatement"; }
};

class SetStmt : public ParseTree {
private:
	string	ident;

public:
	SetStmt(string id, ParseTree *expr) : ParseTree(expr), ident(id) {}

	int checkUseBeforeSet(map<string, int>& symbols) {
		symbols[ident]++;
		return 0;
	}
	string getIdentifierSet()
	{
		//cout << "Identity is: " << ident<<endl;
		return ident;
	}
	bool isSet()
	{
		return true;
	}
	bool isStatement()
	{
		return true;
	}
	string getType() {
		return "SetStatement";
	}
};

class PlusOp : public ParseTree {
public:
	PlusOp(ParseTree *left, ParseTree *right) : ParseTree(left, right) {}
	int isPlus() { return 1; }
	string getType(){ return "Plus"; }
};

class StarOp : public ParseTree {
public:
	StarOp(ParseTree *left, ParseTree *right) : ParseTree(left, right) {}
	int isStar() { return 1; }
	string getType()
	{
		return "Star";
	}
};

class BracketOp : public ParseTree {
private:
	Token sTok;

public:
	BracketOp(const Token& sTok, ParseTree *left, ParseTree *right = 0) : ParseTree(left, right), sTok(sTok) {}
	int isBrack() { return 1; }
	string getType() { return "Bracket"; }
	string getSubString() { return sTok.getLexeme(); }

};

class StringConst : public ParseTree {
private:
	Token sTok;

public:
	StringConst(const Token& sTok) : ParseTree(), sTok(sTok) {}

	string	getString() { return sTok.getLexeme(); }

	int isEmptyString() {
		if (sTok.getLexeme().length() == 2) {
			error("Empty string not permitted on line " + to_string(onWhichLine()), false);
			return 1;
		}
		return 0;
	}

	bool isString()
	{
		return true;
	}

	string getType()
	{
		return "String";
	}

};

//// for example, an integer...
class Integer : public ParseTree {
private:
	Token	iTok;

public:
	Integer(const Token& iTok) : ParseTree(), iTok(iTok) {}

	string	getInteger() { return iTok.getLexeme(); }
	bool isInteger() { return true; }

	string getType()
	{
		return "Integer";
	}
};

class Identifier : public ParseTree {
private:
	Token	iTok;

public:
	Identifier(const Token& iTok) : ParseTree(), iTok(iTok) {}

	int checkUseBeforeSet(map<string, int>& symbols) {
		if (symbols.find(iTok.getLexeme()) == symbols.end()) {
			error("Symbol " + iTok.getLexeme() + " used without being set at line " + to_string(onWhichLine()), false);
			return 1;
		}
		return 0;
	}

	bool isIdentifier()
	{
		return true;
	}
	string getType()
	{
		return "Identifier";
	}
	string getIdentifier()
	{
		return iTok.getLexeme();
	}
};

/// function prototypes
ParseTree *Program(istream *in);
ParseTree *StmtList(istream *in);
ParseTree *Stmt(istream *in);
ParseTree *Expr(istream *in);
ParseTree *Term(istream *in);
ParseTree *Primary(istream *in);
ParseTree *String(istream *in);


ParseTree *Program(istream *in)
{
	ParseTree *result = StmtList(in);

	// make sure there are no more tokens...
	if (GetAToken(in).getTok() != DONE)
		return 0;

	return result;
}


ParseTree *StmtList(istream *in)
{
	ParseTree *stmt = Stmt(in);

	if (stmt == 0)
		return 0;

	return new Slist(stmt, StmtList(in));
}


ParseTree *Stmt(istream *in)
{
	Token t;

	t = GetAToken(in);

	if (t.getTok() == ERR) {
		error("Invalid token");
		return 0;
	}

	if (t.getTok() == DONE)
		return 0;

	if (t.getTok() == PRINT) {
		// process PRINT
		ParseTree *ex = Expr(in);

		if (ex == 0) {
			error("Expecting expression after print");
			return 0;
		}

		if (GetAToken(in).getTok() != SC) {
			error("Missing semicolon");
			return 0;
		}

		return new PrintStmt(ex);
	}
	else if (t.getTok() == SET) {
		// process SET
		Token tid = GetAToken(in);

		if (tid.getTok() != ID) {
			error("Expecting identifier after set");
			return 0;
		}

		ParseTree *ex = Expr(in);

		if (ex == 0) {
			error("Expecting expression after identifier");
			return 0;
		}

		if (GetAToken(in).getTok() != SC) {
			error("Missing semicolon");
			return 0;
		}

		return new SetStmt(tid.getLexeme(), ex);
	}
	else {
		error("Syntax error, invalid statement");
	}

	return 0;
}


ParseTree *Expr(istream *in)
{
	ParseTree *exp = Term(in);

	if (exp == 0) return 0;

	while (true) {

		Token t = GetAToken(in);

		if (t.getTok() != PLUS) {
			PushbackToken(t);
			break;
		}

		ParseTree *exp2 = Term(in);
		if (exp2 == 0) {
			error("missing operand after +");
			return 0;
		}

		exp = new PlusOp(exp, exp2);
	}

	return exp;
}


ParseTree *Term(istream *in)
{
	ParseTree *pri = Primary(in);

	if (pri == 0) return 0;

	while (true) {

		Token t = GetAToken(in);

		if (t.getTok() != STAR) {
			PushbackToken(t);
			break;
		}

		ParseTree *pri2 = Primary(in);
		if (pri2 == 0) {
			error("missing operand after *");
			return 0;
		}

		pri = new StarOp(pri, pri2);
	}

	return pri;
}


ParseTree *Primary(istream *in)
{
	Token t = GetAToken(in);

	if (t.getTok() == ID) {
		return new Identifier(t);
	}
	else if (t.getTok() == INT) {
		return new Integer(t);
	}
	else if (t.getTok() == STR) {
		PushbackToken(t);
		return String(in);
	}
	else if (t.getTok() == LPAREN) {
		ParseTree *ex = Expr(in);
		if (ex == 0)
			return 0;
		t = GetAToken(in);
		if (t.getTok() != RPAREN) {
			error("expected right parens");
			return 0;
		}

		return ex;
	}

	return 0;
}


ParseTree *String(istream *in)
{
	Token t = GetAToken(in); // I know it's a string!
	ParseTree *lexpr, *rexpr;

	Token lb = GetAToken(in);
	if (lb.getTok() != LEFTSQ) {
		PushbackToken(lb);
		return new StringConst(t);
	}

	lexpr = Expr(in);
	if (lexpr == 0) {
		error("missing expression after [");
		return 0;
	}

	lb = GetAToken(in);
	if (lb.getTok() == RIGHTSQ) {
		return new BracketOp(t, lexpr);
	}
	else if (lb.getTok() != SC) {
		error("expected ; after first expression in []");
		return 0;
	}

	rexpr = Expr(in);
	if (rexpr == 0) {
		error("missing expression after ;");
		return 0;
	}

	lb = GetAToken(in);
	if (lb.getTok() == RIGHTSQ) {
		return new BracketOp(t, lexpr, rexpr);
	}

	error("expected ]");
	return 0;
}


int main(int argc, char *argv[])
{
	ifstream infile;
	istream *in = &cin;

	//infile.open("p4-test5.in");
	
	ParseTree *root = new ParseTree();
	
	//in = &infile;
	
	switch (argc)
	{
	case 1:

		cout << "Type in your argument: " << endl << endl;
		in = &cin;
		break;
	case 2:

		infile.open(argv[1]);

		if (infile.is_open())
		{
			in = &infile;
			break;
		}
		else
		{
			cerr << "Couldn't open!" << endl;
			return 1;
		}

	}
	root = Program(in);
	map<string, int>symbols;
	int useBeforeSetCount = root->countUseBeforeSet(symbols);


	root->evaluate(root);
	

	return 0;
}