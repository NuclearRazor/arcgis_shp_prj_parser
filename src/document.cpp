#include "wkt_parser.hpp"
#include <sstream>
#include <cmath>

namespace wkt 
{

// ============================================================================
// WKTDocument
// ============================================================================

WKTDocument WKTDocument::parse(std::string_view input) 
{
    WKTDocument doc;
    doc.source_ = std::string(input);
    
    Lexer lexer(input);
    auto tokens = lexer.tokenize();
    
    Parser parser(std::move(tokens));
    doc.root_ = parser.parse();
    
    return doc;
}

std::optional<WKTDocument> WKTDocument::tryParse(std::string_view input, std::string* errorOut)
{
    try 
    {
        return parse(input);
    }
    catch (const LexerError& e) 
    {
        if (errorOut) *errorOut = e.what();
            return std::nullopt;
    }
    catch (const ParseError& e) 
    {
        if (errorOut) *errorOut = e.what();
            return std::nullopt;
    }
}

WKTNode* WKTDocument::find(std::string_view path) 
{
    if (!root_) 
        return nullptr;
    
    const size_t slashPos = path.find('/');
    std::string_view first = (slashPos == std::string_view::npos) ? path : path.substr(0, slashPos);
    
    if (root_->name() == first) 
    {
        if (slashPos == std::string_view::npos) 
        {
            return root_.get();
        }
        return root_->findByPath(path.substr(slashPos + 1));
    }
    
    return root_->findByPath(path);
}

const WKTNode* WKTDocument::find(std::string_view path) const 
{
    return const_cast<WKTDocument*>(this)->find(path);
}

bool WKTDocument::setValue(std::string_view sectionName, std::string_view value) 
{
    WKTNode* node = find(sectionName);
    if (!node) 
        return false;
    
    node->setStringValue(std::string(value));
    return true;
}

bool WKTDocument::setNumber(std::string_view sectionName, size_t index, double value) 
{
    WKTNode* node = find(sectionName);
    if (!node) 
        return false;
    
    return node->setNumber(index, value);
}

bool WKTDocument::setNumbers(std::string_view sectionName, const std::vector<double>& values) 
{
    WKTNode* node = find(sectionName);
    if (!node) 
        return false;
    
    if (node->numbers().size() != values.size()) 
    {
        return false;
    }
    
    for (size_t i = 0; i < values.size(); ++i) 
    {
        node->setNumber(i, values[i]);
    }
    
    return true;
}

std::string WKTDocument::toString(bool pretty) const 
{
    if (!root_) 
        return "";
    return root_->toString(pretty ? 2 : -1);
}

std::optional<std::string> WKTDocument::getProjectionName() const
 {
    if (const WKTNode* proj = find("PROJECTION")) 
    {
        return proj->stringValue();
    }
    return std::nullopt;
}

std::optional<std::string> WKTDocument::getDatumName() const 
{
    if (const WKTNode* datum = find("DATUM")) 
    {
        return datum->stringValue();
    }
    return std::nullopt;
}

std::optional<std::string> WKTDocument::getSpheroidName() const 
{
    if (const WKTNode* spheroid = find("SPHEROID")) 
    {
        return spheroid->stringValue();
    }
    return std::nullopt;
}

std::optional<std::pair<double, double>> WKTDocument::getSpheroidParams() const 
{
    if (const WKTNode* spheroid = find("SPHEROID")) 
    {
        const auto& nums = spheroid->numbers();
        if (nums.size() >= 2) 
        {
            return std::make_pair(nums[0], nums[1]);
        }
    }
    return std::nullopt;
}

// ============================================================================
// Utilities
// ============================================================================

namespace utils 
{

std::optional<int> guessEPSG(const WKTDocument& doc) 
{
    // basic EPSG guessing based on datum name
    auto datumName = doc.getDatumName();
    if (!datumName) 
        return std::nullopt;
    
    // Basic datum -> EPSG mapping for GEOGCS (geographic CRS) only.
    // For PROJCS (projected CRS), EPSG depends on projection type, zone number, and other parameters - not implemented here.
    // Example: D_Pulkovo_1942 -> 4284 (GEOGCS)
    //          but Pulkovo 1942 / Gauss-Kruger zone 19 -> 28419 (PROJCS)
    static const std::unordered_map<std::string, int> datumToEPSG = 
    {
        {"D_WGS_1984", 4326},
        {"WGS_1984", 4326},
        {"D_North_American_1983", 4269},
        {"D_NAD83", 4269},
        {"D_North_American_1927", 4267},
        {"D_NAD27", 4267},
        {"D_ETRS_1989", 4258},
        {"D_Pulkovo_1942", 4284},
        {"D_S_JTSK", 4156},
    };
    
    auto it = datumToEPSG.find(*datumName);
    if (it != datumToEPSG.end()) 
    {
        return it->second;
    }
    
    return std::nullopt;
}

bool validateWKT(std::string_view input, std::string* errorOut) 
{
    auto result = WKTDocument::tryParse(input, errorOut);
    return result.has_value();
}

bool areEquivalent(const WKTDocument& a, const WKTDocument& b, double tolerance) 
{
    // Compare structure recursively
    std::function<bool(const WKTNode*, const WKTNode*)> compare;
    
    compare = [&](const WKTNode* nodeA, const WKTNode* nodeB) -> bool 
    {
        if (!nodeA || !nodeB) return nodeA == nodeB;
        
        // Compare name
        if (nodeA->name() != nodeB->name()) 
            return false;
        
        // Compare string value
        if (nodeA->stringValue() != nodeB->stringValue()) 
            return false;
        
        // Compare numbers
        const auto& numsA = nodeA->numbers();
        const auto& numsB = nodeB->numbers();
        if (numsA.size() != numsB.size()) 
            return false;
        
        for (size_t i = 0; i < numsA.size(); ++i) 
        {
            if (std::abs(numsA[i] - numsB[i]) > tolerance) 
            {
                return false;
            }
        }
        
        // Compare children
        const auto& childrenA = nodeA->children();
        const auto& childrenB = nodeB->children();
        if (childrenA.size() != childrenB.size()) return false;
        
        for (size_t i = 0; i < childrenA.size(); ++i) 
        {
            if (!compare(childrenA[i].get(), childrenB[i].get()))
            {
                return false;
            }
        }
        
        return true;
    };
    
    return compare(a.root(), b.root());
}

} // namespace utils

}

