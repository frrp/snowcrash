//
//  test-ResourceParser.cc
//  snowcrash
//
//  Created by Zdenek Nemec on 5/4/13.
//  Copyright (c) 2013 Apiary Inc. All rights reserved.
//

#include <iterator>
#include "catch.hpp"
#include "ResourceParser.h"
#include "ResourceGroupParser.h"
#include "Fixture.h"

using namespace snowcrash;
using namespace snowcrashtest;

MarkdownBlock::Stack snowcrashtest::CanonicalResourceFixture()
{
    // Blueprint in question:
    //R"(
    //# My Resource [/resource]
    //Resource Description
    //
    //+ My Resource Object (text/plain)
    //
    //        X.O.
    //
    //+ Headers
    //
    //        X-Resource-Header: Swordfighter XXII
    //
    // <see CanonicalMethodFixture()>
    //
    //)";
    
    MarkdownBlock::Stack markdown;
    markdown.push_back(MarkdownBlock(HeaderBlockType, "My Resource [/resource]", 1, MakeSourceDataBlock(0, 1)));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "Resource Description", 0, MakeSourceDataBlock(1, 1)));    

    markdown.push_back(MarkdownBlock(ListBlockBeginType, SourceData(), 0, SourceDataBlock()));

    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "My Resource Object (text/plain)", 0, MakeSourceDataBlock(2, 1)));
    markdown.push_back(MarkdownBlock(CodeBlockType, "X.O.", 0, MakeSourceDataBlock(3, 1)));
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, SourceData(), 0, MakeSourceDataBlock(4, 1)));
    
    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "Headers", 0, MakeSourceDataBlock(5, 1)));
    markdown.push_back(MarkdownBlock(CodeBlockType, "X-Resource-Header: Swordfighter XXII", 0, MakeSourceDataBlock(6, 1)));
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, SourceData(), 0, MakeSourceDataBlock(7, 1)));
    
    markdown.push_back(MarkdownBlock(ListBlockEndType, SourceData(), 0, MakeSourceDataBlock(8, 1)));
    
    MarkdownBlock::Stack methodBlocks = CanonicalMethodFixture();
    markdown.insert(markdown.end(), methodBlocks.begin(), methodBlocks.end());
    
    return markdown;
}

TEST_CASE("rparser/classifier", "Resource block classifier")
{
    MarkdownBlock::Stack markdown = CanonicalResourceFixture();
    
    BlockIterator cur = markdown.begin();
    // Named resource: "My Resource [/resource]"
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), UndefinedSection) == ResourceSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ResourceSection) == UndefinedSection);
    
    ++cur; // "Resource Description"
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), UndefinedSection) == UndefinedSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ResourceSection) == ResourceSection);

    ++cur; // ListBlockBeginType - "My Resource Object"
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), UndefinedSection) == ObjectSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ResourceSection) == ObjectSection);
    
    ++cur; // ListItemBlockBeginType - "My Resource Object"
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), UndefinedSection) == ObjectSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ObjectSection) == ObjectSection);

    std::advance(cur, 4); // ListItemBlockBeginType - "Headers"
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), UndefinedSection) == HeadersSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ResourceSection) == HeadersSection);
    
    std::advance(cur, 5); // Method
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), UndefinedSection) == MethodSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ResourceSection) == MethodSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), HeadersSection) == MethodSection);
    
    // Nameless resource: "/resource"
    markdown[0].content = "/resource";
    cur = markdown.begin();
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), UndefinedSection) == ResourceSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ResourceSection) == UndefinedSection);
    
    // Keyword "group"
    markdown[0].content = "Group A";
    cur = markdown.begin();
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), UndefinedSection) == UndefinedSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ResourceSection) == UndefinedSection);
}

TEST_CASE("rparser/classifier-abbrev", "Abbreviated Resource Method block classifier")
{
    MarkdownBlock::Stack markdown;
    markdown.push_back(MarkdownBlock(HeaderBlockType, "GET /resource", 1, MakeSourceDataBlock(0, 1)));
    markdown.push_back(MarkdownBlock(HeaderBlockType, "POST", 1, MakeSourceDataBlock(1, 1)));
    
    BlockIterator cur = markdown.begin();
    // "GET /resource"
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), UndefinedSection) == ResourceMethodSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ResourceMethodSection) == UndefinedSection);
    
    ++cur; // "POST"
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ResourceSection) == MethodSection);
    REQUIRE(ClassifyBlock<Resource>(cur, markdown.end(), ResourceMethodSection) == UndefinedSection);
}

