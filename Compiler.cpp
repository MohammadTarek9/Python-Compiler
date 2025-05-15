#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <unordered_set>
#include <unordered_map>
#include <regex>

using namespace std;

// ----------------------------------------------
// 1. Token Types
// ----------------------------------------------
enum class TokenType
{
	FalseKeyword,
	NoneKeyword,
	TrueKeyword,
	AndKeyword,
	AsKeyword,
	AssertKeyword,
	AsyncKeyword,
	AwaitKeyword,
	BreakKeyword,
	ClassKeyword,
	ContinueKeyword,
	DefKeyword,
	DelKeyword,
	ElifKeyword,
	ElseKeyword,
	ExceptKeyword,
	FinallyKeyword,
	ForKeyword,
	FromKeyword,
	GlobalKeyword,
	IfKeyword,
	ImportKeyword,
	InKeyword,
	IsKeyword,
	LambdaKeyword,
	NonlocalKeyword,
	NotKeyword,
	OrKeyword,
	PassKeyword,
	RaiseKeyword,
	ReturnKeyword,
	TryKeyword,
	WhileKeyword,
	WithKeyword,
	YieldKeyword,
	IDENTIFIER,
	NUMBER,
	OPERATOR,
	STRING_LITERAL,
	COMMENT,
	UNKNOWN,
	LeftParenthesis,
	RightParenthesis,
	LeftBracket,
	RightBracket,
	LeftBrace,
	RightBrace,
	Colon,
	Comma,
	Dot,
	Semicolon,
	INDENT,
	DEDENT
};

// ----------------------------------------------
// 2. Token Structure
// ----------------------------------------------
struct Token
{
	TokenType type;
	string lexeme;
	int lineNumber;
	string scope;

	Token() {};

	Token(TokenType t, const string &l, int line, const string &s = "")
		: type(t), lexeme(l), lineNumber(line), scope(s) {}
};

// ----------------------------------------------
// 3. Error Structure
// ----------------------------------------------
struct Error
{
	string message;
	int line;
	size_t position;

	void print() const
	{
		cerr << "Error at line " << line << ", position " << position
			 << ": " << message << endl;
	}
};

// printing errors
void printErrors(const vector<Error> &errors)
{
	if (errors.empty())
	{
		cout << "\nNo errors found during tokenization." << endl;
		return;
	}

	cerr << "\nTokenization errors (" << errors.size() << "):" << endl;
	for (const auto &error : errors)
	{
		error.print();
	}
}

// Exception for string handling
class UnterminatedStringError : public std::exception
{
public:
	int line_number;
	size_t index;
	// Constructor that takes line number and index
	UnterminatedStringError(int line, int idx)
		: line_number(line), index(idx) {}
};

class consumeError : public std::exception
{
};

// ----------------------------------------------
// 4. Scope Info Structure
// ----------------------------------------------
struct ScopeInfo
{
	std::string name;
	int indentLevel; // Indentation level when the scope started
};

// ----------------------------------------------
// 5. Symbol Table
// ----------------------------------------------
class SymbolTable
{
public:
	struct SymbolInfo
	{
		int entry;				  // unique entry number
		string type = "unknown";  // e.g., "function", "class", "int", etc.
		string scope = "unknown"; // e.g., "global" or function name
		int firstAppearance = -1; // line of first appearance
		int usageCount = 0;		  // how many times it is referenced

		// A new field to store a literal value if we know it (optional).
		string value;
	};

	unordered_map<string, SymbolInfo> table;
	int nextEntry = 1;

	void addSymbol(const string &name, const string &type,
				   int lineNumber, const string &scope,
				   const string &val = "")
	{
		string uniqueKey = name + "@" + scope;

		auto it = table.find(uniqueKey);
		if (it == table.end())
		{
			SymbolInfo info;
			info.entry = nextEntry++;
			info.type = type;
			info.scope = scope;
			info.firstAppearance = lineNumber;
			info.usageCount = 1;
			info.value = val;
			table[uniqueKey] = info;
		}
		else
		{
			it->second.usageCount++;
			if (it->second.type == "unknown" && type != "unknown")
			{
				it->second.type = type;
			}
			if (!val.empty())
			{
				it->second.value = val;
			}
		}
	}

	// Allows updating a symbol's type after creation.
	void updateType(const string &name, const string &scope, const string &newType)
	{
		string key = name + "@" + scope;
		if (table.find(key) != table.end())
		{
			table[key].type = newType;
		}
	}

	// Allows updating a symbol's literal value after creation.
	void updateValue(const string &name, const string &scope, const string &newValue)
	{
		string key = name + "@" + scope;
		if (table.find(key) != table.end())
		{
			table[key].value = newValue;
		}
	}

	// Retrieve the type of a symbol if it exists
	bool exist(const string &name, const string &scope)
	{
		return table.find(name + "@" + scope) != table.end();
	}

	string getType(const string &name, const string &scope)
	{
		auto it = table.find(name + "@" + scope);
		return it != table.end() ? it->second.type : "unknown";
	}

	string getValue(const string &name, const string &scope)
	{
		auto it = table.find(name + "@" + scope);
		return it != table.end() ? it->second.value : "";
	}

	void printSymbols()
	{
		cout << "Symbol Table:\n";

		// Create a vector of pairs to sort by entry
		vector<pair<string, SymbolInfo>> sortedSymbols(table.begin(), table.end());
		sort(sortedSymbols.begin(), sortedSymbols.end(),
			 [](const pair<string, SymbolInfo> &a, const pair<string, SymbolInfo> &b)
			 {
				 return a.second.entry < b.second.entry;
			 });

		for (auto &[key, info] : sortedSymbols)
		{
			auto at = key.find('@');
			string name = key.substr(0, at);
			string scope = key.substr(at + 1);
			cout << "Entry: " << info.entry
				 << ", Name: " << name
				 << ", Scope: " << scope
				 << ", Type: " << info.type
				 << ", First Appearance: Line " << info.firstAppearance
				 << ", Usage Count: " << info.usageCount;
			if (!info.value.empty())
				cout << ", Value: " << info.value;
			cout << "\n";
		}
	}
};

// ----------------------------------------------
// 6. Lexer (purely lexical analysis)
// ----------------------------------------------
class Lexer
{
public:
	unordered_map<string, TokenType> pythonKeywords = {
		{"False", TokenType::FalseKeyword},
		{"None", TokenType::NoneKeyword},
		{"True", TokenType::TrueKeyword},
		{"and", TokenType::AndKeyword},
		{"as", TokenType::AsKeyword},
		{"assert", TokenType::AssertKeyword},
		{"async", TokenType::AsyncKeyword},
		{"await", TokenType::AwaitKeyword},
		{"break", TokenType::BreakKeyword},
		{"class", TokenType::ClassKeyword},
		{"continue", TokenType::ContinueKeyword},
		{"def", TokenType::DefKeyword},
		{"del", TokenType::DelKeyword},
		{"elif", TokenType::ElifKeyword},
		{"else", TokenType::ElseKeyword},
		{"except", TokenType::ExceptKeyword},
		{"finally", TokenType::FinallyKeyword},
		{"for", TokenType::ForKeyword},
		{"from", TokenType::FromKeyword},
		{"global", TokenType::GlobalKeyword},
		{"if", TokenType::IfKeyword},
		{"import", TokenType::ImportKeyword},
		{"in", TokenType::InKeyword},
		{"is", TokenType::IsKeyword},
		{"lambda", TokenType::LambdaKeyword},
		{"nonlocal", TokenType::NonlocalKeyword},
		{"not", TokenType::NotKeyword},
		{"or", TokenType::OrKeyword},
		{"pass", TokenType::PassKeyword},
		{"raise", TokenType::RaiseKeyword},
		{"return", TokenType::ReturnKeyword},
		{"try", TokenType::TryKeyword},
		{"while", TokenType::WhileKeyword},
		{"with", TokenType::WithKeyword},
		{"yield", TokenType::YieldKeyword}};

	// Some common single/multi/triple-character operators
	unordered_set<string> operators = {
		"+", "-", "*", "/", "%", "//", "**", "=", "==", "!=", "<", "<=", ">",
		">=", "+=", "-=", "*=", "/=", "%=", "//=", "**=", "|", "&", "^", "~", "<<", ">>"};

	// Common delimiters
	unordered_map<char, TokenType> punctuationSymbols = {
		{'(', TokenType::LeftParenthesis},
		{')', TokenType::RightParenthesis},
		{':', TokenType::Colon},
		{',', TokenType::Comma},
		{'.', TokenType::Dot},
		{'[', TokenType::LeftBracket},
		{']', TokenType::RightBracket},
		{'{', TokenType::LeftBrace},
		{'}', TokenType::RightBrace},
		{';', TokenType::Semicolon}};

	vector<ScopeInfo> scopeStack;

