#include "wkt_parser.hpp"
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace wkt 
{

// ============================================================================
// Token
// ============================================================================

std::string Token::typeName() const 
{
    switch (type) 
    {
        case TokenType::Identifier:  return "Identifier";
        case TokenType::String:      return "String";
        case TokenType::Number:      return "Number";
        case TokenType::LBracket:    return "LBracket";
        case TokenType::RBracket:    return "RBracket";
        case TokenType::Comma:       return "Comma";
        case TokenType::EndOfInput:  return "EndOfInput";
    }
    return "Unknown";
}

// ============================================================================
// LexerError
// ============================================================================

LexerError::LexerError(const std::string& msg, size_t pos, size_t line, size_t col)
    : std::runtime_error(msg)
    , pos_(pos)
    , line_(line)
    , col_(col)
{}

// ============================================================================
// Lexer
// ============================================================================

Lexer::Lexer(std::string_view input)
    : input_(input)
{}

std::vector<Token> Lexer::tokenize() 
{
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        Token token = nextToken();
        tokens.push_back(std::move(token));
        
        if (tokens.back().type == TokenType::EndOfInput) 
        {
            break;
        }
    }
    
    if (tokens.empty() || tokens.back().type != TokenType::EndOfInput) 
    {
        tokens.push_back(makeToken(TokenType::EndOfInput));
    }
    
    return tokens;
}

Token Lexer::nextToken() 
{
    skipWhitespace();
    
    if (isAtEnd()) 
    {
        return makeToken(TokenType::EndOfInput);
    }
    
    tokenStart_ = current_;
    char c = advance();
    
    // Single-character tokens
    switch (c) 
    {
        case '[': return makeToken(TokenType::LBracket, "[");
        case ']': return makeToken(TokenType::RBracket, "]");
        case ',': return makeToken(TokenType::Comma, ",");
        case '"': return readString();
    }
    
    // Identifier (starts with letter or underscore)
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
    {
        return readIdentifier();
    }
    
    // Number (starts with digit, minus, plus, or dot)
    if (std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+' || c == '.') 
    {
        return readNumber();
    }
    
    error("Unexpected character: '" + std::string(1, c) + "'");
}

void Lexer::skipWhitespace() 
{
    while (!isAtEnd()) 
    {
        char c = peek();
        switch (c) 
        {
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
            case '\n':
                advance();
                line_++;
                column_ = 1;
                break;
            default:
                return;
        }
    }
}

Token Lexer::readIdentifier() 
{
    // Already consumed first character
    size_t start = current_ - 1;
    
    while (!isAtEnd()) 
    {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
        {
            advance();
        }
        else 
        {
            break;
        }
    }
    
    std::string value(input_.substr(start, current_ - start));
    return makeToken(TokenType::Identifier, std::move(value));
}

Token Lexer::readString() 
{
    // Opening quote already consumed
    size_t start = current_;
    
    while (!isAtEnd() && peek() != '"') 
    {
        if (peek() == '\n') 
        {
            line_++;
            column_ = 0;
        }
        // Handle escape sequences if needed
        if (peek() == '\\' && current_ + 1 < input_.size()) 
        {
            advance(); // skip backslash
        }
        advance();
    }
    
    if (isAtEnd()) {
        error("Unterminated string");
    }
    
    std::string value(input_.substr(start, current_ - start));
    advance(); // consume closing quote
    
    return makeToken(TokenType::String, std::move(value));
}

Token Lexer::readNumber() 
{
    const size_t start = current_ - 1;
    if (input_[start] == '-' || input_[start] == '+') 
    {
        // need at least one digit after sign
        if (isAtEnd() || (!std::isdigit(static_cast<unsigned char>(peek())) && peek() != '.')) 
        {
            error("Invalid number: expected digit after sign");
        }
    }
    
    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) 
    {
        advance();
    }
    
    if (!isAtEnd() && peek() == '.') 
    {
        advance();

        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) 
        {
            advance();
        }
    }
    
    // exponent part (e.g., 1.5e-10)
    if (!isAtEnd() && (peek() == 'e' || peek() == 'E')) 
    {
        advance();

        // sign
        if (!isAtEnd() && (peek() == '+' || peek() == '-')) 
        {
            advance();
        }
        
        // digits
        if (isAtEnd() || !std::isdigit(static_cast<unsigned char>(peek()))) 
        {
            error("Invalid number: expected exponent digits");
        }
        
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) 
        {
            advance();
        }
    }
    
    std::string value(input_.substr(start, current_ - start));
    
    // validate
    try 
    {
        size_t pos = 0;
        std::stod(value, &pos);
        if (pos != value.size()) 
        {
            error("Invalid number format: " + value);
        }
    } 
    catch (const std::exception&) 
    {
        error("Invalid number format: " + value);
    }
    
    return makeToken(TokenType::Number, std::move(value));
}

char Lexer::peek() const 
{
    if (isAtEnd()) 
        return '\0';
    return input_[current_];
}

char Lexer::advance() 
{
    const char c = input_[current_++];
    column_++;
    return c;
}

bool Lexer::isAtEnd() const 
{
    return current_ >= input_.size();
}

Token Lexer::makeToken(TokenType type, std::string value) 
{
    return Token
    {
        type,
        std::move(value),
        tokenStart_,
        line_,
        column_
    };
}

void Lexer::error(const std::string& msg) 
{
    std::ostringstream ss;
    ss << "Lexer error at line " << line_ << ", column " << column_ << ": " << msg;
    throw LexerError(ss.str(), current_, line_, column_);
}

} // namespace wkt
