/**
 * Copyright 2024 AntGroup CO., Ltd.
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

#pragma once

#include <string>
#include <vector>

#include "cypher/resultset/record.h"
#include "cypher/arithmetic/ast_expr_evaluator.h"
#include "geax-front-end/ast/expr/Expr.h"


namespace experiment::plan {

using ColumnId = uint32_t;

enum class OperatorStatus {
    kDepleted = 0,
    kOk = 1,
};

class OperatorRecord {
 public:
    std::vector<cypher::Entry *> entries;
};

class Operator {
 public:
    std::vector<cypher::Entry *> entries;
    OperatorStatus status;

    virtual OperatorStatus RealConsume() = 0;
    virtual void Init() = 0;
};

class LeafOp : public Operator {};

class UnaryOp : public Operator {
 public:
    Operator *child;
};

class BinaryOp : public Operator {
 public:
    Operator *lhs_child;
    Operator *rhs_child;
};

class Singleton : public LeafOp {
    virtual void Init() { status = OperatorStatus::kOk; }
    virtual OperatorStatus RealConsume() {
        if (status == OperatorStatus::kOk) {
            status = OperatorStatus::kDepleted;
            return OperatorStatus::kOk;
        }
        return status;
    }
};

class Mapping : public UnaryOp {
 public:
    std::vector<geax::frontend::Expr *> items;
    virtual void Init() {
        child->Init();
        // entries = child->entries;
        for (size_t i = 0; i < items.size(); i++) {
            entries.push_back(new cypher::Entry());
        }
    }
    virtual OperatorStatus RealConsume() {
        if (child->RealConsume() == OperatorStatus::kDepleted) {
            return OperatorStatus::kDepleted;
        }
        for (size_t i = 0; i < items.size(); i++) {
            cypher::AstExprEvaluator evaluator(items[i], nullptr);
            *entries[i] = evaluator.Evaluate(nullptr, nullptr);
        }
        return OperatorStatus::kOk;
    }
};

class ProduceResults : public UnaryOp {
 public:
    std::vector<std::string> aliases;
    virtual void Init() {
        child->Init();
        entries = child->entries;
    }
    virtual OperatorStatus RealConsume() {
        if (child->RealConsume() == OperatorStatus::kDepleted) {
            return OperatorStatus::kDepleted;
        }
        for (auto entry : entries) {
            std::cout << entry->ToString() << std::endl;
        }
        return OperatorStatus::kOk;
    }
};

class NodePrimaryLookup : public UnaryOp {
 public:
    std::string graph_name;
    std::string label_name;
    ColumnId primary;
};

class NodeIndexLookup : public UnaryOp {};

class NodeVidLookup : public UnaryOp {
 public:
    std::string graph_name;
    ColumnId vid;
};

}  // namespace experiment::plan
