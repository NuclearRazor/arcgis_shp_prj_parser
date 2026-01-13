#include "wkt_parser.hpp"
#include <sstream>
#include <stdexcept>

namespace wkt {

// ============================================================================
// ParseError
// ============================================================================

ParseError::ParseError(const std::string& msg, const Token& token)
    : std::runtime_error(msg)
    , token_(token)
{}

// ============================================================================
// Parser
// ============================================================================

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens))
{}

std::unique_ptr<WKTNode> Parser::parse() 
{
    if (isAtEnd()) 
    {
        error("Empty input");
    }
    
    auto node = parseNode();
    
    if (!isAtEnd()) {
        error("Unexpected token after end of WKT: " + peek().value);
    }
    
    return node;
}

std::unique_ptr<WKTNode> Parser::parseNode()
{
    // expect: IDENTIFIER '[' content ']'
    Token nameToken = consume(TokenType::Identifier, "Expected section name");
    const size_t startPos = nameToken.position;
    
    auto node = std::make_unique<WKTNode>(nameToken.value);
    
    consume(TokenType::LBracket, "Expected '[' after section name");
    
    parseNodeContent(*node);
    
    Token closeToken = consume(TokenType::RBracket, "Expected ']' to close section");
    
    node->setSourceRange(startPos, closeToken.position + 1);
    
    return node;
}

void Parser::parseNodeContent(WKTNode& node) {
    // content can be:
    // - Empty: []
    // - String only: ["name"]
    // - String + numbers: ["name", 123, 456]
    // - String + numbers + children: ["name", 123, CHILD[...]]
    // - Numbers only (rare): [123, 456]
    // - Children only: [CHILD1[...], CHILD2[...]]
    
    if (check(TokenType::RBracket)) {
        // Empty content
        return;
    }
    
    bool expectComma = false;
    
    while (!check(TokenType::RBracket) && !isAtEnd())
    {
        if (expectComma) 
        {
            if (check(TokenType::Comma)) 
            {
                advance();
            }
            if (check(TokenType::RBracket)) 
            {
                break;
            }
        }
        
        if (check(TokenType::String)) 
        {
            // String value (usually first)
            Token strToken = advance();
            node.setStringValue(strToken.value);
            expectComma = true;
        }
        else if (check(TokenType::Number))
        {
            // Numeric value
            Token numToken = advance();
            
            try 
            {
                double value = std::stod(numToken.value);
                node.addNumber(value);
            } 
            catch (const std::exception&) 
            {
                error("Invalid number: " + numToken.value);
            }
            
            expectComma = true;
        }
        else if (check(TokenType::Identifier)) 
        {
            // Nested node
            auto child = parseNode();
            node.addChild(std::move(child));
            expectComma = true;
        }
        else if (check(TokenType::Comma)) 
        {
            // Empty value (,,) - skip
            advance();
            expectComma = false;
        }
        else 
        {
            error("Unexpected token in section content: " + peek().typeName());
        }
    }
}

const Token& Parser::peek() const 
{
    return tokens_[current_];
}

const Token& Parser::previous() const 
{
    return tokens_[current_ - 1];
}

Token Parser::advance() 
{
    if (!isAtEnd()) {
        current_++;
    }
    return previous();
}

bool Parser::check(TokenType type) const 
{
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) 
{
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) 
{
    if (check(type))
    {
        return advance();
    }
    error(message + " (got " + peek().typeName() + ": '" + peek().value + "')");
}

bool Parser::isAtEnd() const 
{
    return peek().type == TokenType::EndOfInput;
}

void Parser::error(const std::string& msg) 
{
    const Token& tok = peek();
    std::ostringstream ss;
    ss << "Parse error at line " << tok.line << ", column " << tok.column << ": " << msg;
    throw ParseError(ss.str(), tok);
}

} // namespace wkt