	// The tokenize() function produces tokens without modifying the symbol table.
	vector<Token> tokenize(const string &source, vector<Error> &errors)
	{
		vector<Token> tokens;
		int lineNumber = 1;
		size_t i = 0;
		indentStack = {0}; // Reset state
		atLineStart = true;
		lineContinuation = false;

		while (i < source.size())
		{
			// Handle indentation at the start of a line (if not a continuation)
			if (atLineStart && !lineContinuation)
			{
				processIndentation(source, i, lineNumber, tokens, errors);
				atLineStart = false;
			}

			skipNonLeadingWhitespace(source, i);

			if (i >= source.size())
				break;

			char c = source[i];

			// Handle newlines and reset flags
			if (c == '\n')
			{
				lineNumber++;
				i++;
				atLineStart = true;
				lineContinuation = false; // Reset continuation
				continue;
			}

			// Check for line continuation (backslash before newline)
			if (c == '\\' && i + 1 < source.size() && source[i + 1] == '\n')
			{
				lineContinuation = true;
				i += 2; // Skip both '\' and '\n'
				lineNumber++;
				atLineStart = true;
				continue;
			}

			// Handle single-line comments (# ...)
			if (c == '#')
			{
				while (i < source.size() && source[i] != '\n')
				{
					i++;
				}
				continue;
			}

			int startlineNumber = lineNumber;
			try
			{
				string triplestring = handleTripleQuotedString(source, i, lineNumber);
				// Handle triple-quoted strings
				if (triplestring.length())
				{
					tokens.push_back(Token(
						TokenType::STRING_LITERAL,
						triplestring,
						startlineNumber));
					continue;
				}
			}
			catch (const UnterminatedStringError &e)
			{
				errors.push_back({"Unterminated triple-quoted string", e.line_number, e.index});
				continue;
			}

			// Identify keywords and identifiers
			if (isalpha(static_cast<unsigned char>(c)) || c == '_')
			{
				size_t start = i;
				while (i < source.size() &&
					   (isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_'))
				{
					i++;
				}
				string word = source.substr(start, i - start);
				if (pythonKeywords.find(word) != pythonKeywords.end())
				{
					// change the scope if it is a function or class
					if (word == "def" || word == "class")
					{
						tokens.push_back(Token(pythonKeywords[word], word, lineNumber));
						skipNonLeadingWhitespace(source, i);
						size_t identifierStart = i;
						while (i < source.size() && (isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_'))
						{
							i++;
						}
						if (identifierStart < i)
						{
							string identifier = source.substr(identifierStart, i - identifierStart);
							scopeStack.push_back({identifier, indentStack.back()});
							// cout<<"Current scope: " << scopeStack << endl;
							tokens.push_back(Token(TokenType::IDENTIFIER, identifier, lineNumber, getScope(scopeStack)));
						}
					}
					else
					{
						tokens.push_back(Token(pythonKeywords[word], word, lineNumber));
					}
				}
				else
				{
					tokens.push_back(Token(TokenType::IDENTIFIER, word, lineNumber, getScope(scopeStack)));
					// cout<< "scope of " << word << " is " << scopeStack << endl;
				}
				continue;
			}

			if (isOperatorStart(c))
			{
				if ((i + 2) < source.size())
				{
					string threeChars = source.substr(i, 3);
					if (operators.find(threeChars) != operators.end())
					{
						tokens.push_back(Token(TokenType::OPERATOR, threeChars, lineNumber));
						i += 3;
						continue;
					}
				}
				if ((i + 1) < source.size())
				{
					string twoChars = source.substr(i, 2);
					if (operators.find(twoChars) != operators.end())
					{
						tokens.push_back(Token(TokenType::OPERATOR, twoChars, lineNumber));
						i += 2;
						continue;
					}
				}
				string oneChar(1, c);
				if (operators.find(oneChar) != operators.end())
				{
					tokens.push_back(Token(TokenType::OPERATOR, oneChar, lineNumber));
					i++;
					continue;
				}
			}

			// Handle string literals with error checking
			if (c == '"' || c == '\'')
			{
				try
				{
					string str = handleDoubleQuotedString(source, i, lineNumber);
					tokens.push_back(Token(
						TokenType::STRING_LITERAL,
						str,
						lineNumber));
				}
				catch (const UnterminatedStringError &e)
				{
					errors.push_back({"Unterminated string literal", e.line_number, e.index});
				}
				continue;
			}

			// Handle numeric literals
			if (isdigit(static_cast<unsigned char>(c)))
			{
				size_t start = i;
				bool hasDot = false;
				while (i < source.size() && (isdigit(source[i]) || source[i] == '.'))
				{
					if (source[i] == '.' && hasDot)
						break;
					else if (source[i] == '.')
						hasDot = true;
					i++;
				}
				string num = source.substr(start, i - start);
				if (num[0] == '0' && std::stoi(num) != 0 && !hasDot)
				{
					errors.push_back({"leading zeros in decimal integer literals are not permitted", lineNumber, start});
					continue;
				}
				tokens.push_back(Token(TokenType::NUMBER, num, lineNumber));
				continue;
			}

			// Handle punctuation symbols
			if (punctuationSymbols.find(c) != punctuationSymbols.end())
			{
				tokens.push_back(Token(punctuationSymbols[c], string(1, c), lineNumber));
				i++;
				continue;
			}

			// Unknown character - add error but keep going
			errors.push_back({"Invalid character '" + string(1, c) + "'", lineNumber, i});
			i++;
			atLineStart = false;
		}

		// Add DEDENT tokens for remaining indentation levels at EOF
		while (indentStack.size() > 1)
		{
			indentStack.pop_back();
			tokens.push_back(Token(TokenType::DEDENT, "", lineNumber));
		}

		return tokens;
	}

private:
	vector<int> indentStack = {0}; // Track indentation levels (e.g., [0, 4, 8])
	bool atLineStart = true;	   // Flag for newline handling
	bool lineContinuation = false; // Track line continuation via '\'

	void skipNonLeadingWhitespace(const string &source, size_t &idx)
	{
		static const regex ws_regex(R"(^[ \t\r]+)");
		smatch match;

		if (idx >= source.size())
			return;

		string remaining = source.substr(idx);
		if (regex_search(remaining, match, ws_regex, regex_constants::match_continuous))
		{
			idx += match.length(); // Advance past matched whitespace
		}
	}

	string handleTripleQuotedString(const string &source, size_t &idx, int &lineNumber)
	{
		int start_line = lineNumber;
		if (idx + 2 < source.size())
		{
			char c = source[idx];
			if ((c == '"' || c == '\'') &&
				source[idx + 1] == c &&
				source[idx + 2] == c)
			{
				char quoteChar = c;
				size_t start = idx;
				idx += 3; // skip opening triple quotes
				while (idx + 2 < source.size())
				{
					if (source[idx] == '\\')
					{
						idx++; // Skip the escape character (actual handling depends on your needs)
					}
					else if (source[idx] == '\n')
					{
						lineNumber++;
						idx++;
					}
					else if (source[idx] == '\r' && idx + 1 < source.size() && source[idx + 1] == '\n')
					{
						lineNumber++;
						idx++; // Skip \r\n
					}
					if (source[idx] == quoteChar &&
						source[idx + 1] == quoteChar &&
						source[idx + 2] == quoteChar)
					{
						idx += 3;								  // skip closing triple quotes
						return source.substr(start, idx - start); // Include closing quotes
					}
					idx++;
				}
				// If we get here, the string was never closed
				idx = source.size();
				throw UnterminatedStringError(start_line, start);
			}
		}
		return "";
	}

	bool isOperatorStart(char c)
	{
		regex operatorRegex("[~+\\-*/%=!<>&|^]");
		return regex_match(string(1, c), operatorRegex);
	}

	string handleDoubleQuotedString(const string &source, size_t &idx, int &lineNumber)
	{
		int start_line = lineNumber;
		if (idx < source.size())
		{
			char quoteChar = source[idx];
			size_t start = idx;
			idx++; // skip opening quote
			while (idx < source.size())
			{
				if (source[idx] == '\\')
				{
					idx++; // Skip the escape character (actual handling depends on your needs)
				}
				else if (source[idx] == '\n')
				{
					idx++;
					throw UnterminatedStringError(start_line, start);
				}
				else if (source[idx] == quoteChar)
				{
					idx++;
					return source.substr(start, idx - start); // Include closing quotes
				}
				idx++;
			}
			// If we get here, the string was never closed
			throw UnterminatedStringError(start_line, start);
		}
		throw UnterminatedStringError(start_line, idx);
	}

	void processIndentation(const string &source, size_t &i, int lineNumber,
							vector<Token> &tokens, vector<Error> &errors)
	{
		size_t start = i;
		int spaces = 0, tabs = 0;

		// Count leading spaces/tabs
		while (i < source.size() && (source[i] == ' ' || source[i] == '\t'))
		{
			if (source[i] == ' ')
				spaces++;
			else
				tabs++;
			i++;
		}

		// Error: Mixed tabs and spaces
		if (spaces > 0 && tabs > 0)
		{
			errors.push_back({"Mixed tabs and spaces in indentation", lineNumber, start});
		}
		if (source[i] == '\n')
		{
			return;
		}
		// Calculate indentation level (1 tab = 4 spaces, adjust as needed)
		int newIndent = tabs * 4 + spaces;

		// Compare with current indentation
		if (newIndent > indentStack.back())
		{
			indentStack.push_back(newIndent);
			tokens.push_back(Token(TokenType::INDENT, "", lineNumber));
		}
		else if (newIndent < indentStack.back())
		{
			// Pop until matching indentation level
			while (indentStack.back() > newIndent)
			{
				indentStack.pop_back();
				tokens.push_back(Token(TokenType::DEDENT, "", lineNumber));
				// Pop scope ONLY if dedenting past its original indentation level
				while (!scopeStack.empty() && indentStack.back() <= scopeStack.back().indentLevel)
				{
					scopeStack.pop_back();
				}
				if (indentStack.empty())
				{
					errors.push_back({"Dedent exceeds indentation level", lineNumber, start});
					indentStack.push_back(0); // Recover
					break;
				}
			}
			// Error: No matching indentation level
			if (indentStack.back() != newIndent)
			{
				errors.push_back({"Unindent does not match outer level", lineNumber, start});
			}
		}
		// Equal indentation: Do nothing
	}

	string getScope(const vector<ScopeInfo> &scopeStack)
	{
		if (scopeStack.empty())
		{
			return "global";
		}
		else
		{
			string hierarchy = scopeStack.back().name;
			for (auto it = scopeStack.rbegin() + 1; it != scopeStack.rend(); ++it)
			{
				if (!scopeStack.empty())
				{
					hierarchy += "@";
				}
				hierarchy += it->name;
			}

			return hierarchy;
		}
	}
};

// ----------------------------------------------
// 7. Parser for basic type inference
// ----------------------------------------------
class Parser
{
public:
	Parser(const vector<Token> &tokens, SymbolTable &symTable)
		: tokens(tokens), symbolTable(symTable) {}

	void parse()
	{
		size_t i = 0;
		while (i < tokens.size())
		{
			const Token &tk = tokens[i];

			if (tk.type == TokenType::DefKeyword || tk.type == TokenType::ClassKeyword)
			{
				lastKeyword = tk.lexeme;
				i++;
			}
			else if (tk.type == TokenType::IDENTIFIER)
			{
				// If last keyword was 'def' or 'class', then this is a new function/class name
				if (lastKeyword == "def")
				{
					symbolTable.addSymbol(tk.lexeme, "function", tk.lineNumber, tk.scope);
					lastKeyword.clear();
					i++;
				}
				else if (lastKeyword == "class")
				{
					symbolTable.addSymbol(tk.lexeme, "class", tk.lineNumber, tk.scope);
					lastKeyword.clear();
					i++;
				}
				else
				{
					// handle multiple assignment like x,y = 2,3 -> assigns x = 2 and y = 3
					size_t temp = i;
					vector<Token> lhsIdentifiers;
					while (temp < tokens.size())
					{
						if (tokens[temp].type == TokenType::IDENTIFIER)
						{
							lhsIdentifiers.push_back(tokens[temp]);
							temp++;
							if (temp < tokens.size() && tokens[temp].type == TokenType::Comma)
							{
								temp++;
							}
							else
							{
								break;
							}
						}
						else
						{
							break;
						}
					}

					if (temp < tokens.size() && tokens[temp].type == TokenType::OPERATOR && tokens[temp].lexeme == "=")
					{
						temp++;
						vector<pair<string, string>> rhsValues;
						while (temp < tokens.size())
						{
							auto [type, value] = parseExpression(temp);
							rhsValues.push_back({type, value});
							if (temp < tokens.size() && tokens[temp].type == TokenType::Comma)
							{
								temp++;
							}
							else
							{
								break;
							}
						}

						for (size_t j = 0; j < lhsIdentifiers.size(); ++j)
						{
							const Token &var = lhsIdentifiers[j];
							string key = var.lexeme + "@" + var.scope;
							if (!symbolTable.exist(var.lexeme, var.scope))
							{
								symbolTable.addSymbol(var.lexeme, "unknown", var.lineNumber, var.scope);
							}
							else
							{
								symbolTable.table[key].usageCount++;
							}
							if (j < rhsValues.size())
							{
								if (rhsValues[j].first != "unknown")
								{
									symbolTable.updateType(var.lexeme, var.scope, rhsValues[j].first);
								}
								if (!rhsValues[j].second.empty())
								{
									symbolTable.updateValue(var.lexeme, var.scope, rhsValues[j].second);
								}
							}
						}
						i = temp;
						continue;
					}
					// Check if next token is '=' (assignment)
					if ((i + 1) < tokens.size() &&
						tokens[i + 1].type == TokenType::OPERATOR &&
						tokens[i + 1].lexeme == "=")
					{
						// We have "identifier = ..."
						const string &lhsName = tk.lexeme;
						int lineNumber = tk.lineNumber;
						// Add symbol if not exist
						string fullName = lhsName + "@" + tk.scope;
						if (!symbolTable.exist(fullName, tk.scope))
						{
							symbolTable.addSymbol(lhsName, "unknown", lineNumber, tk.scope);
						}
						else
						{
							// If it exists, usage count will increment
							symbolTable.table[lhsName + "@" + tk.scope].usageCount++;
						}
						i += 2; // skip past "identifier" and "="
						auto [rhsType, rhsValue] = parseExpression(i);

						// Update the LHS symbol with the inferred type/value
						if (rhsType != "unknown")
						{
							symbolTable.updateType(lhsName, tk.scope, rhsType);
						}
						if (!rhsValue.empty())
						{
							symbolTable.updateValue(lhsName, tk.scope, rhsValue);
						}
					}
					else
					{

						if (symbolTable.exist(tk.lexeme, tk.scope))
						{
							symbolTable.table[tk.lexeme + "@" + tk.scope].usageCount++;
						}

						else
						{
							symbolTable.addSymbol(tk.lexeme, "unknown", tk.lineNumber, tk.scope);
						}
						i++;
					}
				}
			}
			else
			{
				// We ignore other tokens (operators, delimiters, etc.) for now
				i++;
			}
		}
	}

private:
private:
	const vector<Token> &tokens;
	SymbolTable &symbolTable;
	string lastKeyword;

	// ------------------------------------------------------
	// parseExpression:
	// Parses a simple expression with multiple operands, e.g.
	//   y + 20 + z
	// We unify the types of each operand as we go.
	// We'll keep it very basic: no parentheses, no precedence.
	// We'll return the final type and a single literal value only
	// if the entire expression is a single literal. Otherwise, "".
	// ------------------------------------------------------
	pair<string, string> parseExpression(size_t &i)
	{
		// Parse the first operand
		auto [accumType, accumValue] = parseOperand(i);
		while (i < tokens.size())
		{
			if (tokens[i].type == TokenType::OPERATOR)
			{
				string op = tokens[i].lexeme;
				if (op == "+" || op == "-" || op == "*" || op == "/")
				{
					i++;
					auto [nextType, nextValue] = parseOperand(i);
					accumType = unifyTypes(accumType, nextType);
					accumValue = "";
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		return {accumType, accumValue};
	}

	// ------------------------------------------------------
	// parseOperand:
	// Returns (type, literalValue) for a single operand,
	// advancing 'i' by one or more tokens.
	// This re-uses the same logic from a simplified version
	// of "inferTypeOfRHS" but for a single operand only.
	// ------------------------------------------------------
	pair<string, string> parseOperand(size_t &i)
	{
		if (i >= tokens.size())
		{
			return {"unknown", ""};
		}

		const Token &tk = tokens[i];

		// If it's a numeric literal
		if (tk.type == TokenType::NUMBER)
		{
			// check if there's a '.' => float
			if (tk.lexeme.find('.') != string::npos)
			{
				i++;
				return {"float", tk.lexeme};
			}
			else
			{
				i++;
				return {"int", tk.lexeme};
			}
		}

		// If it's a string literal
		if (tk.type == TokenType::STRING_LITERAL)
		{
			i++;
			return {"string", tk.lexeme};
		}

		// If it's a keyword => might be True/False
		if (tk.type == TokenType::FalseKeyword || tk.type == TokenType::TrueKeyword)
		{
			if (tk.lexeme == "True" || tk.lexeme == "False")
			{
				i++;
				return {"bool", tk.lexeme};
			}
			i++;
			return {"unknown", ""};
		}

		// If it's an identifier
		if (tk.type == TokenType::IDENTIFIER)
		{
			string name = tk.lexeme;
			string knownType = symbolTable.getType(name, tk.scope);
			string knownValue = symbolTable.getValue(name, tk.scope);
			string fullName = name + "@" + tk.scope;
			if (!symbolTable.exist(fullName, tk.scope))
			{
				symbolTable.addSymbol(name, "unknown", tk.lineNumber, tk.scope);
			}
			else
			{
				symbolTable.table[fullName].usageCount++;
			}
			i++;
			return {knownType, knownType == "unknown" ? "" : knownValue};
		}

		// if it's a tuple
		if (tk.lexeme == "(")
		{
			string value = "(";
			i++;
			vector<string> elementTypes;
			vector<string> elementValues;

			while (i < tokens.size() && tokens[i].lexeme != ")")
			{
				auto [innerType, innerValue] = parseExpression(i);
				elementTypes.push_back(innerType);
				elementValues.push_back(innerValue);

				if (i < tokens.size() && tokens[i].lexeme == ",")
				{
					value += innerValue + ",";
					i++; // Skip the comma
				}
				else
				{
					value += innerValue;
					break;
				}
			}

			if (i < tokens.size() && tokens[i].lexeme == ")")
			{
				i++;
				value += ")";
				if (elementTypes.size() == 1)
				{
					// Single element in parentheses, treat as the element itself
					return {elementTypes[0], value};
				}
				else
				{
					// Multiple elements, treat as a tuple
					return {"tuple", value};
				}
			}
			else
			{
				// If no closing parenthesis, return unknown
				return {"unknown", value};
			}
		}

		// if it's a list
		if (tk.lexeme == "[")
		{
			string value = "[";
			i++;
			while (i < tokens.size() && tokens[i].lexeme != "]")
			{
				value = value + tokens[i].lexeme;
				i++;
			}
			if (i < tokens.size() && tokens[i].lexeme == "]")
			{
				i++;
			}
			value = value + "]";
			return {"list", value};
		}

		// if it's a dictionary or set
		if (tk.lexeme == "{")
		{
			string value = "{";
			i++;
			bool isSet = true;
			while (i < tokens.size() && tokens[i].lexeme != "}")
			{
				if (tokens[i].lexeme == ":")
				{
					isSet = false;
				}
				value = value + tokens[i].lexeme;
				i++;
			}
			if (i < tokens.size() && tokens[i].lexeme == "}")
			{
				i++;
			}
			value = value + "}";
			return {isSet ? "set" : "dictionary", value};
		}

		// Otherwise unknown
		i++;
		return {"unknown", ""};
	}

	// ------------------------------------------------------
	// unifyTypes:
	// A minimal "unify" function for numeric/string/bool/unknown.
	// - if either is "float", result => "float"
	// - if one is "int" and the other "int", => "int"
	// - if one is "unknown", => the other (or "unknown" if both unknown)
	// - if there's a conflict (e.g., "string" + "int"), => "unknown"
	// Expand this if you want to handle more complex rules
	// ------------------------------------------------------
	string unifyTypes(const string &t1, const string &t2)
	{
		if (t1 == "unknown" && t2 == "unknown")
			return "unknown";
		if (t1 == "unknown")
			return t2;
		if (t2 == "unknown")
			return t1;

		// If either is float => float
		if (t1 == "float" || t2 == "float")
		{
			// If one is string and other is float => unknown, or you can define a rule
			if (t1 == "string" || t2 == "string" ||
				t1 == "bool" || t2 == "bool")
			{
				return "unknown";
			}
			return "float";
		}

		// If both are int => int
		if (t1 == "int" && t2 == "int")
		{
			return "int";
		}

		// If both are bool => bool (some languages let you do bool+bool => int, but we'll keep it simple)
		if (t1 == "bool" && t2 == "bool")
		{
			return "bool";
		}

		if (t1 == "string" && t2 != "string")
		{
			return "unknown";
		}

		if (t2 == "string" && t1 != "string")
		{
			return "unknown";
		}

		// If they're the same, return it
		if (t1 == t2)
		{
			return t1;
		}

		// Anything else => unknown
		return "unknown";
	}
};

// ----------------------------------------------
// Syntax_Analyzer
// ----------------------------------------------

std::string tokenTypeToString(TokenType type)
{
	static const std::unordered_map<TokenType, std::string> typeStrings = {
		// Keywords
		{TokenType::FalseKeyword, "False"},
		{TokenType::NoneKeyword, "None"},
		{TokenType::TrueKeyword, "True"},
		{TokenType::AndKeyword, "and"},
		{TokenType::AsKeyword, "as"},
		{TokenType::AssertKeyword, "assert"},
		{TokenType::AsyncKeyword, "async"},
		{TokenType::AwaitKeyword, "await"},
		{TokenType::BreakKeyword, "break"},
		{TokenType::ClassKeyword, "class"},
		{TokenType::ContinueKeyword, "continue"},
		{TokenType::DefKeyword, "def"},
		{TokenType::DelKeyword, "del"},
		{TokenType::ElifKeyword, "elif"},
		{TokenType::ElseKeyword, "else"},
		{TokenType::ExceptKeyword, "except"},
		{TokenType::FinallyKeyword, "finally"},
		{TokenType::ForKeyword, "for"},
		{TokenType::FromKeyword, "from"},
		{TokenType::GlobalKeyword, "global"},
		{TokenType::IfKeyword, "if"},
		{TokenType::ImportKeyword, "import"},
		{TokenType::InKeyword, "in"},
		{TokenType::IsKeyword, "is"},
		{TokenType::LambdaKeyword, "lambda"},
		{TokenType::NonlocalKeyword, "nonlocal"},
		{TokenType::NotKeyword, "not"},
		{TokenType::OrKeyword, "or"},
		{TokenType::PassKeyword, "pass"},
		{TokenType::RaiseKeyword, "raise"},
		{TokenType::ReturnKeyword, "return"},
		{TokenType::TryKeyword, "try"},
		{TokenType::WhileKeyword, "while"},
		{TokenType::WithKeyword, "with"},
		{TokenType::YieldKeyword, "yield"},

		// Literals and identifiers
		{TokenType::IDENTIFIER, "identifier"},
		{TokenType::NUMBER, "number"},
		{TokenType::STRING_LITERAL, "string literal"},
		{TokenType::COMMENT, "comment"},
		{TokenType::UNKNOWN, "unknown"},

		// Operators and punctuation
		{TokenType::OPERATOR, "operator"},
		{TokenType::LeftParenthesis, "("},
		{TokenType::RightParenthesis, ")"},
		{TokenType::LeftBracket, "["},
		{TokenType::RightBracket, "]"},
		{TokenType::LeftBrace, "{"},
		{TokenType::RightBrace, "}"},
		{TokenType::Colon, ":"},
		{TokenType::Comma, ","},
		{TokenType::Dot, "."},
		{TokenType::Semicolon, ";"},

		// Indentation
		{TokenType::INDENT, "indent"},
		{TokenType::DEDENT, "dedent"}};

	auto it = typeStrings.find(type);
	if (it != typeStrings.end())
	{
		return it->second;
	}
	return "unknown token"; // Fallback for any unhandled types
}

class ParseTreeNode
{
public:
	string label;
	vector<ParseTreeNode *> children;
	Token token; // For leaf nodes

	ParseTreeNode(string lbl) : label(lbl) {}
	void addChild(ParseTreeNode *child)
	{
		children.push_back(child);
	}
};

class Syntax_Analyzer
{
private:
	size_t current = 0;

	Token currentToken() { return tokens[current]; }

public:
	vector<Token> tokens;

	void error(const string &message)
	{
		cerr << "Syntax Error at line " << currentToken().lineNumber
			 << ": " << message << endl;
	}

	Token consume(TokenType expected)
	{
		if (current >= tokens.size())
		{
			return Token(TokenType::UNKNOWN, "Error", -1, "Error");
		}
		if (currentToken().type == expected)
		{
			return tokens[current++]; // Match: return token and advance
		}
		error("Expected " + tokenTypeToString(expected) +
			  " but found " + tokenTypeToString(currentToken().type));
		throw consumeError();
	}

	void synchronize(int lineNumber)
	{
		// Error recovery - skip until synchronization point
		while (current < tokens.size() &&
			   currentToken().lineNumber <= lineNumber)
		{
			current++;
		}
	}

	Token &peekToken()
	{
		static Token dummy(TokenType::UNKNOWN, "", -1, "");
		return (current + 1 < tokens.size()) ? tokens[current + 1] : dummy;
	}

	ParseTreeNode *parseProgram()
	{
		ParseTreeNode *programNode = new ParseTreeNode("program");
		while (current < tokens.size())
		{
			try
			{
				if (current != 0 &&
					currentToken().lineNumber <= tokens[current - 1].lineNumber &&
					tokens[current - 1].type != TokenType::DEDENT)
				{
					error("Statements must be separated by NEWLINE");
					synchronize(currentToken().lineNumber);
					continue;
				}
				if (currentToken().type == TokenType::DEDENT)
				{
					consume(TokenType::DEDENT);
				}
				if (currentToken().type == TokenType::DefKeyword)
					programNode->addChild(parseFunction());
				else
					programNode->addChild(parseStatement());
			}
			catch (const consumeError &)
			{
				synchronize(currentToken().lineNumber);
			}
		}

		return programNode;
	}

	ParseTreeNode *parseFunction()
	{
		ParseTreeNode *funcNode = new ParseTreeNode("function");
		try
		{
			funcNode->addChild(new ParseTreeNode(consume(TokenType::DefKeyword).lexeme));
			funcNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
			funcNode->addChild(new ParseTreeNode(consume(TokenType::LeftParenthesis).lexeme));
			funcNode->addChild(parseParameters());
			funcNode->addChild(new ParseTreeNode(consume(TokenType::RightParenthesis).lexeme));
			funcNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
			funcNode->addChild(parseBlock());
		}
		catch (const consumeError &)
		{
			error("Could not parse function");
			throw consumeError();
		}
		return funcNode;
	}

	ParseTreeNode *parseParameters()
	{
		ParseTreeNode *paramsNode = new ParseTreeNode("parameters");
		try
		{
			if (current < tokens.size() && currentToken().type != TokenType::RightParenthesis)
			{
				paramsNode->addChild(parseParameter());

				while (current < tokens.size() && currentToken().type == TokenType::Comma)
				{
					paramsNode->addChild(new ParseTreeNode(consume(TokenType::Comma).lexeme));
					paramsNode->addChild(parseParameter());
				}
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse paramters");
			throw consumeError();
		}
		return paramsNode;
	}

	ParseTreeNode *parseParameter()
	{
		ParseTreeNode *paramNode = new ParseTreeNode("parameter");
		try
		{
			paramNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
			if (current < tokens.size() && currentToken().type == TokenType::OPERATOR && currentToken().lexeme == "=")
			{
				paramNode->addChild(new ParseTreeNode(consume(TokenType::OPERATOR).lexeme));
				paramNode->addChild(parseExpression());
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse parameter");
			throw consumeError();
		}
		return paramNode;
	}

	ParseTreeNode *parseStatement()
	{
		ParseTreeNode *stmtNode = new ParseTreeNode("statement");
		try
		{
			switch (currentToken().type)
			{
			case TokenType::IDENTIFIER:
			{
				// Look ahead to decide: dotted name or function call or assignment
				Token next = peekToken();
				if (next.type == TokenType::LeftParenthesis)
				{
					// Simple function call
					stmtNode->addChild(parseFunctionCall());
				}
				else if (next.type == TokenType::Dot)
				{
					// Look further: after dotted name, is it '(' → function call, or '=' → assignment?
					size_t temp = current;
					ParseTreeNode *dotted = parseDottedName();
					if (current < tokens.size())
					{
						if (currentToken().type == TokenType::LeftParenthesis)
						{
							// Reset and parse as function call
							current = temp;
							stmtNode->addChild(parseFunctionCall());
						}
						else if (currentToken().type == TokenType::OPERATOR && currentToken().lexeme == "=")
						{
							// Reset and parse as assignment
							current = temp;
							stmtNode->addChild(parseAssignmentStmt());
						}
						else
						{
							error("Expected '(' or '=' after dotted name");
							throw consumeError();
						}
					}
					else
					{
						error("Unexpected end after dotted name");
						throw consumeError();
					}
				}
				else
				{
					stmtNode->addChild(parseAssignmentStmt());
				}
				break;
			}

			case TokenType::WhileKeyword:
				stmtNode->addChild(parseWhileStmt());
				break;
			case TokenType::ForKeyword:
				stmtNode->addChild(parseForStmt());
				break;
			case TokenType::IfKeyword:
				stmtNode->addChild(parseConditionalStmt());
				break;
			case TokenType::ClassKeyword:
				stmtNode->addChild(parseClassDef());
				break;
			case TokenType::ImportKeyword:
			case TokenType::FromKeyword:
				stmtNode->addChild(parseImport());
				break;
			case TokenType::ReturnKeyword:
				stmtNode->addChild(parseReturn());
				break;
			case TokenType::PassKeyword:
				stmtNode->addChild(parsePass());
				break;
			case TokenType::BreakKeyword:
				stmtNode->addChild(parseBreak());
				break;
			case TokenType::ContinueKeyword:
				stmtNode->addChild(parseContinue());
				break;
			case TokenType::RaiseKeyword:
				stmtNode->addChild(parseRaise());
				break;
			case TokenType::TryKeyword:
				stmtNode->addChild(parseTryStmt());
				break;
			case TokenType::STRING_LITERAL:
				stmtNode->addChild(parseFactor());
				break;
			default:
				error("Cannot Parse Statement!");
				throw consumeError();
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse statement");
			throw consumeError();
		}
		return stmtNode;
	}

	ParseTreeNode *parseReturn()
	{
		ParseTreeNode *returnNode = new ParseTreeNode("return_statement");
		returnNode->addChild(new ParseTreeNode(consume(TokenType::ReturnKeyword).lexeme));
		try
		{
			returnNode->addChild(parseExpression());
		}
		catch (const consumeError &)
		{
			error("Could not parse return statement");
			throw consumeError();
		}
		return returnNode;
	}

	ParseTreeNode *parsePass()
	{
		ParseTreeNode *passNode = new ParseTreeNode("pass_statement");
		try
		{
			passNode->addChild(new ParseTreeNode(consume(TokenType::PassKeyword).lexeme));
		}
		catch (const consumeError &)
		{
			error("Could not parse pass statement");
			throw consumeError();
		}
		return passNode;
	}

	ParseTreeNode *parseBreak()
	{
		ParseTreeNode *breakNode = new ParseTreeNode("break_statement");
		try
		{
			breakNode->addChild(new ParseTreeNode(consume(TokenType::BreakKeyword).lexeme));
		}
		catch (const consumeError &)
		{
			error("Could not parse break");
			throw consumeError();
		}
		return breakNode;
	}

	ParseTreeNode *parseContinue()
	{
		ParseTreeNode *continueNode = new ParseTreeNode("continue_statement");
		try
		{
			continueNode->addChild(new ParseTreeNode(consume(TokenType::ContinueKeyword).lexeme));
		}
		catch (const consumeError &)
		{
			error("Could not continue statement");
			throw consumeError();
		}
		return continueNode;
	}

	ParseTreeNode *parseClassBlock()
	{
		ParseTreeNode *classBlockNode = new ParseTreeNode("class_block");
		int prevLine = currentToken().lineNumber;

		try
		{
			consume(TokenType::INDENT);
			classBlockNode->addChild(new ParseTreeNode("INDENT"));

			while (current < tokens.size() && currentToken().type != TokenType::DEDENT)
			{
				if (currentToken().type == TokenType::DefKeyword)
				{
					classBlockNode->addChild(parseFunction());
				}
				else
				{
					classBlockNode->addChild(parseAssignmentStmt());
				}

				// Prevent same-line statements
				if (currentToken().lineNumber <= prevLine)
				{
					error("Class members must be on separate lines");
					synchronize(currentToken().lineNumber);
					continue;
				}
				prevLine = currentToken().lineNumber;
			}

			classBlockNode->addChild(new ParseTreeNode("DEDENT"));
			consume(TokenType::DEDENT);
		}
		catch (const consumeError &)
		{
			error("Could not parse class block");
			throw consumeError();
		}

		return classBlockNode;
	}

	ParseTreeNode *parseBlock()
	{
		ParseTreeNode *blockNode = new ParseTreeNode("block");
		bool isSingleLine = (currentToken().type != TokenType::INDENT);
		int prevLine = currentToken().lineNumber;
		try
		{
			if (!isSingleLine)
			{
				consume(TokenType::INDENT);
				blockNode->addChild(new ParseTreeNode("INDENT"));
				if (currentToken().type == TokenType::DefKeyword)
				{
					blockNode->addChild(parseFunction());
				}
				else
					blockNode->addChild(parseStatement());
				while (current < tokens.size() && currentToken().type != TokenType::DEDENT)
				{
					if (currentToken().lineNumber <= prevLine)
					{
						error("Statements must be separated by NEWLINE");
						synchronize(currentToken().lineNumber);
						continue;
					}
					prevLine = currentToken().lineNumber;
					blockNode->addChild(parseStatement());
				}
				try
				{
					blockNode->addChild(new ParseTreeNode("DEDENT"));
					consume(TokenType::DEDENT);
				}
				catch (const consumeError &)
				{
					return blockNode;
				}
			}
			else
			{
				blockNode->addChild(parseStatement());
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse block");
			throw consumeError();
		}
		return blockNode;
	}

	ParseTreeNode *parseWhileStmt()
	{
		ParseTreeNode *whileNode = new ParseTreeNode("while_statement");
		try
		{
			whileNode->addChild(new ParseTreeNode(consume(TokenType::WhileKeyword).lexeme));
			whileNode->addChild(parseExpression());
			whileNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
			whileNode->addChild(parseBlock());
		}
		catch (const consumeError &)
		{
			error("Could not parse while statement");
			throw consumeError();
		}
		return whileNode;
	}

	ParseTreeNode *parseForStmt()
	{
		ParseTreeNode *forNode = new ParseTreeNode("for_statement");
		try
		{
			forNode->addChild(new ParseTreeNode(consume(TokenType::ForKeyword).lexeme));
			forNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
			forNode->addChild(new ParseTreeNode(consume(TokenType::InKeyword).lexeme));
			forNode->addChild(parseExpression());
			forNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
			forNode->addChild(parseBlock());
		}
		catch (const consumeError &)
		{
			error("Could not parse for statement");
			throw consumeError();
		}
		return forNode;
	}

	ParseTreeNode *parseImport()
	{
		ParseTreeNode *importNode = new ParseTreeNode("import_statement");
		try
		{
			if (currentToken().type == TokenType::ImportKeyword)
			{
				importNode->addChild(new ParseTreeNode(consume(TokenType::ImportKeyword).lexeme));
				importNode->addChild(parseDottedName());
				if (currentToken().type == TokenType::AsKeyword)
				{
					importNode->addChild(new ParseTreeNode(consume(TokenType::AsKeyword).lexeme));
					importNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
				}
				while (currentToken().type == TokenType::Comma)
				{
					importNode->addChild(new ParseTreeNode(consume(TokenType::Comma).lexeme));
					importNode->addChild(parseDottedName());
					if (currentToken().type == TokenType::AsKeyword)
					{
						importNode->addChild(new ParseTreeNode(consume(TokenType::AsKeyword).lexeme));
						importNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
					}
				}
			}
			else
			{
				importNode->addChild(new ParseTreeNode(consume(TokenType::FromKeyword).lexeme));
				importNode->addChild(parseDottedName());
				importNode->addChild(new ParseTreeNode(consume(TokenType::ImportKeyword).lexeme));
				if (currentToken().type == TokenType::IDENTIFIER)
				{
					importNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
					if (currentToken().type == TokenType::AsKeyword)
					{
						importNode->addChild(new ParseTreeNode(consume(TokenType::AsKeyword).lexeme));
						importNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
					}
				}
				else if (currentToken().lexeme == "*")
					importNode->addChild(new ParseTreeNode(consume(TokenType::OPERATOR).lexeme));
				else
					throw new consumeError();
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse import");
			throw consumeError();
		}
		return importNode;
	}

	ParseTreeNode *parseDottedName()
	{
		ParseTreeNode *dottedNode = new ParseTreeNode("dotted_name");
		try
		{
			dottedNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
			while (currentToken().type == TokenType::Dot)
			{
				dottedNode->addChild(new ParseTreeNode(consume(TokenType::Dot).lexeme));
				dottedNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse dotted name");
			throw consumeError();
		}
		return dottedNode;
	}

	ParseTreeNode *parseRaise()
	{
		ParseTreeNode *raiseNode = new ParseTreeNode("raise_statement");
		try
		{
			raiseNode->addChild(new ParseTreeNode(consume(TokenType::RaiseKeyword).lexeme));
			raiseNode->addChild(parseExpression());
		}
		catch (const consumeError &)
		{
			error("Could not parse raise statement");
			throw consumeError();
		}
		return raiseNode;
	}

	ParseTreeNode *parseTryStmt()
	{
		ParseTreeNode *tryNode = new ParseTreeNode("try_statement");
		try
		{
			tryNode->addChild(new ParseTreeNode(consume(TokenType::TryKeyword).lexeme));
			tryNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
			tryNode->addChild(parseBlock());

			while (current < tokens.size() && currentToken().type == TokenType::ExceptKeyword)
			{
				auto *exceptNode = new ParseTreeNode("except_clause");
				exceptNode->addChild(new ParseTreeNode(consume(TokenType::ExceptKeyword).lexeme));
				if (currentToken().type == TokenType::IDENTIFIER)
				{
					exceptNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
					if (currentToken().type == TokenType::AsKeyword)
					{
						exceptNode->addChild(new ParseTreeNode(consume(TokenType::AsKeyword).lexeme));
						exceptNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
					}
				}
				exceptNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
				exceptNode->addChild(parseBlock());
				tryNode->addChild(exceptNode);
			}

			if (current < tokens.size() && currentToken().type == TokenType::ElseKeyword)
			{
				auto *elseNode = new ParseTreeNode("else_clause");
				elseNode->addChild(new ParseTreeNode(consume(TokenType::ElseKeyword).lexeme));
				elseNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
				elseNode->addChild(parseBlock());
				tryNode->addChild(elseNode);
			}

			if (current < tokens.size() && currentToken().type == TokenType::FinallyKeyword)
			{
				auto *finallyNode = new ParseTreeNode("finally_clause");
				finallyNode->addChild(new ParseTreeNode(consume(TokenType::FinallyKeyword).lexeme));
				finallyNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
				finallyNode->addChild(parseBlock());
				tryNode->addChild(finallyNode);
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse try statement");
			throw consumeError();
		}

		return tryNode;
	}

	ParseTreeNode *parseClassDef()
	{
		ParseTreeNode *classNode = new ParseTreeNode("class_def");
		try
		{
			classNode->addChild(new ParseTreeNode(consume(TokenType::ClassKeyword).lexeme));
			classNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));

			if (currentToken().type == TokenType::LeftParenthesis)
			{

				classNode->addChild(new ParseTreeNode(consume(TokenType::LeftParenthesis).lexeme));
				classNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
				classNode->addChild(new ParseTreeNode(consume(TokenType::RightParenthesis).lexeme));
			}

			classNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
			classNode->addChild(parseClassBlock());
		}
		catch (const consumeError &)
		{
			error("Could not parse class def");
			throw consumeError();
		}
		return classNode;
	}

	ParseTreeNode *parseConditionalStmt()
	{

		auto *ifNode = new ParseTreeNode("conditional_statement");
		try
		{
			ifNode->addChild(new ParseTreeNode(consume(TokenType::IfKeyword).lexeme));
			ifNode->addChild(parseExpression());
			ifNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
			ifNode->addChild(parseBlock());
		}
		catch (const consumeError &)
		{
			error("Could not parse conditional");
			throw consumeError();
		}

		while (current < tokens.size() && currentToken().type == TokenType::ElifKeyword)
		{
			auto *elifNode = new ParseTreeNode("elif_clause");
			try
			{
				elifNode->addChild(new ParseTreeNode(consume(TokenType::ElifKeyword).lexeme));
				elifNode->addChild(parseExpression());
				elifNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
				elifNode->addChild(parseBlock());
				ifNode->addChild(elifNode);
			}
			catch (const consumeError &)
			{
				error("Could not parse elif");
				throw consumeError();
			}
		}

		if (current < tokens.size() && currentToken().type == TokenType::ElseKeyword)
		{
			auto *elseNode = new ParseTreeNode("else_clause");
			try
			{
				elseNode->addChild(new ParseTreeNode(consume(TokenType::ElseKeyword).lexeme));
				elseNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
				elseNode->addChild(parseBlock());
				ifNode->addChild(elseNode);
			}
			catch (const consumeError &)
			{
				error("Could not parse else");
				throw consumeError();
			}
		}

		return ifNode;
	}

	ParseTreeNode *parseAssignmentStmt()
	{
		ParseTreeNode *assignNode = new ParseTreeNode("assignment");
		try
		{
			// Parse LHS identifiers or dotted names
			ParseTreeNode *lhs = new ParseTreeNode("lhs");

			// Handle first identifier or dotted name
			if (peekToken().type == TokenType::Dot)
				lhs->addChild(parseDottedName());
			else
				lhs->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));

			// Support multiple identifiers: x, y = ...
			while (current < tokens.size() && currentToken().type == TokenType::Comma)
			{
				lhs->addChild(new ParseTreeNode(consume(TokenType::Comma).lexeme));
				if (peekToken().type == TokenType::Dot)
					lhs->addChild(parseDottedName());
				else
					lhs->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
			}
			assignNode->addChild(lhs);

			// Assign operator (=, +=, etc.)
			assignNode->addChild(parseAssignOp());

			// Parse RHS expressions
			ParseTreeNode *rhs = new ParseTreeNode("rhs");
			rhs->addChild(parseExpression());
			while (current < tokens.size() && currentToken().type == TokenType::Comma)
			{
				rhs->addChild(new ParseTreeNode(consume(TokenType::Comma).lexeme));
				rhs->addChild(parseExpression());
			}
			assignNode->addChild(rhs);
		}
		catch (const consumeError &)
		{
			error("Could not parse assignment");
			throw consumeError();
		}

		return assignNode;
	}

	ParseTreeNode *parseAssignOp()
	{
		ParseTreeNode *assignOpNode = new ParseTreeNode("Assign_OP");
		try
		{
			if (current < tokens.size() && currentToken().type == TokenType::OPERATOR)
			{
				if (currentToken().lexeme == "=" ||
					currentToken().lexeme == "+=" ||
					currentToken().lexeme == "-=" ||
					currentToken().lexeme == "*=" ||
					currentToken().lexeme == "/=" ||
					currentToken().lexeme == "%=" ||
					currentToken().lexeme == "//=" ||
					currentToken().lexeme == "**=")
				{
					assignOpNode->addChild(new ParseTreeNode(consume(TokenType::OPERATOR).lexeme));
				}
			}
			else
				throw consumeError();
		}
		catch (const consumeError &)
		{
			error("Could not parse Assign OP");
			throw consumeError();
		}
		return assignOpNode;
	}

	ParseTreeNode *parseFunctionCall()
	{
		ParseTreeNode *callNode = new ParseTreeNode("function_call");
		try
		{
			if (peekToken().type == TokenType::Dot)
			{
				callNode->addChild(parseDottedName());
			}
			else
			{
				callNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));
			}

			callNode->addChild(new ParseTreeNode(consume(TokenType::LeftParenthesis).lexeme));

			if (currentToken().type != TokenType::RightParenthesis)
			{
				callNode->addChild(parseArguments());
			}

			callNode->addChild(new ParseTreeNode(consume(TokenType::RightParenthesis).lexeme));
			
			if (current < tokens.size() && currentToken().type == TokenType::IfKeyword)
			{
				callNode->addChild(new ParseTreeNode(consume(TokenType::IfKeyword).lexeme));
				callNode->addChild(parseOrExpr());
				callNode->addChild(new ParseTreeNode(consume(TokenType::ElseKeyword).lexeme));
				callNode->addChild(parseFunctionCall());
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse function call");
			throw consumeError();
		}
		return callNode;
	}

	ParseTreeNode *parseExpression()
	{
		ParseTreeNode *exprNode = new ParseTreeNode("expression");
		try
		{
			exprNode->addChild(parseOrExpr());
			
			if (current < tokens.size() && currentToken().type == TokenType::IfKeyword)
			{
				exprNode->addChild(new ParseTreeNode(consume(TokenType::IfKeyword).lexeme));
				exprNode->addChild(parseOrExpr());
				exprNode->addChild(new ParseTreeNode(consume(TokenType::ElseKeyword).lexeme));
				exprNode->addChild(parseExpression());
			}
			
		}
		catch (const consumeError &)
		{
			error("Could not parse expression");
			throw consumeError();
		}
		return exprNode;
	}

	ParseTreeNode *parseOrExpr()
	{
		ParseTreeNode *orNode = new ParseTreeNode("or_expression");
		try
		{
			orNode->addChild(parseAndExpr());
			while (current < tokens.size() && currentToken().type == TokenType::OrKeyword)
			{
				orNode->addChild(new ParseTreeNode(consume(TokenType::OrKeyword).lexeme));
				orNode->addChild(parseAndExpr());
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse or expression");
			throw consumeError();
		}
		return orNode;
	}

	ParseTreeNode *parseAndExpr()
	{
		ParseTreeNode *andNode = new ParseTreeNode("and_expression");
		try
		{
			andNode->addChild(parseNotExpr());
			while (current < tokens.size() && currentToken().type == TokenType::AndKeyword)
			{
				andNode->addChild(new ParseTreeNode(consume(TokenType::AndKeyword).lexeme));
				andNode->addChild(parseNotExpr());
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse and expression");
			throw consumeError();
		}
		return andNode;
	}

	ParseTreeNode *parseNotExpr()
	{
		ParseTreeNode *notNode = new ParseTreeNode("not_expression");
		try
		{
			if (current < tokens.size() && currentToken().type == TokenType::NotKeyword)
			{
				notNode->addChild(new ParseTreeNode(consume(TokenType::NotKeyword).lexeme));
				notNode->addChild(parseNotExpr());
			}
			else
			{
				notNode->addChild(parseComparison());
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse not expression");
			throw consumeError();
		}
		return notNode;
	}

	ParseTreeNode *parseComparison()
	{
		ParseTreeNode *compNode = new ParseTreeNode("comparison");
		try
		{
			compNode->addChild(parseArithmetic());
			while (current < tokens.size() && currentToken().type == TokenType::OPERATOR)
			{
				compNode->addChild(parseOp());
				compNode->addChild(parseArithmetic());
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse comparison");
			throw consumeError();
		}
		return compNode;
	}

	ParseTreeNode *parseOp()
	{
		ParseTreeNode *assignOpNode = new ParseTreeNode("OP");
		if (currentToken().lexeme == "==" ||
			currentToken().lexeme == "!=" ||
			currentToken().lexeme == "<" ||
			currentToken().lexeme == ">" ||
			currentToken().lexeme == ">=" ||
			currentToken().lexeme == "<=" ||
			currentToken().lexeme == "&" ||
			currentToken().lexeme == "|")

		{
			try
			{
				assignOpNode->addChild(new ParseTreeNode(consume(TokenType::OPERATOR).lexeme));
			}
			catch (const consumeError &)
			{
				error("Could not COMP OP");
				throw consumeError();
			}
		}
		return assignOpNode;
	}

	ParseTreeNode *parseArithmetic()
	{
		ParseTreeNode *arithmNode = new ParseTreeNode("arithmetic");
		try
		{
			arithmNode->addChild(parseTerm());
			while (current < tokens.size() && currentToken().type == TokenType::OPERATOR)
			{
				if (currentToken().lexeme == "+" || currentToken().lexeme == "-")
				{
					arithmNode->addChild(new ParseTreeNode(consume(TokenType::OPERATOR).lexeme));
					arithmNode->addChild(parseTerm());
				}
				else
					break;
			}
		}
		catch (const consumeError &)
		{
			error("Could not Arithmetic");
			throw consumeError();
		}
		return arithmNode;
	}

	ParseTreeNode *parseTerm()
	{
		ParseTreeNode *termNode = new ParseTreeNode("term");
		try
		{
			termNode->addChild(parseFactor());
			while (current < tokens.size() && currentToken().type == TokenType::OPERATOR)
			{
				if (currentToken().lexeme == "*" || currentToken().lexeme == "/" || currentToken().lexeme == "%")
				{
					termNode->addChild(new ParseTreeNode(consume(TokenType::OPERATOR).lexeme));
					termNode->addChild(parseFactor());
				}
				else
					break;
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse Term");
			throw consumeError();
		}
		return termNode;
	}

	ParseTreeNode *parseArguments()
	{
		ParseTreeNode *argNode = new ParseTreeNode("arguments");
		try
		{
			argNode->addChild(parseExpression());
			while (current < tokens.size() && currentToken().type == TokenType::Comma)
			{
				argNode->addChild(new ParseTreeNode(consume(TokenType::Comma).lexeme));
				argNode->addChild(parseExpression());
			}
		}
		catch (const consumeError &)
		{
			error("Could not arguments");
			throw consumeError();
		}
		return argNode;
	}

	ParseTreeNode *parseFactor()
	{
		ParseTreeNode *factorNode = new ParseTreeNode("factor");
		try
		{
			if (currentToken().type == TokenType::NUMBER)
			{
				factorNode->addChild(new ParseTreeNode(consume(TokenType::NUMBER).lexeme));
			}
			else if (currentToken().type == TokenType::IDENTIFIER)
			{
				if (peekToken().type == TokenType::Dot)
				{
					// Handle dotted_name (e.g., car1.printname or car1.printname())
					factorNode->addChild(parseDottedName());

					// Check for method call
					if (currentToken().type == TokenType::LeftParenthesis)
					{
						factorNode->addChild(new ParseTreeNode(consume(TokenType::LeftParenthesis).lexeme));
						if (currentToken().type != TokenType::RightParenthesis)
							factorNode->addChild(parseArguments());
						factorNode->addChild(new ParseTreeNode(consume(TokenType::RightParenthesis).lexeme));
					}
				}
				else
				{
					// Simple identifier or function call
					factorNode->addChild(new ParseTreeNode(consume(TokenType::IDENTIFIER).lexeme));

					if (currentToken().type == TokenType::LeftParenthesis)
					{
						factorNode->addChild(new ParseTreeNode(consume(TokenType::LeftParenthesis).lexeme));
						if (currentToken().type != TokenType::RightParenthesis)
							factorNode->addChild(parseArguments());
						factorNode->addChild(new ParseTreeNode(consume(TokenType::RightParenthesis).lexeme));
					}
				}
			}
			else if (currentToken().type == TokenType::STRING_LITERAL)
			{
				factorNode->addChild(new ParseTreeNode(consume(TokenType::STRING_LITERAL).lexeme));
			}
			else if (currentToken().type == TokenType::LeftParenthesis)
			{
				ParseTreeNode *tupleOrParenNode = new ParseTreeNode("tuple_or_group");
				tupleOrParenNode->addChild(new ParseTreeNode(consume(TokenType::LeftParenthesis).lexeme));

				tupleOrParenNode->addChild(parseExpression());

				if (currentToken().type == TokenType::Comma)
				{
					// It's a tuple
					while (currentToken().type == TokenType::Comma)
					{
						tupleOrParenNode->addChild(new ParseTreeNode(consume(TokenType::Comma).lexeme));
						tupleOrParenNode->addChild(parseExpression());
					}
				}

				tupleOrParenNode->addChild(new ParseTreeNode(consume(TokenType::RightParenthesis).lexeme));
				factorNode->addChild(tupleOrParenNode);
			}

			else if (currentToken().type == TokenType::FalseKeyword)
			{
				factorNode->addChild(new ParseTreeNode(consume(TokenType::FalseKeyword).lexeme));
			}
			else if (currentToken().type == TokenType::TrueKeyword)
			{
				factorNode->addChild(new ParseTreeNode(consume(TokenType::TrueKeyword).lexeme));
			}
			else if (currentToken().type == TokenType::LeftBracket)
			{
				// list
				ParseTreeNode *listNode = new ParseTreeNode("list_literal");
				listNode->addChild(new ParseTreeNode(consume(TokenType::LeftBracket).lexeme));
				if (currentToken().type != TokenType::RightBracket)
				{
					listNode->addChild(parseExpression());
					while (currentToken().type == TokenType::Comma)
					{
						listNode->addChild(new ParseTreeNode(consume(TokenType::Comma).lexeme));
						listNode->addChild(parseExpression());
					}
				}
				listNode->addChild(new ParseTreeNode(consume(TokenType::RightBracket).lexeme));
				factorNode->addChild(listNode);
			}
			else if (currentToken().type == TokenType::LeftBrace)
			{
				// dict
				ParseTreeNode *dictNode = new ParseTreeNode("dict_literal");
				dictNode->addChild(new ParseTreeNode(consume(TokenType::LeftBrace).lexeme));
				if (currentToken().type != TokenType::RightBrace)
				{
					dictNode->addChild(parseExpression());
					dictNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
					dictNode->addChild(parseExpression());
					while (currentToken().type == TokenType::Comma)
					{
						dictNode->addChild(new ParseTreeNode(consume(TokenType::Comma).lexeme));
						dictNode->addChild(parseExpression());
						dictNode->addChild(new ParseTreeNode(consume(TokenType::Colon).lexeme));
						dictNode->addChild(parseExpression());
					}
				}
				dictNode->addChild(new ParseTreeNode(consume(TokenType::RightBrace).lexeme));
				factorNode->addChild(dictNode);
			}

			else
			{
				error("Could not parse Factor");
				throw consumeError();
			}
		}
		catch (const consumeError &)
		{
			error("Could not parse factor");
			throw consumeError();
		}
		return factorNode;
	}
};

void printParseTree(const ParseTreeNode *node, int depth = 0)
{
	if (node == nullptr)
		return;

	// Print indentation based on depth
	std::cout << std::string(depth * 2, ' '); // 2 spaces per depth level

	// Print current node's content
	std::cout << "|- " << node->label << std::endl; // Assuming 'content' is a string

	// Recursively print children
	for (const auto &child : node->children)
	{ // Assuming 'children' is a collection
		printParseTree(child, depth + 1);
	}
}

// ----------------------------------------------
// 8. Utility function to read the entire file
// ----------------------------------------------
string readFile(const string &filename)
{
	ifstream fileStream(filename);
	if (!fileStream.is_open())
	{
		throw runtime_error("Could not open file: " + filename);
	}
	stringstream buffer;
	buffer << fileStream.rdbuf();
	return buffer.str();
}

// Recursive helper to export node and its children
void exportToDot(ParseTreeNode *node, ofstream &out, int &nodeId, int parentId = -1)
{
	int currentId = nodeId++;

	// Escape double quotes in the label
	std::string safeLabel = node->label;
	size_t pos = 0;
	while ((pos = safeLabel.find('"', pos)) != std::string::npos)
	{
		safeLabel.replace(pos, 1, "\\\"");
		pos += 2; // move past the escaped quote
	}

	out << "    node" << currentId << " [label=\"" << safeLabel << "\"];\n";
	if (parentId != -1)
	{
		out << "    node" << parentId << " -> node" << currentId << ";\n";
	}
	for (auto child : node->children)
	{
		exportToDot(child, out, nodeId, currentId);
	}
}

// Export the full tree to a DOT file
void saveTreeToDot(ParseTreeNode *root, const string &filename)
{
	ofstream out(filename);
	out << "digraph ParseTree {\n";
	out << "    node [shape=box];\n";
	int id = 0;
	exportToDot(root, out, id);
	out << "}\n";
	out.close();
}

#include <string>
#include <unordered_map>

// ----------------------------------------------
// 9. Main
// ----------------------------------------------
int main()
{
	try
	{
		string sourceCode = readFile("seif.py");

		vector<Error> errors;
		// 2. Lexical analysis: produce tokens
		Lexer lexer;
		vector<Token> tokens = lexer.tokenize(sourceCode, errors);
		SymbolTable symTable;

		// 4. Parse/semantic pass: build the symbol table with type inference
		Parser parser(tokens, symTable);
		parser.parse();

		// 5. Print final symbol table
		symTable.printSymbols();

		// 3. Print out tokens (for demonstration)
		cout << "\n\nTokens:\n";
		for (auto &tk : tokens)
		{
			cout << "< ";
			switch (tk.type)
			{
			case TokenType::FalseKeyword:
				cout << "FalseKeyword";
				break;
			case TokenType::NoneKeyword:
				cout << "NoneKeyword";
				break;
			case TokenType::TrueKeyword:
				cout << "TrueKeyword";
				break;
			case TokenType::AndKeyword:
				cout << "AndKeyword";
				break;
			case TokenType::AsKeyword:
				cout << "AsKeyword";
				break;
			case TokenType::AssertKeyword:
				cout << "AssertKeyword";
				break;
			case TokenType::AsyncKeyword:
				cout << "AsyncKeyword";
				break;
			case TokenType::AwaitKeyword:
				cout << "AwaitKeyword";
				break;
			case TokenType::BreakKeyword:
				cout << "BreakKeyword";
				break;
			case TokenType::ClassKeyword:
				cout << "ClassKeyword";
				break;
			case TokenType::ContinueKeyword:
				cout << "ContinueKeyword";
				break;
			case TokenType::DefKeyword:
				cout << "DefKeyword";
				break;
			case TokenType::DelKeyword:
				cout << "DelKeyword";
				break;
			case TokenType::ElifKeyword:
				cout << "ElifKeyword";
				break;
			case TokenType::ElseKeyword:
				cout << "ElseKeyword";
				break;
			case TokenType::ExceptKeyword:
				cout << "ExceptKeyword";
				break;
			case TokenType::FinallyKeyword:
				cout << "FinallyKeyword";
				break;
			case TokenType::ForKeyword:
				cout << "ForKeyword";
				break;
			case TokenType::FromKeyword:
				cout << "FromKeyword";
				break;
			case TokenType::GlobalKeyword:
				cout << "GlobalKeyword";
				break;
			case TokenType::IfKeyword:
				cout << "IfKeyword";
				break;
			case TokenType::ImportKeyword:
				cout << "ImportKeyword";
				break;
			case TokenType::InKeyword:
				cout << "InKeyword";
				break;
			case TokenType::IsKeyword:
				cout << "IsKeyword";
				break;
			case TokenType::LambdaKeyword:
				cout << "LambdaKeyword";
				break;
			case TokenType::NonlocalKeyword:
				cout << "NonlocalKeyword";
				break;
			case TokenType::NotKeyword:
				cout << "NotKeyword";
				break;
			case TokenType::OrKeyword:
				cout << "OrKeyword";
				break;
			case TokenType::PassKeyword:
				cout << "PassKeyword";
				break;
			case TokenType::RaiseKeyword:
				cout << "RaiseKeyword";
				break;
			case TokenType::ReturnKeyword:
				cout << "ReturnKeyword";
				break;
			case TokenType::TryKeyword:
				cout << "TryKeyword";
				break;
			case TokenType::WhileKeyword:
				cout << "WhileKeyword";
				break;
			case TokenType::WithKeyword:
				cout << "WithKeyword";
				break;
			case TokenType::YieldKeyword:
				cout << "YieldKeyword";
				break;
			case TokenType::IDENTIFIER:
				cout << "IDENTIFIER";
				break;
			case TokenType::NUMBER:
				cout << "NUMBER";
				break;
			case TokenType::OPERATOR:
				cout << "OPERATOR";
				break;
			case TokenType::LeftParenthesis:
				cout << "LeftParenthesis";
				break;
			case TokenType::RightParenthesis:
				cout << "RightParenthesis";
				break;
			case TokenType::LeftBracket:
				cout << "LeftBracket";
				break;
			case TokenType::RightBracket:
				cout << "RightBracket";
				break;
			case TokenType::LeftBrace:
				cout << "LeftBrace";
				break;
			case TokenType::RightBrace:
				cout << "RightBrace";
				break;
			case TokenType::Colon:
				cout << "Colon";
				break;
			case TokenType::Comma:
				cout << "Comma";
				break;
			case TokenType::Dot:
				cout << "Dot";
				break;
			case TokenType::Semicolon:
				cout << "Semicolon";
				break;
			case TokenType::STRING_LITERAL:
				cout << "STRING_LITERAL";
				break;
			case TokenType::INDENT:
				cout << "INDENT";
				break;
			case TokenType::DEDENT:
				cout << "DEDENT";
				break;
			case TokenType::UNKNOWN:
				cout << "UNKNOWN";
				break;
			}
			cout << ", ";
			if (tk.type == TokenType::IDENTIFIER)
			{
				string key = tk.lexeme + "@" + tk.scope;
				if (symTable.table.find(key) != symTable.table.end())
				{
					cout << "symbol table entry : " << symTable.table[key].entry;
				}
				else
				{
					cout << "symbol table entry: not found";
				}
			}
			else
			{
				cout << tk.lexeme;
			}
			cout << " > ";
			cout << " | LINE NUMBER: " << tk.lineNumber << endl;
		}
		cout << endl;

		// print errors
		printErrors(errors);

		Syntax_Analyzer sa = Syntax_Analyzer();
		sa.tokens = tokens;
		ParseTreeNode *root = sa.parseProgram();
		cout << "\n\n\n\n";
		printParseTree(root);
		saveTreeToDot(root, "tree.dot");
	}
	catch (const exception &ex)
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}