TEST_CASE("rparser/parse", "Parse resource")
{
    MarkdownBlock::Stack markdown = CanonicalResourceFixture();
    Resource resource;
    BlueprintParserCore parser(0, SourceDataFixture, Blueprint());
    ParseSectionResult result = ResourceParser::Parse(markdown.begin(), markdown.end(), parser, resource);
    
    REQUIRE(result.first.error.code == Error::OK);
    CHECK(result.first.warnings.empty());
    
    const MarkdownBlock::Stack &blocks = markdown;
    REQUIRE(std::distance(blocks.begin(), result.second) == 42);
    
    REQUIRE(resource.name == "My Resource");
    REQUIRE(resource.uriTemplate == "/resource");
    REQUIRE(resource.object.name == "My Resource");
    REQUIRE(resource.object.body == "X.O.");
    REQUIRE(resource.object.headers.size() == 1);
    REQUIRE(resource.object.headers[0].first == "Content-Type");
    REQUIRE(resource.object.headers[0].second == "text/plain");
    REQUIRE(resource.description == "1");
    REQUIRE(resource.headers.size() == 1);
    REQUIRE(resource.headers[0].first == "X-Resource-Header");
    REQUIRE(resource.headers[0].second == "Swordfighter XXII");
    REQUIRE(resource.methods.size() == 1);
    REQUIRE(resource.methods.front().method == "GET");
}

TEST_CASE("rparser/parse-partial", "Parse partially defined resource")
{
    MarkdownBlock::Stack markdown;
    markdown.push_back(MarkdownBlock(HeaderBlockType, "/1", 1, MakeSourceDataBlock(0, 1)));
    markdown.push_back(MarkdownBlock(HeaderBlockType, "GET", 1, MakeSourceDataBlock(1, 1)));
    
    markdown.push_back(MarkdownBlock(ListBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "Request", 0, MakeSourceDataBlock(2, 1)));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "p1", 0, MakeSourceDataBlock(3, 1)));
    
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, SourceData(), 0, MakeSourceDataBlock(4, 1)));
    markdown.push_back(MarkdownBlock(ListBlockEndType, SourceData(), 0, MakeSourceDataBlock(5, 1)));
    
    Resource resource;
    BlueprintParserCore parser(0, SourceDataFixture, Blueprint());
    ParseSectionResult result = ResourceParser::Parse(markdown.begin(), markdown.end(), parser, resource);
    
    REQUIRE(result.first.error.code == Error::OK);
    REQUIRE(result.first.warnings.size() == 2); // no response & preformatted asset
    
    const MarkdownBlock::Stack &blocks = markdown;
    REQUIRE(std::distance(blocks.begin(), result.second) == 8);

    REQUIRE(resource.name.empty());
    REQUIRE(resource.uriTemplate == "/1");
    REQUIRE(resource.description.empty());
    REQUIRE(resource.object.name.empty());
    REQUIRE(resource.object.body.empty());
    REQUIRE(resource.methods.size() == 1);
    REQUIRE(resource.methods.front().method == "GET");
    REQUIRE(resource.methods.front().description.empty());
    REQUIRE(resource.methods.front().requests.size() == 1);
    REQUIRE(resource.methods.front().requests.front().name.empty());
    REQUIRE(resource.methods.front().requests.front().description.empty());
    REQUIRE(resource.methods.front().requests.front().body == "3");
}

TEST_CASE("rparser/parse-multi-method-desc", "Parse multiple method descriptions")
{
    MarkdownBlock::Stack markdown;
    markdown.push_back(MarkdownBlock(HeaderBlockType, "/1", 1, MakeSourceDataBlock(0, 1)));    
    markdown.push_back(MarkdownBlock(HeaderBlockType, "GET", 1, MakeSourceDataBlock(0, 1)));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "p1", 0, MakeSourceDataBlock(1, 1)));
    markdown.push_back(MarkdownBlock(HeaderBlockType, "POST", 1, MakeSourceDataBlock(2, 1)));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "p2", 0, MakeSourceDataBlock(3, 1)));
    
    Resource resource;
    BlueprintParserCore parser(0, SourceDataFixture, Blueprint());
    ParseSectionResult result = ResourceParser::Parse(markdown.begin(), markdown.end(), parser, resource);
    
    REQUIRE(result.first.error.code == Error::OK);
    REQUIRE(result.first.warnings.size() == 2); // 2x no response
    
    const MarkdownBlock::Stack &blocks = markdown;
    REQUIRE(std::distance(blocks.begin(), result.second) == 5);
    
    REQUIRE(resource.uriTemplate == "/1");
    REQUIRE(resource.description.empty());
    REQUIRE(resource.object.name.empty());
    REQUIRE(resource.object.body.empty());
    REQUIRE(resource.methods.size() == 2);
    REQUIRE(resource.methods[0].method == "GET");
    REQUIRE(resource.methods[0].description == "1");
    REQUIRE(resource.methods[1].method == "POST");
    REQUIRE(resource.methods[1].description == "3");
}

