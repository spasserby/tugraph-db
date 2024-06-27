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

#include "experiment/logical_plan/operators.h"
#include "experiment/simple_physical_plan/operators.h"

#include "./graph_factory.h"
#include "./ut_utils.h"
#include "db/galaxy.h"
#include "geax-front-end/ast/expr/BAdd.h"
#include "geax-front-end/ast/expr/VInt.h"
#include "lgraph/lgraph_db.h"

class TestExprimentPlan : public TuGraphTest {};

TEST_F(TestExprimentPlan, TestDemo) {
    auto *singleton = new experiment::logical::Singleton();

    auto *mapping = new experiment::logical::Mapping();
    mapping->child = singleton;

    geax::frontend::BAdd *badd = new geax::frontend::BAdd();
    geax::frontend::VInt *lhs = new geax::frontend::VInt();
    lhs->setVal(1);
    badd->setLeft(lhs);
    geax::frontend::VInt *rhs = new geax::frontend::VInt();
    rhs->setVal(2);
    badd->setRight(rhs);
    geax::frontend::VString *vstring = new geax::frontend::VString();
    vstring->setVal("Vanessa Redgrave");
    mapping->items = std::vector<geax::frontend::Expr *>{badd, vstring};

    auto node_index_lookup = new experiment::logical::NodeIndexLookup();
    node_index_lookup->child = mapping;
    node_index_lookup->graph_name = "default";
    node_index_lookup->label_name = "Person";
    node_index_lookup->field_name = "name";
    node_index_lookup->index = 1;

    // physical
    auto *physical_singleton = new experiment::simple::physical::Singleton();
    physical_singleton->Init();
    physical_singleton->singleton = singleton;

    auto *physical_mapping = new experiment::simple::physical::Mapping();
    physical_mapping->mapping = mapping;
    physical_mapping->child = physical_singleton;

    auto *physical_node_index_lookup = new experiment::simple::physical::NodeIndexLookup();
    physical_node_index_lookup->node_index_lookup = node_index_lookup;
    physical_node_index_lookup->child = physical_mapping;

    auto last_physical = physical_node_index_lookup;

    {
        std::string graph_name = "default";
        std::string db_dir = "./testdb";
        GraphFactory::GRAPH_DATASET_TYPE graph_type = GraphFactory::GRAPH_DATASET_TYPE::YAGO;
        GraphFactory::create_graph(graph_type, db_dir);
        lgraph::Galaxy::Config gconf;
        gconf.dir = db_dir;
        cypher::RTContext ctx;
        lgraph::Galaxy galaxy = lgraph::Galaxy(gconf, true, nullptr);
        ctx = cypher::RTContext(nullptr, &galaxy, lgraph::_detail::DEFAULT_ADMIN_NAME, graph_name);
        ctx.ac_db_ =
            std::make_unique<lgraph::AccessControlledDB>(galaxy.OpenGraph(ctx.user_, ctx.graph_));
        lgraph_api::GraphDB db(ctx.ac_db_.get(), true);
        ctx.txn_ = std::make_unique<lgraph_api::Transaction>(db.CreateReadTxn());
        while (last_physical->RetrieveSingleRow(&ctx) ==
               experiment::simple::physical::Status::kOk) {
            std::cout << last_physical->record.ToString() << std::endl;
        }
        ctx.txn_->Abort();
        ctx.txn_.reset(nullptr);
        ctx.ac_db_.reset(nullptr);
    }
}

TEST_F(TestExprimentPlan, TestPhysicalMapping) {
    // [[maybe_unused]]
    // experiment::simple::physical::Mapping mapping;
}
