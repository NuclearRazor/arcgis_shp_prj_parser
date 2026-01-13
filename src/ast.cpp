#include "wkt_parser.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace wkt 
{

// ============================================================================
// WKTNode
// ============================================================================

WKTNode::WKTNode(std::string name)
    : name_(std::move(name))
{}

void WKTNode::setStringValue(std::string value)
{
    stringValue_ = std::move(value);
}

void WKTNode::addNumber(double value) 
{
    numbers_.push_back(value);
}

void WKTNode::addChild(std::unique_ptr<WKTNode> child) 
{
    children_.push_back(std::move(child));
}

void WKTNode::setSourceRange(size_t start, size_t end) 
{
    sourceStart_ = start;
    sourceEnd_ = end;
}

WKTNode* WKTNode::findChild(std::string_view name) 
{
    for (const auto& child : children_) 
    {
        if (child->name() == name) 
        {
            return child.get();
        }
    }
    return nullptr;
}

const WKTNode* WKTNode::findChild(std::string_view name) const 
{
    for (const auto& child : children_) 
    {
        if (child->name() == name) 
        {
            return child.get();
        }
    }
    return nullptr;
}

std::vector<WKTNode*> WKTNode::findAllChildren(std::string_view name) 
{
    std::vector<WKTNode*> result;
    for (const auto& child : children_) 
    {
        if (child->name() == name) 
        {
            result.push_back(child.get());
        }
    }
    return result;
}

WKTNode* WKTNode::findByPath(std::string_view path) 
{
    if (path.empty()) 
    {
        return this;
    }
    
    const size_t slashPos = path.find('/');
    const std::string_view first = (slashPos == std::string_view::npos)  ? path  : path.substr(0, slashPos);
    const std::string_view rest = (slashPos == std::string_view::npos) ? std::string_view{} : path.substr(slashPos + 1);

    for (auto& child : children_) 
    {
        if (child->name() == first) 
        {
            if (rest.empty()) 
            {
                return child.get();
            }
            return child->findByPath(rest);
        }
    }
    
    for (auto& child : children_) 
    {
        if (WKTNode* found = child->findByPath(path)) 
        {
            return found;
        }
    }
    
    return nullptr;
}

const WKTNode* WKTNode::findByPath(std::string_view path) const 
{
    return const_cast<WKTNode*>(this)->findByPath(path);
}

bool WKTNode::setStringValue(std::string_view path, std::string_view value) 
{
    WKTNode* node = findByPath(path);
    if (!node) 
    {
        return false;
    }
    node->stringValue_ = std::string(value);
    return true;
}

bool WKTNode::setNumber(size_t index, double value) 
{
    if (index >= numbers_.size()) 
    {
        return false;
    }
    numbers_[index] = value;
    return true;
}

bool WKTNode::setNumber(std::string_view path, size_t index, double value) 
{
    WKTNode* node = findByPath(path);
    if (!node) 
    {
        return false;
    }
    return node->setNumber(index, value);
}

std::string WKTNode::toString(int indent) const 
{
    std::ostringstream ss;
    toStringImpl(ss, indent, 0);
    return ss.str();
}

void WKTNode::toStringImpl(std::ostringstream& ss, int indent, int depth) const 
{
    bool pretty = (indent >= 0);
    const std::string indentStr = pretty ? std::string(depth * indent, ' ') : "";
    const std::string childIndent = pretty ? std::string((depth + 1) * indent, ' ') : "";
    const std::string newline = pretty ? "\n" : "";
    
    ss << name_ << "[";
    
    bool needComma = false;
    if (stringValue_) 
    {
        ss << "\"" << *stringValue_ << "\"";
        needComma = true;
    }

    for (double num : numbers_) 
    {
        if (needComma) ss << ",";
        if (num == static_cast<int64_t>(num)) 
        {
            ss << static_cast<int64_t>(num);
        } 
        else 
        {
            ss << std::setprecision(15) << num;
        }
        needComma = true;
    }
    
    for (const auto& child : children_) 
    {
        if (needComma) 
        {
            ss << ",";
        }

        if (pretty) 
        {
            ss << newline << childIndent;
        }
        child->toStringImpl(ss, indent, depth + 1);
        needComma = true;
    }
    
    if (pretty && !children_.empty()) 
    {
        ss << newline << indentStr;
    }
    
    ss << "]";
}

} // namespace wkt