TEST_CASE("rparser/parse-multi-method", "Parse multiple method")
{    
    // Blueprint in question:
    //R"(
    //# /1
    //A
    //
    //## GET
    //B
    //
    //+ Response 200
    //    + Body
    //
    //            Code 1
    //
    //## HEAD
    //C
    //
    //+ Response 200
    //    + Body
    //
    //+ Request D
    //
    //## PUT
    //E
    //");
    
    MarkdownBlock::Stack markdown;
    markdown.push_back(MarkdownBlock(HeaderBlockType, "/1", 1, MakeSourceDataBlock(0, 1)));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "A", 0, MakeSourceDataBlock(1, 1)));
    
    markdown.push_back(MarkdownBlock(HeaderBlockType, "GET", 2, MakeSourceDataBlock(2, 1)));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "B", 0, MakeSourceDataBlock(3, 1)));
    markdown.push_back(MarkdownBlock(ListBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "Response 200", 0, MakeSourceDataBlock(4, 1)));
    markdown.push_back(MarkdownBlock(ListBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "Body", 0, MakeSourceDataBlock(5, 1)));
    markdown.push_back(MarkdownBlock(CodeBlockType, "Code 1", 0, MakeSourceDataBlock(6, 1)));
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, SourceData(), 0, MakeSourceDataBlock(7, 1)));
    markdown.push_back(MarkdownBlock(ListBlockEndType, SourceData(), 0, MakeSourceDataBlock(8, 1)));
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, SourceData(), 0, MakeSourceDataBlock(9, 1)));
    markdown.push_back(MarkdownBlock(ListBlockEndType, SourceData(), 0, MakeSourceDataBlock(10, 1)));
    
    markdown.push_back(MarkdownBlock(HeaderBlockType, "HEAD", 2, MakeSourceDataBlock(11, 1)));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "C", 0, MakeSourceDataBlock(12, 1)));
    markdown.push_back(MarkdownBlock(ListBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "Response 200", 0, MakeSourceDataBlock(13, 1)));
    markdown.push_back(MarkdownBlock(ListBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, "Body", 0, MakeSourceDataBlock(14, 1)));
    markdown.push_back(MarkdownBlock(ListBlockEndType, SourceData(), 0, MakeSourceDataBlock(15, 1)));
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, SourceData(), 0, MakeSourceDataBlock(16, 1)));
    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, "Request D", 0, MakeSourceDataBlock(17, 1)));
    markdown.push_back(MarkdownBlock(ListBlockEndType, SourceData(), 0, MakeSourceDataBlock(18, 1)));
    
    markdown.push_back(MarkdownBlock(HeaderBlockType, "PUT", 2, MakeSourceDataBlock(19, 1)));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "E", 0, MakeSourceDataBlock(20, 1)));
    
    Resource resource;
    BlueprintParserCore parser(0, SourceDataFixture, Blueprint());
    ParseSectionResult result = ResourceParser::Parse(markdown.begin(), markdown.end(), parser, resource);
    
    REQUIRE(result.first.error.code == Error::OK);
    CHECK(result.first.warnings.size() == 3); // 2x empty body asset & no response
    
    const MarkdownBlock::Stack &blocks = markdown;
    REQUIRE(std::distance(blocks.begin(), result.second) == 30);
    
    REQUIRE(resource.uriTemplate == "/1");
    REQUIRE(resource.description == "1");
    REQUIRE(resource.object.name.empty());
    REQUIRE(resource.object.body.empty());
    REQUIRE(resource.methods.size() == 3);
    REQUIRE(resource.methods[0].method == "GET");
    REQUIRE(resource.methods[0].description == "3");
    REQUIRE(resource.methods[0].requests.empty());
    REQUIRE(resource.methods[0].responses.size() == 1);
    REQUIRE(resource.methods[0].responses[0].name == "200");
    REQUIRE(resource.methods[0].responses[0].description.empty());
    REQUIRE(resource.methods[0].responses[0].body == "Code 1");
    
    REQUIRE(resource.methods[1].method == "HEAD");
    REQUIRE(resource.methods[1].description == "C");
    REQUIRE(resource.methods[1].requests.size() == 1);
    REQUIRE(resource.methods[1].requests[0].name == "D");
    REQUIRE(resource.methods[1].requests[0].description.empty());
    REQUIRE(resource.methods[1].requests[0].description.empty());
    
    REQUIRE(resource.methods[1].responses.size() == 1);
    REQUIRE(resource.methods[1].responses[0].name == "200");
    REQUIRE(resource.methods[1].responses[0].description.empty());
    REQUIRE(resource.methods[1].responses[0].body.empty());
    
    REQUIRE(resource.methods[2].method == "PUT");
    REQUIRE(resource.methods[2].description == "K");
    REQUIRE(resource.methods[2].requests.empty());
    REQUIRE(resource.methods[2].responses.empty());
}

