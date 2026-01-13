# WKT Parser

A modern C++17 parser for ESRI WKT (Well-Known Text) projection files (.prj).

## Features

- Clean Lexer → Parser → AST architecture
- Header-only friendly, no external dependencies
- Cross-platform (Linux, macOS, Windows)
- High precision number handling (scientific notation support)
- Path-based navigation (`find("DATUM/SPHEROID")`)
- In-place modification with serialization

## Build

```bash
g++ -std=c++17 -O2 -I include src/*.cpp -o wkt_parser
```

Or with CMake:
```bash
mkdir build && cd build
cmake ..
make
```

## Quick Start

```cpp
#include "wkt_parser.hpp"
using namespace wkt;

// Parse
auto doc = WKTDocument::parse(
    "GEOGCS[\"GCS_WGS_1984\","
    "DATUM[\"D_WGS_1984\","
    "SPHEROID[\"WGS_1984\",6378137.0,298.257224]]]"
);

// Query
auto datum = doc.getDatumName();           // "D_WGS_1984"
auto [a, f] = *doc.getSpheroidParams();    // 6378137.0, 298.257224

// Navigate
WKTNode* spheroid = doc.find("SPHEROID");
WKTNode* nested = doc.find("DATUM/SPHEROID");

// Modify
doc.setValue("SPHEROID", "ITRF_2008");
doc.setNumber("SPHEROID", 0, 6378140.0);

// Serialize
std::string output = doc.toString();        // compact
std::string pretty = doc.toString(true);    // formatted
```

## API

### WKTDocument

```cpp
static WKTDocument parse(std::string_view input);
static std::optional<WKTDocument> tryParse(std::string_view input, std::string* error);

WKTNode* find(std::string_view path);
bool setValue(std::string_view section, std::string_view value);
bool setNumber(std::string_view section, size_t index, double value);
bool setNumbers(std::string_view section, const std::vector<double>& values);
std::string toString(bool pretty = false) const;

// Convenience
std::optional<std::string> getDatumName() const;
std::optional<std::string> getSpheroidName() const;
std::optional<std::pair<double, double>> getSpheroidParams() const;
```

### WKTNode

```cpp
const std::string& name() const;
const std::optional<std::string>& stringValue() const;
const std::vector<double>& numbers() const;

WKTNode* findChild(std::string_view name);
WKTNode* findByPath(std::string_view path);
bool setNumber(size_t index, double value);
```

## Error Handling

```cpp
try {
    auto doc = WKTDocument::parse(wkt);
} catch (const wkt::LexerError& e) {
    // e.position(), e.line(), e.column()
} catch (const wkt::ParseError& e) {
    // e.token()
}

// Or safe version:
std::string error;
if (auto doc = WKTDocument::tryParse(wkt, &error)) {
    // use *doc
} else {
    std::cerr << error << "\n";
}
```

## License

MIT
