#include "wkt_parser.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace wkt;

// ============================================================================
// test utilities
// ============================================================================

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "... "; \
    try { test_##name(); std::cout << "OK\n"; } \
    catch (const std::exception& e) { std::cout << "FAILED: " << e.what() << "\n"; failures++; } \
} while(0)

// ============================================================================
// lexer tests
// ============================================================================

TEST(lexer_simple) {
    Lexer lexer("GEOGCS[\"WGS_84\"]");
    auto tokens = lexer.tokenize();
    
    // tokens: GEOGCS [ "WGS_84" ] EOF
    //         0      1  2       3  4
    assert(tokens.size() == 5);
    assert(tokens[0].type == TokenType::Identifier && tokens[0].value == "GEOGCS");
    assert(tokens[1].type == TokenType::LBracket);
    assert(tokens[2].type == TokenType::String && tokens[2].value == "WGS_84");
    assert(tokens[3].type == TokenType::RBracket);
    assert(tokens[4].type == TokenType::EndOfInput);
}

TEST(lexer_numbers) {
    Lexer lexer("SPHEROID[\"test\",6378137.0,-298.257,1.5e-10]");
    auto tokens = lexer.tokenize();
    
    // tokens: SPHEROID [ "test" , 6378137.0 , -298.257 , 1.5e-10 ] EOF
    //         0        1  2     3  4         5  6        7  8      9 10
    assert(tokens[4].type == TokenType::Number && tokens[4].value == "6378137.0");
    assert(tokens[6].type == TokenType::Number && tokens[6].value == "-298.257");
    assert(tokens[8].type == TokenType::Number && tokens[8].value == "1.5e-10");
}

TEST(lexer_whitespace) {
    Lexer lexer("GEOGCS [ \"name\" , 123 ]");
    auto tokens = lexer.tokenize();
    
    // tokens: GEOGCS [ "name" , 123 ] EOF
    //         0      1  2     3  4  5  6
    assert(tokens.size() == 7);
    assert(tokens[0].type == TokenType::Identifier);
    assert(tokens[2].type == TokenType::String);
    assert(tokens[4].type == TokenType::Number);
}

// ============================================================================
// parser tests
// ============================================================================

TEST(parser_simple_section) {
    auto doc = WKTDocument::parse("GEOGCS[\"GCS_WGS_1984\"]");
    
    assert(doc.root() != nullptr);
    assert(doc.root()->name() == "GEOGCS");
    assert(doc.root()->stringValue() == "GCS_WGS_1984");
    assert(doc.root()->numbers().empty());
    assert(doc.root()->children().empty());
}

TEST(parser_section_with_numbers) {
    auto doc = WKTDocument::parse("SPHEROID[\"WGS_1984\",6378137.0,298.257224]");
    
    assert(doc.root()->stringValue() == "WGS_1984");
    assert(doc.root()->numbers().size() == 2);
    assert(std::abs(doc.root()->numbers()[0] - 6378137.0) < 0.001);
    assert(std::abs(doc.root()->numbers()[1] - 298.257224) < 0.000001);
}

TEST(parser_nested) {
    auto doc = WKTDocument::parse(
        "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.0,298.257224]]"
    );
    
    assert(doc.root()->name() == "DATUM");
    assert(doc.root()->children().size() == 1);
    
    auto* spheroid = doc.root()->findChild("SPHEROID");
    assert(spheroid != nullptr);
    assert(spheroid->stringValue() == "WGS_1984");
    assert(spheroid->numbers().size() == 2);
}

TEST(parser_complex) {
    std::string wkt = 
        "GEOGCS[\"GCS_WGS_1984\","
        "DATUM[\"D_WGS_1984\","
        "SPHEROID[\"WGS_1984\",6378137.0,298.257224]],"
        "PRIMEM[\"Greenwich\",0.0],"
        "UNIT[\"Degree\",0.0174532925199433]]";
    
    auto doc = WKTDocument::parse(wkt);
    
    assert(doc.root()->name() == "GEOGCS");
    assert(doc.root()->children().size() == 3);
    
    assert(doc.find("DATUM") != nullptr);
    assert(doc.find("SPHEROID") != nullptr);
    assert(doc.find("PRIMEM") != nullptr);
    assert(doc.find("UNIT") != nullptr);
}

