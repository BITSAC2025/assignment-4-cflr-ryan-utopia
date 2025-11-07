/**
 * CFLR.cpp
 * @author kisslune 
 */

#include "A4Header.h"

#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    pag->dump();

    CFLR solver;
    solver.buildGraph(pag);
    // TODO: complete this method
    solver.solve();
    solver.dumpResult();

    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFLR::solve()
{
    if (!graph)
        return;

    auto &succMap = graph->getSuccessorMap();
    auto &predMap = graph->getPredecessorMap();

    // Helper to insert a derived edge and enqueue it for further processing when it is new.
    auto addEdgeIfAbsent = [&](unsigned src, unsigned dst, EdgeLabel label) {
        if (!graph->hasEdge(src, dst, label))
        {
            graph->addEdge(src, dst, label);
            workList.push(CFLREdge(src, dst, label));
        }
    };

    // Grammar encoded as unary and binary production rules.
    std::unordered_map<EdgeLabel, std::vector<EdgeLabel>> unaryRules{
            {Copy,    {VF}},
            {CopyBar, {VFBar}},
    };

    std::vector<std::tuple<EdgeLabel, EdgeLabel, EdgeLabel>> binaryRules = {
            {PT,     VFBar,   AddrBar},
            {PTBar,  Addr,    VF},
            {VF,     VF,      VF},
            {VF,     SV,      Load},
            {VF,     PV,      Load},
            {VF,     Store,   VP},
            {VFBar,  VFBar,   VFBar},
            {VFBar,  LoadBar, SV},
            {VFBar,  LoadBar, VP},
            {VFBar,  PV,      StoreBar},
            {VA,     LV,      Load},
            {VA,     VFBar,   VA},
            {VA,     VA,      VF},
            {SV,     Store,   VA},
            {SVBar,  VA,      StoreBar},
            {PV,     PTBar,   VA},
            {VP,     VA,      PT},
            {LV,     LoadBar, VA},
    };

    std::unordered_map<EdgeLabel, std::vector<std::pair<EdgeLabel, EdgeLabel>>> leftRules;
    std::unordered_map<EdgeLabel, std::vector<std::pair<EdgeLabel, EdgeLabel>>> rightRules;
    for (const auto &[result, left, right] : binaryRules)
    {
        leftRules[left].emplace_back(result, right);
        rightRules[right].emplace_back(result, left);
    }

    // Collect existing edges as worklist seeds and record all nodes for epsilon rules.
    std::vector<CFLREdge> initialEdges;
    std::unordered_set<unsigned> allNodes;
    for (auto &srcEntry : succMap)
    {
        unsigned src = srcEntry.first;
        allNodes.insert(src);
        for (auto &labelEntry : srcEntry.second)
        {
            EdgeLabel lbl = labelEntry.first;
            for (unsigned dst : labelEntry.second)
            {
                initialEdges.emplace_back(src, dst, lbl);
                allNodes.insert(dst);
            }
        }
    }

    for (const auto &edge : initialEdges)
        workList.push(edge);

    // Non-terminals with epsilon productions yield self-loops on every node.
    for (unsigned node : allNodes)
    {
        addEdgeIfAbsent(node, node, VF);
        addEdgeIfAbsent(node, node, VFBar);
        addEdgeIfAbsent(node, node, VA);
    }

    // Process worklist until closure is reached.
    while (!workList.empty())
    {
        CFLREdge edge = workList.pop();

        // Apply unary rules
        if (auto itUnary = unaryRules.find(edge.label); itUnary != unaryRules.end())
        {
            for (EdgeLabel result : itUnary->second)
                addEdgeIfAbsent(edge.src, edge.dst, result);
        }

        // Apply binary rules where current edge is on the left-hand side component.
        if (auto itLeft = leftRules.find(edge.label); itLeft != leftRules.end())
        {
            auto succIt = succMap.find(edge.dst);
            if (succIt != succMap.end())
            {
                for (const auto &[result, rightLbl] : itLeft->second)
                {
                    auto succLblIt = succIt->second.find(rightLbl);
                    if (succLblIt == succIt->second.end())
                        continue;
                    for (unsigned next : succLblIt->second)
                        addEdgeIfAbsent(edge.src, next, result);
                }
            }
        }

        // Apply binary rules where current edge is the right-hand side component.
        if (auto itRight = rightRules.find(edge.label); itRight != rightRules.end())
        {
            auto predIt = predMap.find(edge.src);
            if (predIt != predMap.end())
            {
                for (const auto &[result, leftLbl] : itRight->second)
                {
                    auto predLblIt = predIt->second.find(leftLbl);
                    if (predLblIt == predIt->second.end())
                        continue;
                    for (unsigned prev : predLblIt->second)
                        addEdgeIfAbsent(prev, edge.dst, result);
                }
            }
        }
    }
}
