#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include <stdexcept>
#include <unordered_map>
#include <functional>

namespace wkt 
{

// ============================================================================
// Tokens
// ============================================================================

enum class TokenType 
{
    Identifier,     // GEOGCS, DATUM, SPHEROID...
    String,         // "GCS_WGS_1984"
    Number,         // 6378137.0, -298.257
    LBracket,       // [
    RBracket,       // ]
    Comma,          // ,
    EndOfInput
};

struct Token 
{
    TokenType type;
    std::string value;
    size_t position;
    size_t line;
    size_t column;
    
    std::string typeName() const;
};

// ============================================================================
// Lexer
// ============================================================================

class LexerError : public std::runtime_error 
{
public:
    LexerError(const std::string& msg, size_t pos, size_t line, size_t col);
    
    size_t position() const { return pos_; }
    size_t line() const { return line_; }
    size_t column() const { return col_; }
    
private:
    size_t pos_, line_, col_;
};

class Lexer 
{
public:
    explicit Lexer(std::string_view input);
    
    std::vector<Token> tokenize();
    Token nextToken();
    
private:
    void skipWhitespace();
    Token readIdentifier();
    Token readString();
    Token readNumber();
    
    char peek() const;
    char advance();
    bool isAtEnd() const;
    
    Token makeToken(TokenType type, std::string value = "");
    [[noreturn]] void error(const std::string& msg);
    
    std::string_view input_;
    size_t current_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;
    size_t tokenStart_ = 0;
};

// ============================================================================
// AST Node
// ============================================================================

class WKTNode 
{
public:
    WKTNode(std::string name);
    
    // Accessors
    const std::string& name() const { return name_; }
    const std::optional<std::string>& stringValue() const { return stringValue_; }
    const std::vector<double>& numbers() const { return numbers_; }
    const std::vector<std::unique_ptr<WKTNode>>& children() const { return children_; }
    
    // Source position tracking
    size_t sourceStart() const { return sourceStart_; }
    size_t sourceEnd() const { return sourceEnd_; }
    
    // Mutators
    void setStringValue(std::string value);
    void addNumber(double value);
    void addChild(std::unique_ptr<WKTNode> child);
    void setSourceRange(size_t start, size_t end);
    
    // Navigation
    WKTNode* findChild(std::string_view name);
    const WKTNode* findChild(std::string_view name) const;
    std::vector<WKTNode*> findAllChildren(std::string_view name);
    
    // Deep search by path: "DATUM/SPHEROID"
    WKTNode* findByPath(std::string_view path);
    const WKTNode* findByPath(std::string_view path) const;
    
    // Modification
    bool setStringValue(std::string_view path, std::string_view value);
    bool setNumber(size_t index, double value);
    bool setNumber(std::string_view path, size_t index, double value);
    
    // Serialization
    std::string toString(int indent = -1) const;
    
    // Visitor pattern
    template<typename Visitor>
    void visit(Visitor&& visitor) 
    {
        visitor(*this);
        for (auto& child : children_) 
        {
            child->visit(std::forward<Visitor>(visitor));
        }
    }
    
private:
    void toStringImpl(std::ostringstream& ss, int indent, int depth) const;
    
    std::string name_;
    std::optional<std::string> stringValue_;
    std::vector<double> numbers_;
    std::vector<std::unique_ptr<WKTNode>> children_;
    
    size_t sourceStart_ = 0;
    size_t sourceEnd_ = 0;
};

// ============================================================================
// Parser
// ============================================================================

class ParseError : public std::runtime_error 
{
public:
    ParseError(const std::string& msg, const Token& token);
    
    const Token& token() const { return token_; }
    
private:
    Token token_;
};

class Parser 
{
public:
    explicit Parser(std::vector<Token> tokens);
    
    std::unique_ptr<WKTNode> parse();
    
private:
    std::unique_ptr<WKTNode> parseNode();
    void parseNodeContent(WKTNode& node);
    
    const Token& peek() const;
    const Token& previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& message);
    bool isAtEnd() const;
    
    [[noreturn]] void error(const std::string& msg);
    
    std::vector<Token> tokens_;
    size_t current_ = 0;
};

// ============================================================================
// WKT Document - High-level API
// ============================================================================

class WKTDocument 
{
public:
    // Parsing
    static WKTDocument parse(std::string_view input);
    static std::optional<WKTDocument> tryParse(std::string_view input, std::string* errorOut = nullptr);
    
    // Access
    WKTNode* root() { return root_.get(); }
    const WKTNode* root() const { return root_.get(); }
    const std::string& originalSource() const { return source_; }
    
    // Navigation shortcuts
    WKTNode* find(std::string_view path);
    const WKTNode* find(std::string_view path) const;
    
    // Modification with automatic re-serialization
    bool setValue(std::string_view sectionName, std::string_view value);
    bool setNumber(std::string_view sectionName, size_t index, double value);
    bool setNumbers(std::string_view sectionName, const std::vector<double>& values);
    
    // Get modified source
    std::string toString(bool pretty = false) const;
    
    // Validation
    bool isValid() const { return root_ != nullptr; }
    
    // Common WKT queries
    std::optional<std::string> getProjectionName() const;
    std::optional<std::string> getDatumName() const;
    std::optional<std::string> getSpheroidName() const;
    std::optional<std::pair<double, double>> getSpheroidParams() const;  // semi-major axis, inverse flattening
    
private:
    WKTDocument() = default;
    
    std::unique_ptr<WKTNode> root_;
    std::string source_;
};

// ============================================================================
// Utility functions
// ============================================================================

namespace utils 
{
    // EPSG code lookup (basic)
    std::optional<int> guessEPSG(const WKTDocument& doc);
    
    // Validation
    bool validateWKT(std::string_view input, std::string* errorOut = nullptr);
    
    // Comparison
    bool areEquivalent(const WKTDocument& a, const WKTDocument& b, double tolerance = 1e-10);
}

} // namespace wkt