TEST(parser_pulkovo) {
    std::string wkt = 
        "GEOGCS[\"GCS_Pulkovo_1942\","
        "DATUM[\"D_Pulkovo_1942\","
        "SPHEROID[\"Krasovsky_1940\",6378245.0,298.3]],"
        "PRIMEM[\"Greenwich\",0.0],"
        "UNIT[\"Degree\",0.0174532925199433,,666.0010098,1.0]]";
    
    auto doc = WKTDocument::parse(wkt);
    
    assert(doc.getDatumName() == "D_Pulkovo_1942");
    assert(doc.getSpheroidName() == "Krasovsky_1940");
    
    auto params = doc.getSpheroidParams();
    assert(params.has_value());
    assert(std::abs(params->first - 6378245.0) < 0.1);
    assert(std::abs(params->second - 298.3) < 0.01);
}

// ============================================================================
// real-world wkt samples (from original codebase)
// ============================================================================

TEST(sample_simple_primem) {
    // simple geogcs with just primem
    auto doc = WKTDocument::parse(
        "GEOGCS[\"GCS_WGS_1984\",PRIMEM[\"Greenwich\",0.0]]"
    );
    
    assert(doc.root()->name() == "GEOGCS");
    assert(doc.root()->stringValue() == "GCS_WGS_1984");
    assert(doc.find("PRIMEM") != nullptr);
    assert(doc.find("PRIMEM")->stringValue() == "Greenwich");
    assert(doc.find("PRIMEM")->numbers().size() == 1);
    assert(doc.find("PRIMEM")->numbers()[0] == 0.0);
}

TEST(sample_unit_parameter) {
    // geogcs with unit and parameter
    auto doc = WKTDocument::parse(
        "GEOGCS[\"GCS_WGS_1984\",UNIT[\"T\",10.0],PARAMETER[\"False_Easting\",500000.0]]"
    );
    
    assert(doc.find("UNIT") != nullptr);
    assert(doc.find("UNIT")->stringValue() == "T");
    assert(doc.find("UNIT")->numbers()[0] == 10.0);
    
    assert(doc.find("PARAMETER") != nullptr);
    assert(doc.find("PARAMETER")->stringValue() == "False_Easting");
    assert(doc.find("PARAMETER")->numbers()[0] == 500000.0);
}

TEST(sample_full_wgs84) {
    // complete wgs84 definition
    auto doc = WKTDocument::parse(
        "GEOGCS[\"GCS_WGS_1984\","
        "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.000000,298.257224]],"
        "PRIMEM[\"Greenwich\",0.0],"
        "UNIT[\"Kilometer\",1000.0]]"
    );
    
    assert(doc.getDatumName() == "D_WGS_1984");
    assert(doc.getSpheroidName() == "WGS_1984");
    
    auto params = doc.getSpheroidParams();
    assert(params.has_value());
    assert(std::abs(params->first - 6378137.0) < 0.01);
    assert(std::abs(params->second - 298.257224) < 0.000001);
    
    assert(doc.find("UNIT")->stringValue() == "Kilometer");
    assert(doc.find("UNIT")->numbers()[0] == 1000.0);
}

TEST(sample_pulkovo_with_extra_values) {
    // pulkovo with extra empty values in unit section
    auto doc = WKTDocument::parse(
        "GEOGCS[\"GCS_Pulkovo_1942\","
        "DATUM[\"D_Pulkovo_1942\",SPHEROID[\"Krasovsky_1940\",6378245.0,298.3]],"
        "PRIMEM[\"Greenwich\",0.0],"
        "UNIT[\"Degree\",0.0174532925199433,,666.0010098,1.0]]"
    );
    
    assert(doc.getDatumName() == "D_Pulkovo_1942");
    assert(doc.getSpheroidName() == "Krasovsky_1940");
    
    // unit should have multiple numbers (parser handles empty values)
    auto* unit = doc.find("UNIT");
    assert(unit != nullptr);
    assert(unit->stringValue() == "Degree");
    assert(unit->numbers().size() >= 1);
}