TEST_CASE("rparser/parse-list-description", "Parse description with list")
{
    // Blueprint in question:
    //R"(
    //# /1
    //+ A
    //+ B
    //
    //p1
    //");
    
    MarkdownBlock::Stack markdown;
    markdown.push_back(MarkdownBlock(HeaderBlockType, "/1", 1, MakeSourceDataBlock(0, 1)));
    
    markdown.push_back(MarkdownBlock(ListBlockBeginType, SourceData(), 0, SourceDataBlock()));

    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, "A", 0, MakeSourceDataBlock(1, 1)));
    
    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, "B", 0, MakeSourceDataBlock(2, 1)));
    
    markdown.push_back(MarkdownBlock(ListBlockEndType, SourceData(), 0, MakeSourceDataBlock(3, 1)));
    
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "p1", 0, MakeSourceDataBlock(4, 1)));
    
    Resource resource;
    BlueprintParserCore parser(0, SourceDataFixture, Blueprint());
    ParseSectionResult result = ResourceParser::Parse(markdown.begin(), markdown.end(), parser, resource);
    
    REQUIRE(result.first.error.code == Error::OK);
    REQUIRE(result.first.warnings.empty());
    
    const MarkdownBlock::Stack &blocks = markdown;
    REQUIRE(std::distance(blocks.begin(), result.second) == 8);
    
    REQUIRE(resource.uriTemplate == "/1");
    REQUIRE(resource.description == "34");
    REQUIRE(resource.object.name.empty());
    REQUIRE(resource.object.body.empty());
    REQUIRE(resource.methods.empty());
}

TEST_CASE("rparser/parse-hr", "Parse resource with a HR")
{
    
    // Blueprint in question:
    //R"(
    //# /1
    //---
    //A
    
    MarkdownBlock::Stack markdown;
    markdown.push_back(MarkdownBlock(HeaderBlockType, "/1", 1, MakeSourceDataBlock(0, 1)));
    markdown.push_back(MarkdownBlock(HRuleBlockType, SourceData(), 0, MakeSourceDataBlock(1, 1)));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "A", 0, MakeSourceDataBlock(2, 1)));
    
    Resource resource;
    BlueprintParserCore parser(0, SourceDataFixture, Blueprint());
    ParseSectionResult result = ResourceParser::Parse(markdown.begin(), markdown.end(), parser, resource);
    
    REQUIRE(result.first.error.code == Error::OK);
    CHECK(result.first.warnings.empty());
    
    const MarkdownBlock::Stack &blocks = markdown;
    REQUIRE(std::distance(blocks.begin(), result.second) == 3);
    
    REQUIRE(resource.uriTemplate == "/1");
    REQUIRE(resource.description == "12");
    REQUIRE(resource.object.name.empty());
    REQUIRE(resource.object.body.empty());
    REQUIRE(resource.methods.empty());
}

TEST_CASE("rparser/header-warnings", "Check warnings on overshadowing a header")
{
    MarkdownBlock::Stack markdown = CanonicalResourceFixture();
    Resource resource;
    resource.headers.push_back(std::make_pair("X-Header", "24"));
    
    BlueprintParserCore parser(0, SourceDataFixture, Blueprint());
    ParseSectionResult result = ResourceParser::Parse(markdown.begin(),
                                                      markdown.end(),
                                                      parser,
                                                      resource);
    
    REQUIRE(result.first.error.code == Error::OK);
    REQUIRE(result.first.warnings.size() == 1); // overshadowing header
    
    const MarkdownBlock::Stack &blocks = markdown;
    REQUIRE(std::distance(blocks.begin(), result.second) == 42);
}

