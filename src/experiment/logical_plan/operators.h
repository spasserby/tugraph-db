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

#include "geax-front-end/ast/expr/Expr.h"


namespace experiment::logical {

using ColumnId = uint32_t;

class Operator {};

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

class Singleton : public LeafOperator {};

class Mapping : public UnaryOperator {
 public:
    std::vector<geax::frontend::Expr *> items;
};

class ProduceResults : public UnaryOperator {
 public:
    std::vector<std::string> aliases;
};

class NodeIndexLookup : public UnaryOperator {
 public:
    std::string graph_name;
    std::string label_name;
    std::string field_name;
    ColumnId index;
};

class NodeVidLookup : public UnaryOperator {
 public:
    std::string graph_name;
    std::string label_name;
    ColumnId vid;
};

}  // namespace experiment::logical
