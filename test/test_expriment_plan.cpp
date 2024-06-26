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

#include "experiment/plan/operators.h"

#include "./ut_utils.h"
#include "geax-front-end/ast/expr/BAdd.h"
#include "geax-front-end/ast/expr/VInt.h"
#include "geax-front-end/ast/expr/Expr.h"

class TestExprimentPlan : public TuGraphTest {};

TEST_F(TestExprimentPlan, TestDemo) {
    auto *singleton = new experiment::plan::Singleton();

    auto *mapping = new experiment::plan::Mapping();
    mapping->child = singleton;

    geax::frontend::BAdd *badd = new geax::frontend::BAdd();
    geax::frontend::VInt *lhs = new geax::frontend::VInt();
    lhs->setVal(1);
    badd->setLeft(lhs);
    geax::frontend::VInt *rhs = new geax::frontend::VInt();
    rhs->setVal(2);
    badd->setRight(rhs);
    mapping->items = {badd};

    auto *produce_results = new experiment::plan::ProduceResults();
    produce_results->aliases = {"a"};
    produce_results->child = mapping;

    produce_results->Init();
    while (produce_results->RealConsume() == experiment::plan::OperatorStatus::kOk) {
    }
}
