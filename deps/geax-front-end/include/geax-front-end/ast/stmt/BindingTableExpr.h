/**
 * Copyright 2023 AntGroup CO., Ltd.
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
 *
 *  Author:
 *         Yaochi <boyao.zby@alibaba-inc.com>
 */

#ifndef GEAXFRONTEND_AST_STMT_BINDINGTABLEEXPR_H_
#define GEAXFRONTEND_AST_STMT_BINDINGTABLEEXPR_H_

#include "geax-front-end/ast/AstNode.h"

namespace geax {
namespace frontend {

class BindingTableExpr : public AstNode {
public:
    explicit BindingTableExpr(AstNodeType nodeType) : AstNode(nodeType) {}
    ~BindingTableExpr() = default;
};  // class BindingTableExpr

}  // namespace frontend
}  // namespace geax

#endif  // GEAXFRONTEND_AST_STMT_BINDINGTABLEEXPR_H_