TEST_CASE("rparser/parse-abbrev", "Parse resource method abbreviation")
{
    // Blueprint in question:
    //R"(
    //# GET /resource
    //Description
    //
    //+ Response 200
    //    + Body
    //
    //            { ... }
    //");
    
    MarkdownBlock::Stack markdown;
    markdown.push_back(MarkdownBlock(HeaderBlockType, "GET /resource", 1, MakeSourceDataBlock(0, 1)));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "Description", 1, MakeSourceDataBlock(1, 1)));

    markdown.push_back(MarkdownBlock(ListBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "Response 200", 0, MakeSourceDataBlock(2, 1)));

    markdown.push_back(MarkdownBlock(ListBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ListItemBlockBeginType, SourceData(), 0, SourceDataBlock()));
    markdown.push_back(MarkdownBlock(ParagraphBlockType, "Body", 0, MakeSourceDataBlock(3, 1)));
    markdown.push_back(MarkdownBlock(CodeBlockType, "{ ... }", 0, MakeSourceDataBlock(4, 1)));
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, SourceData(), 0, MakeSourceDataBlock(5, 1)));
    markdown.push_back(MarkdownBlock(ListBlockEndType, SourceData(), 0, MakeSourceDataBlock(6, 1)));
    
    markdown.push_back(MarkdownBlock(ListItemBlockEndType, SourceData(), 0, MakeSourceDataBlock(7, 1)));
    markdown.push_back(MarkdownBlock(ListBlockEndType, SourceData(), 0, MakeSourceDataBlock(8, 1)));
    
    Resource resource;
    BlueprintParserCore parser(0, SourceDataFixture, Blueprint());    
    ParseSectionResult result = ResourceParser::Parse(markdown.begin(), markdown.end(), parser, resource);
    
    REQUIRE(result.first.error.code == Error::OK);
    REQUIRE(result.first.warnings.empty());
    
    const MarkdownBlock::Stack &blocks = markdown;
    REQUIRE(std::distance(blocks.begin(), result.second) == 13);
    
    REQUIRE(resource.name.empty());
    REQUIRE(resource.object.name.empty());
    REQUIRE(resource.object.body.empty());
    REQUIRE(resource.methods.size() == 1);
    REQUIRE(resource.methods[0].method == "GET");
    REQUIRE(resource.methods[0].description == "1");
    REQUIRE(resource.methods[0].responses.size() == 1);
}

TEST_CASE("rparser/parse-abbrev-ambiguous", "Parse resource method abbreviation followed by a foreign method")
{
    // Blueprint in question:
    //R"(
    //# GET /resource
    //# POST
    //");
    
    MarkdownBlock::Stack markdown;
    markdown.push_back(MarkdownBlock(HeaderBlockType, "GET /resource", 1, MakeSourceDataBlock(0, 1)));
    markdown.push_back(MarkdownBlock(HeaderBlockType, "POST", 1, MakeSourceDataBlock(1, 1)));
    
    Resource resource;
    BlueprintParserCore parser(0, SourceDataFixture, Blueprint());
    ParseSectionResult result = ResourceParser::Parse(markdown.begin(), markdown.end(), parser, resource);
    
    REQUIRE(result.first.error.code == Error::OK);
    REQUIRE(result.first.warnings.size() == 2); // no response & ignoring possible resource method
    
    const MarkdownBlock::Stack &blocks = markdown;
    REQUIRE(std::distance(blocks.begin(), result.second) == 1);
    
    REQUIRE(resource.name.empty());
    REQUIRE(resource.object.name.empty());
    REQUIRE(resource.object.body.empty());
    REQUIRE(resource.methods.size() == 1);
    REQUIRE(resource.methods[0].method == "GET");
}

TEST_CASE("rparser/parse-nameless-resource", "Parse resource without name")
{
    // Blueprint in question:
    //R"(
    //# /resource
    //");
    
    MarkdownBlock::Stack markdown;
    markdown.push_back(MarkdownBlock(HeaderBlockType, "/resource", 1, MakeSourceDataBlock(0, 1)));
    
    Resource resource;
    BlueprintParserCore parser(0, SourceDataFixture, Blueprint());
    ParseSectionResult result = ResourceParser::Parse(markdown.begin(), markdown.end(), parser, resource);
    
    REQUIRE(result.first.error.code == Error::OK);
    CHECK(result.first.warnings.size() == 0);
    
    const MarkdownBlock::Stack &blocks = markdown;
    REQUIRE(std::distance(blocks.begin(), result.second) == 1);

    REQUIRE(resource.uriTemplate == "/resource");
    REQUIRE(resource.name.empty());
    REQUIRE(resource.object.name.empty());
    REQUIRE(resource.object.body.empty());
    REQUIRE(resource.methods.size() == 0);
}