TEST(sample_deeply_nested_units) {
    // deeply nested unit structure
    auto doc = WKTDocument::parse(
        "GEOGCS[\"GCS_WGS_1984\","
        "UNIT1[\"Kilometer\",UNIT2[\"R\",UNIT3[\"Kilometer\",10.0,,-23.0,45,-90,,,90]]]]"
    );
    
    assert(doc.root()->stringValue() == "GCS_WGS_1984");
    
    auto* unit1 = doc.find("UNIT1");
    assert(unit1 != nullptr);
    assert(unit1->stringValue() == "Kilometer");
    
    auto* unit2 = doc.find("UNIT2");
    assert(unit2 != nullptr);
    assert(unit2->stringValue() == "R");
    
    auto* unit3 = doc.find("UNIT3");
    assert(unit3 != nullptr);
    assert(unit3->stringValue() == "Kilometer");
    assert(unit3->numbers()[0] == 10.0);
}

TEST(sample_complex_with_projections) {
    // complex wkt with multiple projections and parameters
    auto doc = WKTDocument::parse(
        "GEOGCS[\"GCS_WGS_1984\","
        "UNIT1[\"Kilometer\",UNIT2[\"Rr\",UNIT3[\"Kilometer\",10.0,,-23.0,45,-90]]],"
        "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.000000,298.257224]],"
        "UNIT[\"Kilometer\",1000.0],"
        "PARAMETER2[\"Central_Meridian_Test\",-124.5],"
        "PROJECTION[\"Lambert_Conformal_Conic\"],"
        "PRIMEM[\"Greenwich\",0.0],"
        "PROJECTION2[\"Lambert_Conformal_Conic_Test\"],"
        "PROJECTION[\"Lambert_Conformal_Conic\"]]"
    );
    
    assert(doc.getDatumName() == "D_WGS_1984");
    assert(doc.getSpheroidName() == "WGS_1984");
    
    // find projections
    auto projections = doc.root()->findAllChildren("PROJECTION");
    assert(projections.size() == 2);
    
    auto* param2 = doc.find("PARAMETER2");
    assert(param2 != nullptr);
    assert(param2->stringValue() == "Central_Meridian_Test");
    assert(param2->numbers()[0] == -124.5);
    
    auto* proj2 = doc.find("PROJECTION2");
    assert(proj2 != nullptr);
    assert(proj2->stringValue() == "Lambert_Conformal_Conic_Test");
}

TEST(sample_simple_projection) {
    // simple projection example
    auto doc = WKTDocument::parse(
        "GEOGCS[\"GCS_WGS_1984\","
        "PROJECTION[\"Lambert_Conformal_Conic\"],"
        "PRIMEM[\"Greenwich2\",0.0]]"
    );
    
    assert(doc.root()->stringValue() == "GCS_WGS_1984");
    
    auto* proj = doc.find("PROJECTION");
    assert(proj != nullptr);
    assert(proj->stringValue() == "Lambert_Conformal_Conic");
    
    auto* primem = doc.find("PRIMEM");
    assert(primem != nullptr);
    assert(primem->stringValue() == "Greenwich2");
}

// ============================================================================
// navigation tests
// ============================================================================

TEST(navigation_find_by_path) {
    std::string wkt = 
        "GEOGCS[\"test\","
        "DATUM[\"D_test\","
        "SPHEROID[\"S_test\",123,456]]]";
    
    auto doc = WKTDocument::parse(wkt);
    
    // direct child
    assert(doc.find("DATUM") != nullptr);
    
    // path-based
    assert(doc.find("DATUM/SPHEROID") != nullptr);
    
    // deep search
    assert(doc.find("SPHEROID") != nullptr);
    assert(doc.find("SPHEROID")->stringValue() == "S_test");
}

// ============================================================================
// modification tests
// ============================================================================

