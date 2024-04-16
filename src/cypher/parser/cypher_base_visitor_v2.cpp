﻿/**
 * Copyright 2022 AntGroup CO., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "cypher/parser/cypher_base_visitor_v2.h"
#include "cypher/utils/geax_util.h"
#include "cypher/parser/data_typedef.h"
#include "geax-front-end/ast/Ast.h"

namespace parser {

#ifndef ALLOC_GEAOBJECT
#define ALLOC_GEAOBJECT(obj) objAlloc_.allocate<obj>()
#endif  // !ALLOC_GEAOBJECT

#ifndef SWITCH_CONTEXT_VISIT
#define SWITCH_CONTEXT_VISIT(ctx, current)                          \
    ({                                                              \
        geax::frontend::AstNode* prev = node_;                      \
        node_ = current;                                            \
        ctx->accept(this);                                          \
        node_ = prev;                                               \
    })
#endif  // !SWITCH_CONTEXT_VISIT

#ifndef SWITCH_CONTEXT_VISIT_CHILDREN
#define SWITCH_CONTEXT_VISIT_CHILDREN(ctx, current)                 \
    ({                                                              \
        geax::frontend::AstNode* prev = node_;                      \
        node_ = current;                                            \
        visitChildren(ctx);                                         \
        node_ = prev;                                               \
    })
#endif  // !SWITCH_CONTEXT_VISIT_CHILDREN

template <typename Base, typename Drive>
void checkedCast(Base* b, Drive*& d) {
    static_assert(std::is_base_of<Base, Drive>::value, "type `Base` must be the base of type `Drive`");
    d = dynamic_cast<Drive*>(b);
    assert(d);
}

template <typename Dst>
void checkedAnyCast(const std::any& s, Dst*& d) {
    try {
        d = std::any_cast<Dst*>(s);
    } catch (...) {
        abort();
        //assert(false);
    }
}

template <typename Dst>
void checkedAnyCast(const std::any& s, Dst& d) {
    try {
        d = std::any_cast<Dst>(s);
    } catch (...) {
        abort();
        //assert(false);
    }
}

std::string CypherBaseVisitorV2::GetFullText(antlr4::ParserRuleContext* ruleCtx) const {
    if (ruleCtx->children.size() == 0) {
        return "";
    }
    auto* startToken = ruleCtx->getStart();
    auto* stopToken = ruleCtx->getStop();
    antlr4::misc::Interval interval(startToken->getStartIndex(), stopToken->getStopIndex());
    return ruleCtx->getStart()->getInputStream()->getText(interval);
}

// TODO support EXPLAIN
CypherBaseVisitorV2::CypherBaseVisitorV2(geax::common::ObjectArenaAllocator& objAlloc,
                                         antlr4::tree::ParseTree *tree)
        : objAlloc_(objAlloc)
        //, node_(objAlloc_.allocate<geax::frontend::ExplainActivity>())
        , node_(ALLOC_GEAOBJECT(geax::frontend::NormalTransaction))
        , anonymous_idx_(0) {
    tree->accept(this);
}

std::any CypherBaseVisitorV2::visitOC_Cypher(LcypherParser::OC_CypherContext *ctx) {
    visitChildren(ctx);
    return 0;
}

std::string CypherBaseVisitorV2::GenAnonymousAlias(bool is_node) {
    std::string alias(ANONYMOUS);
    if (is_node) {
        alias.append("N").append(std::to_string(anonymous_idx_));
    } else {
        alias.append("R").append(std::to_string(anonymous_idx_));
    }
    anonymous_idx_++;
    return alias;
}

// TODO support EXPLAIN
std::any CypherBaseVisitorV2::visitOC_Statement(LcypherParser::OC_StatementContext *ctx) {
    //geax::frontend::ExplainActivity* node = static_cast<geax::frontend::ExplainActivity*>(node_);
    geax::frontend::NormalTransaction* node = nullptr;
    checkedCast(node_, node);
    if (ctx->EXPLAIN()) {
        //node->setIsProfile(false);
    } else if (ctx->PROFILE()) {
        //node->setIsProfile(true);
    } else {
        auto body = ALLOC_GEAOBJECT(geax::frontend::ProcedureBody);
        node->setProcedureBody(body);
        SWITCH_CONTEXT_VISIT_CHILDREN(ctx, body);
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Query(LcypherParser::OC_QueryContext *ctx) {
    visitChildren(ctx);
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_RegularQuery(LcypherParser::OC_RegularQueryContext *ctx) {
    visit(ctx->oC_SingleQuery());
//    for (auto u : ctx->oC_Union()) {
//        NOT_SUPPORT_AND_THROW();
//    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Union(LcypherParser::OC_UnionContext *ctx) {
    NOT_SUPPORT_AND_THROW();
}

std::any CypherBaseVisitorV2::visitOC_SingleQuery(LcypherParser::OC_SingleQueryContext *ctx) {
    geax::frontend::ProcedureBody* node = nullptr;
    checkedCast(node_, node);
    if (ctx->oC_MultiPartQuery()) {
        NOT_SUPPORT_AND_THROW();
    } else {
        auto headStmt = ALLOC_GEAOBJECT(geax::frontend::StatementWithYield);
        node->appendStatement(headStmt);
        SWITCH_CONTEXT_VISIT_CHILDREN(ctx, headStmt);
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_SinglePartQuery(
    LcypherParser::OC_SinglePartQueryContext *ctx) {
    if (ctx->oC_ReadingClause().size() > 2) NOT_SUPPORT_AND_THROW();

    geax::frontend::StatementWithYield* node = nullptr;
    checkedCast(node_, node);
    auto stmt = ALLOC_GEAOBJECT(geax::frontend::QueryStatement);
    node->setStatement(stmt);
    auto join = ALLOC_GEAOBJECT(geax::frontend::JoinQueryExpression);
    stmt->setJoinQuery(join);
    auto co = ALLOC_GEAOBJECT(geax::frontend::CompositeQueryStatement);
    join->setHead(co);
    auto l = ALLOC_GEAOBJECT(geax::frontend::AmbientLinearQueryStatement);
    co->setHead(l);
    SWITCH_CONTEXT_VISIT_CHILDREN(ctx, l);
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_MultiPartQuery(LcypherParser::OC_MultiPartQueryContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_UpdatingClause(LcypherParser::OC_UpdatingClauseContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_ReadingClause(LcypherParser::OC_ReadingClauseContext *ctx) {
    return visitChildren(ctx);
}

std::any CypherBaseVisitorV2::visitOC_Match(LcypherParser::OC_MatchContext *ctx) {
    geax::frontend::AmbientLinearQueryStatement* node = nullptr;
    checkedCast(node_, node);
    auto match = ALLOC_GEAOBJECT(geax::frontend::MatchStatement);
    node->appendQueryStatement(match);
    if (ctx->OPTIONAL_()) match->setStatementMode(geax::frontend::StatementMode::kOptional);
    auto graph_pattern = ALLOC_GEAOBJECT(geax::frontend::GraphPattern);
    match->setGraphPattern(graph_pattern);
    SWITCH_CONTEXT_VISIT(ctx->oC_Pattern(), graph_pattern);
    if (ctx->oC_Where()) {
        SWITCH_CONTEXT_VISIT(ctx->oC_Where(), graph_pattern);
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Unwind(LcypherParser::OC_UnwindContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Merge(LcypherParser::OC_MergeContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_MergeAction(LcypherParser::OC_MergeActionContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Create(LcypherParser::OC_CreateContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Set(LcypherParser::OC_SetContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_SetItem(LcypherParser::OC_SetItemContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Delete(LcypherParser::OC_DeleteContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Remove(LcypherParser::OC_RemoveContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_RemoveItem(LcypherParser::OC_RemoveItemContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_InQueryCall(LcypherParser::OC_InQueryCallContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_StandaloneCall(LcypherParser::OC_StandaloneCallContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_YieldItems(LcypherParser::OC_YieldItemsContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_YieldItem(LcypherParser::OC_YieldItemContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_With(LcypherParser::OC_WithContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Return(LcypherParser::OC_ReturnContext *ctx) {
    geax::frontend::AmbientLinearQueryStatement* node = nullptr;
    checkedCast(node_, node);
    auto result = ALLOC_GEAOBJECT(geax::frontend::PrimitiveResultStatement);
    result->setDistinct(ctx->DISTINCT() != nullptr);
    node->setResultStatement(result);
    SWITCH_CONTEXT_VISIT(ctx->oC_ReturnBody(), result);
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_ReturnBody(LcypherParser::OC_ReturnBodyContext *ctx) {
    geax::frontend::AstNode * prev = node_;
    visit(ctx->oC_ReturnItems());
    node_ = prev;
    if (ctx->oC_Order()) {
        geax::frontend::AstNode * p = node_;
        visit(ctx->oC_Order());
        node_ = p;
    }
    if (ctx->oC_Limit()) {
        geax::frontend::AstNode * p = node_;
        visit(ctx->oC_Limit());
        node_ = p;
    }
    if (ctx->oC_Skip()) {
        geax::frontend::AstNode * p = node_;
        visit(ctx->oC_Skip());
        node_ = p;
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_ReturnItems(LcypherParser::OC_ReturnItemsContext *ctx) {
    for (auto &item : ctx->oC_ReturnItem()) {
        visit(item);
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_ReturnItem(LcypherParser::OC_ReturnItemContext *ctx) {
    geax::frontend::PrimitiveResultStatement* node = nullptr;
    checkedCast(node_, node);
    geax::frontend::Expr* expr = nullptr;
    checkedAnyCast(visit(ctx->oC_Expression()), expr);
    std::string variable;
    if (ctx->oC_Variable()) {
        variable = ctx->oC_Variable()->getText();
    } else {
        variable = GetFullText(ctx->oC_Expression());
    }
    node->appendItem(std::move(variable), expr);
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Order(LcypherParser::OC_OrderContext *ctx) {
    for (size_t idx = 0; idx < ctx->oC_SortItem().size(); ++idx) {
        visit(ctx->oC_SortItem(idx));
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Skip(LcypherParser::OC_SkipContext *ctx) {
    geax::frontend::PrimitiveResultStatement* node = nullptr;
    checkedCast(node_, node);
    geax::frontend::Expr* expr = nullptr;
    checkedAnyCast(visit(ctx->oC_Expression()), expr);
    geax::frontend::VInt* integer = nullptr;
    checkedCast(expr, integer);
    node->setOffset(integer->val());
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Limit(LcypherParser::OC_LimitContext *ctx) {
    geax::frontend::PrimitiveResultStatement* node = nullptr;
    checkedCast(node_, node);
    geax::frontend::Expr* expr = nullptr;
    checkedAnyCast(visit(ctx->oC_Expression()), expr);
    geax::frontend::VInt* integer = nullptr;
    checkedCast(expr, integer);
    node->setLimit(integer->val());
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_SortItem(LcypherParser::OC_SortItemContext *ctx) {
    geax::frontend::PrimitiveResultStatement* node = nullptr;
    checkedCast(node_, node);
    auto order = ALLOC_GEAOBJECT(geax::frontend::OrderByField);
    node->appendOrderBy(order);
    geax::frontend::Expr* expr = nullptr;
    checkedAnyCast(visit(ctx->oC_Expression()), expr);
    bool desc = ctx->DESCENDING() != nullptr || ctx->DESC() != nullptr ||
                     (ctx->ASCENDING() == nullptr && ctx->ASC() == nullptr);
    order->setField(expr);
    order->setOrder(desc);
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Hint(LcypherParser::OC_HintContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Where(LcypherParser::OC_WhereContext *ctx) {
    geax::frontend::GraphPattern* node = nullptr;
    checkedCast(node_, node);
    geax::frontend::Expr* where = nullptr;
    checkedAnyCast(visit(ctx->oC_Expression()), where);
    node->setWhere(where);
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Pattern(LcypherParser::OC_PatternContext *ctx) {
    geax::frontend::GraphPattern* node = nullptr;
    checkedCast(node_, node);
    for (auto &ctx_part : ctx->oC_PatternPart()) {
        auto pp = ALLOC_GEAOBJECT(geax::frontend::PathPattern);
        node->appendPathPattern(pp);
        SWITCH_CONTEXT_VISIT(ctx_part, pp);
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_PatternPart(LcypherParser::OC_PatternPartContext *ctx) {
    geax::frontend::PathPattern* node = nullptr;
    checkedCast(node_, node);
    auto pc = ALLOC_GEAOBJECT(geax::frontend::PathChain);
    node->appendChain(pc);
    SWITCH_CONTEXT_VISIT(ctx->oC_AnonymousPatternPart(), pc);
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_AnonymousPatternPart(
    LcypherParser::OC_AnonymousPatternPartContext *ctx) {
    return visit(ctx->oC_PatternElement());
}

std::any CypherBaseVisitorV2::visitOC_PatternElement(LcypherParser::OC_PatternElementContext *ctx) {
    if (ctx->oC_NodePattern() == nullptr) NOT_SUPPORT_AND_THROW();
    geax::frontend::PathChain* pathChain = nullptr;
    checkedCast(node_, pathChain);
    auto node = ALLOC_GEAOBJECT(geax::frontend::Node);
    pathChain->setHead(node);
    SWITCH_CONTEXT_VISIT(ctx->oC_NodePattern(), node);
    for (auto & chain : ctx->oC_PatternElementChain()) {
        SWITCH_CONTEXT_VISIT(chain, pathChain);
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_NodePattern(LcypherParser::OC_NodePatternContext *ctx) {
    geax::frontend::Node* node = nullptr;
    checkedCast(node_, node);
    auto filler = ALLOC_GEAOBJECT(geax::frontend::ElementFiller);
    node->setFiller(filler);
    if (ctx->oC_Variable() != nullptr) {
        std::string alias;
        checkedAnyCast(visit(ctx->oC_Variable()), alias);
        filler->setV(std::move(alias));
    } else {
        // if alias is absent, generate an alias for the node
        filler->setV(GenAnonymousAlias(true));
    }

    if (ctx->oC_NodeLabels() != nullptr) {
        std::vector<std::string> labels;
        checkedAnyCast(visit(ctx->oC_NodeLabels()), labels);
        if (labels.size() == 1) {
            auto l = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
            l->setLabel(std::move(labels[0]));
            filler->setLabel(l);
        } else if (labels.size() == 2) {
            auto root = ALLOC_GEAOBJECT(geax::frontend::LabelOr);
            auto l = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
            auto r = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
            l->setLabel(std::move(labels[0]));
            r->setLabel(std::move(labels[1]));
            root->setLeft(l);
            root->setRight(r);
            filler->setLabel(root);
        } else {
            auto root = ALLOC_GEAOBJECT(geax::frontend::LabelOr);
            auto label = root;
            for (size_t idx = 0; idx < labels.size() - 2; ++idx) {
                auto o = ALLOC_GEAOBJECT(geax::frontend::LabelOr);
                auto l = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
                l->setLabel(std::move(labels[idx]));
                label->setLeft(l);
                label->setRight(o);
                label = o;
            }
            auto l = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
            auto r = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
            l->setLabel(std::move(labels[labels.size() - 2]));
            r->setLabel(std::move(labels[labels.size() - 1]));
            label->setLeft(l);
            label->setRight(r);
            filler->setLabel(root);
        }
    }

    if (ctx->oC_Properties() != nullptr) {
        SWITCH_CONTEXT_VISIT(ctx->oC_Properties(), filler);
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_PatternElementChain(
    LcypherParser::OC_PatternElementChainContext *ctx) {
    geax::frontend::PathChain* pathChain = nullptr;
    checkedCast(node_, pathChain);
    auto edge = ALLOC_GEAOBJECT(geax::frontend::Edge);
    SWITCH_CONTEXT_VISIT(ctx->oC_RelationshipPattern(), edge);

    auto node = ALLOC_GEAOBJECT(geax::frontend::Node);
    SWITCH_CONTEXT_VISIT(ctx->oC_NodePattern(), node);
    pathChain->appendTail(edge, node);
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_RelationshipPattern(
    LcypherParser::OC_RelationshipPatternContext *ctx) {
    geax::frontend::Edge* edge = nullptr;
    checkedCast(node_, edge);
    geax::frontend::EdgeDirection direction;
    if (ctx->oC_LeftArrowHead() != nullptr && ctx->oC_RightArrowHead() == nullptr) {
        direction = geax::frontend::EdgeDirection::kPointLeft;
    } else if (ctx->oC_RightArrowHead() != nullptr && ctx->oC_LeftArrowHead() == nullptr) {
        direction = geax::frontend::EdgeDirection::kPointRight;
    } else {
        direction = geax::frontend::EdgeDirection::kAnyDirected;
    }
    edge->setDirection(direction);
    if (ctx->oC_RelationshipDetail()) {
        geax::frontend::AstNode *prev = node_;
        visit(ctx->oC_RelationshipDetail());
        node_ = prev;
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_RelationshipDetail(
    LcypherParser::OC_RelationshipDetailContext *ctx) {
    geax::frontend::Edge* edge = nullptr;
    checkedCast(node_, edge);
    auto filler = ALLOC_GEAOBJECT(geax::frontend::ElementFiller);
    edge->setFiller(filler);

    std::string variable;
    if (ctx->oC_Variable() != nullptr) {
        checkedAnyCast(visit(ctx->oC_Variable()), variable);
    } else {
        // if alias is absent, generate an alias for the relationship
        variable = GenAnonymousAlias(false);
    }
    filler->setV(std::move(variable));

    if (ctx->oC_RelationshipTypes() != nullptr) {
        SWITCH_CONTEXT_VISIT(ctx->oC_RelationshipTypes(), filler);
    }

    if (ctx->oC_Properties() != nullptr) {
        SWITCH_CONTEXT_VISIT(ctx->oC_Properties(), filler);
    }

    if (ctx->oC_RangeLiteral() != nullptr) {
        SWITCH_CONTEXT_VISIT(ctx->oC_RangeLiteral(), filler);
    }

    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Properties(LcypherParser::OC_PropertiesContext *ctx) {
    if (ctx->oC_MapLiteral() == nullptr) {
        NOT_SUPPORT_AND_THROW();
    }
    if (ctx->oC_MapLiteral()) {
        return visit(ctx->oC_MapLiteral());
    }
    if (ctx->oC_Parameter()) {
        NOT_SUPPORT_AND_THROW();
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_RelationshipTypes(
    LcypherParser::OC_RelationshipTypesContext *ctx) {
    geax::frontend::ElementFiller* node = nullptr;
    checkedCast(node_, node);
    std::vector<std::string> label_strs;
    for (auto &ctx_label : ctx->oC_RelTypeName()) {
        std::string str;
        checkedAnyCast(visit(ctx_label), str);
        label_strs.emplace_back(str);
    }
    if (label_strs.size() == 1) {
        auto l = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
        l->setLabel(std::move(label_strs[0]));
        node->setLabel(l);
        return 0;
    }
    if (label_strs.size() == 2) {
        auto root = ALLOC_GEAOBJECT(geax::frontend::LabelOr);
        auto l = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
        l->setLabel(std::move(label_strs[0]));
        auto r = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
        r->setLabel(std::move(label_strs[1]));
        root->setLeft(l);
        root->setRight(r);
        node->setLabel(root);
        return 0;
    }
    auto root = ALLOC_GEAOBJECT(geax::frontend::LabelOr);
    auto label = root;
    for (size_t idx = 0; idx < label_strs.size() - 2; ++idx) {
        auto o = ALLOC_GEAOBJECT(geax::frontend::LabelOr);
        auto l = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
        l->setLabel(std::move(label_strs[idx]));
        label->setLeft(l);
        label->setRight(o);
        label = o;
    }
    auto l = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
    l->setLabel(std::move(label_strs[label_strs.size() - 2]));
    auto r = ALLOC_GEAOBJECT(geax::frontend::SingleLabel);
    r->setLabel(std::move(label_strs[label_strs.size() - 1]));
    label->setLeft(l);
    label->setRight(r);
    node->setLabel(root);
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_NodeLabels(LcypherParser::OC_NodeLabelsContext *ctx) {
    std::vector<std::string> label_strs;
    for (auto &ctx_label : ctx->oC_NodeLabel()) {
        std::string str;
        checkedAnyCast(visit(ctx_label), str);
        label_strs.emplace_back(str);
    }
    return label_strs;
}

std::any CypherBaseVisitorV2::visitOC_NodeLabel(LcypherParser::OC_NodeLabelContext *ctx) {
    std::string label;
    checkedAnyCast(visit(ctx->oC_LabelName()), label);
    return label;
}

std::any CypherBaseVisitorV2::visitOC_RangeLiteral(LcypherParser::OC_RangeLiteralContext *ctx) {
    geax::frontend::Edge* edge = nullptr;
    checkedCast(node_, edge);
    int low = 0, hight = 0;
    std::vector<antlr4::tree::ParseTree *> valid_children;
    for (const auto &it : ctx->children) {
        const auto &text = it->getText();
        if (!std::all_of(text.cbegin(), text.cend(), ::isspace)) {
            valid_children.emplace_back(it);
        }
    }
    switch (valid_children.size()) {
    case 2:
        {
            if (!ctx->oC_IntegerLiteral().empty()) {
                low = std::stoi(ctx->oC_IntegerLiteral(0)->getText());
                hight = low;
            } else {
                CYPHER_THROW_ASSERT(valid_children.at(1)->getText() == "..");
                low = 1;
                hight = VAR_LEN_EXPAND_MAX_HOP;
            }
            break;
        }
    case 3:
        {
            if (valid_children.at(2)->getText() == "..") {
                low = std::stoi(ctx->oC_IntegerLiteral(0)->getText());
                hight = VAR_LEN_EXPAND_MAX_HOP;
            } else {
                CYPHER_THROW_ASSERT(valid_children.at(1)->getText() == "..");
                low = 1;
                hight = std::stoi(ctx->oC_IntegerLiteral(0)->getText());
            }
            break;
        }
    case 4:
        {
            CYPHER_THROW_ASSERT(ctx->oC_IntegerLiteral().size() == 2);
            low = std::stoi(ctx->oC_IntegerLiteral(0)->getText());
            hight = std::stoi(ctx->oC_IntegerLiteral(1)->getText());
            break;
        }
    default:
        CYPHER_TODO();
    }
    edge->setHopRange(low, hight);
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_LabelName(LcypherParser::OC_LabelNameContext *ctx) {
    return visit(ctx->oC_SchemaName());
}

std::any CypherBaseVisitorV2::visitOC_RelTypeName(LcypherParser::OC_RelTypeNameContext *ctx) {
    return visit(ctx->oC_SchemaName());
}

std::any CypherBaseVisitorV2::visitOC_Expression(LcypherParser::OC_ExpressionContext *ctx) {
    return ctx->oC_OrExpression()->accept(this);
}

std::any CypherBaseVisitorV2::visitOC_OrExpression(LcypherParser::OC_OrExpressionContext *ctx) {
    geax::frontend::Expr* left = nullptr, * right = nullptr;
    for (size_t idx = 0; idx < ctx->oC_XorExpression().size(); ++idx) {
        left = right;
        checkedAnyCast(visit(ctx->oC_XorExpression(idx)), right);
        if (idx > 0) {
            auto op = ALLOC_GEAOBJECT(geax::frontend::BOr);
            op->setLeft(left);
            op->setRight(right);
            right = op;
        }
    }
    return right;
}

std::any CypherBaseVisitorV2::visitOC_XorExpression(LcypherParser::OC_XorExpressionContext *ctx) {
    geax::frontend::Expr* left = nullptr, * right = nullptr;
    for (size_t idx = 0; idx < ctx->oC_AndExpression().size(); ++idx) {
        left = right;
        checkedAnyCast(visit(ctx->oC_AndExpression(idx)), right);
        if (idx > 0) {
            auto op = ALLOC_GEAOBJECT(geax::frontend::BXor);
            op->setLeft(left);
            op->setRight(right);
            right = op;
        }
    }
    return right;
}

std::any CypherBaseVisitorV2::visitOC_AndExpression(LcypherParser::OC_AndExpressionContext *ctx) {
    geax::frontend::Expr* left = nullptr, * right = nullptr;
    for (size_t idx = 0; idx < ctx->oC_NotExpression().size(); ++idx) {
        left = right;
        checkedAnyCast(visit(ctx->oC_NotExpression(idx)), right);
        if (idx > 0) {
            auto op = ALLOC_GEAOBJECT(geax::frontend::BAnd);
            op->setLeft(left);
            op->setRight(right);
            right = op;
        }
    }
    return right;
}

std::any CypherBaseVisitorV2::visitOC_NotExpression(LcypherParser::OC_NotExpressionContext *ctx) {
    if (ctx->NOT().size() % 2) {
        auto op = ALLOC_GEAOBJECT(geax::frontend::Not);
        geax::frontend::Expr* expr = nullptr;
        checkedAnyCast(visit(ctx->oC_ComparisonExpression()), expr);
        op->setExpr(expr);
        return (geax::frontend::Expr*)op;
    } else {
        return visit(ctx->oC_ComparisonExpression());
    }
}

std::any CypherBaseVisitorV2::visitOC_ComparisonExpression(
    LcypherParser::OC_ComparisonExpressionContext *ctx) {
    geax::frontend::Expr* expr = nullptr;
    checkedAnyCast(visit(ctx->oC_AddOrSubtractExpression()), expr);
    if (ctx->oC_PartialComparisonExpression().empty()) return expr;
    if (ctx->oC_PartialComparisonExpression().size() > 1) NOT_SUPPORT_AND_THROW();
    geax::frontend::BinaryOp* bi = nullptr;
    checkedAnyCast(visit(ctx->oC_PartialComparisonExpression(0)), bi);
    bi->setLeft(expr);
    return (geax::frontend::Expr*)bi;
}

std::any CypherBaseVisitorV2::visitOC_AddOrSubtractExpression(
    LcypherParser::OC_AddOrSubtractExpressionContext *ctx) {
    std::vector<std::string> ops;
    ops.reserve(ctx->children.size());
    for (auto c : ctx->children) {
        if (c->getText() == "+" || c->getText() == "-") {
            ops.emplace_back(c->getText());
        }
    }
    if (ops.size() != ctx->oC_MultiplyDivideModuloExpression().size() - 1) {
        NOT_SUPPORT_AND_THROW();
    }
    geax::frontend::Expr* left = nullptr, * right = nullptr;
    for (size_t idx = 0; idx < ctx->oC_MultiplyDivideModuloExpression().size(); ++idx) {
        left = right;
        checkedAnyCast(visit(ctx->oC_MultiplyDivideModuloExpression(idx)), right);
        if (idx > 0) {
            geax::frontend::BinaryOp* op = nullptr;
            if (ops[idx - 1] == "+") {
                op = ALLOC_GEAOBJECT(geax::frontend::BAdd);
            } else {
                op = ALLOC_GEAOBJECT(geax::frontend::BSub);
            }
            op->setLeft(left);
            op->setRight(right);
            right = op;
        }
    }
    return right;
}

std::any CypherBaseVisitorV2::visitOC_MultiplyDivideModuloExpression(
    LcypherParser::OC_MultiplyDivideModuloExpressionContext *ctx) {
    if (ctx->oC_PowerOfExpression().size() == 1)
        return visitChildren(ctx);
    NOT_SUPPORT_AND_THROW();
}

std::any CypherBaseVisitorV2::visitOC_PowerOfExpression(
    LcypherParser::OC_PowerOfExpressionContext *ctx) {
    if (ctx->oC_UnaryAddOrSubtractExpression().size() == 1)
        return visitChildren(ctx);
    NOT_SUPPORT_AND_THROW();
}

std::any CypherBaseVisitorV2::visitOC_UnaryAddOrSubtractExpression(
    LcypherParser::OC_UnaryAddOrSubtractExpressionContext *ctx) {
    // TODO
    return visitChildren(ctx);
}

std::any CypherBaseVisitorV2::visitOC_StringListNullOperatorExpression(
    LcypherParser::OC_StringListNullOperatorExpressionContext *ctx) {
    // TODO
    return visitChildren(ctx);
}

std::any CypherBaseVisitorV2::visitOC_ListOperatorExpression(
    LcypherParser::OC_ListOperatorExpressionContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_StringOperatorExpression(
    LcypherParser::OC_StringOperatorExpressionContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_NullOperatorExpression(
    LcypherParser::OC_NullOperatorExpressionContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_PropertyOrLabelsExpression(
    LcypherParser::OC_PropertyOrLabelsExpressionContext *ctx) {
    if (ctx->children.size() == 1) return visit(ctx->oC_Atom());

    if (!ctx->oC_PropertyLookup().empty()) {
        if (ctx->oC_PropertyLookup().size() > 1)  NOT_SUPPORT_AND_THROW();
        auto gf = ALLOC_GEAOBJECT(geax::frontend::GetField);
        std::string field_name;
        checkedAnyCast(visit(ctx->oC_PropertyLookup(0)), field_name);
        gf->setFieldName(std::move(field_name));
        geax::frontend::Expr* expr = nullptr;
        checkedAnyCast(visit(ctx->oC_Atom()), expr);
        gf->setExpr(expr);
        return (geax::frontend::Expr*)gf;
    }
    if (ctx->oC_NodeLabels()) {
        NOT_SUPPORT_AND_THROW();
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Atom(LcypherParser::OC_AtomContext *ctx) {
    if (ctx->oC_Variable()) {
        std::string var;
        checkedAnyCast(visit(ctx->oC_Variable()), var);
        auto expr = ALLOC_GEAOBJECT(geax::frontend::Ref);
        expr->setName(std::move(var));
        return (geax::frontend::Expr*)expr;
    } else if (ctx->oC_Literal()) {
        return visit(ctx->oC_Literal());
    } else if (ctx->oC_FunctionInvocation()) {
        return visit(ctx->oC_FunctionInvocation());
    } else if (ctx->oC_ParenthesizedExpression()) {
        return visit(ctx->oC_ParenthesizedExpression());
    }
    NOT_SUPPORT_AND_THROW();
}

std::any CypherBaseVisitorV2::visitOC_Literal(LcypherParser::OC_LiteralContext *ctx) {
    if (ctx->StringLiteral()) {
        std::string str = ctx->StringLiteral()->getText();
        CYPHER_THROW_ASSERT(!str.empty() && (str[0] == '\'' || str[0] == '\"') &&
                            (str[str.size() - 1] == '\'' || str[str.size() - 1] == '\"'));
        str = str.substr(1, str.size() - 2);
        auto expr = ALLOC_GEAOBJECT(geax::frontend::VString);
        expr->setVal(std::move(str));
        return (geax::frontend::Expr*)expr;
    } else if (ctx->oC_NumberLiteral()) {
        return visit(ctx->oC_NumberLiteral());
    }
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_BooleanLiteral(LcypherParser::OC_BooleanLiteralContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_ListLiteral(LcypherParser::OC_ListLiteralContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_PartialComparisonExpression(
    LcypherParser::OC_PartialComparisonExpressionContext *ctx) {
    geax::frontend::Expr* expr = nullptr;
    checkedAnyCast(visit(ctx->oC_AddOrSubtractExpression()), expr);
    geax::frontend::BinaryOp* bi = nullptr;
    auto predicate = ctx->getStart()->getText();
    if (predicate == "=") {
        bi = ALLOC_GEAOBJECT(geax::frontend::BEqual);
    } else if (predicate == "<>") {
        bi = ALLOC_GEAOBJECT(geax::frontend::BNotEqual);
    } else if (predicate == "<") {
        bi = ALLOC_GEAOBJECT(geax::frontend::BSmallerThan);
    } else if (predicate == ">") {
        bi = ALLOC_GEAOBJECT(geax::frontend::BGreaterThan);
    } else if (predicate == "<=") {
        bi = ALLOC_GEAOBJECT(geax::frontend::BNotGreaterThan);
    } else if (predicate == ">=") {
        bi = ALLOC_GEAOBJECT(geax::frontend::BNotSmallerThan);
    }
    bi->setRight(expr);
    return bi;
}

std::any CypherBaseVisitorV2::visitOC_ParenthesizedExpression(
    LcypherParser::OC_ParenthesizedExpressionContext *ctx) {
    return visit(ctx->oC_Expression());
}

std::any CypherBaseVisitorV2::visitOC_RelationshipsPattern(
    LcypherParser::OC_RelationshipsPatternContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_FilterExpression(
    LcypherParser::OC_FilterExpressionContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_IdInColl(LcypherParser::OC_IdInCollContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_FunctionInvocation(
    LcypherParser::OC_FunctionInvocationContext *ctx) {
    std::string name;
    checkedAnyCast(visit(ctx->oC_FunctionName()), name);
    auto func = ALLOC_GEAOBJECT(geax::frontend::Function);
    func->setName(std::move(name));
    for (size_t idx = 0; idx < ctx->oC_Expression().size(); ++idx) {
        geax::frontend::Expr* expr = nullptr;
        checkedAnyCast(visit(ctx->oC_Expression(idx)), expr);
        func->appendArg(expr);
    }
    return (geax::frontend::Expr*)func;
}

std::any CypherBaseVisitorV2::visitOC_FunctionName(LcypherParser::OC_FunctionNameContext *ctx) {
    if (ctx->EXISTS()) {
        return std::string("EXISTS");
    }
    std::string name;
    if (ctx->oC_Namespace()) {
        checkedAnyCast(visit(ctx->oC_Namespace()), name);
    }
    name.append(ctx->oC_SymbolicName()->getText());
    return name;
}

std::any CypherBaseVisitorV2::visitOC_ExplicitProcedureInvocation(
    LcypherParser::OC_ExplicitProcedureInvocationContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_ImplicitProcedureInvocation(
    LcypherParser::OC_ImplicitProcedureInvocationContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_ProcedureResultField(
    LcypherParser::OC_ProcedureResultFieldContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_ProcedureName(LcypherParser::OC_ProcedureNameContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Namespace(LcypherParser::OC_NamespaceContext *ctx) {
    std::string name_space;
    for (auto &s : ctx->oC_SymbolicName()) name_space.append(s->getText()).append(".");
    return name_space;
}

std::any CypherBaseVisitorV2::visitOC_ListComprehension(
    LcypherParser::OC_ListComprehensionContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_PatternComprehension(
    LcypherParser::OC_PatternComprehensionContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_PropertyLookup(LcypherParser::OC_PropertyLookupContext *ctx) {
    return visit(ctx->oC_PropertyKeyName());
}

std::any CypherBaseVisitorV2::visitOC_CaseExpression(LcypherParser::OC_CaseExpressionContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_CaseAlternatives(
    LcypherParser::OC_CaseAlternativesContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Variable(LcypherParser::OC_VariableContext *ctx) {
    return visit(ctx->oC_SymbolicName());
}

std::any CypherBaseVisitorV2::visitOC_NumberLiteral(LcypherParser::OC_NumberLiteralContext *ctx) {
    if (ctx->oC_DoubleLiteral()) {
        return visit(ctx->oC_DoubleLiteral());
    } else if (ctx->oC_IntegerLiteral()) {
        return visit(ctx->oC_IntegerLiteral());
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_MapLiteral(LcypherParser::OC_MapLiteralContext *ctx) {
    geax::frontend::ElementFiller* filler = nullptr;
    checkedCast(node_, filler);
    if (ctx->oC_Expression().size() != ctx->oC_PropertyKeyName().size())
        NOT_SUPPORT_AND_THROW();
    auto ps = ALLOC_GEAOBJECT(geax::frontend::PropStruct);
    filler->appendPredicate(ps);
    for (size_t idx = 0; idx < ctx->oC_PropertyKeyName().size(); ++idx) {
        geax::frontend::Expr* expr = nullptr;
        checkedAnyCast(visit(ctx->oC_Expression(idx)), expr);
        std::string name;
        checkedAnyCast(visit(ctx->oC_PropertyKeyName(idx)), name);
        ps->appendProperty(std::move(name), expr);
    }
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Parameter(LcypherParser::OC_ParameterContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_PropertyExpression(
    LcypherParser::OC_PropertyExpressionContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_PropertyKeyName(
    LcypherParser::OC_PropertyKeyNameContext *ctx) {
    return visit(ctx->oC_SchemaName());
}

std::any CypherBaseVisitorV2::visitOC_IntegerLiteral(LcypherParser::OC_IntegerLiteralContext *ctx) {
    auto integer = ALLOC_GEAOBJECT(geax::frontend::VInt);
    integer->setVal(std::stol(ctx->getText()));
    return (geax::frontend::Expr*) integer;
}

std::any CypherBaseVisitorV2::visitOC_DoubleLiteral(LcypherParser::OC_DoubleLiteralContext *ctx) {
    auto d = ALLOC_GEAOBJECT(geax::frontend::VDouble);
    d->setVal(std::stod(ctx->getText()));
    return (geax::frontend::Expr*) d;
}

std::any CypherBaseVisitorV2::visitOC_SchemaName(LcypherParser::OC_SchemaNameContext *ctx) {
    if (ctx->oC_ReservedWord()) {
        NOT_SUPPORT_AND_THROW();
    } else {
        return visitOC_SymbolicName(ctx->oC_SymbolicName());
    }
    return visitChildren(ctx);
}

std::any CypherBaseVisitorV2::visitOC_ReservedWord(LcypherParser::OC_ReservedWordContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_SymbolicName(LcypherParser::OC_SymbolicNameContext *ctx) {
    std::string name = ctx->getText();
    return name;
}

std::any CypherBaseVisitorV2::visitOC_LeftArrowHead(LcypherParser::OC_LeftArrowHeadContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_RightArrowHead(LcypherParser::OC_RightArrowHeadContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

std::any CypherBaseVisitorV2::visitOC_Dash(LcypherParser::OC_DashContext *ctx) {
    NOT_SUPPORT_AND_THROW();
    return 0;
}

} // end of namespace parser