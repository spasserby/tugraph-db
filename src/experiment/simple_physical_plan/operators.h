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

#include "cypher_types.h"
#include "experiment/logical_plan/operators.h"
#include "cypher/resultset/record.h"
#include "cypher/arithmetic/ast_expr_evaluator.h"
#include "cypher/execution_plan/runtime_context.h"

namespace experiment::simple::physical {

enum class Status {
    kDepleted = 0,
    kOk = 1,
};

class Operator {
 public:
    virtual void Init() = 0;
    virtual Status RetrieveSingleRow(cypher::RTContext *ctx) = 0;
    // std::vector<cypher::Entry *> entries;
    cypher::Record record;
};

class LeafOperator : public Operator {};

class UnaryOperator : public Operator {
 public:
    Operator *child;
};

class BinaryOperator : public Operator {
 public:
    Operator *lhs_child;
    Operator *rhs_child;
};

class Singleton : public LeafOperator {
 public:
    logical::Singleton *singleton;
    bool first_time = true;
    virtual void Init() {}
    virtual Status RetrieveSingleRow(cypher::RTContext *ctx) {
        if (first_time) {
            first_time = false;
            std::cout << "RetrieveSingleRow" << std::endl;
            return Status::kOk;
        } else {
            return Status::kDepleted;
        }
    }
};

class ProduceResults : public UnaryOperator {
 public:
    std::vector<std::string> aliases;
    // virtual void Init() {
    //     child->Init();
    //     entries = child->entries;
    // }

    // // 1 -> 1
    // virtual Status RetrieveSingleRow(cypher::RTContext *ctx) {
    //     if (child->RetrieveSingleRow(ctx) == Status::kDepleted) {
    //         return Status::kDepleted;
    //     }
    //     for (auto entry : entries) {
    //         std::cout << entry->ToString() << std::endl;
    //     }
    //     return Status::kOk;
    // }
};

class NodeIndexLookup : public UnaryOperator {
 public:
    // 1->N
    logical::NodeIndexLookup *node_index_lookup;
    virtual void Init() override {}
    virtual Status RetrieveSingleRow(cypher::RTContext *ctx) override {
        if (child->RetrieveSingleRow(ctx) == Status::kDepleted) {
            std::cout << "NodeIndexLookup: depleted" << std::endl;
            return Status::kDepleted;
        }
        record = child->record;
        auto entry = child->record.values[node_index_lookup->index];
        auto iit = ctx->txn_->GetTxn()->GetVertexIndexIterator(
            node_index_lookup->label_name, node_index_lookup->field_name, entry.constant.scalar,
            entry.constant.scalar);
        if (iit.IsValid()) {
            record.values.emplace_back(cypher::FieldData(lgraph::FieldData(iit.GetVid())));
            return Status::kOk;
        } else {
            return Status::kDepleted;
        }
    }
};

class Mapping : public UnaryOperator {
 public:
    logical::Mapping *mapping;

    virtual void Init() override {}

    virtual Status RetrieveSingleRow(cypher::RTContext *ctx) override {
        if (child->RetrieveSingleRow(ctx) == Status::kDepleted) {
            std::cout << "Mapping: depleted" << std::endl;
            return Status::kDepleted;
        }
        record = child->record;
        for (size_t i = 0; i < mapping->items.size(); i++) {
            cypher::AstExprEvaluator evaluator(mapping->items[i], nullptr);
            record.values.emplace_back(evaluator.Evaluate(ctx, &child->record));
        }
        return Status::kOk;
    }
};

}  // namespace experiment::simple::physical