TEST(modification_set_string) {
    auto doc = WKTDocument::parse("SPHEROID[\"old_name\",123,456]");
    
    bool result = doc.setValue("SPHEROID", "new_name");
    assert(result);
    assert(doc.root()->stringValue() == "new_name");
    
    // check serialization
    std::string output = doc.toString();
    assert(output.find("new_name") != std::string::npos);
    assert(output.find("old_name") == std::string::npos);
}

TEST(modification_set_number) {
    auto doc = WKTDocument::parse("SPHEROID[\"test\",6378137.0,298.257224]");
    
    bool result = doc.setNumber("SPHEROID", 1, 300.0);
    assert(result);
    assert(std::abs(doc.root()->numbers()[1] - 300.0) < 0.0001);
    
    // invalid index should fail
    result = doc.setNumber("SPHEROID", 10, 1.0);
    assert(!result);
}

TEST(modification_set_numbers) {
    auto doc = WKTDocument::parse("SPHEROID[\"test\",100.0,200.0]");
    
    std::vector<double> newValues = {111.0, 222.0};
    bool result = doc.setNumbers("SPHEROID", newValues);
    assert(result);
    
    // wrong size should fail
    std::vector<double> wrongSize = {1.0, 2.0, 3.0};
    result = doc.setNumbers("SPHEROID", wrongSize);
    assert(!result);
}

TEST(modification_nested) {
    std::string wkt = 
        "DATUM[\"D_WGS\","
        "SPHEROID[\"WGS\",6378137.0,298.257]]";
    
    auto doc = WKTDocument::parse(wkt);
    
    // modify nested element
    doc.setValue("SPHEROID", "ITRF_2008");
    doc.setNumber("SPHEROID", 0, 6378140.0);
    
    assert(doc.find("SPHEROID")->stringValue() == "ITRF_2008");
    assert(std::abs(doc.find("SPHEROID")->numbers()[0] - 6378140.0) < 0.1);
}

// ============================================================================
// serialization tests
// ============================================================================

TEST(serialization_roundtrip) {
    std::string original = "GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137,298.257224]],PRIMEM[\"Greenwich\",0],UNIT[\"Degree\",0.0174532925199433]]";
    
    auto doc1 = WKTDocument::parse(original);
    std::string serialized = doc1.toString();
    
    auto doc2 = WKTDocument::parse(serialized);
    
    assert(utils::areEquivalent(doc1, doc2));
}

TEST(serialization_pretty) {
    auto doc = WKTDocument::parse("GEOGCS[\"test\",DATUM[\"d\",SPHEROID[\"s\",1,2]]]");
    std::string pretty = doc.toString(true);
    
    // should contain newlines
    assert(pretty.find('\n') != std::string::npos);
}

// ============================================================================
// utility tests
// ============================================================================

TEST(utils_validate) {
    assert(utils::validateWKT("GEOGCS[\"test\"]"));
    assert(!utils::validateWKT("GEOGCS[\"test\""));  // missing bracket
    assert(!utils::validateWKT(""));
}

TEST(utils_guess_epsg) {
    auto doc = WKTDocument::parse("GEOGCS[\"test\",DATUM[\"D_WGS_1984\"]]");
    auto epsg = utils::guessEPSG(doc);
    
    assert(epsg.has_value());
    assert(*epsg == 4326);
}

// ============================================================================
// edge cases
// ============================================================================

TEST(edge_empty_values) {
    // some wkt has empty slots: UNIT["Degree",0.017,,666,1.0]
    auto doc = WKTDocument::parse("UNIT[\"Degree\",0.017,,666,1.0]");
    
    assert(doc.root()->stringValue() == "Degree");
    // parser should handle empty values gracefully
}

TEST(edge_scientific_notation) {
    auto doc = WKTDocument::parse("TEST[\"n\",1.5e-10,2.3E+5,-4.5e10]");
    
    const auto& nums = doc.root()->numbers();
    assert(nums.size() == 3);
    assert(std::abs(nums[0] - 1.5e-10) < 1e-20);
    assert(std::abs(nums[1] - 2.3e+5) < 1);
    assert(std::abs(nums[2] - -4.5e10) < 1e5);
}

