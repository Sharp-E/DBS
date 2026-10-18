#ifndef PLAN_PROJECTION_H
#define PLAN_PROJECTION_H

#include "tdb.h"
#include "catalog/systables/Type.h"

#include "plan/PlanNode.h"
#include "expr/ExprNode.h"

namespace taco {

class ProjectionState;

/*!
 * `Projection` is the physical plan for projections. This physical plan should
 * remember its child plan and a list of expressions conducted on input columns
 * of its child. It should also derive the correct output schema based on the
 * provided expressions through assembling their return types.
 */
class Projection: public PlanNode {
public:
    static std::unique_ptr<Projection>
    Create(std::unique_ptr<PlanNode>&& child,
           std::vector<std::unique_ptr<ExprNode>>&& exprs);

    ~Projection() override;

    void node_properties_to_string(std::string& buf, int indent) const override;

    std::unique_ptr<PlanExecState> create_exec_state() const override;

    const Schema* get_output_schema() const override;
private:
    Projection(std::unique_ptr<PlanNode>&& child,
               std::vector<std::unique_ptr<ExprNode>>&& exprs);

    // You can add your own states here.

    friend class ProjectionState;
};

}    // namespace taco

#endif