TEST(edge_deeply_nested) {
    std::string wkt = "A[\"a\",B[\"b\",C[\"c\",D[\"d\",E[\"e\",1,2,3]]]]]";
    auto doc = WKTDocument::parse(wkt);
    
    auto* e = doc.find("E");
    assert(e != nullptr);
    assert(e->numbers().size() == 3);
}

TEST(edge_multiple_empty_values) {
    // handle multiple consecutive empty values
    auto doc = WKTDocument::parse("TEST[\"name\",1.0,,,2.0,,,3.0]");
    
    assert(doc.root()->stringValue() == "name");
    // should parse without crashing
}

// ============================================================================
// main
// ============================================================================

int main() 
{
    std::cout << "=== WKT Parser Tests ===\n\n";
    
    int failures = 0;
    
    // lexer tests
    std::cout << "--- Lexer ---\n";
    RUN_TEST(lexer_simple);
    RUN_TEST(lexer_numbers);
    RUN_TEST(lexer_whitespace);
    
    // parser tests
    std::cout << "\n--- Parser ---\n";
    RUN_TEST(parser_simple_section);
    RUN_TEST(parser_section_with_numbers);
    RUN_TEST(parser_nested);
    RUN_TEST(parser_complex);
    RUN_TEST(parser_pulkovo);
    
    // real-world samples
    std::cout << "\n--- Real-world Samples ---\n";
    RUN_TEST(sample_simple_primem);
    RUN_TEST(sample_unit_parameter);
    RUN_TEST(sample_full_wgs84);
    RUN_TEST(sample_pulkovo_with_extra_values);
    RUN_TEST(sample_deeply_nested_units);
    RUN_TEST(sample_complex_with_projections);
    RUN_TEST(sample_simple_projection);
    
    // navigation tests
    std::cout << "\n--- Navigation ---\n";
    RUN_TEST(navigation_find_by_path);
    
    // modification tests
    std::cout << "\n--- Modification ---\n";
    RUN_TEST(modification_set_string);
    RUN_TEST(modification_set_number);
    RUN_TEST(modification_set_numbers);
    RUN_TEST(modification_nested);
    
    // serialization tests
    std::cout << "\n--- Serialization ---\n";
    RUN_TEST(serialization_roundtrip);
    RUN_TEST(serialization_pretty);
    
    // utility tests
    std::cout << "\n--- Utilities ---\n";
    RUN_TEST(utils_validate);
    RUN_TEST(utils_guess_epsg);
    
    // edge cases
    std::cout << "\n--- Edge Cases ---\n";
    RUN_TEST(edge_empty_values);
    RUN_TEST(edge_scientific_notation);
    RUN_TEST(edge_deeply_nested);
    RUN_TEST(edge_multiple_empty_values);
    
    std::cout << "\n=== Summary ===\n";
    if (failures == 0) {
        std::cout << "All tests passed!\n";
    } else {
        std::cout << failures << " test(s) failed.\n";
    }
    
    // demo usage
    std::cout << "\n=== Demo ===\n";
    
    const std::string wkt = 
        "GEOGCS[\"GCS_Pulkovo_1942\","
        "DATUM[\"D_Pulkovo_1942\","
        "SPHEROID[\"Krasovsky_1940\",6378245.0,298.3]],"
        "PRIMEM[\"Greenwich\",0.0],"
        "UNIT[\"Degree\",0.0174532925199433]]";
    
    std::cout << "Original:\n" << wkt << "\n\n";
    
    auto doc = WKTDocument::parse(wkt);
    
    // query
    std::cout << "Datum: " << doc.getDatumName().value_or("unknown") << "\n";
    std::cout << "Spheroid: " << doc.getSpheroidName().value_or("unknown") << "\n";
    
    if (auto params = doc.getSpheroidParams()) {
        std::cout << "Semi-major axis: " << params->first << "\n";
        std::cout << "Inverse flattening: " << params->second << "\n";
    }
    
    // modify
    doc.setValue("SPHEROID", "ITRF_2008");
    doc.setNumber("SPHEROID", 0, 6378140.0);
    
    std::cout << "\nModified (pretty):\n" << doc.toString(true) << "\n";
    
    return failures;
}
